#pragma once

#include <zParadataO/Event.h>

namespace Paradata { class DeviceStateEvent; }


class ZPARADATAO_API Paradata::DeviceStateEvent : public Event
{
    DECLARE_PARADATA_EVENT(DeviceStateEvent)

public:
    struct DeviceState
    {
        static const size_t ValuesToFill = 12;

        bool bluetooth_enabled = false;
        std::optional<bool> gps_enabled;
        bool wifi_enabled = false;
        std::optional<std::string> wifi_ssid;
        std::optional<bool> mobile_network_enabled;
        std::optional<std::string> mobile_network_type;
        std::optional<std::string> mobile_network_name;
        std::optional<double> mobile_network_strength;
        std::optional<double> battery_level;
        std::optional<bool> battery_charging;
        std::optional<double> screen_brightness;
        bool screen_orientation_portrait = false;
    };

public:
    DeviceStateEvent();

private:
#ifdef WIN_DESKTOP
    void GetBluetoothEnabled();
    void GetWiFiEnabledAndWiFiSsid();
    void GetBatteryLevelAndBatteryCharging();
    void GetScreenBrightness();
    void GetScreenOrientation();
#endif

private:
    DeviceState m_deviceState;
};
