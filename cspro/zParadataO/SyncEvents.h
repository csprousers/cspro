#pragma once

#include <zParadataO/Event.h>
#include <zUtilO/SyncConnectionString.h>
#include <zAppO/SyncTypes.h>
#include <zSyncO/SyncRunner.h>

namespace Paradata { class SyncConnectionEvent; class SyncDataEvent; class SyncEvent;
                     class SyncMessageEvent; class SyncParadataEvent; class SyncServiceInstance; }


// --------------------------------------------------------------------------
// SyncServiceInstance
// --------------------------------------------------------------------------

class Paradata::SyncServiceInstance
{
    friend class SyncConnectionEvent;

private:
    SyncServiceInstance(std::string device_id);

public:
    const std::string& GetDeviceId() const { return m_deviceId; }

    long GetInstanceId() const { ASSERT(m_instanceId != -1); return m_instanceId; }

private:
    std::string m_deviceId;
    long m_instanceId;
};


// --------------------------------------------------------------------------
// SyncEvent
// --------------------------------------------------------------------------

class ZPARADATAO_API Paradata::SyncEvent : public Event
{
protected:
    SyncEvent(std::shared_ptr<const SyncServiceInstance> sync_service_instance);

public:
    void SetResultSuccess() { SetResultDuration(); }

    void SetResultFailure(std::string exception_text);
    void SetResultFailure(const std::exception& exception) { SetResultFailure(exception.what()); }

protected:
    long GetSyncServiceInstanceId() const { return m_syncServiceInstance->GetInstanceId(); }

    double GetDuration() const { return m_duration; }

    std::optional<long> GetExceptionTextId(Log& log) const;

private:
    void SetResultDuration();

private:
    std::shared_ptr<const SyncServiceInstance> m_syncServiceInstance;
    double m_duration;
    std::unique_ptr<std::string> m_exception;
};


// --------------------------------------------------------------------------
// SyncConnectionEvent
// --------------------------------------------------------------------------

class ZPARADATAO_API Paradata::SyncConnectionEvent : public SyncEvent
{
    DECLARE_PARADATA_EVENT(SyncConnectionEvent)

public:
    enum class Action { Connect, Disconnect };

private:
    SyncConnectionEvent(std::shared_ptr<const SyncServiceInstance> sync_service_instance, Action action);

public:
    static std::tuple<std::unique_ptr<SyncConnectionEvent>,
                      std::shared_ptr<const SyncServiceInstance>> CreateConnectEvent(std::string device_id,
                                                                                     SyncRunner::ConnectionSource connection_source,
                                                                                     SyncConnectionString sync_connection_string);

    static std::unique_ptr<SyncConnectionEvent> CreateDisconnectEvent(std::shared_ptr<const SyncServiceInstance> sync_service_instance);

private:
    static constexpr int SyncServiceTypeToParadataInt(SyncServiceType sync_service_type);

private:
    Action m_action;
    std::optional<std::tuple<std::shared_ptr<SyncServiceInstance>,
                             SyncRunner::ConnectionSource,
                             SyncConnectionString>> m_connectData;
};


// --------------------------------------------------------------------------
// SyncDataEvent
// --------------------------------------------------------------------------

class ZPARADATAO_API Paradata::SyncDataEvent : public SyncEvent
{
    DECLARE_PARADATA_EVENT(SyncDataEvent)

public:
    SyncDataEvent(std::shared_ptr<const SyncServiceInstance> sync_service_instance,
                  const void* data_repository, SyncDirection sync_direction, std::string universe);

    void SetStatistics(const DataSyncStatistics& sync_stats) { m_syncStats = sync_stats; }

private:
    const void* m_dataRepository;
    SyncDirection m_syncDirection;
    std::string m_universe;
    std::optional<DataSyncStatistics> m_syncStats;
};


// --------------------------------------------------------------------------
// SyncMessageEvent
// --------------------------------------------------------------------------

class ZPARADATAO_API Paradata::SyncMessageEvent : public SyncEvent
{
    DECLARE_PARADATA_EVENT(SyncMessageEvent)

public:
    SyncMessageEvent(std::shared_ptr<const SyncServiceInstance> sync_service_instance,
                     SharableString message_name, SharableString message_value);

    void SetResponse(const std::optional<JsonNode>& response_json_node);

private:
    SharableString m_name;
    SharableString m_value;
    SharableString m_response;
};


// --------------------------------------------------------------------------
// SyncParadataEvent
// --------------------------------------------------------------------------

class ZPARADATAO_API Paradata::SyncParadataEvent : public SyncEvent
{
    DECLARE_PARADATA_EVENT(SyncParadataEvent)

public:
    SyncParadataEvent(std::shared_ptr<const SyncServiceInstance> sync_service_instance,
                      SyncDirection sync_direction, std::string file_path);

private:
    SyncDirection m_syncDirection;
    std::string m_filePath;
};
