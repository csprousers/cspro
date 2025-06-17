#include "Stdafx.h"
#include "ConnectionString.h"
#include <zToolsO/PortableFunctions.h>
#include <zUtilO/ConnectionString.h>
#include <zDataO/DataRepositoryHelpers.h>
#include <zBridgeO/DataFileDlg.h>


CSPro::Util::ConnectionString::ConnectionString(::ConnectionString connection_string)
    :   m_nativeConnectionString(new ::ConnectionString(std::move(connection_string)))
{
}


CSPro::Util::ConnectionString::ConnectionString(System::String^ connection_string_text)
{
    m_nativeConnectionString = ( connection_string_text == nullptr ) ? new ::ConnectionString() :
                                                                       new ::ConnectionString(clr_helpers::to_string(connection_string_text));
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


System::String^ CSPro::Util::ConnectionString::FilePath::get()
{
    return clr_helpers::to_SystemString(m_nativeConnectionString->GetFilePath());
}


CSPro::Util::DataRepositoryType CSPro::Util::ConnectionString::Type::get()
{
    return CSPro::Util::DataRepositoryType(m_nativeConnectionString->GetType());
}


bool CSPro::Util::ConnectionString::HasResource::get()
{
    return m_nativeConnectionString->HasResource();
}


bool CSPro::Util::ConnectionString::HasFilePath::get()
{
    return m_nativeConnectionString->HasFilePath();
}


bool CSPro::Util::ConnectionString::TypeContainsEmbeddedDictionary::get()
{
    return DataRepositoryHelpers::IsTypeFileBasedWithAnEmbeddedDictionary(m_nativeConnectionString->GetType());
}


System::String^ CSPro::Util::ConnectionString::ToString()
{
    return clr_helpers::to_SystemString(m_nativeConnectionString->ToString());
}


System::String^ CSPro::Util::ConnectionString::ToDisplayString(bool use_filename_only)
{
    return clr_helpers::to_SystemString(m_nativeConnectionString->ToDisplayString(use_filename_only));
}


System::String^ CSPro::Util::ConnectionString::ToDisplayString()
{
    return clr_helpers::to_SystemString(m_nativeConnectionString->ToDisplayString());
}


System::String^ CSPro::Util::ConnectionString::ToRelativeString(System::String^ directory_name)
{
    return clr_helpers::to_SystemString(m_nativeConnectionString->ToRelativeString(clr_helpers::to_string(directory_name)));
}


void CSPro::Util::ConnectionString::AdjustRelativePath(System::String^ directory_name)
{
    m_nativeConnectionString->AdjustRelativePath(clr_helpers::to_string(directory_name));
}


const ::ConnectionString& CSPro::Util::ConnectionString::GetNativeConnectionString()
{
    return *m_nativeConnectionString;
}


// some methods used by the PFF Editor
System::String^ CSPro::Util::ConnectionString::GetDataRepositoryTypeDisplayText(DataRepositoryType type)
{
    return clr_helpers::to_SystemString(::ToString((::DataRepositoryType)type));
}


System::String^ CSPro::Util::ConnectionString::ToStringWithModifiedType(DataRepositoryType new_type)
{
    if( new_type == DataRepositoryType::Null )
    {
        return clr_helpers::to_SystemString(::ConnectionString::CreateNullRepositoryConnectionString().ToString());
    }

    else
    {
        std::string file_path = m_nativeConnectionString->HasFilePath() ? m_nativeConnectionString->GetFilePath() : "data-file";
        std::string directory = PortableFunctions::PathGetDirectory(file_path);
        std::string new_file_path_without_extension = PortableFunctions::PathEnsureTrailingSlash(directory) +
                                                      Path::GetFilenameWithoutExtension(file_path);

        // add the default extension for this new type
        ::ConnectionString new_connection_string(PortableFunctions::PathAppendFileExtension(new_file_path_without_extension,
                                                                                            DataRepositoryTypeDefaultExtensions[static_cast<size_t>(new_type)]));

        // add any properties from the old connection string
        for( const auto& [attribute, value] : m_nativeConnectionString->GetProperties() )
            new_connection_string.SetProperty(attribute, value);

        // create a new connection string with the new type forced on
        std::string new_connection_string_text = new_connection_string.ToString();

        const char separator = ( new_connection_string_text.find(PropertyString::PropertySeparatorInitial) == std::string::npos ) ? PropertyString::PropertySeparatorInitial :
                                                                                                                                    PropertyString::PropertySeparatorAdditional;
        new_connection_string_text.push_back(separator);

        new_connection_string_text.append(ConnectionStringDataRepositoryPropertyType);
        new_connection_string_text.push_back('=');
        new_connection_string_text.append(DataRepositoryTypeNames[static_cast<size_t>(new_type)]);

        return clr_helpers::to_SystemString(::ConnectionString(new_connection_string_text).ToString());
    }
}



CSPro::Util::ConnectionString^ CSPro::Util::ConnectionString::ShowDataFileDlg(System::IntPtr^ hWndOwner, DataFileDlgType type, const bool add_only_readable_types,
                                                                              ConnectionString^ connection_string)
{
    const DataFileDlg::Type native_type = ( type == DataFileDlgType::OpenExisting )  ? DataFileDlg::Type::OpenExisting :
                                          ( type == DataFileDlgType::OpenOrCreate )  ? DataFileDlg::Type::OpenOrCreate :
                                        /*( type == DataFileDlgType::CreateNew )*/     DataFileDlg::Type::CreateNew;

    ::ConnectionString native_connection_string = ( connection_string != nullptr ) ? connection_string->GetNativeConnectionString() :
                                                                                     ::ConnectionString();

    std::optional<::ConnectionString> selected_connection_string =
        DataFileDlg::ShowDialogFromWinForms(CWnd::FromHandle(reinterpret_cast<HWND>(hWndOwner->ToInt32())),
                                            native_type,
                                            add_only_readable_types,
                                            std::move(native_connection_string));

    return selected_connection_string.has_value() ? gcnew ConnectionString(std::move(*selected_connection_string)) :
                                                    nullptr;
}
