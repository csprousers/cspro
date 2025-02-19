#pragma once

#include <zAppO/SyncTypes.h>


class SyncHistoryEntry
{
public:
    enum class SyncState
    {
        Complete   = 0,
        PartialGet = 1,
        PartialPut = 2
    };

    SyncHistoryEntry(int serial_number, int file_revision, DeviceId device_id,
                     std::string device_name, SyncDirection direction, std::string universe, int64_t date_time, std::string server_file_revision,
                     SyncState state = SyncState::Complete, std::string last_case_uuid = std::string());

    int GetSerialNumber() const { return m_serialNumber; }

    int GetFileRevision() const { return m_fileRevision; }

    SyncState GetState() const { return m_state; }

    const DeviceId& GetDeviceId() const { return m_deviceId; }

    const std::string& GetDeviceName() const { return m_deviceName; }

    const std::string& GetUniverse() const { return m_universe; }

    SyncDirection GetDirection() const { return m_direction; }

    int64_t GetDateTime() const { return m_dateTime; }

    const std::string& GetServerFileRevision() const { return m_serverFileRevision; }

    const std::string& GetLastCaseUuid() const { return m_lastCaseUuid; }

    bool IsPartialGet() const { return m_state == SyncState::PartialGet; }
    bool IsPartialPut() const { return m_state == SyncState::PartialPut; }

private:
    int m_serialNumber;
    int m_fileRevision;
    DeviceId m_deviceId;
    std::string m_deviceName;
    SyncDirection m_direction;
    std::string m_universe;
    int64_t m_dateTime;
    std::string m_serverFileRevision;
    SyncState m_state;
    std::string m_lastCaseUuid;
};



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

inline SyncHistoryEntry::SyncHistoryEntry(const int serial_number, const int file_revision, DeviceId device_id,
                                          std::string device_name, const SyncDirection direction, std::string universe, const int64_t date_time, std::string server_file_revision,
                                          const SyncState state/* = SyncState::Complete*/, std::string last_case_uuid/* = std::string()*/)
    :   m_serialNumber(serial_number),
        m_fileRevision(file_revision),
        m_deviceId(std::move(device_id)),
        m_deviceName(std::move(device_name)),
        m_direction(direction),
        m_universe(std::move(universe)),
        m_dateTime(date_time),
        m_serverFileRevision(std::move(server_file_revision)),
        m_state(state),
        m_lastCaseUuid(std::move(last_case_uuid))
{
    ASSERT(serial_number >= 0);
}
