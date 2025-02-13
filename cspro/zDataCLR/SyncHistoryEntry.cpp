#include "Stdafx.h"
#include "SyncHistoryEntry.h"
#include <zDataO/SyncHistoryEntry.h>


CSPro::Data::SyncHistoryEntry::SyncHistoryEntry(const ::SyncHistoryEntry& sync_history_entry)
    :   m_deviceId(gcnew System::String(sync_history_entry.getDeviceId())),
        m_deviceName(gcnew System::String(sync_history_entry.getDeviceName())),
        m_direction(sync_history_entry.getDirection() == ::SyncDirection::Both ? CSPro::Util::SyncDirection::BOTH :
                    sync_history_entry.getDirection() == ::SyncDirection::Get  ? CSPro::Util::SyncDirection::GET :
                                                                                 CSPro::Util::SyncDirection::PUT),
        m_universe(gcnew System::String(sync_history_entry.getUniverse())),
        m_time(System::DateTime(1970, 1, 1, 0, 0, 0, 0, System::DateTimeKind::Unspecified).AddSeconds((double) sync_history_entry.getDateTime()))
{
}

System::String^ CSPro::Data::SyncHistoryEntry::DeviceId::get()
{
    return m_deviceId;
}

System::String^ CSPro::Data::SyncHistoryEntry::DeviceName::get()
{
    return m_deviceName;
}

CSPro::Util::SyncDirection CSPro::Data::SyncHistoryEntry::Direction::get()
{
    return m_direction;
}

System::String^ CSPro::Data::SyncHistoryEntry::Universe::get()
{
    return m_universe;
}

System::DateTime CSPro::Data::SyncHistoryEntry::Time::get()
{
    return m_time;
}
