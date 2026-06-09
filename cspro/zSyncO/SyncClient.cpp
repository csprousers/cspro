#include "stdafx.h"
#include "SyncClient.h"
#include "ApplicationPackageManager.h"
#include "BluetoothSyncService.h"
#include "ISyncServiceFactory.h"
#include "JsonConverter.h"
#include "SyncDictionaryInfo.h"
#include <zNetwork/DropboxConnection.h>
#include <zNetwork/SyncConnectionStringProperties.h>
#include <zNetwork/SyncListenerRAII.h>
#include <zNetwork/SyncListenerWrapper.h>
#include <zDictO/DDClass.h>
#include <zParadataO/Logger.h>
#include <zParadataO/Syncer.h>


SyncClient::SyncClient(DeviceId device_id, std::unique_ptr<ISyncServiceFactory> sync_service_factory)
    :   m_deviceId(std::move(device_id)),
        m_syncServiceFactory(std::move(sync_service_factory))
{
    ASSERT(m_syncServiceFactory != nullptr);

    SyncLog::EnableLogging();
}


SyncClient::SyncClient(std::tuple<std::shared_ptr<ISyncService>, std::shared_ptr<ConnectResponse>> sync_runner_connection)
    :   m_deviceId(GetDeviceId()),
        m_syncService(std::move(std::get<0>(sync_runner_connection))),
        m_connectResponse(std::move(std::get<1>(sync_runner_connection)))
{
    ASSERT(m_syncService != nullptr && m_connectResponse != nullptr);

    SyncLog::EnableLogging();
}


SyncClient::SyncClient(SyncClient&&) = default;


SyncClient::~SyncClient()
{
    if( m_syncService != nullptr )
        DisconnectFromSyncService();
}


SyncClient::SyncResult SyncClient::Connect(const SyncConnectionString& sync_connection_string)
{
    SyncResult result;

    if( Paradata::Logger::IsOpen() )
    {
        result = ConnectWithParadataSupport(sync_connection_string);
    }

    else
    {
        result = ConnectWorker(sync_connection_string);
        m_paradataLogger.reset();
    }

    return result;
}


SyncClient::SyncResult SyncClient::ConnectWorker(const SyncConnectionString& sync_connection_string)
{
    // SYNC_TODO take the code here, harmonize it with SyncRunner's (if necessary),
    // and use SyncRunner::Connect instead
    if( !sync_connection_string.IsDefined() )
        return SyncResult::SYNC_ERROR;

    switch( sync_connection_string.GetType() )
    {
        case SyncServiceType::Bluetooth:  return ConnectBluetooth(sync_connection_string);
        case SyncServiceType::CSWeb:      return ConnectCSWeb(sync_connection_string);
        case SyncServiceType::Dropbox:    return ConnectDropbox(sync_connection_string);
        case SyncServiceType::Ftp:        return ConnectFtp(sync_connection_string);
        case SyncServiceType::LocalFiles: return ConnectLocalFiles(sync_connection_string);
        default:                          return ReturnProgrammingError(SyncResult::SYNC_ERROR);
    }
}


SyncClient::SyncResult SyncClient::ConnectWithParadataSupport(const SyncConnectionString& sync_connection_string)
{
    // because errors are reported using the SyncListener, we'll use a wrapper to get the error
    class OnErrorInterceptorSyncListener : public WrapperSyncListener
    {
    public:
        OnErrorInterceptorSyncListener(std::shared_ptr<SyncListener> sync_listener, std::unique_ptr<std::string>& exception_text)
            :   WrapperSyncListener(std::move(sync_listener)),
                m_exceptionText(exception_text)
        {
        }

    protected:
        void OnError(const int message_number, const std::string& message_text) override
        {
            ASSERT(m_exceptionText == nullptr);
            m_exceptionText = std::make_unique<std::string>(message_text);
            WrapperSyncListener::OnError(message_number, message_text);
        }

    private:
        std::unique_ptr<std::string>& m_exceptionText;
    };

    std::unique_ptr<std::string> exception_text;
    const RAII::SetValueAndRestoreOnDestruction sync_listener_modifier(m_syncListener,
                                                                       std::make_unique<OnErrorInterceptorSyncListener>(m_syncListener, exception_text));

    // set up the paradata data
    auto [paradata_logger, sync_connect_event_holder] = SyncRunner::ParadataLogger::Create(m_deviceId,
                                                                                           SyncRunner::ConnectionSource::Logic,
                                                                                           sync_connection_string);

    // run the connection routines
    const SyncResult result = ConnectWorker(sync_connection_string);

    if( exception_text != nullptr )
    {
        ASSERT(result == SyncResult::SYNC_ERROR);
        sync_connect_event_holder.SetResultFailure(std::move(*exception_text));
    }

    else if( result == SyncResult::SYNC_CANCELED )
    {
        const SyncCancelException sync_cancel_exception;
        sync_connect_event_holder.SetResultFailure(sync_cancel_exception);
    }

    else if( result != SyncResult::SYNC_OK )
    {
        sync_connect_event_holder.SetResultFailure("Sync Error");
    }

    m_paradataLogger = ( result == SyncResult::SYNC_OK ) ? std::move(paradata_logger) :
                                                           nullptr;

    return result;
}


SyncClient::SyncResult SyncClient::ConnectCSWeb(const SyncConnectionString& sync_connection_string, std::unique_ptr<LoginCredentials> login_credentials/* = nullptr*/)
{
    ASSERT(sync_connection_string.GetType() == SyncServiceType::CSWeb);
    ASSERT(m_syncServiceFactory != nullptr);

    std::unique_ptr<ISyncService> sync_service = m_syncServiceFactory->CreateCSWebSyncService(sync_connection_string, std::move(login_credentials));

    if( sync_service == nullptr )
    {
        if( m_syncListener != nullptr )
            m_syncListener->ReportError(100120, sync_connection_string.GetUrl().c_str());

        SYNCLOG_ERROR << "Error failed to create connection to CSWeb server: " << sync_connection_string.GetUrl();
        return SyncResult::SYNC_ERROR;
    }

    SYNCLOG_INFO << "Connect to CSWeb server: " << sync_connection_string.GetUrl();

    return ConnectToSyncService(std::move(sync_service), sync_connection_string.GetUrl());
}


SyncClient::SyncResult SyncClient::ConnectBluetooth(const SyncConnectionString& sync_connection_string)
{
    ASSERT(sync_connection_string.GetType() == SyncServiceType::Bluetooth);
    ASSERT(m_syncServiceFactory != nullptr);

    // when using a sync connection string, the service device name is specified as the path
    const std::string service_device_name = sync_connection_string.GetEvaluatedBluetoothServerDeviceName();
    std::unique_ptr<ISyncService> sync_service;

    if( service_device_name.empty() )
    {
        SYNCLOG_INFO << "Connect to Bluetooth device, name not specified";
        sync_service = m_syncServiceFactory->CreateBluetoothSyncService();
    }

    else
    {
        SYNCLOG_INFO << "Connect to Bluetooth device " << service_device_name;
        sync_service = m_syncServiceFactory->CreateBluetoothSyncService(BluetoothDeviceInfo { service_device_name, std::string() });
    }

    if( sync_service == nullptr )
        return SyncResult::SYNC_ERROR;

    return ConnectToSyncService(std::move(sync_service), !service_device_name.empty() ? service_device_name : "Bluetooth");
}


SyncClient::SyncResult SyncClient::ConnectDropbox(cs::cref_optional<SyncConnectionString> sync_connection_string/* = std::nullopt*/)
{
    ASSERT(!sync_connection_string.has_value() || sync_connection_string->GetType() == SyncServiceType::Dropbox);
    ASSERT(m_syncServiceFactory != nullptr);

    // determine if a local Dropbox connection should be made
    if( sync_connection_string.has_value() )
    {
        try
        {
            std::string local_dropbox_directory = DropboxConnection::GetEvaluatedLocalDropboxDirectory(*sync_connection_string);

            if( !local_dropbox_directory.empty() )
            {
                SYNCLOG_INFO << "Connect to local Dropbox directory: " << local_dropbox_directory;
                std::unique_ptr<ISyncService> sync_service = m_syncServiceFactory->CreateDropboxLocalSyncService(std::move(local_dropbox_directory));

                if( sync_service == nullptr )
                    return ReturnProgrammingError(SyncResult::SYNC_ERROR);

                return ConnectToSyncService(std::move(sync_service), "LocalDropbox");
            }
        }

        catch( const SyncError& exception )
        {
            SYNCLOG_ERROR << "Error failed to connect to local Dropbox: " << exception.what();

            if( m_syncListener != nullptr )
                m_syncListener->ReportError(exception);

            return SyncResult::SYNC_ERROR;
        }
    }

    // create a remote Dropbox connection
    const std::string* const email = sync_connection_string.has_value() ? sync_connection_string->GetProperty(SCSProperty::email) :
                                                                          nullptr;

    if( email != nullptr )
    {
        SYNCLOG_INFO << "Connect to Dropbox using email: " << *email;
    }

    else
    {
        SYNCLOG_INFO << "Connect to Dropbox";
    }

    std::unique_ptr<ISyncService> sync_service = m_syncServiceFactory->CreateDropboxSyncService(std::move(sync_connection_string));

    if( sync_service == nullptr )
        return SyncResult::SYNC_ERROR;

    return ConnectToSyncService(std::move(sync_service), "Dropbox");
}


SyncClient::SyncResult SyncClient::ConnectFtp(const SyncConnectionString& sync_connection_string, std::unique_ptr<LoginCredentials> login_credentials/* = nullptr*/)
{
    ASSERT(sync_connection_string.GetType() == SyncServiceType::Ftp);
    ASSERT(m_syncServiceFactory != nullptr);

    std::unique_ptr<ISyncService> sync_service = m_syncServiceFactory->CreateFtpSyncService(sync_connection_string, std::move(login_credentials));

    if( sync_service == nullptr )
    {
        if( m_syncListener != nullptr )
            m_syncListener->ReportError(100125, sync_connection_string.GetUrl().c_str());

        SYNCLOG_ERROR << "Error failed to create connection to FTP server: " << sync_connection_string.GetUrl();
        return SyncResult::SYNC_ERROR;
    }

    SYNCLOG_INFO << "Connect to FTP server: " << sync_connection_string.GetUrl();

    return ConnectToSyncService(std::move(sync_service), sync_connection_string.GetUrl());
}


SyncClient::SyncResult SyncClient::ConnectLocalFiles(const SyncConnectionString& sync_connection_string)
{
    ASSERT(sync_connection_string.GetType() == SyncServiceType::LocalFiles);
    ASSERT(m_syncServiceFactory != nullptr);

    std::unique_ptr<ISyncService> sync_service = m_syncServiceFactory->CreateLocalFileSyncService(sync_connection_string);

    const std::string root_directory = sync_connection_string.GetEvaluatedDirectoryPath();

    if( sync_service == nullptr )
    {
        SYNCLOG_ERROR << "Error failed to create connection to local files: " << root_directory;
        return SyncResult::SYNC_ERROR;
    }

    SYNCLOG_INFO << "Connect to local files: " << root_directory;

    return ConnectToSyncService(std::move(sync_service), root_directory);
}


SyncClient::SyncResult SyncClient::ConnectToSyncService(std::unique_ptr<ISyncService> sync_service, const std::string& sync_service_name)
{
    ASSERT(sync_service != nullptr);

    SyncResult unsuccessful_result;

    try
    {
        // Disconnect old connection before connecting to a new sync service
        if( m_syncService != nullptr )
        {
            SYNCLOG_INFO << "Disconnect from existing sync service";
            DisconnectFromSyncService();
        }

        m_syncService = std::move(sync_service);

        const SyncListenerServerSaverAndCloser sync_listener_saver_and_closer(m_syncService.get(), m_syncListener, 100102, sync_service_name.c_str());

        SYNCLOG_INFO << "Connect to sync service " << sync_service_name;

        m_connectResponse = m_syncService->Connect();
        ASSERT(m_connectResponse != nullptr);

        SYNCLOG_INFO << "Connection successful. Server id: " << GetServerDeviceId();

        return SyncResult::SYNC_OK;
    }

    catch( const SyncError& exception )
    {
        SYNCLOG_ERROR << "Error connecting to sync service: " << exception.what();

        if( m_syncListener != nullptr )
            m_syncListener->ReportError(exception);

        unsuccessful_result = SyncResult::SYNC_ERROR;
    }

    catch( const SyncCancelException& )
    {
        SYNCLOG_INFO << "Canceled connection";

        unsuccessful_result = SyncResult::SYNC_CANCELED;
    }

    m_syncService.reset();
    m_connectResponse.reset();

    return unsuccessful_result;
}


SyncClient::SyncResult SyncClient::Disconnect()
{
    if( m_syncService == nullptr )
        return SyncResult::SYNC_ERROR;

    return DisconnectFromSyncService();
}


SyncClient::SyncResult SyncClient::DisconnectFromSyncService()
{
    try
    {
        if( m_syncService == nullptr )
            return SyncResult::SYNC_ERROR;

        const std::shared_ptr<ISyncService> sync_service = std::move(m_syncService);
        m_connectResponse.reset();
        const std::unique_ptr<SyncRunner::ParadataLogger> paradata_logger = std::move(m_paradataLogger);

        SyncRunner sync_runner(m_syncListener);
        sync_runner.Disconnect(*sync_service, paradata_logger.get());

        return SyncResult::SYNC_OK;
    }

    catch(...)
    {
        // Ignore disconnect errors since at this point the sync is done
        // and 90% of the time there was already an error during a call
        // to sync_data or sync_file prior to the disconnect.
        return SyncResult::SYNC_ERROR;
    }
}


const DeviceId& SyncClient::GetServerDeviceId() const
{
    ASSERT(m_connectResponse != nullptr);
    return m_connectResponse->GetServerDeviceId();
}


SyncClient::SyncResult SyncClient::SyncData(ISyncableDataRepository& syncable_data_repository,
                                            const SyncDirection sync_direction, const std::string& universe)
{
    try
    {
        if( m_syncService == nullptr )
            throw SyncError(100132);

        ASSERT(m_connectResponse != nullptr);

        SyncRunner sync_runner(m_syncListener);

        sync_runner.SyncData(*m_syncService, *m_connectResponse, m_deviceId,
                             syncable_data_repository, sync_direction, universe,
                             m_paradataLogger.get());

        return SyncResult::SYNC_OK;
    }

    catch( const SyncError& exception )
    {
        if( m_syncListener != nullptr )
            m_syncListener->ReportError(exception);
    }

    catch( const CSProException& exception )
    {
        if( m_syncListener != nullptr )
            m_syncListener->ReportError(100114, exception.what());

        if( dynamic_cast<const SyncCancelException*>(&exception) != nullptr )
            return SyncResult::SYNC_CANCELED;
    }

    return SyncResult::SYNC_ERROR;
}


SyncClient::SyncResult SyncClient::SyncFile(const SyncDirection direction, std::string from_path, std::string to_path)
{
    if( direction == SyncDirection::Get )
    {
        ASSERT(!to_path.empty() && to_path == PortableFunctions::PathToNativeSlash(to_path));

        PortableFunctions::MakePathToForwardSlash(from_path);
    }

    else
    {
        ASSERT(direction == SyncDirection::Put);
        ASSERT(!from_path.empty() && from_path == PortableFunctions::PathToNativeSlash(from_path));

        PortableFunctions::MakePathToForwardSlash(to_path);

        if( to_path.empty() )
            to_path = "/";
    }

    SYNCLOG_INFO << "Sync file: " << ToString(direction)
                 << " from: " << from_path
                 << " to: " << to_path;

    const bool from_path_has_wildcard_characters = Path::HasWildcardCharacters(PortableFunctions::PathGetFilename(from_path));

    if( direction == SyncDirection::Get )
    {
        return from_path_has_wildcard_characters ? GetFilesWithWildcard(from_path, to_path) :
                                                   GetFile(from_path, to_path);
    }

    else
    {
        return from_path_has_wildcard_characters ? PutFilesWithWildcard(from_path, to_path) :
                                                   PutFile(from_path, to_path);

    }
}


SyncClient::SyncResult SyncClient::GetDictionaries(std::vector<SyncDictionaryInfo>& dictionaries)
{
    try
    {
        if( m_syncService == nullptr )
            throw SyncError(100132);

        const SyncListenerServerSaverAndCloser sync_listener_saver_and_closer(m_syncService.get(), m_syncListener, 100128);

        SYNCLOG_INFO << "Downloading dictionary list";
        dictionaries = m_syncService->GetDictionaries();

        SYNCLOG_INFO << "Downloading dictionary list complete";
        return SyncResult::SYNC_OK;
    }

    catch( const SyncError& exception )
    {
        if( m_syncListener != nullptr )
            m_syncListener->ReportError(exception);

        SYNCLOG_ERROR << "Error downloading dictionaries: " << exception.what();
        return SyncResult::SYNC_ERROR;
    }

    catch( const SyncCancelException& )
    {
        SYNCLOG_INFO << "Dictionary list download canceled";
        return SyncResult::SYNC_CANCELED;
    }
}


SyncClient::SyncResult SyncClient::DownloadDictionary(const std::string& dictionary_name, std::string& dictionary_text)
{
    try
    {
        if( m_syncService == nullptr )
            throw SyncError(100132);

        const SyncListenerServerSaverAndCloser sync_listener_saver_and_closer(m_syncService.get(), m_syncListener, 100107, dictionary_name.c_str());

        SYNCLOG_INFO << "Downloading dictionary file: " << dictionary_name;
        dictionary_text = m_syncService->GetDictionary(dictionary_name);

        SYNCLOG_INFO << "Downloading dictionary file complete";
        return SyncResult::SYNC_OK;
    }

    catch( const SyncError& exception )
    {
        if( m_syncListener != nullptr )
            m_syncListener->ReportError(exception);

        SYNCLOG_ERROR << "Error downloading dictionary: " << exception.what();
        return SyncResult::SYNC_ERROR;
    }

    catch( const SyncCancelException& )
    {
        SYNCLOG_INFO << "Dictionary download canceled";
        return SyncResult::SYNC_CANCELED;
    }
}


void SyncClient::UploadDictionaryWorker(const std::string& dictionary_file_path)
{
    ASSERT(m_syncService != nullptr);

    SYNCLOG_INFO << "Uploading dictionary file: " << dictionary_file_path;

    try
    {
        CDataDict dictionary;
        dictionary.Open(dictionary_file_path, true);
        m_syncService->PutDictionary(dictionary);
    }

    catch( const CSProException& exception )
    {
        SYNCLOG_ERROR << "Error uploading dictionary: " << exception.what();
        throw SyncError(100143, exception);
    }

    SYNCLOG_INFO << "Upload dictionary completed";
}


SyncClient::SyncResult SyncClient::UploadDictionary(const std::string& dictionary_file_path)
{
    try
    {
        if( m_syncService == nullptr )
            throw SyncError(100132);

        const SyncListenerServerSaverAndCloser sync_listener_saver_and_closer(m_syncService.get(), m_syncListener,
                                                                              100108, Path::GetFilename(dictionary_file_path).c_str());

        UploadDictionaryWorker(dictionary_file_path);

        return SyncResult::SYNC_OK;
    }

    catch( const SyncError& exception )
    {
        if( m_syncListener != nullptr )
            m_syncListener->ReportError(exception);

        return SyncResult::SYNC_ERROR;
    }

    catch( const SyncCancelException& )
    {
        SYNCLOG_INFO << "Dictionary upload canceled";
        return SyncResult::SYNC_CANCELED;
    }
}


SyncClient::SyncResult SyncClient::GetFilesWithWildcard(const std::string& from_path, const std::string& to_path)
{
    ASSERT(!to_path.empty());

    try
    {
        if( m_syncService == nullptr )
            throw SyncError(100132);

        const SyncListenerServerSaverAndCloser sync_listener_saver_and_closer(m_syncService.get(), m_syncListener, 100107, from_path.c_str());

        SYNCLOG_INFO << "Get files from: " << from_path << " to: " << to_path;

        // Get the directory listing
        std::string from_directory = PortableFunctions::PathGetDirectory(from_path);

        // If directory is empty send root directory since sending an empty string
        // gives us the server info.
        if( from_directory.empty() )
            from_directory = "/";

        const std::vector<FileInfo> directory_listing = m_syncService->GetDirectoryListing(from_directory, true);

        // Download each file in the directory that matches the pattern
        const std::regex from_regex(CreateRegularExpressionFromFileSpec(PortableFunctions::PathGetFilename(from_path)));

        for( const FileInfo& file_info : directory_listing )
        {
            if( file_info.GetType() == FileInfo::FileType::File &&
                std::regex_match(file_info.GetName(), from_regex) )
            {
                const std::string evaluated_to_path = Path::Combine(to_path, file_info.GetName());
                const std::string existing_file_md5 = PortableFunctions::FileMd5(evaluated_to_path);

                if( existing_file_md5.empty() || !SO::EqualsNoCase(existing_file_md5, file_info.GetMd5()) )
                {
                    SYNCLOG_INFO << "Downloading file " << file_info.GetName();
                    DownloadOneFile(file_info.GetDirectory() + file_info.GetName(), evaluated_to_path, existing_file_md5);
                }

                else
                {
                    SYNCLOG_INFO << "Skipping file " << file_info.GetName() << " version on sync service matches local version";
                }
            }
        }

        SYNCLOG_INFO << "Sync file completed";

        return SyncResult::SYNC_OK;
    }

    catch( const std::regex_error& )
    {
        SYNCLOG_INFO << "Syncfile has an invalid wildcard specification";
        throw SyncError(100113, PortableFunctions::PathGetFilename(from_path));
    }

    catch( const SyncError& exception )
    {
        if( m_syncListener != nullptr )
            m_syncListener->ReportError(exception);

        SYNCLOG_ERROR << "Error downloading files: " << exception.what();
        return SyncResult::SYNC_ERROR;
    }

    catch( const SyncCancelException& )
    {
        SYNCLOG_INFO << "File download canceled";
        return SyncResult::SYNC_CANCELED;
    }
}


SyncClient::SyncResult SyncClient::GetFile(const std::string& from_path, cs::cref_optional<std::string> to_path)
{
    ASSERT(to_path.has_value() && !to_path->empty());

    try
    {
        if( m_syncService == nullptr )
            throw SyncError(100132);

        const SyncListenerServerSaverAndCloser sync_listener_saver_and_closer(m_syncService.get(), m_syncListener, 100107, from_path.c_str());

        // if the to path is a directory, add the filename
        if( Path::IsSlashChar(to_path->back()) || PortableFunctions::FileIsDirectory(*to_path) )
            to_path = Path::Combine(*to_path, PortableFunctions::PathGetFilename(from_path));

        ASSERT(*to_path == PortableFunctions::PathToNativeSlash(*to_path));

        const std::string existing_file_md5 = PortableFunctions::FileMd5(*to_path);

        SYNCLOG_INFO << "Downloading file: " << from_path << " to: " << *to_path;

        DownloadOneFile(from_path, *to_path, existing_file_md5);

        SYNCLOG_INFO << "Sync file completed";
        return SyncResult::SYNC_OK;
    }

    catch( const SyncError& exception )
    {
        if( m_syncListener != nullptr )
            m_syncListener->ReportError(exception);

        SYNCLOG_ERROR << "Error downloading file: " << exception.what();
        return SyncResult::SYNC_ERROR;
    }

    catch( const SyncCancelException& )
    {
        SYNCLOG_INFO << "File download canceled";
        return SyncResult::SYNC_CANCELED;
    }
}


void SyncClient::DownloadOneFile(const std::string& from_path, const std::string& to_path, const std::string& existing_file_md5)
{
    if( m_syncListener != nullptr )
        m_syncListener->Progress(0, 100107, PortableFunctions::PathGetFilename(to_path).c_str());

    // Store the downloaded file in a temporary file first so we don't
    // corrupt the original if the download fails. Put the temp file
    // in same directory as destination file so that we can rename
    // it after. On Android it is not possible to rename files
    // on different mount points and there is gaurantee that system
    // temp is on same mount point as dest file.

    const std::string to_directory = PortableFunctions::PathGetDirectory(to_path);

    // Make sure that the directory we are saving the file to exists
    PortableFunctions::PathMakeDirectories(to_directory);

    TemporaryFile temporary_file(to_directory);

    if( m_syncService->GetFile(from_path, temporary_file.GetPath(), existing_file_md5) )
    {
        // Delete the existing file first, otherwise rename will fail
        PortableFunctions::FileDelete(to_path);

        // Move the temp file to correct location
        if( !temporary_file.Rename_noexcept(to_path) )
            throw SyncError(100109, to_path);
    }
}


SyncClient::SyncResult SyncClient::PutFilesWithWildcard(const std::string& from_path, cs::cref_optional<std::string> to_path)
{
    ASSERT(to_path.has_value() && !to_path->empty() && *to_path == PortableFunctions::PathToForwardSlash(*to_path));

    try
    {
        if( m_syncService == nullptr )
            throw SyncError(100132);

        const SyncListenerServerSaverAndCloser sync_listener_saver_and_closer(m_syncService.get(), m_syncListener, 100108, from_path.c_str());

        SYNCLOG_INFO << "Put files from: " << from_path << " to: " << *to_path;

        if( !PortableFunctions::FileIsDirectory(PortableFunctions::PathGetDirectory(from_path)) )
            throw SyncError(100113, from_path);

        // Make sure that destination path ends in "/" so that it gets treated as a directory and not a file
        if( to_path->back() != '/' )
            to_path = PortableFunctions::PathEnsureTrailingForwardSlash(*to_path);

        for( const std::string& file_path : DirectoryLister::GetFilePathsWithPossibleWildcard(from_path, true) )
            UploadOneFile(file_path, *to_path);

        SYNCLOG_INFO << "File upload completed";
        return SyncResult::SYNC_OK;
    }

    catch( const SyncError& exception )
    {
        if( m_syncListener != nullptr )
            m_syncListener->ReportError(exception);

        SYNCLOG_ERROR << "Error uploading files: " << exception.what();
        return SyncResult::SYNC_ERROR;
    }

    catch( const SyncCancelException& )
    {
        SYNCLOG_INFO << "File upload canceled";
        return SyncResult::SYNC_CANCELED;
    }
}


SyncClient::SyncResult SyncClient::PutFile(const std::string& from_path, const std::string& to_path)
{
    try
    {
        if( m_syncService == nullptr )
            throw SyncError(100132);

        const SyncListenerServerSaverAndCloser sync_listener_saver_and_closer(m_syncService.get(), m_syncListener, 100108, from_path.c_str());

        UploadOneFile(from_path, to_path);

        SYNCLOG_INFO << "File upload completed";
        return SyncResult::SYNC_OK;
    }

    catch( const SyncError& exception )
    {
        if( m_syncListener != nullptr )
            m_syncListener->ReportError(exception);

        SYNCLOG_ERROR << "Error uploading file: " << exception.what();
        return SyncResult::SYNC_ERROR;
    }

    catch( const SyncCancelException& )
    {
        SYNCLOG_INFO << "File upload canceled";
        return SyncResult::SYNC_CANCELED;
    }
}


void SyncClient::UploadOneFile(const std::string& from_path, cs::cref_optional<std::string> to_path)
{
    ASSERT(to_path.has_value() && !to_path->empty() && *to_path == PortableFunctions::PathToForwardSlash(*to_path));

    if( m_syncListener != nullptr )
        m_syncListener->Progress(0, 100108, PortableFunctions::PathGetFilename(from_path).c_str());

    if( !PortableFunctions::FileIsRegular(from_path) )
        throw SyncError(100118, from_path);

    // if the to path is a directory, add the filename
    if( to_path->back() == '/' )
        to_path = PortableFunctions::PathAppendForwardSlashToPath(*to_path, PortableFunctions::PathGetFilename(from_path));

    SYNCLOG_INFO << "Uploading file: " << from_path << " to: " << *to_path;

    // Send request to sync service
    m_syncService->PutFile(from_path, *to_path);
}


void SyncClient::DownloadAndInstallPackage(const ApplicationPackageManager& application_package_manager, const std::string& package_name,
                                           const ApplicationPackage* const current_package, const std::string* const current_package_signature)
{
    const TemporaryFile temporary_zip_file;

    if( !m_syncService->DownloadApplicationPackage(package_name, temporary_zip_file.GetPath(), current_package, current_package_signature) )
    {
        SYNCLOG_INFO << "Application package " << package_name << " already up to date";
        return;
    }

    application_package_manager.InstallApplication(package_name, temporary_zip_file.GetPath(), current_package);
}


SyncClient::SyncResult SyncClient::DeleteDictionary(const std::string& dictionary_name)
{
    try
    {
        if( m_syncService == nullptr )
            throw SyncError(100132);

        const SyncListenerServerSaverAndCloser sync_listener_saver_and_closer(m_syncService.get(), m_syncListener, 100123, dictionary_name.c_str());

        SYNCLOG_INFO << "Deleting dictionary " << dictionary_name;
        m_syncService->DeleteDictionary(dictionary_name);

        SYNCLOG_INFO << "Delete dictionary completed";
        return SyncResult::SYNC_OK;
    }

    catch( const SyncError& exception )
    {
        if( m_syncListener != nullptr )
            m_syncListener->ReportError(exception);

        SYNCLOG_ERROR << "Error deleting dictionary: " << exception.what();
        return SyncResult::SYNC_ERROR;
    }

    catch( const SyncCancelException& )
    {
        SYNCLOG_INFO << "Dictionary delete canceled";
        return SyncResult::SYNC_CANCELED;
    }
}


SyncClient::SyncResult SyncClient::ListApplicationPackages(std::vector<ApplicationPackage>& packages)
{
    try
    {
        if( m_syncService == nullptr )
            throw SyncError(100132);

        const SyncListenerServerSaverAndCloser sync_listener_saver_and_closer(m_syncService.get(), m_syncListener, 100136);

        SYNCLOG_INFO << "Downloading application deployment package list";
        packages = m_syncService->ListApplicationPackages();
        SYNCLOG_INFO << "Found " << packages.size() << " application packages";

        return SyncResult::SYNC_OK;
    }

    catch( const SyncError& exception )
    {
        if( m_syncListener != nullptr )
            m_syncListener->ReportError(exception);

        SYNCLOG_ERROR << "Error downloading package list: " << exception.what();
        return SyncResult::SYNC_ERROR;
    }

    catch( const SyncCancelException& )
    {
        SYNCLOG_INFO << "Package listing canceled";
        return SyncResult::SYNC_CANCELED;
    }
}


SyncClient::SyncResult SyncClient::DownloadApplicationPackage(const ApplicationPackageManager& application_package_manager, const std::string& package_name, const bool force_full_install)
{
    try
    {
        if( m_syncService == nullptr )
            throw SyncError(100132);

        const SyncListenerServerSaverAndCloser sync_listener_saver_and_closer(m_syncService.get(), m_syncListener, 100107, package_name.c_str());

        SYNCLOG_INFO << "Downloading package " << package_name;

        if( force_full_install )
        {
            DownloadAndInstallPackage(application_package_manager, package_name, nullptr, nullptr);
        }

        else
        {
            const std::unique_ptr<ApplicationPackageManager::ApplicationWithSignature> current_package = application_package_manager.GetInstalledApplicationPackageWithSignature(package_name);

            if( current_package != nullptr )
            {
                DownloadAndInstallPackage(application_package_manager, package_name, &current_package->package, &current_package->signature);
            }

            else
            {
                DownloadAndInstallPackage(application_package_manager, package_name, nullptr, nullptr);
            }
        }

        SYNCLOG_INFO << "Package download completed";
        return SyncResult::SYNC_OK;
    }

    catch( const SyncError& exception )
    {
        if( m_syncListener != nullptr )
            m_syncListener->ReportError(exception);

        SYNCLOG_ERROR << "Error downloading package: " << exception.what();
        return SyncResult::SYNC_ERROR;
    }

    catch( const SyncCancelException& )
    {
        SYNCLOG_INFO << "Package download canceled";
        return SyncResult::SYNC_CANCELED;
    }
}


SyncClient::SyncResult SyncClient::UploadApplicationPackage(const std::string& local_package_zip_file_path, const std::string& package_name,
                                                            const std::string& package_spec_json, const std::string& directory_for_dictionary_upload_evaluation)
{
    try
    {
        if( m_syncService == nullptr )
            throw SyncError(100132);

        const SyncListenerServerSaverAndCloser sync_listener_saver_and_closer(m_syncService.get(), m_syncListener, 100108, package_name.c_str());

        SYNCLOG_INFO << "Uploading package " << package_name;

        m_syncService->UploadApplicationPackage(local_package_zip_file_path, package_name, package_spec_json);

        // upload dictionaries
        if( !directory_for_dictionary_upload_evaluation.empty() )
        {
            try
            {
                const ApplicationPackage application_package = JsonConverter::CreateApplicationPackageFromJson(Json::Parse(package_spec_json));

                for( const ApplicationPackage::Dictionary& app_package_dictionary : application_package.GetDictionaries() )
                {
                    if( app_package_dictionary.upload_for_sync )
                    {
                        const std::string dictionary_file_path = MakeFullPath(directory_for_dictionary_upload_evaluation, app_package_dictionary.path);
                        UploadDictionaryWorker(dictionary_file_path);
                    }
                }
            }

            catch( const CSProException& exception )
            {
                SYNCLOG_ERROR << "Error uploading dictionary as part of package " << package_name << ": " << exception.what();
                throw;
            }
        }

        SYNCLOG_INFO << "Package upload completed";
        return SyncResult::SYNC_OK;
    }

    catch( const SyncError& exception )
    {
        if( m_syncListener != nullptr )
            m_syncListener->ReportError(exception);

        SYNCLOG_ERROR << "Error uploading package: " << exception.what();
        return SyncResult::SYNC_ERROR;
    }

    catch( const SyncCancelException& )
    {
        SYNCLOG_INFO << "Package upload canceled";
        return SyncResult::SYNC_CANCELED;
    }
}


SyncClient::SyncResult SyncClient::UpdateApplication(const ApplicationPackageManager& application_package_manager, const std::string& application_file_path)
{
    try
    {
        const std::unique_ptr<ApplicationPackageManager::ApplicationWithSignature> current_package = application_package_manager.GetApplicationPackageWithSignatureFromApplicationDirectory(application_file_path);

        if( current_package == nullptr )
        {
            SYNCLOG_ERROR << "No installed package found when trying to update application package from " << application_file_path;
            throw SyncError(100151);
        }

        if( m_syncService == nullptr )
            throw SyncError(100132);

        const SyncListenerServerSaverAndCloser sync_listener_saver_and_closer(m_syncService.get(), m_syncListener,
                                                                              100107, current_package->package.GetName().c_str());

        SYNCLOG_INFO << "Checking for updates for package " << current_package->package.GetName();

        // Download and install the new package
        DownloadAndInstallPackage(application_package_manager, current_package->package.GetName(), &current_package->package, &current_package->signature);

        SYNCLOG_INFO << "Done checking for updates for package " << current_package->package.GetName();

        return SyncResult::SYNC_OK;
    }

    catch( const SyncError& exception )
    {
        if( m_syncListener != nullptr )
            m_syncListener->ReportError(exception);

        SYNCLOG_ERROR << "Error downloading package list: " << exception.what();
        return SyncResult::SYNC_ERROR;
    }

    catch( const SyncCancelException& )
    {
        SYNCLOG_INFO << "Package listing canceled";
        return SyncResult::SYNC_CANCELED;
    }
}


SyncClient::SyncResult SyncClient::DeleteApplication(const std::string& package_name)
{
    try
    {
        if( m_syncService == nullptr )
            throw SyncError(100132);

        const SyncListenerServerSaverAndCloser sync_listener_saver_and_closer(m_syncService.get(), m_syncListener, 100123, package_name.c_str());

        SYNCLOG_INFO << "Deleting package " << package_name;
        m_syncService->DeleteApplication(package_name);

        SYNCLOG_INFO << "Delete package completed";
        return SyncResult::SYNC_OK;
    }

    catch( const SyncError& exception )
    {
        if( m_syncListener != nullptr )
            m_syncListener->ReportError(exception);

        SYNCLOG_ERROR << "Error deleting package: " << exception.what();
        return SyncResult::SYNC_ERROR;
    }

    catch( const SyncCancelException& )
    {
        SYNCLOG_INFO << "Package delete canceled";
        return SyncResult::SYNC_CANCELED;
    }
}


std::optional<JsonNode> SyncClient::SendSyncMessage(const SyncMessage& sync_message)
{
    try
    {
        if( m_syncService == nullptr )
            throw SyncError(100132);

        SyncRunner sync_runner(m_syncListener);
        return sync_runner.SendSyncMessage(*m_syncService, m_deviceId, sync_message, m_paradataLogger.get());
    }

    catch( const SyncError& exception )
    {
        if( m_syncListener != nullptr )
            m_syncListener->ReportError(exception);

        return std::nullopt;
    }
}


SyncClient::SyncResult SyncClient::SyncParadata(const SyncDirection sync_direction)
{
    ASSERT(Paradata::Logger::IsOpen());

    try
    {
        if( m_syncService == nullptr )
            throw SyncError(100132);

        const std::unique_ptr<Paradata::Syncer> paradata_syncer = Paradata::Logger::GetSyncer();
        ASSERT(paradata_syncer != nullptr);

        SyncRunner sync_runner(m_syncListener);
        sync_runner.SyncParadata(*m_syncService, *paradata_syncer, sync_direction, m_paradataLogger.get());

        return SyncResult::SYNC_OK;
    }

    catch( const SyncError& exception )
    {
        if( m_syncListener != nullptr )
            m_syncListener->ReportError(exception);
    }

    catch( const CSProException& exception )
    {
        if( m_syncListener != nullptr )
            m_syncListener->ReportError(8295, exception.what());

        if( dynamic_cast<const SyncCancelException*>(&exception) != nullptr )
            return SyncResult::SYNC_CANCELED;
    }

    return SyncResult::SYNC_ERROR;
}
