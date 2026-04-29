#include "stdafx.h"
#include "SyncRunner.h"
#include "ISyncService.h"
#include "SyncDictionaryInfo.h"
#include "SyncMessage.h"
#include "SyncServiceFactory.h"
#include <zNetwork/CSWebConnection.h>
#include <zNetwork/DropboxConnection.h>
#include <zNetwork/SyncConnectionStringProperties.h>
#include <zNetwork/SyncListenerRAII.h>
#include <zParadataO/Logger.h>
#include <zParadataO/SyncEvents.h>
#include <zParadataO/Syncer.h>


// --------------------------------------------------------------------------
// SyncRunner
// --------------------------------------------------------------------------

SyncRunner::SyncRunner(std::shared_ptr<SyncListener> sync_listener)
    :   m_syncListener(std::move(sync_listener))
{
    ASSERT(m_syncListener != nullptr);

    SyncLog::EnableLogging();
}


SyncRunner::SyncRunner(SyncRunner&&) = default;


SyncRunner::~SyncRunner()
{
}


void SyncRunner::ReportError(const CSProException& exception) const
{
    if( dynamic_cast<const SyncCancelException*>(&exception) == nullptr )
        m_syncListener->ReportError(exception);
}


template<typename ServerType, typename CF>
std::shared_ptr<ConnectResponse> SyncRunner::Connect(ServerType& server, const char* const server_description,
                                                     const SyncConnectionString& sync_connection_string, const CF& callback_function)
{
    try
    {
        const SyncListenerServerSaverAndCloser sync_listener_saver_and_closer(&server, m_syncListener,
                                                                              100102, sync_connection_string.ToDisplayString().c_str());

        SYNCLOG_INFO << "Connect to " << ( ( server_description != nullptr ) ? server_description : "server" )
                     << ": " << sync_connection_string.ToSafeString();

        std::shared_ptr<ConnectResponse> connect_response = callback_function();
        ASSERT(connect_response != nullptr);

        SYNCLOG_INFO << "Connection successful. Server id: " << connect_response->GetServerDeviceId();

        return connect_response;
    }

    catch( const SyncCancelException& )
    {
        SYNCLOG_INFO << "Canceled connection";
        throw;
    }

    catch( const std::exception& exception )
    {
        SYNCLOG_ERROR << "Error connecting to sync service: " << exception.what();
        throw;
    }
}


SyncRunner::Connection SyncRunner::Connect(const SyncConnectionString& sync_connection_string,
                                           ParadataData* const paradata_data/* = nullptr*/)
{
    std::optional<ParadataLogger::EventHolder<Paradata::SyncConnectionEvent>> sync_connect_event_holder;

    if( paradata_data != nullptr )
    {
        std::tie(paradata_data->paradata_logger, sync_connect_event_holder) =
            ParadataLogger::Create(std::move(paradata_data->device_id), paradata_data->connection_source, sync_connection_string);
    }

    try
    {
        sync_connection_string.Validate();
        ASSERT(sync_connection_string.IsDefined());

        SyncServiceFactory factory(std::make_unique<LoginAccessorWithoutBluetoothSupport>());

        switch( sync_connection_string.GetType() )
        {
            case SyncServiceType::CSWeb:      return ConnectCSWeb(factory, sync_connection_string);
            case SyncServiceType::Dropbox:    return ConnectDropbox(factory, sync_connection_string);
            case SyncServiceType::Ftp:        return ConnectFtp(factory, sync_connection_string);
            case SyncServiceType::LocalFiles: return ConnectLocalFiles(factory, sync_connection_string);
            default:                          ASSERT(false); throw SyncConnectionError(sync_connection_string.ToSafeString());
        }
    }

    catch( const std::exception& exception )
    {
        SYNCLOG_ERROR << "Error connecting to the sync service: " << exception.what();

        if( sync_connect_event_holder.has_value() )
            sync_connect_event_holder->SetResultFailure(exception);

        throw;
    }
}


SyncRunner::Connection SyncRunner::Connect(const char* const sync_service_description, const SyncConnectionString& sync_connection_string,
                                           std::unique_ptr<ISyncService> sync_service)
{
    ASSERT(sync_service != nullptr);

    std::shared_ptr<ConnectResponse> connect_response = Connect(*sync_service, sync_service_description, sync_connection_string,
        [&]()
        {
            return sync_service->Connect();
        });

    return std::make_tuple(std::move(sync_service), std::move(connect_response));
}


SyncRunner::Connection SyncRunner::ConnectCSWeb(SyncServiceFactory& factory, const SyncConnectionString& sync_connection_string)
{
    ASSERT(sync_connection_string.GetType() == SyncServiceType::CSWeb);

    std::unique_ptr<ISyncService> sync_service = factory.CreateCSWebSyncService(sync_connection_string);
    ASSERT(sync_service != nullptr);

    return Connect(nullptr, sync_connection_string, std::move(sync_service));
}


SyncRunner::Connection SyncRunner::ConnectDropbox(SyncServiceFactory& factory, const SyncConnectionString& sync_connection_string)
{
    ASSERT(sync_connection_string.GetType() == SyncServiceType::Dropbox);

    // determine if a local Dropbox connection should be made
    std::string local_dropbox_directory = DropboxConnection::GetEvaluatedLocalDropboxDirectory(sync_connection_string);

    if( !local_dropbox_directory.empty() )
    {
        SYNCLOG_INFO << "Connect to : " << local_dropbox_directory;

        std::unique_ptr<ISyncService> sync_service = factory.CreateDropboxLocalSyncService(std::move(local_dropbox_directory));
        ASSERT(sync_service != nullptr);

        return Connect("local Dropbox directory", sync_connection_string, std::move(sync_service));
    }

    // create a remote Dropbox connection
    std::unique_ptr<ISyncService> sync_service = factory.CreateDropboxSyncService(sync_connection_string);
    ASSERT(sync_service != nullptr);

    const std::string* const email = sync_connection_string.GetProperty(SCSProperty::email);

    return Connect(( email != nullptr ) ? FormatText("Dropbox using email '%s'", email->c_str()).c_str() : "Dropbox",
                   sync_connection_string,
                   std::move(sync_service));
}


SyncRunner::Connection SyncRunner::ConnectFtp(SyncServiceFactory& factory, const SyncConnectionString& sync_connection_string)
{
    ASSERT(sync_connection_string.GetType() == SyncServiceType::Ftp);

    std::unique_ptr<ISyncService> sync_service = factory.CreateFtpSyncService(sync_connection_string);

    if( sync_service == nullptr )
        throw SyncConnectionError("FTP access is not supported on this platform.");

    return Connect(nullptr, sync_connection_string, std::move(sync_service));
}


SyncRunner::Connection SyncRunner::ConnectLocalFiles(SyncServiceFactory& factory, const SyncConnectionString& sync_connection_string)
{
    ASSERT(sync_connection_string.GetType() == SyncServiceType::LocalFiles);

    std::unique_ptr<ISyncService> sync_service = factory.CreateLocalFileSyncService(sync_connection_string);
    ASSERT(sync_service != nullptr);

    return Connect("local files directory", sync_connection_string, std::move(sync_service));
}


std::shared_ptr<ConnectResponse> SyncRunner::Connect(const SyncConnectionString& sync_connection_string,
                                                     CSWebConnection& csweb_connection, const double minimum_api_version_required)
{
    return Connect(csweb_connection, nullptr, sync_connection_string,
        [&]()
        {
            return csweb_connection.Connect(minimum_api_version_required);
        });
}


void SyncRunner::Disconnect(ISyncService& sync_service,
                            ParadataLogger* const paradata_logger/* = nullptr*/)
{
    std::optional<ParadataLogger::EventHolder<Paradata::SyncConnectionEvent>> sync_disconnect_event_holder;

    if( paradata_logger != nullptr )
        sync_disconnect_event_holder.emplace(paradata_logger->CreateDisconnectEvent());

    try
    {
        const SyncListenerServerSaverAndCloser sync_listener_saver_and_closer(&sync_service, m_syncListener, 100103);

        SYNCLOG_INFO << "Disconnecting from sync service";

        sync_service.Disconnect();
    }

    catch( const std::exception& exception )
    {
        SYNCLOG_ERROR << "Error disconnecting from the sync service: " << exception.what();

        if( sync_disconnect_event_holder.has_value() )
            sync_disconnect_event_holder->SetResultFailure(exception);

        throw;
    }
}


template<typename ServerType, typename CF>
std::vector<SyncDictionaryInfo> SyncRunner::GetDictionaries(ServerType& server, const CF& callback_function)
{
    try
    {
        const SyncListenerServerSaverAndCloser sync_listener_saver_and_closer(&server, m_syncListener, 100128);

        SYNCLOG_INFO << "Downloading dictionary list";

        std::vector<SyncDictionaryInfo> dictionaries = callback_function();

        SYNCLOG_INFO << "Downloading dictionary list complete (" << dictionaries.size() << " dictionaries)";

        return dictionaries;
    }

    catch( const SyncCancelException& )
    {
        SYNCLOG_INFO << "Dictionary list download canceled";
        throw;
    }

    catch( const std::exception& exception )
    {
        SYNCLOG_ERROR << "Error downloading dictionaries: " << exception.what();
        throw;
    }
}


std::vector<SyncDictionaryInfo> SyncRunner::GetDictionaries(CSWebConnection& csweb_connection)
{
    return GetDictionaries(csweb_connection,
        [&]()
        {
            const JsonNode json_node = csweb_connection.GetDictionariesList();
            return json_node.GetArray().GetVector<SyncDictionaryInfo>();
        });
}


std::vector<SyncDictionaryInfo> SyncRunner::GetDictionaries(ISyncService& sync_service)
{
    return GetDictionaries(sync_service,
        [&]()
        {
            return sync_service.GetDictionaries();
        });
}


std::string SyncRunner::GetDictionarySpec(ISyncService& sync_service, const std::string& dictionary_name)
{
    try
    {
        const SyncListenerServerSaverAndCloser sync_listener_saver_and_closer(&sync_service, m_syncListener, 100107, dictionary_name.c_str());

        SYNCLOG_INFO << "Downloading dictionary: " << dictionary_name;

        std::string dictionary_spec = sync_service.GetDictionary(dictionary_name);

        SYNCLOG_INFO << "Downloading dictionary file complete (" << dictionary_spec.length() << " bytes)";

        return dictionary_spec;
    }

    catch( const SyncCancelException& )
    {
        SYNCLOG_INFO << "Dictionary download canceled";
        throw;
    }

    catch( const std::exception& exception )
    {
        SYNCLOG_ERROR << "Error downloading dictionary: " << exception.what();
        throw;
    }
}


std::optional<JsonNode> SyncRunner::SendSyncMessage(ISyncService& sync_service, const DeviceId& device_id, const SyncMessage& sync_message,
                                                    ParadataLogger* const paradata_logger/* = nullptr*/)
{
    std::optional<ParadataLogger::EventHolder<Paradata::SyncMessageEvent>> sync_message_event_holder;

    if( paradata_logger != nullptr )
    {
        sync_message_event_holder.emplace(paradata_logger->CreateSyncEvent<Paradata::SyncMessageEvent>(sync_message.GetName(),
                                                                                                       sync_message.GetValueAsOptionalJsonText()));
    }

    try
    {
        const SyncListenerServerSaverAndCloser sync_listener_saver_and_closer(&sync_service, m_syncListener, 100155);

        SYNCLOG_INFO << "Syncing the message named '" << sync_message.GetName().GetString() << '\'';

        std::optional<JsonNode> response_json_node = sync_service.SendSyncMessage(device_id, sync_message);

        if( sync_message_event_holder.has_value() )
            sync_message_event_holder->event->SetResponse(response_json_node);

        return response_json_node;
    }

    catch( const SyncCancelException& exception )
    {
        SYNCLOG_INFO << "Syncing message canceled";

        if( sync_message_event_holder.has_value() )
            sync_message_event_holder->SetResultFailure(exception);

        throw;
    }

    catch( const std::exception& exception )
    {
        SYNCLOG_ERROR << "Error syncing message: " << exception.what();

        if( sync_message_event_holder.has_value() )
            sync_message_event_holder->SetResultFailure(exception);

        throw;
    }
}


void SyncRunner::SyncParadata(ISyncService& sync_service, Paradata::Syncer& paradata_syncer, const SyncDirection sync_direction,
                              ParadataLogger* const paradata_logger/* = nullptr*/)
{
    std::optional<ParadataLogger::EventHolder<Paradata::SyncParadataEvent>> sync_paradata_event_holder;

    if( paradata_logger != nullptr )
    {
        sync_paradata_event_holder.emplace(paradata_logger->CreateSyncEvent<Paradata::SyncParadataEvent>(sync_direction,
                                                                                                         paradata_syncer.GetLogFilePath()));
    }

    try
    {
        const SyncListenerServerSaverAndCloser sync_listener_saver_and_closer(&sync_service, m_syncListener, 100171);

        // start the sync, sending the client's log UUID and getting the sync service's log UUID
        paradata_syncer.SetPeerLogUuid(sync_service.StartParadataSync(paradata_syncer.GetLogUuid()));
        ASSERT(!paradata_syncer.GetPeerLogUuid().empty());

        // receive paradata from the sync service
        if( sync_direction != SyncDirection::Put )
        {
            SYNCLOG_INFO << "Requesting paradata events to be added to the log "
                         << paradata_syncer.GetLogUuid();

            std::vector<TemporaryFile> received_database_temporary_files = sync_service.GetParadata();

            if( !received_database_temporary_files.empty() )
            {
                SYNCLOG_INFO << "Received paradata events from the log "
                             << paradata_syncer.GetPeerLogUuid();

                paradata_syncer.SetReceivedSyncableDatabases(std::move(received_database_temporary_files));
            }

            else
            {
                SYNCLOG_INFO << "No paradata events from the log "
                             << paradata_syncer.GetPeerLogUuid()
                             << " received as all events are up-to-date";
            }
        }

        // send paradata to the sync service
        if( sync_direction != SyncDirection::Get )
        {
            const std::optional<std::string> extracted_syncable_database_file_path = paradata_syncer.GetExtractedSyncableDatabaseFilePath();

            if( extracted_syncable_database_file_path.has_value() )
            {
                SYNCLOG_INFO << "Sending paradata events to be added to the log "
                             << paradata_syncer.GetPeerLogUuid();

                sync_service.PutParadata(*extracted_syncable_database_file_path);
            }

            else
            {
                SYNCLOG_INFO << "Skipping sending paradata events to the log "
                             << paradata_syncer.GetPeerLogUuid()
                             << " as all events are up-to-date";
            }
        }

        // after merging any received data, inform the sync service that all was transfered well
        paradata_syncer.MergeReceivedSyncableDatabases();

        sync_service.StopParadataSync();

        paradata_syncer.RunPostSuccessfulSyncTasks();
    }

    catch( const SyncCancelException& exception )
    {
        SYNCLOG_INFO << "Syncing paradata canceled";

        if( sync_paradata_event_holder.has_value() )
            sync_paradata_event_holder->SetResultFailure(exception);

        throw;
    }

    catch( const std::exception& exception )
    {
        SYNCLOG_ERROR << "Error syncing paradata: " << exception.what();

        if( sync_paradata_event_holder.has_value() )
            sync_paradata_event_holder->SetResultFailure(exception);

        throw;
    }
}



// --------------------------------------------------------------------------
// SyncRunner::ParadataLogger
// SyncRunner::ParadataLogger::EventHolder
// --------------------------------------------------------------------------

SyncRunner::ParadataLogger::ParadataLogger(std::shared_ptr<const Paradata::SyncServiceInstance> sync_service_instance)
    :   m_syncServiceInstance(std::move(sync_service_instance))
{
    ASSERT(m_syncServiceInstance != nullptr);
}


SyncRunner::ParadataLogger::~ParadataLogger()
{
}


std::tuple<std::unique_ptr<SyncRunner::ParadataLogger>,
           SyncRunner::ParadataLogger::EventHolder<Paradata::SyncConnectionEvent>>
    SyncRunner::ParadataLogger::Create(std::string device_id, const ConnectionSource connection_source, const SyncConnectionString& sync_connection_string)
{
    ASSERT(Paradata::Logger::IsOpen());

    std::unique_ptr<Paradata::SyncConnectionEvent> sync_connection_event;
    std::shared_ptr<const Paradata::SyncServiceInstance> sync_service_instance;

    std::tie(sync_connection_event, sync_service_instance) =
        Paradata::SyncConnectionEvent::CreateConnectEvent(std::move(device_id), connection_source, sync_connection_string);

    auto paradata_logger = std::make_unique<ParadataLogger>(std::move(sync_service_instance));
    ParadataLogger* const paradata_logger_ptr = paradata_logger.get();

    return { std::move(paradata_logger),
             EventHolder(paradata_logger_ptr, std::move(sync_connection_event)) };
}


SyncRunner::ParadataLogger::EventHolder<Paradata::SyncConnectionEvent> SyncRunner::ParadataLogger::CreateDisconnectEvent()
{
    return EventHolder(this, Paradata::SyncConnectionEvent::CreateDisconnectEvent(m_syncServiceInstance));
}


template<typename EventType, typename... Args>
SyncRunner::ParadataLogger::EventHolder<EventType> SyncRunner::ParadataLogger::CreateSyncEvent(Args&&... args)
{
    return EventHolder<EventType>(this, std::make_unique<EventType>(m_syncServiceInstance, std::forward<Args>(args)...));
}


template<typename EventType>
SyncRunner::ParadataLogger::EventHolder<EventType>::EventHolder(ParadataLogger* const paradata_logger, std::unique_ptr<EventType> event_)
    :   event(std::move(event_)),
        m_paradataLogger(paradata_logger)
{
    ASSERT(event != nullptr && m_paradataLogger != nullptr);
}


template<typename EventType>
SyncRunner::ParadataLogger::EventHolder<EventType>::~EventHolder()
{
    // event will be null when the object has been moved
    if( event == nullptr )
        return;

    if( m_exceptionText == nullptr )
    {
        event->SetResultSuccess();
    }

    else
    {
        event->SetResultFailure(std::move(*m_exceptionText));
    }

    Paradata::Logger::LogEvent(std::move(event));
}


template<typename EventType>
void SyncRunner::ParadataLogger::EventHolder<EventType>::SetResultFailure(std::string exception_text)
{
    m_exceptionText = std::make_unique<std::string>(std::move(exception_text));
}


template<typename EventType>
void SyncRunner::ParadataLogger::EventHolder<EventType>::SetResultFailure(const std::exception& exception)
{
    if( m_paradataLogger->m_syncErrorFormatter == nullptr )
        m_paradataLogger->m_syncErrorFormatter = std::make_unique<SyncErrorFormatter>();

    SetResultFailure(m_paradataLogger->m_syncErrorFormatter->GetFormattedError(exception));
}


template class SyncRunner::ParadataLogger::EventHolder<Paradata::SyncConnectionEvent>;
