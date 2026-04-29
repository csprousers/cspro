#pragma once

#include <zSyncO/zSyncO.h>

class ConnectResponse;
class CSWebConnection;
class ISyncService;
namespace Paradata { class SyncConnectionEvent; class Syncer; class SyncEvent; class SyncServiceInstance; }
class SyncConnectionString;
class SyncDictionaryInfo;
class SyncErrorFormatter;
class SyncListener;
class SyncMessage;
class SyncServiceFactory;


// --------------------------------------------------------------------------
// SyncRunner is an alternative to SyncClient that runs sync-related
// operations using a sync listener, throwing exceptions rather than
// reporting them using SyncListener::ReportError.
// --------------------------------------------------------------------------

class SYNC_API SyncRunner
{
    friend class SyncClient;

public:
    class ParadataLogger;

    SyncRunner(std::shared_ptr<SyncListener> sync_listener);
    SyncRunner(const SyncRunner&) = delete;
    SyncRunner(SyncRunner&&);
    ~SyncRunner();

    // Returns the sync listener.
    std::shared_ptr<SyncListener> GetSyncListener() { return m_syncListener; }

    // Reports an error message using SyncListener::ReportError.
    // If the exception is of type SyncCancelException, nothing is reported.
    // If the exception is of type SyncError, the formatted message is reported.
    void ReportError(const CSProException& exception) const;


    // --------------------------------------------------------------------------
    // Connection Routines
    // --------------------------------------------------------------------------

    // Connects to CSWeb.
    std::shared_ptr<ConnectResponse> Connect(const SyncConnectionString& sync_connection_string,
                                             CSWebConnection& csweb_connection, double minimum_api_version_required);

    // Connection data...
    using Connection = std::tuple<std::shared_ptr<ISyncService>, std::shared_ptr<ConnectResponse>>;

    enum class ConnectionSource { Logic, ActionInvoker };
    struct ParadataData { std::string device_id;
                          ConnectionSource connection_source;
                          std::unique_ptr<ParadataLogger> paradata_logger; };

    // Instantiates and connects to the appropriate sync service subclass, potentially setting up support for paradata logging.
    Connection Connect(const SyncConnectionString& sync_connection_string,
                       ParadataData* paradata_data = nullptr);

    // Disconnects from the sync service.
    void Disconnect(ISyncService& sync_service,
                    ParadataLogger* paradata_logger = nullptr);


    // --------------------------------------------------------------------------
    // Data Routines
    // --------------------------------------------------------------------------

    // Returns a list of dictionaries on the sync service.
    std::vector<SyncDictionaryInfo> GetDictionaries(CSWebConnection& csweb_connection);
    std::vector<SyncDictionaryInfo> GetDictionaries(ISyncService& sync_service);

    // Downloads a dictionary specification from the sync service.
    std::string GetDictionarySpec(ISyncService& sync_service, const std::string& dictionary_name);


    // --------------------------------------------------------------------------
    // Message Routines
    // --------------------------------------------------------------------------

    // Sends the message to the sync service, returning an optional response.
    std::optional<JsonNode> SendSyncMessage(ISyncService& sync_service, const DeviceId& device_id, const SyncMessage& sync_message,
                                            ParadataLogger* paradata_logger = nullptr);


    // --------------------------------------------------------------------------
    // Paradata Routines
    // --------------------------------------------------------------------------

    // Syncs the paradata with the sync service.
    void SyncParadata(ISyncService& sync_service, Paradata::Syncer& paradata_syncer, SyncDirection sync_direction,
                      ParadataLogger* paradata_logger = nullptr);


private:
    template<typename ServerType, typename CF>
    std::shared_ptr<ConnectResponse> Connect(ServerType& server, const char* server_description, const SyncConnectionString& sync_connection_string, const CF& callback_function);

    Connection Connect(const char* sync_service_description, const SyncConnectionString& sync_connection_string, std::unique_ptr<ISyncService> sync_service);
    Connection ConnectBluetooth(SyncServiceFactory& factory, const SyncConnectionString& sync_connection_string);
    Connection ConnectCSWeb(SyncServiceFactory& factory, const SyncConnectionString& sync_connection_string);
    Connection ConnectDropbox(SyncServiceFactory& factory, const SyncConnectionString& sync_connection_string);
    Connection ConnectFtp(SyncServiceFactory& factory, const SyncConnectionString& sync_connection_string);
    Connection ConnectLocalFiles(SyncServiceFactory& factory, const SyncConnectionString& sync_connection_string);

    template<typename ServerType, typename CF>
    std::vector<SyncDictionaryInfo> GetDictionaries(ServerType& server, const CF& callback_function);

private:
    std::shared_ptr<SyncListener> m_syncListener;
};


// --------------------------------------------------------------------------
// SyncRunner::ParadataLogger
// SyncRunner::ParadataLogger::EventHolder
//
// ParadataLogger is used by clients to keep a record of the sync service
// instance, which is used when logging all sync-related events.
//
// The EventHolder is a RAII class that logs an event on destruction (after
// calling SyncEvent::SetResultSuccess or SetResultFailure.
// --------------------------------------------------------------------------

class SyncRunner::ParadataLogger
{
    friend class SyncRunner;

public:
    template<typename EventType> class EventHolder;

    ParadataLogger(std::shared_ptr<const Paradata::SyncServiceInstance> sync_service_instance);
    ~ParadataLogger();

    // Instantiates a ParadataLogger and returns the holder for the sync connect event.
    static std::tuple<std::unique_ptr<ParadataLogger>,
                      EventHolder<Paradata::SyncConnectionEvent>>
        Create(std::string device_id, ConnectionSource connection_source, const SyncConnectionString& sync_connection_string);

private:
    EventHolder<Paradata::SyncConnectionEvent> CreateDisconnectEvent();

    template<typename EventType, typename... Args>
    EventHolder<EventType> CreateSyncEvent(Args&&... args);

private:
    std::shared_ptr<const Paradata::SyncServiceInstance> m_syncServiceInstance;
    std::unique_ptr<SyncErrorFormatter> m_syncErrorFormatter;
};


template<typename EventType>
class SyncRunner::ParadataLogger::EventHolder
{
public:
    EventHolder(ParadataLogger* paradata_logger, std::unique_ptr<EventType> event);
    EventHolder(EventHolder&&) = default;
    ~EventHolder();

    EventHolder& operator=(EventHolder&&) = default;

    void SetResultFailure(std::string exception_text);
    void SetResultFailure(const std::exception& exception);

public:
    std::shared_ptr<EventType> event;

private:
    ParadataLogger* m_paradataLogger;
    std::unique_ptr<std::string> m_exceptionText;
};
