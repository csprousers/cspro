#include "Stdafx.h"
#include "ConnectionString.h"
#include <zToolsO/PortableFunctions.h>
#include <zUtilO/ConnectionString.h>
#include <zDataO/DataRepositoryHelpers.h>


CSPro::Util::ConnectionString::ConnectionString(::ConnectionString connection_string)
    :   m_nativeConnectionString(new ::ConnectionString(std::move(connection_string)))
{
}


CSPro::Util::ConnectionString::ConnectionString(System::String^ connection_string_text)
{
    m_nativeConnectionString = ( connection_string_text == nullptr ) ? new ::ConnectionString() :
                                                                       new ::ConnectionString(connection_string_text);
}


CSPro::Util::ConnectionString^ CSPro::Util::ConnectionString::CreateNullableConnectionString(::ConnectionString connection_string)
{
    return connection_string.IsDefined() ? gcnew CSPro::Util::ConnectionString(std::move(connection_string)) :
                                           nullptr;
}


CSPro::Util::ConnectionString::!ConnectionString()
{
    delete m_nativeConnectionString;
}


System::String^ CSPro::Util::ConnectionString::Filename::get()
{
    return gcnew System::String(m_nativeConnectionString->GetFilename().c_str());
}


bool CSPro::Util::ConnectionString::FilenameMatches(System::String^ filename)
{
    return m_nativeConnectionString->FilenameMatches(CString(filename));
}


CSPro::Util::DataRepositoryType CSPro::Util::ConnectionString::Type::get()
{
    return CSPro::Util::DataRepositoryType(m_nativeConnectionString->GetType());
}


bool CSPro::Util::ConnectionString::TypeContainsEmbeddedDictionary::get()
{
    return DataRepositoryHelpers::DoesTypeContainEmbeddedDictionary(m_nativeConnectionString->GetType());
}


System::String^ CSPro::Util::ConnectionString::ToString()
{
    return gcnew System::String(m_nativeConnectionString->ToString().c_str());
}


System::String^ CSPro::Util::ConnectionString::ToRelativeString(System::String^ directory_name)
{
    return gcnew System::String(m_nativeConnectionString->ToRelativeString(CS2WS(directory_name)).c_str());
}


void CSPro::Util::ConnectionString::AdjustRelativePath(System::String^ directory_name)
{
    m_nativeConnectionString->AdjustRelativePath(CS2WS(directory_name));
}


const ::ConnectionString& CSPro::Util::ConnectionString::GetNativeConnectionString()
{
    return *m_nativeConnectionString;
}


// some methods used by the PFF Editor
System::String^ CSPro::Util::ConnectionString::GetDataRepositoryTypeDisplayText(DataRepositoryType type)
{
    return gcnew System::String(::ToString((::DataRepositoryType)type));
}


System::String^ CSPro::Util::ConnectionString::ToStringWithModifiedType(DataRepositoryType new_type)
{
    if( new_type == DataRepositoryType::Null )
    {
        return gcnew System::String(::ConnectionString::CreateNullRepositoryConnectionString().ToString().c_str());
    }

    else
    {
        CString filename = m_nativeConnectionString->IsFilenamePresent() ? WS2CS(m_nativeConnectionString->GetFilename()) : _T("data-file");
        CString directory = PortableFunctions::PathGetDirectory<CString>(filename);
        CString new_filename_without_extension = PortableFunctions::PathEnsureTrailingSlash<CString>(directory) +
                                                 PortableFunctions::PathGetFilenameWithoutExtension<CString>(filename);

        // add the default extension for this new type
        ::ConnectionString new_connection_string(new_filename_without_extension + _T(".") +
                                                 DataRepositoryTypeDefaultExtensions[(size_t)new_type]);

        // add any properties from the old connection string
        for( const auto& [attribute, value] : m_nativeConnectionString->GetProperties() )
            new_connection_string.SetProperty(attribute, value);

        // create a new connection string with the new type forced on
        CString new_connection_string_text = WS2CS(new_connection_string.ToString());

        new_connection_string_text.AppendFormat(_T("%c%s=%s"),
            ( new_connection_string_text.Find(_T('|')) < 0 ) ? _T('|'): _T('&'),
            ConnectionStringDataRepositoryPropertyType, DataRepositoryTypeNames[(size_t)new_type]);
        
        return gcnew System::String(::ConnectionString(new_connection_string_text).ToString().c_str());
    }
}
