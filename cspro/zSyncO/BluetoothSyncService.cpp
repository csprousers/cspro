#include "stdafx.h"
#include "BluetoothSyncService.h"
#include "BluetoothChunk.h"
#include "BluetoothObexConnection.h"
#include "CaseObservable.h"
#include "IBluetoothAdapter.h"
#include "IDataChunk.h"
#include "JsonConverter.h"
#include "SyncMessage.h"
#include <zUtilO/Versioning.h>
#include <zNetwork/SyncCustomHeaders.h>
#include <zDictO/DDClass.h>
#include <zDataO/ISyncableDataRepository.h>
#include <zEngineO/Messages/EngineMessages.h>


namespace
{
    std::streamsize getStreamSize(std::istream& iStream)
    {
        iStream.seekg(0, std::ios::end);
        const std::streamsize streamSize = iStream.tellg();
        iStream.seekg(0, std::ios::beg);

        return streamSize;
    }
}


BluetoothSyncService::BluetoothSyncService(std::shared_ptr<IBluetoothAdapter> pAdapter, BluetoothDeviceInfo device_info)
    :   m_pAdapter(std::move(pAdapter)),
        m_pObexConnection(std::make_unique<BluetoothObexConnection>(m_pAdapter)),
        m_deviceInfo(std::move(device_info)),
        m_bWasBluetoothEnabled(m_pAdapter != nullptr && m_pAdapter->IsEnabled()),
        m_serverCSProVersion(0)
{
    ASSERT(m_pAdapter != nullptr);
}


BluetoothSyncService::BluetoothSyncService(std::shared_ptr<IBluetoothAdapter> pAdapter,  std::shared_ptr<LoginAccessor> login_accessor)
    :   m_pAdapter(std::move(pAdapter)),
        m_pObexConnection(std::make_unique<BluetoothObexConnection>(m_pAdapter)),
        m_loginAccessor(std::move(login_accessor)),
        m_bWasBluetoothEnabled(m_pAdapter != nullptr && m_pAdapter->IsEnabled()),
        m_serverCSProVersion(0)
{
    ASSERT(m_pAdapter != nullptr && m_loginAccessor != nullptr);
}


BluetoothSyncService::~BluetoothSyncService()
{
    m_pObexConnection.reset();
    if (m_pAdapter != nullptr && !m_bWasBluetoothEnabled)
        m_pAdapter->Disable();
}


std::shared_ptr<ConnectResponse> BluetoothSyncService::Connect()
{
    // No bluetooth support on this device
    if (m_pAdapter == nullptr)
        throw SyncError(MGF::sync_feature_not_supported_100146, "Bluetooth");

    if (!m_pAdapter->IsEnabled())
        m_pAdapter->Enable();

    if (m_loginAccessor != nullptr) {
        std::optional<BluetoothDeviceInfo> device_info = m_loginAccessor->ChooseBluetoothDevice();
        if (!device_info.has_value())
            throw SyncCancelException();
        m_deviceInfo = std::move(*device_info);
    }

    bool status = m_pObexConnection->connect(m_deviceInfo);

    if (!status) {
        throw SyncError(100100, m_deviceInfo.name);
    }
    else {
        // empty path is "default" object - for CSPro Obex
        // this is the server info
        std::ostringstream ss;
        HeaderList responseHeaders;
        m_pObexConnection->get(L"", L"", HeaderList(), ss, responseHeaders);
        const std::string connectResponse = ss.str();
        try {
            ConnectResponse response = Json::FromJson<ConnectResponse>(connectResponse);
            response.SetServerName(m_deviceInfo.name);
            m_serverCSProVersion = response.GetApiVersion();
            return std::make_unique<ConnectResponse>(std::move(response));
        }
        catch (const JsonParseException& e) {
            SYNCLOG_ERROR << "Invalid server response: " << e.what();
            SYNCLOG_ERROR << connectResponse;
            throw SyncError(100121);
        }
    }
}


void BluetoothSyncService::Disconnect()
{
    m_pObexConnection->disconnect();
}


SyncGetResponse BluetoothSyncService::GetCases(const std::shared_ptr<const CaseAccess> case_access,
                                               const DeviceId& device_id, const std::string& universe, const std::string& last_server_revision,
                                               const std::string& last_case_uuid, const std::vector<std::string>& excluded_revisions)
{
    ASSERT(case_access != nullptr);

    std::string server_revision = last_server_revision;

    SYNCLOG_INFO << "Start case download with chunk size of " << GetChunk().GetCaseSize();

    if( !last_case_uuid.empty() )
        SYNCLOG_INFO << "Download chunk starting from case " << last_case_uuid;

    while( true )
    {

        HeaderList responseHeaders;
        std::string responseBody;

        const ObexResponseCode result = GetDataChunk(case_access->GetDataDict(), device_id,
                                                     universe, server_revision, last_case_uuid,
                                                     excluded_revisions, responseHeaders, responseBody);

        if( !ZLib::Inflate(responseBody) )
            throw SyncError(100139);

        SYNCLOG_INFO << "Status : " << ObexResponseCodeToString(result);

        if( result == OBEX_OK || result == OBEX_PARTIAL_CONTENT)
        {
            try
            {
                SyncCaseSerializer sync_case_serializer = SyncCaseSerializer::CreateFromCSProVersion(case_access, m_serverCSProVersion);
                std::vector<std::shared_ptr<Case>> server_cases = sync_case_serializer.ParseSyncableCaseData(responseBody);

                SYNCLOG_INFO << "Cases in chunk " << server_cases.size() << ". Server count = " << responseHeaders.GetValue(SyncCustomHeaders::RANGE_COUNT_HEADER);

                const SyncGetResponse::SyncGetResult responseResult = ( result == OBEX_OK ) ? SyncGetResponse::SyncGetResult::Complete :
                                                                                              SyncGetResponse::SyncGetResult::MoreData;
                server_revision = responseHeaders.GetValue("Etag");

                return SyncGetResponse(responseResult,
                                       std::make_unique<CaseObservable>(rxcpp::observable<>::iterate(std::move(server_cases))),
                                       std::move(server_revision));
            }

            catch( const JsonParseException& exception )
            {
                SYNCLOG_ERROR << "Invalid server response: " << exception.what();
                SYNCLOG_ERROR << responseBody;
                throw SyncError(100121);
            }
        }

        else if( result == OBEX_PRECONDITION_FAILED )
        {
            return SyncGetResponse::SyncGetResult::RevisionNotFound;
        }

        else if( result == OBEX_CANCELED_BY_USER )
        {
            throw SyncError(100122);
        }

        else
        {
            throw SyncConnectionError(ObexResponseCodeToString(result));
        }
    }
}


SyncPutResponse BluetoothSyncService::PutCases(const std::shared_ptr<const CaseAccess> case_access,
                                               const cs::span<const Case* const> cases, const SyncBinaryDataUploadManager* const sync_binary_data_upload_manager,
                                               const DeviceId& device_id, const std::string& universe, const std::string& last_server_revision)
{
    HeaderList requestHeaders;

    requestHeaders.Add(SyncCustomHeaders::DICTIONARY_NAME, case_access->GetDataDict().GetName())
                  .AddIfNotBlank(SyncCustomHeaders::IF_REVISION_EXISTS_HEADER, last_server_revision)
                  .Add(SyncCustomHeaders::DEVICE_ID_HEADER, device_id)
                  .AddIfNotBlank(SyncCustomHeaders::UNIVERSE_HEADER, universe)
                  .Add_UserAgent_CSProSyncClient();

    SyncCaseSerializer sync_case_serializer = SyncCaseSerializer::CreateFromCSProVersion(case_access, m_serverCSProVersion);
    std::string case_data = sync_case_serializer.GetSyncableCaseData(cases, sync_binary_data_upload_manager);

    if( !ZLib::Deflate(case_data) )
        throw SyncError(100138);

    std::istringstream postDataStream(case_data);
    std::ostringstream oss;
    HeaderList responseHeaders;

    const ObexResponseCode result = m_pObexConnection->put(OBEX_SYNC_DATA_MEDIA_TYPE, UTF8_TODO::GetCString(case_access->GetDataDict().GetSyncableName()), false,
                                                           postDataStream, case_data.size(), requestHeaders, oss, responseHeaders);

    if( result == OBEX_OK )
    {
        return SyncPutResponse(SyncPutResponse::SyncPutResult::Complete, responseHeaders.GetValue("Etag"));
    }

    else if( result == OBEX_PRECONDITION_FAILED )
    {
        return SyncPutResponse(SyncPutResponse::SyncPutResult::RevisionNotFound);
    }

    else if( result == OBEX_CANCELED_BY_USER )
    {
        throw SyncError(100122);
    }

    else
    {
        throw SyncConnectionError(ObexResponseCodeToString(result));
    }
}


IDataChunk& BluetoothSyncService::GetChunk()
{
    return m_pObexConnection->getChunk();
}


std::vector<SyncDictionaryInfo> BluetoothSyncService::GetDictionaries()
{
    throw SyncNotSupportedOperationError();
}


std::string BluetoothSyncService::GetDictionary(const std::string& /*dictionary_name*/)
{
    throw SyncNotSupportedOperationError();
}


void BluetoothSyncService::PutDictionary(const CDataDict& /*dictionary*/)
{
    throw SyncNotSupportedOperationError();
}


void BluetoothSyncService::DeleteDictionary(const std::string& /*dictionary_name*/)
{
    throw SyncNotSupportedOperationError();
}


std::vector<FileInfo> BluetoothSyncService::GetDirectoryListing(const std::string& remote_path, const bool request_file_md5s)
{
    HeaderList requestHeaders;
    requestHeaders.Add_UserAgent_CSProSyncClient();
    requestHeaders.Add(SyncCustomHeaders::GET_FILE_MD5_HEADER, Json::ToJson(request_file_md5s));

    std::ostringstream ss;
    HeaderList responseHeaders;

    const ObexResponseCode result = m_pObexConnection->get(OBEX_DIRECTORY_LISTING_MEDIA_TYPE, UTF8_TODO::GetCString(remote_path), requestHeaders, ss, responseHeaders);

    switch( result )
    {
        case OBEX_OK:               break;
        case OBEX_CANCELED_BY_USER: throw SyncError(100122);
        default:                    throw SyncConnectionError("Server responded with error %d", static_cast<int>(result));
    }

    const std::string dirlistResponse = ss.str();

    try
    {
        return Json::Parse(dirlistResponse).GetArray().GetVector<FileInfo>();
    }

    catch( const JsonParseException& exception )
    {
        SYNCLOG_ERROR << "Invalid server response: " << exception.what();
        SYNCLOG_ERROR << dirlistResponse;
        throw SyncError(100121);
    }
}


ObexResponseCode BluetoothSyncService::GetDataChunk(const CDataDict& dictionary, const DeviceId& device_id, const std::string& universe,
                                                    const std::string& last_server_revision, const std::string& last_case_uuid,
                                                    const std::vector<std::string>& excludeRevisions,
                                                    HeaderList& responseHeaders, std::string& responseBody)
{
    std::ostringstream oss;
    HeaderList requestHeaders;

    requestHeaders.Add(SyncCustomHeaders::DICTIONARY_NAME, dictionary.GetName())
                  .AddIfNotBlank(SyncCustomHeaders::IF_REVISION_EXISTS_HEADER, last_server_revision)
                  .Add(SyncCustomHeaders::DEVICE_ID_HEADER, device_id)
                  .AddIfNotBlank(SyncCustomHeaders::UNIVERSE_HEADER, universe)
                  .AddIfNotBlank(SyncCustomHeaders::START_AFTER_HEADER, last_case_uuid);

    if( !excludeRevisions.empty() )
        requestHeaders.Add(SyncCustomHeaders::EXCLUDE_REVISIONS_HEADER, SO::CreateSingleString(excludeRevisions, ","));

    requestHeaders.Add(SyncCustomHeaders::RANGE_COUNT_HEADER, IntToString(GetChunk().GetCaseSize()));
    requestHeaders.Add_UserAgent_CSProSyncClient();

    ObexResponseCode result = m_pObexConnection->get(OBEX_SYNC_DATA_MEDIA_TYPE, UTF8_TODO::GetCString(dictionary.GetSyncableName()), requestHeaders, oss, responseHeaders);
    responseBody = oss.str();

    return result;
}


ObexResponseCode BluetoothSyncService::getFileWorker(const CString& type, const CString& remotePath, const CString& localPath,
                                                     std::optional<HeaderList> requestHeaders/* = std::nullopt*/)
{
    if( !requestHeaders.has_value() )
    {
        requestHeaders = HeaderList();
        requestHeaders->Add_UserAgent_CSProSyncClient();
    }

    HeaderList responseHeaders;

    std::ofstream fileStream(localPath, std::ios::binary);

    while( true )
    {
        // The file is modified and the last file chunk has not been processed
        std::ostringstream response;
        ObexResponseCode getResult = m_pObexConnection->get(type, remotePath, *requestHeaders, response, responseHeaders);

        if( getResult == OBEX_OK || getResult == OBEX_IS_LAST_FILE_CHUNK )
        {
            std::istringstream inStrm(response.str());

            if( !ZLib::Inflate(inStrm, fileStream) )
                throw SyncError(100139);
        }

        else if( getResult == OBEX_CANCELED_BY_USER )
            throw SyncError(100122);

        if( getResult != OBEX_OK )
            return getResult;
    }
}


bool BluetoothSyncService::GetFile(const std::string& remote_file_path, const std::string& local_file_path, const std::string& existing_file_md5)
{
    HeaderList requestHeaders;
    requestHeaders.Add_UserAgent_CSProSyncClient();

    if( !existing_file_md5.empty() )
        requestHeaders.Add_IfNoneMatch(existing_file_md5);

    ObexResponseCode getResult = getFileWorker(OBEX_BINARY_FILE_MEDIA_TYPE, UTF8_TODO::GetCString(remote_file_path), UTF8_TODO::GetCString(local_file_path), requestHeaders);

    return ( getResult == OBEX_IS_LAST_FILE_CHUNK ) ? true :
           ( getResult == OBEX_NOT_MODIFIED )       ? false :
                                                      throw SyncConnectionError(ObexResponseCodeToString(getResult));
}


std::unique_ptr<TemporaryFile> BluetoothSyncService::GetFileIfExists(const std::string& /*remote_file_path*/)
{
    // this method is only used by the FileBasedParadataSyncer ... implement it eventually if necessary
    throw ProgrammingErrorException();
}


void BluetoothSyncService::putFileWorker(const CString& type, const CString& localPath, const CString& remotePath)
{
    HeaderList requestHeaders;
    requestHeaders.Add_UserAgent_CSProSyncClient();

    if (m_syncListener != nullptr) {
        m_syncListener->SetProgressTotal(PortableFunctions::FileSize(localPath));
    }

    std::ifstream fileStream(localPath, std::ios::binary);
    const size_t bufferSize = BluetoothDataChunk::FileChunkSize;
    std::streamsize chunkSize = bufferSize;
    std::vector<char> chunk(bufferSize, 0);

    while (!fileStream.eof()) {
        fileStream.read(chunk.data(), bufferSize);

        bool isLastFileChunk = false;
        if (fileStream.bad()) {
            throw SyncError(100134);
        }
        else if (fileStream.eof()) {
            isLastFileChunk = true;
            chunkSize = fileStream.gcount();
        }

        std::string stringChunk(chunk.data(), static_cast<size_t>(chunkSize));

        if (!ZLib::Deflate(stringChunk)) {
            throw SyncError(100138);
        }

        std::istringstream iStreamChunk(stringChunk);
        std::streamsize iStreamChunkSize = getStreamSize(iStreamChunk);

        std::ostringstream oss;
        HeaderList responseHeaders;

        ObexResponseCode putResult = m_pObexConnection->put(type, remotePath, isLastFileChunk,
            iStreamChunk, static_cast<size_t>(iStreamChunkSize), requestHeaders, oss, responseHeaders);

        switch (putResult) {
        case OBEX_OK:
            break;
        case OBEX_CANCELED_BY_USER:
            throw SyncError(100122);
            break;
        default:
            throw SyncConnectionError("Server responded with error %d", static_cast<int>(putResult));
        }

        if (m_syncListener != nullptr) {
            m_syncListener->AddToProgressPreviousStepsTotal(chunkSize);
        }
    }
}


void BluetoothSyncService::PutFile(const std::string& local_file_path, const std::string& remote_path)
{
    putFileWorker(OBEX_BINARY_FILE_MEDIA_TYPE, UTF8_TODO::GetCString(local_file_path), UTF8_TODO::GetCString(remote_path));
}


std::vector<ApplicationPackage> BluetoothSyncService::ListApplicationPackages()
{
    return std::vector<ApplicationPackage>();
}


bool BluetoothSyncService::DownloadApplicationPackage(const std::string& package_name, const std::string& local_file_path,
                                                      const ApplicationPackage* const current_package, const std::string* const current_package_signature)
{
    HeaderList responseHeaders;
    HeaderList requestHeaders;
    requestHeaders.Add_UserAgent_CSProSyncClient();

    if (current_package != nullptr) {
        // For an existing package we add the build time and the package file list in headers
        if( current_package_signature != nullptr ) {
            requestHeaders.Add_IfNoneMatch(*current_package_signature);
        }
        requestHeaders.Add(SyncCustomHeaders::APP_PACKAGE_BUILD_TIME_HEADER, DateTime::TimeToRFC3339(current_package->GetBuildTime()));

        // Convert the file list to base64 encoded, compressed json
        std::vector<ApplicationPackage::File> packageFiles = current_package->GetFiles();
        // No need to send the only on first install flag to server, setting to false removes from JSON
        for (ApplicationPackage::File& f : packageFiles) {
            f.only_on_first_install = false;
        }

        std::string fileJson = JsonConverter::ToJson(packageFiles);
        if (!ZLib::Deflate(fileJson)) {
            SYNCLOG_ERROR << "Failed to compress package json, fallback to full package download";
        } else {
            requestHeaders.AddAsBase64(SyncCustomHeaders::APP_PACKAGE_FILES_HEADER, fileJson);
        }
    }

    TemporaryFile tmpZip(PortableFunctions::PathGetDirectory(local_file_path));
    std::ofstream fileStream(UTF8_TODO::GetWide(tmpZip.GetPath()).c_str(), std::ios::binary);

    ObexResponseCode getResult = OBEX_OK;
    while (getResult == OBEX_OK) {
        // The file is modified and the last file chunk has not been processed
        std::ostringstream response;
        getResult = m_pObexConnection->get(OBEX_SYNC_APP_MEDIA_TYPE, UTF8_TODO::GetCString(package_name), requestHeaders, response, responseHeaders);

        switch (getResult) {
            case OBEX_OK:
            case OBEX_IS_LAST_FILE_CHUNK: {
                std::istringstream inStrm(response.str());
                if (!ZLib::Inflate(inStrm, fileStream)) {
                    throw SyncError(100139);
                }
                break;
            }
            case OBEX_NOT_MODIFIED:
                SYNCLOG_INFO << "Application package already up to date";
                break;
            case OBEX_CANCELED_BY_USER:
                throw SyncError(100122);
            case OBEX_NOT_IMPLEMENTED:
                SYNCLOG_WARNING << "syncapp not implemented by the Bluetooth server device";
                break;
            default:
                throw SyncConnectionError("Server responded with error %d", static_cast<int>(getResult));
        }
    }

    if (getResult != OBEX_IS_LAST_FILE_CHUNK) {
        return false;
    } else {
        PortableFunctions::FileDelete(local_file_path);
        if (!tmpZip.Rename_noexcept(local_file_path)) {
            throw SyncError(100109, local_file_path);
        }
        SYNCLOG_INFO << "Downloaded application package " << package_name;
        return true;
    }
}


void BluetoothSyncService::UploadApplicationPackage(const std::string& /*local_package_zip_file_path*/, const std::string& /*package_name*/, const std::string& /*package_spec_json*/)
{
    throw SyncNotSupportedOperationError();
}


void BluetoothSyncService::DeleteApplication(const std::string& /*package_name*/)
{
    throw SyncNotSupportedOperationError();
}


std::optional<JsonNode> BluetoothSyncService::SendSyncMessage(const DeviceId& /*device_id*/, const SyncMessage& sync_message)
{
    HeaderList request_headers;
    request_headers.Add_UserAgent_CSProSyncClient();

    const std::unique_ptr<JsonStringWriter> json_writer = Json::CreateStringWriter();
    sync_message.WriteJsonForBluetooth(*json_writer);

    request_headers.AddJson(SyncCustomHeaders::MESSAGE_HEADER, json_writer->GetString());

    std::ostringstream ss;
    HeaderList response_headers;
    ObexResponseCode get_result = m_pObexConnection->get(OBEX_SYNC_MESSAGE_MEDIA_TYPE,
        _T("a message"), request_headers, ss, response_headers);

    if( get_result == OBEX_OK )
    {
        try
        {
            const JsonNode json_node = Json::Parse(ss.str());
            const SyncMessage sync_message_response = SyncMessage::CreateFromJsonFromBluetooth(json_node);
            ASSERT(sync_message_response.GetName().GetString() == sync_message.GetName().GetString());
            return sync_message_response.GetValueAsOptionalJson();
        }

        catch( const JsonParseException& exception )
        {
            SYNCLOG_ERROR << "Invalid server response: " << exception.what();
            SYNCLOG_ERROR << get_result;
            throw SyncError(100121);
        }
    }

    else if( get_result == OBEX_METHOD_NOT_ALLOWED )
    {
        throw SyncError(100156);
    }

    else
    {
        throw SyncConnectionError(ObexResponseCodeToString(get_result));
    }
}


std::string BluetoothSyncService::StartParadataSync(const std::string& log_uuid)
{
    HeaderList request_headers;
    request_headers.Add_UserAgent_CSProSyncClient();
    request_headers.Add(SyncCustomHeaders::PARADATA_LOG_UUID, log_uuid);

    std::ostringstream ss;
    HeaderList response_headers;

    ObexResponseCode get_result = m_pObexConnection->get(OBEX_SYNC_PARADATA_SYNC_HANDSHAKE,
        _T("paradata log details"), request_headers, ss, response_headers);

    if( get_result == OBEX_OK )
    {
        std::string server_log_uuid = response_headers.GetValue(SyncCustomHeaders::PARADATA_LOG_UUID);

        // throw an error if no paradata log is open on the server
        if( server_log_uuid.empty() )
            throw SyncError(100172);

        return server_log_uuid;
    }

    else
    {
        throw SyncConnectionError(ObexResponseCodeToString(get_result));
    }
}


void BluetoothSyncService::PutParadata(const std::string& paradata_log_file_path)
{
    putFileWorker(OBEX_SYNC_PARADATA_TYPE, UTF8_TODO::GetCString(paradata_log_file_path), CString());
}


std::vector<TemporaryFile> BluetoothSyncService::GetParadata()
{
    std::vector<TemporaryFile> received_temporary_files;
    TemporaryFile temporary_file;

    const ObexResponseCode get_result = getFileWorker(OBEX_SYNC_PARADATA_TYPE, _T("paradata log"), UTF8_TODO::GetCString(temporary_file.GetPath()));

    if( get_result == OBEX_IS_LAST_FILE_CHUNK )
    {
        received_temporary_files.emplace_back(std::move(temporary_file));
    }

    else if( get_result != OBEX_NOT_MODIFIED )
    {
        throw SyncConnectionError(ObexResponseCodeToString(get_result));
    }

    return received_temporary_files;
}


void BluetoothSyncService::StopParadataSync()
{
    HeaderList request_headers;
    request_headers.Add_UserAgent_CSProSyncClient();

    HeaderList response_headers;
    std::istringstream iss;
    std::ostringstream oss;

    ObexResponseCode put_result = m_pObexConnection->put(OBEX_SYNC_PARADATA_SYNC_HANDSHAKE, _T("paradata sync"),
        true, iss, 0, request_headers, oss, response_headers);

    if( put_result != OBEX_OK )
        throw SyncConnectionError(ObexResponseCodeToString(put_result));
}


std::shared_ptr<SyncListener> BluetoothSyncService::GetSharedSyncListener()
{
    return m_syncListener;
}


void BluetoothSyncService::SetSyncListener(std::shared_ptr<SyncListener> sync_listener)
{
    m_syncListener = sync_listener;
    m_pObexConnection->SetSyncListener(std::move(sync_listener));
}


std::shared_ptr<FileBasedConnection> BluetoothSyncService::GetFileBasedConnection()
{
    return nullptr;
}
