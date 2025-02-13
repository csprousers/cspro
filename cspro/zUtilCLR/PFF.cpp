#include "Stdafx.h"
#include "PFF.h"
#include <zUtilO/FileExtensions.h>
#include <zAppO/PFF.h>


CSPro::Util::PFF::PFF()
    :   m_pff(new ::PFF())
{
}


CSPro::Util::PFF::PFF(System::String^ filename)
    :   m_pff(new ::PFF(CString(filename)))
{
}


CSPro::Util::PFF::!PFF()
{
    delete m_pff;
}


System::String^ CSPro::Util::PFF::Extension::get()
{
     return gcnew System::String(FileExtensions::WithDot::Pff);
}


bool CSPro::Util::PFF::Load()
{
    return m_pff->LoadPifFile(true);
}


bool CSPro::Util::PFF::Save(System::String^ filename)
{
    m_pff->SetPifFileName(CString(filename));
    return m_pff->Save();
}


void CSPro::Util::PFF::ExecuteOnExitPff()
{
    m_pff->ExecuteOnExitPff();
}


System::String^ CSPro::Util::PFF::PffFilename::get()
{
    return gcnew System::String(m_pff->GetPifFileName());
}

void CSPro::Util::PFF::PffFilename::set(System::String^ value)
{
    m_pff->SetPifFileName(CString(value));
}


CSPro::Util::AppType CSPro::Util::PFF::AppType::get()
{
    return CSPro::Util::AppType(m_pff->GetAppType());
}

void CSPro::Util::PFF::AppType::set(CSPro::Util::AppType value)
{
    m_pff->SetAppType(::APPTYPE(value));
}


System::String^ CSPro::Util::PFF::AppFName::get()
{
    return gcnew System::String(m_pff->GetAppFName());
}

void CSPro::Util::PFF::AppFName::set(System::String^ value)
{
    m_pff->SetAppFName(CString(value));
}


System::String^ CSPro::Util::PFF::ExcelFilename::get()
{
    return gcnew System::String(m_pff->GetExcelFilename());
}


System::String^ CSPro::Util::PFF::InputDictFName::get()
{
    return gcnew System::String(m_pff->GetInputDictFName());
}


CSPro::Util::ConnectionString^ CSPro::Util::PFF::SingleOutputDataConnectionString::get()
{
    return CSPro::Util::ConnectionString::CreateNullableConnectionString(m_pff->GetSingleOutputDataConnectionString());
}


array<System::String^>^ CSPro::Util::PFF::ExternalDictionaryNames::get()
{
    const auto& external_data_connection_strings = m_pff->GetExternalDataConnectionStrings();
    auto dictionary_names = gcnew array<System::String^>(external_data_connection_strings.size());

    int i = 0;

    for( const auto& [dictionary_name, connection_string] : external_data_connection_strings )
    {
        dictionary_names[i] = gcnew System::String(dictionary_name);
        i++;
    }

    return dictionary_names;
}

CSPro::Util::ConnectionString^ CSPro::Util::PFF::GetExternalDataConnectionString(System::String^ dictionary_name)
{
    return CSPro::Util::ConnectionString::CreateNullableConnectionString(m_pff->GetExternalDataConnectionString(dictionary_name));
}


System::String^ CSPro::Util::PFF::SyncUrl::get()
{
    return gcnew System::String(m_pff->GetSyncUrl());
}

void CSPro::Util::PFF::SyncUrl::set(System::String^ value)
{
    m_pff->SetSyncUrl(CString(value));
}


CSPro::Util::SyncDirection CSPro::Util::PFF::SyncDirection::get()
{
    return CSPro::Util::SyncDirection(m_pff->GetSyncDirection());
}

void CSPro::Util::PFF::SyncDirection::set(CSPro::Util::SyncDirection value)
{
    m_pff->SetSyncDirection(::SyncDirection(value));
}


CSPro::Util::SyncServerType CSPro::Util::PFF::SyncServerType::get()
{
    return CSPro::Util::SyncServerType(m_pff->GetSyncServerType());
}

void CSPro::Util::PFF::SyncServerType::set(CSPro::Util::SyncServerType value)
{
    m_pff->SetSyncServerType(::SyncServerType(value));
}


bool CSPro::Util::PFF::Silent::get()
{
    return m_pff->GetSilent();
}


System::Collections::Generic::List<System::String^>^ CSPro::Util::PFF::CustomParameterMappings::get()
{
    auto mappings = gcnew System::Collections::Generic::List<System::String^>;

    for( const std::wstring& mapping : m_pff->GetCustomParamMappings() )
        mappings->Add(gcnew System::String(mapping.c_str()));

    return mappings;
}


CSPro::Util::DeployToOverride CSPro::Util::PFF::DeployToOverride::get()
{
    return CSPro::Util::DeployToOverride(m_pff->GetDeployToOverride());
}

void CSPro::Util::PFF::DeployToOverride::set(CSPro::Util::DeployToOverride value)
{
    m_pff->SetDeployToOverride(::DeployToOverride(value));
}
