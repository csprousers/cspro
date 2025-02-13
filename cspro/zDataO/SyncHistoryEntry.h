#pragma once

#include <zAppO/SyncTypes.h>


class SyncHistoryEntry
{
public:
    enum class SyncState
    {
        Complete = 0,
        PartialGet = 1,
        PartialPut = 2
    };

    SyncHistoryEntry()
        : m_serialNumber(-1),
          m_fileRevision(-1),
          m_dateTime(0)
    {
    }

    SyncHistoryEntry(int serialNumber, int fileRevision, DeviceId deviceId,
        CString deviceName, SyncDirection direction,
        CString universe, time_t dateTime, CString serverFileRevision,
        SyncState state = SyncState::Complete, CString lastCaseUuid = CString()) :
        m_serialNumber(serialNumber),
        m_fileRevision(fileRevision),
        m_deviceId(deviceId),
        m_deviceName(deviceName),
        m_direction(direction),
        m_universe(universe),
        m_dateTime(dateTime),
        m_serverFileRevision(serverFileRevision),
        m_state(state),
        m_lastCaseUuid(lastCaseUuid)
    {
    }

    int getSerialNumber() const { return m_serialNumber; }
    int getFileRevision() const { return m_fileRevision; }
    SyncState getState() const { return m_state; }
    DeviceId getDeviceId() const { return m_deviceId; }
    CString getDeviceName() const { return m_deviceName; }
    CString getUniverse() const { return m_universe; }
    SyncDirection getDirection() const { return m_direction; }
    time_t getDateTime() const { return m_dateTime; }
    CString getServerFileRevision() const { return m_serverFileRevision; }
    CString getLastCaseUuid() const { return m_lastCaseUuid; }

    bool valid() const { return m_serialNumber >= 0; }
    bool isPartialGet() const { return m_state == SyncState::PartialGet; }
    bool isPartialPut() const { return m_state == SyncState::PartialPut; }

private:
    int m_serialNumber;
    int m_fileRevision;
    DeviceId m_deviceId;
    CString m_deviceName;
    SyncDirection m_direction;
    CString m_universe;
    time_t m_dateTime;
    CString m_serverFileRevision;
    SyncState m_state;
    CString m_lastCaseUuid;
};
