#pragma once

#include <zParadataO/Event.h>

namespace Paradata { class ApplicationEvent; }


class ZPARADATAO_API Paradata::ApplicationEvent : public Event
{
    DECLARE_PARADATA_EVENT(ApplicationEvent)

public:
    struct DeviceInfo
    {
        std::string user_name;
        std::string device_id;
        std::string operating_system;
        std::string operating_system_detailed;

        static constexpr size_t ValuesToFill = 12;

        std::string screen_width;
        std::string screen_height;
        std::string screen_inches;
        std::string memory_ram;
        std::string battery_capacity;
        std::string device_brand;
        std::string device_device;
        std::string device_hardware;
        std::string device_manufacturer;
        std::string device_model;
        std::string device_processor;
        std::string device_product;
    };

private:
    struct StartEventData;
    ApplicationEvent(std::unique_ptr<StartEventData> start_event_data);

public:
    static std::unique_ptr<ApplicationEvent> CreateStartEvent(std::string file_path, std::string app_type,
                                                              std::string name, double version,
                                                              std::string pff_file_path, int serializer);

    static std::unique_ptr<ApplicationEvent> CreateStopEvent();

private:
    bool PreSave(Log& log) const override;

    static DeviceInfo GetDeviceInfo();
    static double GetDeviceBootTime();

private:
    std::shared_ptr<StartEventData> m_startEventData;
};
