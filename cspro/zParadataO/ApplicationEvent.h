#pragma once

#include <zParadataO/Event.h>


namespace Paradata
{
    class ZPARADATAO_API ApplicationEvent : public Event
    {
        DECLARE_PARADATA_EVENT(ApplicationEvent)

    public:
        struct DeviceInfo
        {
            CString user_name;
            CString device_id;
            std::wstring operating_system;
            std::wstring operating_system_detailed;

            static const int ValuesToFill = 12;

            CString screen_width;
            CString screen_height;
            CString screen_inches;
            CString memory_ram;
            CString battery_capacity;
            CString device_brand;
            CString device_device;
            CString device_hardware;
            CString device_manufacturer;
            CString device_model;
            CString device_processor;
            CString device_product;
        };

    private:
        ApplicationEvent(bool start);

    public:
        static std::shared_ptr<ApplicationEvent> CreateStartEvent(const CString& filename, const CString& app_type,
                                                                  const CString& name, double version,
                                                                  const CString& pff_filename, int serializer);

        static std::shared_ptr<ApplicationEvent> CreateStopEvent();

    private:
        void GetDeviceInfo();

        bool PreSave(Log& log) const override;

    private:
        bool m_start;

        CString m_filename;
        CString m_appType;
        CString m_name;
        double m_version;
        CString m_pffFilename;
        int m_serializer;

        DeviceInfo m_deviceInfo;
        double m_deviceBootTime;
    };
}
