#include "stdafx.h"
#include "ApplicationEvent.h"
#include <zToolsO/Serializer.h>
#include <zToolsO/WinRegistry.h>
#include <zUtilO/Interapp.h>

using namespace Paradata;


void ApplicationEvent::SetupTables(Log& log)
{
    log.CreateTable(ParadataTable::ApplicationInfo)
            .AddColumn(_T("filename"), Table::ColumnType::Text)
            .AddColumn(_T("type"), Table::ColumnType::Text)
            .AddColumn(_T("name"), Table::ColumnType::Text)
            .AddColumn(_T("cspro_version"), Table::ColumnType::Double)
            .AddColumn(_T("pff_filename"), Table::ColumnType::Text)
            .AddColumn(_T("serializer"), Table::ColumnType::Integer)
        ;

    log.CreateTable(ParadataTable::DiagnosticsInfo)
            .AddColumn(_T("version"), Table::ColumnType::Double)
            .AddColumn(_T("version_detailed"), Table::ColumnType::Text)
            .AddColumn(_T("releasedate"), Table::ColumnType::Integer)
            .AddColumn(_T("beta"), Table::ColumnType::Integer)
            .AddColumn(_T("serializer"), Table::ColumnType::Integer)
        ;

    log.CreateTable(ParadataTable::DeviceInfo)
            .AddColumn(_T("username"), Table::ColumnType::Text)
            .AddColumn(_T("deviceid"), Table::ColumnType::Text)
            .AddColumn(_T("os"), Table::ColumnType::Text)
            .AddColumn(_T("os_detailed"), Table::ColumnType::Text)
            .AddColumn(_T("screen_width"), Table::ColumnType::Text)
            .AddColumn(_T("screen_height"), Table::ColumnType::Text)
            .AddColumn(_T("screen_inches"), Table::ColumnType::Text)
            .AddColumn(_T("memory_ram"), Table::ColumnType::Text)
            .AddColumn(_T("battery_capacity"), Table::ColumnType::Text)
            .AddColumn(_T("device_brand"), Table::ColumnType::Text)
            .AddColumn(_T("device_device"), Table::ColumnType::Text)
            .AddColumn(_T("device_hardware"), Table::ColumnType::Text)
            .AddColumn(_T("device_manufacturer"), Table::ColumnType::Text)
            .AddColumn(_T("device_model"), Table::ColumnType::Text)
            .AddColumn(_T("device_processor"), Table::ColumnType::Text)
            .AddColumn(_T("device_product"), Table::ColumnType::Text)
        ;

    log.CreateTable(ParadataTable::ApplicationInstance)
            .AddColumn(_T("application_info"), Table::ColumnType::Long)
            .AddColumn(_T("diagnostics_info"), Table::ColumnType::Long)
            .AddColumn(_T("device_info"), Table::ColumnType::Long)
            .AddColumn(_T("device_boot_time"), Table::ColumnType::Double)
            .AddColumn(_T("time_offset"), Table::ColumnType::Long)
            .AddColumn(_T("system_locale"), Table::ColumnType::Text)
            .AddColumn(_T("uuid"), Table::ColumnType::Text)
        ;

    log.CreateTable(ParadataTable::ApplicationEvent)
            .AddColumn(_T("action"), Table::ColumnType::Boolean)
                    .AddCode(0, _T("stop"))
                    .AddCode(1, _T("start"))
        ;
}


ApplicationEvent::ApplicationEvent(bool start)
    :   m_start(start),
        m_version(0),
        m_serializer(0),
        m_deviceBootTime(0)
{
}


std::shared_ptr<ApplicationEvent> ApplicationEvent::CreateStartEvent(const CString& filename, const CString& app_type,
                                                                     const CString& name, double version,
                                                                     const CString& pff_filename, int serializer)
{
    std::shared_ptr<ApplicationEvent> application_event(new ApplicationEvent(true));

    application_event->m_filename = filename;

    application_event->m_appType = app_type;
    application_event->m_name = name;
    application_event->m_version = version;
    application_event->m_pffFilename = pff_filename;
    application_event->m_serializer = serializer;

    application_event->GetDeviceInfo();

    return application_event;
}


std::shared_ptr<ApplicationEvent> ApplicationEvent::CreateStopEvent()
{
    return std::shared_ptr<ApplicationEvent>(new ApplicationEvent(false));
}


bool ApplicationEvent::PreSave(Log& log) const
{
    if( m_start )
    {
        // fill the application info table
        Table& application_info_table = log.GetTable(ParadataTable::ApplicationInfo);
        long application_info_id = 0;
        application_info_table.Insert(&application_info_id,
            m_filename.GetString(),
            m_appType.GetString(),
            m_name.GetString(),
            m_version,
            m_pffFilename.GetString(),
            m_serializer
        );


        // fill the diagnostics info table
        Table& diagnostics_info_table = log.GetTable(ParadataTable::DiagnosticsInfo);
        long diagnostics_info_id = 0;
        diagnostics_info_table.Insert(&diagnostics_info_id,
            CSPRO_VERSION_NUMBER,
            CSPRO_VERSION_NUMBER_DETAILED_TEXT,
            Versioning::GetReleaseDate(),
            IsBetaBuild() ? 1 : 0,
            Serializer::GetCurrentVersion()
        );


        // fill the device info table
        Table& device_info_table = log.GetTable(ParadataTable::DeviceInfo);
        long device_info_id = 0;
        device_info_table.Insert(&device_info_id,
            m_deviceInfo.user_name.GetString(),
            m_deviceInfo.device_id.GetString(),
            m_deviceInfo.operating_system.c_str(),
            m_deviceInfo.operating_system_detailed.c_str(),
            m_deviceInfo.screen_width.GetString(),
            m_deviceInfo.screen_height.GetString(),
            m_deviceInfo.screen_inches.GetString(),
            m_deviceInfo.memory_ram.GetString(),
            m_deviceInfo.battery_capacity.GetString(),
            m_deviceInfo.device_brand.GetString(),
            m_deviceInfo.device_device.GetString(),
            m_deviceInfo.device_hardware.GetString(),
            m_deviceInfo.device_manufacturer.GetString(),
            m_deviceInfo.device_model.GetString(),
            m_deviceInfo.device_processor.GetString(),
            m_deviceInfo.device_product.GetString()
        );


        // fill the application instance table
        Table& application_instance_table = log.GetTable(ParadataTable::ApplicationInstance);
        long application_instance_id = 0;

        application_instance_table.Insert(&application_instance_id,
            application_info_id,
            diagnostics_info_id,
            device_info_id,
            m_deviceBootTime,
            GetUtcOffset(),
            GetLocaleLanguage().GetString(),
            CreateUuid().c_str()
        );

        log.StartInstance(Log::Instance::Application, application_instance_id);
    }

    return log.GetInstance(Log::Instance::Application).has_value();
}


void ApplicationEvent::Save(Log& log, long base_event_id) const
{
    Table& application_event_table = log.GetTable(ParadataTable::ApplicationEvent);
    application_event_table.Insert(&base_event_id,
        m_start
    );

    if( !m_start )
        log.StopInstance(Log::Instance::Application);
}


void ApplicationEvent::GetDeviceInfo()
{
    // fill in the attributes common to all platforms
    m_deviceInfo.device_id = GetDeviceId();

    m_deviceInfo.user_name = GetDeviceUserName();

    const OperatingSystemDetails& operating_system_details = GetOperatingSystemDetails();
    m_deviceInfo.operating_system = operating_system_details.operating_system;
    m_deviceInfo.operating_system_detailed = operating_system_details.version_number;


    // fill in the platform-specific values
#ifdef WIN_DESKTOP
    m_deviceInfo.screen_width = IntToString(GetSystemMetrics(SM_CXVIRTUALSCREEN));

    m_deviceInfo.screen_height = IntToString(GetSystemMetrics(SM_CYVIRTUALSCREEN));

    // m_deviceInfo.screen_inches will not be assigned

    ULONGLONG physically_installed_system_memory;
    GetPhysicallyInstalledSystemMemory(&physically_installed_system_memory);
    // the value is reported in kilobytes so convert it to bytes
    physically_installed_system_memory *= 1024; 
    m_deviceInfo.memory_ram = IntToString(physically_installed_system_memory);

    // m_deviceInfo.battery_capacity will not be assigned
    // m_deviceInfo.device_brand will not be assigned
    // m_deviceInfo.device_device will not be assigned
    // m_deviceInfo.device_hardware will not be assigned

    WinRegistry registry;
    registry.Open(HKEY_LOCAL_MACHINE, _T("Hardware\\Description\\System\\BIOS"));
    registry.ReadString(_T("SystemManufacturer"), &m_deviceInfo.device_manufacturer);
    registry.ReadString(_T("SystemProductName"), &m_deviceInfo.device_model);

    registry.Open(HKEY_LOCAL_MACHINE, _T("Hardware\\Description\\System\\CentralProcessor\\0"));
    registry.ReadString(_T("ProcessorNameString"), &m_deviceInfo.device_processor);

    // pDeviceInfo->device_product will not be assigned

#else
    PlatformInterface::GetInstance()->GetApplicationInterface()->ParadataDeviceInfoQuery(m_deviceInfo);
#endif


    // calculate the device boot time
    double up_time =
#ifdef WIN_DESKTOP
        GetTickCount64() / 1000.0;
#else
        PlatformInterface::GetInstance()->GetApplicationInterface()->GetUpTime();
#endif
    m_deviceBootTime = ::GetTimestamp() - up_time;
}
