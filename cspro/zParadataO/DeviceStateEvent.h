#pragma once
#include "Event.h"

namespace Paradata
{
    class ZPARADATAO_API DeviceStateEvent : public Event
    {
        DECLARE_PARADATA_EVENT(DeviceStateEvent)

    public:
        struct DeviceState
        {
            static const int ValuesToFill = 12;

            bool bluetooth_enabled;
            std::optional<bool> gps_enabled;
            bool wifi_enabled;
            std::optional<CString> wifi_ssid;
            std::optional<bool> mobile_network_enabled;
            std::optional<CString> mobile_network_type;
            std::optional<CString> mobile_network_name;
            std::optional<double> mobile_network_strength;
            std::optional<double> battery_level;
            std::optional<bool> battery_charging;
            std::optional<double> screen_brightness;
            bool screen_orientation_portrait;

            DeviceState();
        };

    private:
        DeviceState m_deviceState;

#ifdef WIN_DESKTOP
        void GetBluetoothEnabled();
        void GetWiFiEnabledAndWiFiSsid();
        void GetBatteryLevelAndBatteryCharging();
        void GetScreenBrightness();
        void GetScreenOrientation();
#endif

    public:
        DeviceStateEvent();
    };
}
