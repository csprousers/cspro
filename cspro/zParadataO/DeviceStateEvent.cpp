#include "stdafx.h"
#include "DeviceStateEvent.h"
#include <zUtilO/Interapp.h>

#ifdef WIN_DESKTOP
#include <zUtilO/WinBluetoothFunctions.h>

#include <Wlanapi.h>
#pragma comment(lib,"Wlanapi.lib")

#include <HighLevelMonitorConfigurationAPI.h>
#pragma comment(lib,"Dxva2.lib")
#endif


namespace Paradata
{
    DeviceStateEvent::DeviceState::DeviceState()
        :   bluetooth_enabled(false),
            wifi_enabled(false),
            screen_orientation_portrait(false)
    {
    }

    void DeviceStateEvent::SetupTables(Log& log)
    {
        log.CreateTable(ParadataTable::DeviceStateEvent)
                .AddColumn(_T("bluetooth_enabled"), Table::ColumnType::Boolean)
                .AddColumn(_T("gps_enabled"), Table::ColumnType::Boolean, true)
                .AddColumn(_T("wifi_enabled"), Table::ColumnType::Boolean)
                .AddColumn(_T("wifi_ssid_text"), Table::ColumnType::Long, true)
                .AddColumn(_T("mobile_network_enabled"), Table::ColumnType::Boolean, true)
                .AddColumn(_T("mobile_network_type"), Table::ColumnType::Text, true)
                .AddColumn(_T("mobile_network_name_text"), Table::ColumnType::Long, true)
                .AddColumn(_T("mobile_network_strength"), Table::ColumnType::Double, true)
                .AddColumn(_T("battery_level"), Table::ColumnType::Double, true)
                .AddColumn(_T("battery_charging"), Table::ColumnType::Boolean, true)
                .AddColumn(_T("screen_brightness"), Table::ColumnType::Double, true)
                .AddColumn(_T("screen_orientation_portrait"), Table::ColumnType::Boolean)
            ;
    }

    DeviceStateEvent::DeviceStateEvent()
    {
        // fill in the platform-specific values
#ifdef WIN_DESKTOP
        GetBluetoothEnabled();

        // m_deviceState.gps_enabled will not be assigned

        GetWiFiEnabledAndWiFiSsid();

        // m_deviceState.mobile_network_enabled will not be assigned
        // m_deviceState.mobile_network_type will not be assigned
        // m_deviceState.mobile_network_name will not be assigned
        // m_deviceState.mobile_network_strength will not be assigned

        GetBatteryLevelAndBatteryCharging();

        GetScreenBrightness();
        GetScreenOrientation();

#else
        PlatformInterface::GetInstance()->GetApplicationInterface()->ParadataDeviceStateQuery(m_deviceState);
#endif
    }

    void DeviceStateEvent::Save(Log& log, long base_event_id) const
    {
        Table& device_state_event_table = log.GetTable(ParadataTable::DeviceStateEvent);
        device_state_event_table.Insert(&base_event_id,
            m_deviceState.bluetooth_enabled,
            GetOptionalValueOrNull(m_deviceState.gps_enabled),
            m_deviceState.wifi_enabled,
            GetOptionalValueOrNull(log.AddNullableText(m_deviceState.wifi_ssid)),
            GetOptionalValueOrNull(m_deviceState.mobile_network_enabled),
            GetOptionalTextValueOrNull(m_deviceState.mobile_network_type),
            GetOptionalValueOrNull(log.AddNullableText(m_deviceState.mobile_network_name)),
            GetOptionalValueOrNull(m_deviceState.mobile_network_strength),
            GetOptionalValueOrNull(m_deviceState.battery_level),
            GetOptionalValueOrNull(m_deviceState.battery_charging),
            GetOptionalValueOrNull(m_deviceState.screen_brightness),
            m_deviceState.screen_orientation_portrait
        );
    }

#ifdef WIN_DESKTOP
    void DeviceStateEvent::GetBluetoothEnabled()
    {
        std::shared_ptr<WinBluetoothFunctions> functions = WinBluetoothFunctions::instance();

        if( functions != nullptr )
        {
            HANDLE hRadio = nullptr;
            BLUETOOTH_FIND_RADIO_PARAMS btFindParams = { sizeof(BLUETOOTH_FIND_RADIO_PARAMS) };
            m_deviceState.bluetooth_enabled = ( functions->BluetoothFindFirstRadio(&btFindParams, &hRadio) != nullptr );
        }
    }

    void DeviceStateEvent::GetWiFiEnabledAndWiFiSsid()
    {
        DWORD dwNegotiatedVersion = 0;
        HANDLE hClientHandle = nullptr;

        if( WlanOpenHandle(2, nullptr, &dwNegotiatedVersion, &hClientHandle) == ERROR_SUCCESS )
        {
            PWLAN_INTERFACE_INFO_LIST pIfList = nullptr;

            if( WlanEnumInterfaces(hClientHandle, nullptr, &pIfList) == ERROR_SUCCESS )
            {
                for( DWORD i = 0; i < pIfList->dwNumberOfItems; ++i )
                {
                    PWLAN_INTERFACE_INFO pIfInfo = (WLAN_INTERFACE_INFO*)&pIfList->InterfaceInfo[i];
                    DWORD pdwDataSize = 0;
                    WLAN_CONNECTION_ATTRIBUTES* pConnectionAttributes = nullptr;

                    if( WlanQueryInterface(hClientHandle, &pIfInfo->InterfaceGuid,
                        wlan_intf_opcode_current_connection, nullptr,
                        &pdwDataSize, (PVOID*)&pConnectionAttributes, nullptr) == ERROR_SUCCESS )
                    {
                        m_deviceState.wifi_enabled = true;

                        if( pConnectionAttributes->isState == wlan_interface_state_connected )
                        {
                            m_deviceState.wifi_ssid = CString(pConnectionAttributes->strProfileName);
                            break;
                        }

                        WlanFreeMemory(pConnectionAttributes);
                    }
                }

                WlanFreeMemory(pIfList);
            }

            WlanCloseHandle(hClientHandle, nullptr);
        }
    }

    void DeviceStateEvent::GetBatteryLevelAndBatteryCharging()
    {
        SYSTEM_POWER_STATUS system_power_status;

        if( GetSystemPowerStatus(&system_power_status) )
        {
            if( ( system_power_status.BatteryFlag != 128 ) && // no system battery
                ( system_power_status.BatteryFlag != 255 ) )  // unknown status
            {
                if( system_power_status.BatteryLifePercent != 255 )
                    m_deviceState.battery_level = (double)system_power_status.BatteryLifePercent;

                m_deviceState.battery_charging = ( system_power_status.BatteryFlag == 8 );
            }
        }
    }

    void DeviceStateEvent::GetScreenBrightness()
    {
        HMONITOR hMonitor = MonitorFromWindow(GetDesktopWindow(), MONITOR_DEFAULTTOPRIMARY);

        if( hMonitor != nullptr )
        {
            DWORD dwPhysicalMonitors = 0;

            if( GetNumberOfPhysicalMonitorsFromHMONITOR(hMonitor, &dwPhysicalMonitors) == TRUE )
            {
                LPPHYSICAL_MONITOR pPhysicalMonitors = (LPPHYSICAL_MONITOR)malloc(dwPhysicalMonitors * sizeof(PHYSICAL_MONITOR));

                GetPhysicalMonitorsFromHMONITOR(hMonitor, dwPhysicalMonitors, pPhysicalMonitors);

                HANDLE hPhysicalMonitor = pPhysicalMonitors[0].hPhysicalMonitor;

                DWORD dwMinimumBrightness = 0;
                DWORD dwCurrentBrightness = 0;
                DWORD dwMaximumBrightness = 0;

                if( GetMonitorBrightness(hPhysicalMonitor, &dwMinimumBrightness, &dwCurrentBrightness, &dwMaximumBrightness) )
                    m_deviceState.screen_brightness = (double)dwCurrentBrightness;

                DestroyPhysicalMonitors(dwPhysicalMonitors, pPhysicalMonitors);

                free(pPhysicalMonitors);
            }
        }
    }

    void DeviceStateEvent::GetScreenOrientation()
    {
        DEVMODE DeviceMode;
        ZeroMemory(&DeviceMode, sizeof(DeviceMode));
        DeviceMode.dmSize = sizeof(DEVMODE);

        EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &DeviceMode);
        m_deviceState.screen_orientation_portrait = ( ( DeviceMode.dmDisplayOrientation == DMDO_90 ) || ( DeviceMode.dmDisplayOrientation == DMDO_270 ) );
    }
#endif
}
