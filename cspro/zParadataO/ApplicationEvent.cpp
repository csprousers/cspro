#include "stdafx.h"
#include "ApplicationEvent.h"
#include <zToolsO/Serializer.h>
#include <zToolsO/WinRegistry.h>
#include <zUtilO/Interapp.h>

using namespace Paradata;


struct ApplicationEvent::StartEventData
{
    std::string file_path;
    std::string app_type;
    std::string name;
    double version;
    std::string pff_file_path;
    int serializer;

    DeviceInfo device_info;
    double device_boot_time;
};


void ApplicationEvent::SetupTables(Log& log)
{
    log.CreateTable(ParadataTable::ApplicationInfo)
            .AddColumn("filename", Table::ColumnType::Text)
            .AddColumn("type", Table::ColumnType::Text)
            .AddColumn("name", Table::ColumnType::Text)
            .AddColumn("cspro_version", Table::ColumnType::Double)
            .AddColumn("pff_filename", Table::ColumnType::Text)
            .AddColumn("serializer", Table::ColumnType::Integer)
        ;

    log.CreateTable(ParadataTable::DiagnosticsInfo)
            .AddColumn("version", Table::ColumnType::Double)
            .AddColumn("version_detailed", Table::ColumnType::Text)
            .AddColumn("releasedate", Table::ColumnType::Integer)
            .AddColumn("beta", Table::ColumnType::Integer)
            .AddColumn("serializer", Table::ColumnType::Integer)
        ;

    log.CreateTable(ParadataTable::DeviceInfo)
            .AddColumn("username", Table::ColumnType::Text)
            .AddColumn("deviceid", Table::ColumnType::Text)
            .AddColumn("os", Table::ColumnType::Text)
            .AddColumn("os_detailed", Table::ColumnType::Text)
            .AddColumn("screen_width", Table::ColumnType::Text)
            .AddColumn("screen_height", Table::ColumnType::Text)
            .AddColumn("screen_inches", Table::ColumnType::Text)
            .AddColumn("memory_ram", Table::ColumnType::Text)
            .AddColumn("battery_capacity", Table::ColumnType::Text)
            .AddColumn("device_brand", Table::ColumnType::Text)
            .AddColumn("device_device", Table::ColumnType::Text)
            .AddColumn("device_hardware", Table::ColumnType::Text)
            .AddColumn("device_manufacturer", Table::ColumnType::Text)
            .AddColumn("device_model", Table::ColumnType::Text)
            .AddColumn("device_processor", Table::ColumnType::Text)
            .AddColumn("device_product", Table::ColumnType::Text)
        ;

    log.CreateTable(ParadataTable::ApplicationInstance)
            .AddColumn("application_info", Table::ColumnType::Long)
            .AddColumn("diagnostics_info", Table::ColumnType::Long)
            .AddColumn("device_info", Table::ColumnType::Long)
            .AddColumn("device_boot_time", Table::ColumnType::Double)
            .AddColumn("time_offset", Table::ColumnType::Long)
            .AddColumn("system_locale", Table::ColumnType::Text)
            .AddColumn("uuid", Table::ColumnType::Text)
        ;

    log.CreateTable(ParadataTable::ApplicationEvent)
            .AddColumn("action", Table::ColumnType::Boolean)
                    .AddCode(0, "stop")
                    .AddCode(1, "start")
        ;
}


ApplicationEvent::ApplicationEvent(std::unique_ptr<StartEventData> start_event_data)
    :   m_startEventData(std::move(start_event_data))
{
}


std::unique_ptr<ApplicationEvent> ApplicationEvent::CreateStartEvent(std::string file_path, std::string app_type,
                                                                     std::string name, const double version,
                                                                     std::string pff_file_path, const int serializer)
{
    return std::unique_ptr<ApplicationEvent>(new ApplicationEvent(std::make_unique<ApplicationEvent::StartEventData>(ApplicationEvent::StartEventData
    {
        std::move(file_path),
        std::move(app_type),
        std::move(name),
        version,
        std::move(pff_file_path),
        serializer,
        GetDeviceInfo(),
        GetDeviceBootTime()
    })));
}


std::unique_ptr<ApplicationEvent> ApplicationEvent::CreateStopEvent()
{
    return std::unique_ptr<ApplicationEvent>(new ApplicationEvent(nullptr));
}


bool ApplicationEvent::PreSave(Log& log) const
{
    if( m_startEventData != nullptr )
    {
        // fill the application info table
        Table& application_info_table = log.GetTable(ParadataTable::ApplicationInfo);
        long application_info_id = 0;
        application_info_table.Insert(&application_info_id,
            m_startEventData->file_path.c_str(),
            m_startEventData->app_type.c_str(),
            m_startEventData->name.c_str(),
            m_startEventData->version,
            m_startEventData->pff_file_path.c_str(),
            m_startEventData->serializer
        );


        // fill the diagnostics info table
        Table& diagnostics_info_table = log.GetTable(ParadataTable::DiagnosticsInfo);
        long diagnostics_info_id = 0;
        diagnostics_info_table.Insert(&diagnostics_info_id,
            Versioning::Number,
            Versioning::NumberDetailedText,
            Versioning::GetReleaseDate(),
            Versioning::IsPrerelease ? 1 : 0,
            Serializer::GetCurrentVersion()
        );


        // fill the device info table
        Table& device_info_table = log.GetTable(ParadataTable::DeviceInfo);
        long device_info_id = 0;
        device_info_table.Insert(&device_info_id,
            m_startEventData->device_info.user_name.c_str(),
            m_startEventData->device_info.device_id.c_str(),
            m_startEventData->device_info.operating_system.c_str(),
            m_startEventData->device_info.operating_system_detailed.c_str(),
            m_startEventData->device_info.screen_width.c_str(),
            m_startEventData->device_info.screen_height.c_str(),
            m_startEventData->device_info.screen_inches.c_str(),
            m_startEventData->device_info.memory_ram.c_str(),
            m_startEventData->device_info.battery_capacity.c_str(),
            m_startEventData->device_info.device_brand.c_str(),
            m_startEventData->device_info.device_device.c_str(),
            m_startEventData->device_info.device_hardware.c_str(),
            m_startEventData->device_info.device_manufacturer.c_str(),
            m_startEventData->device_info.device_model.c_str(),
            m_startEventData->device_info.device_processor.c_str(),
            m_startEventData->device_info.device_product.c_str()
        );


        // fill the application instance table
        Table& application_instance_table = log.GetTable(ParadataTable::ApplicationInstance);
        long application_instance_id = 0;

        application_instance_table.Insert(&application_instance_id,
            application_info_id,
            diagnostics_info_id,
            device_info_id,
            m_startEventData->device_boot_time,
            DateTime::GetUtcOffsetNow(),
            GetLocaleLanguage().c_str(),
            CreateUuid().c_str()
        );

        log.StartInstance(Log::Instance::Application, application_instance_id);
    }

    return log.GetInstance(Log::Instance::Application).has_value();
}


void ApplicationEvent::Save(Log& log, long base_event_id) const
{
    const bool start_event = ( m_startEventData != nullptr );

    Table& application_event_table = log.GetTable(ParadataTable::ApplicationEvent);
    application_event_table.Insert(&base_event_id,
        start_event
    );

    if( !start_event )
        log.StopInstance(Log::Instance::Application);
}


ApplicationEvent::DeviceInfo ApplicationEvent::GetDeviceInfo()
{
    const OperatingSystemDetails& operating_system_details = GetOperatingSystemDetails();

    // fill in the attributes common to all platforms
    DeviceInfo device_info
    {
        GetDeviceId(),
        GetDeviceUserName(),
        operating_system_details.operating_system,
        operating_system_details.version_number
    };

    // fill in the platform-specific values
#ifdef WIN_DESKTOP
    device_info.screen_width = IntToString(GetSystemMetrics(SM_CXVIRTUALSCREEN));

    device_info.screen_height = IntToString(GetSystemMetrics(SM_CYVIRTUALSCREEN));

    // device_info.screen_inches will not be assigned

    ULONGLONG physically_installed_system_memory;
    GetPhysicallyInstalledSystemMemory(&physically_installed_system_memory);
    // the value is reported in kilobytes so convert it to bytes
    physically_installed_system_memory *= 1024;
    device_info.memory_ram = IntToString(physically_installed_system_memory);

    // device_info.battery_capacity will not be assigned
    // device_info.device_brand will not be assigned
    // device_info.device_device will not be assigned
    // device_info.device_hardware will not be assigned

    WinRegistry registry;
    registry.Open(HKEY_LOCAL_MACHINE, "Hardware\\Description\\System\\BIOS");
    registry.ReadString("SystemManufacturer", device_info.device_manufacturer);
    registry.ReadString("SystemProductName", device_info.device_model);

    registry.Open(HKEY_LOCAL_MACHINE, "Hardware\\Description\\System\\CentralProcessor\\0");
    registry.ReadString("ProcessorNameString", device_info.device_processor);

    // device_info.device_product will not be assigned

#else
    PlatformInterface::GetInstance()->GetApplicationInterface()->ParadataDeviceInfoQuery(device_info);

#endif

    return device_info;
}


double ApplicationEvent::GetDeviceBootTime()
{
    // calculate the device boot time
    const double up_time =
#ifdef WIN_DESKTOP
        GetTickCount64() / 1000.0;
#else
        PlatformInterface::GetInstance()->GetApplicationInterface()->GetUpTime();
#endif
    return ::GetTimestamp<double>() - up_time;
}
