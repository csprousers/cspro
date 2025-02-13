#include "StdAfx.h"
#include "ConnectionString.h"
#include <zDataO/DataRepositoryHelpers.h>


const TCHAR* const DataRepositoryTypeNames[] =
{
    _T("None"),
    _T("Text"),
    _T("CSProDB"),
    _T("EncryptedCSProDB"),
    _T("Memory"),
    _T("JSON"),
    _T("CSV"),
    _T("Semicolon"),
    _T("Tab"),
    _T("Excel"),
    _T("CSProExport"),
    _T("R"),
    _T("SAS"),
    _T("SPSS"),
    _T("Stata"),
};


const TCHAR* const DataRepositoryTypeDefaultExtensions[] =
{
    _T(""), // Null
    FileExtensions::Data::TextDataDefault,
    FileExtensions::Data::CSProDB,
    FileExtensions::Data::EncryptedCSProDB,
    _T(""), // Memory
    FileExtensions::Data::Json,
    FileExtensions::CSV,
    FileExtensions::SemicolonDelimited,
    FileExtensions::TabDelimited,
    FileExtensions::Excel,
    FileExtensions::Data::TextDataDefault, // CSProExport
    FileExtensions::RData,
    FileExtensions::SasData,
    FileExtensions::SpssData,
    FileExtensions::StataData,
};

static_assert(_countof(DataRepositoryTypeDefaultExtensions) == _countof(DataRepositoryTypeNames));


const TCHAR* ToString(const DataRepositoryType type)
{
    static const TCHAR* names[] =
    {
        DataRepositoryTypeNames[static_cast<size_t>(DataRepositoryType::Null)],
        DataRepositoryTypeNames[static_cast<size_t>(DataRepositoryType::Text)],
        _T("CSPro DB"),
        _T("Encrypted CSPro DB"),
        _T("In-Memory"),
        DataRepositoryTypeNames[static_cast<size_t>(DataRepositoryType::Json)],
        _T("Comma Delimited (CSV)"),
        _T("Semicolon Delimited"),
        _T("Tab Delimited"),
        DataRepositoryTypeNames[static_cast<size_t>(DataRepositoryType::Excel)],
        _T("CSPro (Exported)"),
        DataRepositoryTypeNames[static_cast<size_t>(DataRepositoryType::R)],
        DataRepositoryTypeNames[static_cast<size_t>(DataRepositoryType::SAS)],
        DataRepositoryTypeNames[static_cast<size_t>(DataRepositoryType::SPSS)],
        DataRepositoryTypeNames[static_cast<size_t>(DataRepositoryType::Stata)],
    };

    static_assert(_countof(names) == _countof(DataRepositoryTypeNames));

    return names[static_cast<size_t>(type)];
}


DataRepositoryType GetDataRepositoryTypeByExtension(const wstring_view filename_sv)
{
    const std::wstring extension = PortableFunctions::PathGetFileExtension(filename_sv);

    return ( extension.empty() && SO::IsBlank(filename_sv) )                       ? DataRepositoryType::Null :
           ( SO::EqualsNoCase(extension, FileExtensions::Data::CSProDB) )          ? DataRepositoryType::SQLite :
           ( SO::EqualsNoCase(extension, FileExtensions::Data::EncryptedCSProDB) ) ? DataRepositoryType::EncryptedSQLite :
           ( SO::EqualsNoCase(extension, FileExtensions::Data::Json) )             ? DataRepositoryType::Json :
           ( SO::EqualsNoCase(extension, FileExtensions::CSV) )                    ? DataRepositoryType::CommaDelimited :
           ( SO::EqualsNoCase(extension, FileExtensions::SemicolonDelimited) )     ? DataRepositoryType::SemicolonDelimited :
           ( SO::EqualsNoCase(extension, FileExtensions::TabDelimited) )           ? DataRepositoryType::TabDelimited :
           ( SO::EqualsNoCase(extension, FileExtensions::Excel) )                  ? DataRepositoryType::Excel :
           ( SO::EqualsOneOfNoCase(extension, FileExtensions::RData, _T("rda")) )  ? DataRepositoryType::R :
           ( SO::EqualsNoCase(extension, FileExtensions::SasData) )                ? DataRepositoryType::SAS :
           ( SO::EqualsNoCase(extension, FileExtensions::SpssData) )               ? DataRepositoryType::SPSS :
           ( SO::EqualsNoCase(extension, FileExtensions::StataData) )              ? DataRepositoryType::Stata :
                                                                                     DataRepositoryType::Text;
}


ConnectionString::ConnectionString(const wstring_view connection_string_text_sv)
    :   ConnectionString(true, DataRepositoryType::Null)
{
    InitializeFromString(connection_string_text_sv);

    InitializeDataRepositoryType();

    // blank connection strings will be undefined
    if( m_dataRepositoryType == DataRepositoryType::Null && connection_string_text_sv.find('|') == wstring_view::npos )
        m_defined = false;
}


void ConnectionString::InitializeDataRepositoryType()
{
    // prior to CSPro 7.7, the file extension always overrode any type specified in the connection string;
    // now, to support the ability to write to filenames with a extension that that might have defaulted
    // to an export format, the extension will only sometimes override the type
    const DataRepositoryType data_repository_type_from_extension = GetDataRepositoryTypeByExtension(m_filename);

    if( m_dataRepositoryType == data_repository_type_from_extension )
        return;

    // if exporting to CSPro format, let the filename be handled by the repository
    if( m_dataRepositoryType == DataRepositoryType::CSProExport )
        return;

    // text-based files can be written to any extension
    if( DataRepositoryHelpers::TypeWritesToText(m_dataRepositoryType) && DataRepositoryHelpers::TypeWritesToText(data_repository_type_from_extension) )
        return;

    // some connection strings without filenames are allowed
    if( data_repository_type_from_extension == DataRepositoryType::Null && DataRepositoryHelpers::TypeDoesNotUseFilename(m_dataRepositoryType) )
        return;

    // if here, then the type will be changed to the type based on the extension
    m_dataRepositoryType = data_repository_type_from_extension;
}


bool ConnectionString::PreprocessProperty(const wstring_view attribute_sv, const wstring_view value_sv)
{
    auto process_type = [&](const wstring_view text_sv) -> bool
    {
        for( size_t i = 0; i < _countof(DataRepositoryTypeNames); ++i )
        {
            if( SO::EqualsNoCase(text_sv, DataRepositoryTypeNames[i]) )
            {
                m_dataRepositoryType = static_cast<DataRepositoryType>(i);
                return true;
            }
        }

        return false;
    };

    // special handling for the type
    if( SO::EqualsNoCase(attribute_sv, ConnectionStringDataRepositoryPropertyType) )
    {
        process_type(value_sv);
        return true;
    }

    // prior to CSPro 7.3, only the repository type was specified (without an =),
    // so for backwards compatability, we will still process these
    else if( process_type(attribute_sv) )
    {
        return true;
    }

    else
    {
        return false;
    }
}


bool ConnectionString::Equals(const ConnectionString& rhs_connection_string) const
{
    // the properties are not compared, though they could be in the future for
    // some other repository type
    return ( m_defined && rhs_connection_string.m_defined ) &&
           ( m_dataRepositoryType == rhs_connection_string.m_dataRepositoryType ) &&
           ( SO::EqualsNoCase(m_filename, rhs_connection_string.m_filename) );
}


std::wstring ConnectionString::ToString(std::wstring filename) const
{
    ASSERT(m_defined);

    // the type information needs to be added:
    //   - when using the null or memory repositories (because there is no filename)
    //   - when the data repository type that would be calculated based on the extension differs from the actual type
    if( DataRepositoryHelpers::TypeDoesNotUseFilename(m_dataRepositoryType) || m_dataRepositoryType != GetDataRepositoryTypeByExtension(filename) )
    {
        std::vector<std::tuple<std::wstring, std::wstring>> properties = m_properties;
        properties.insert(properties.begin(), std::make_tuple(ConnectionStringDataRepositoryPropertyType, DataRepositoryTypeNames[static_cast<size_t>(m_dataRepositoryType)]));

        return PropertyString::ToString(std::move(filename), properties);
    }

    else
    {
        return PropertyString::ToString(std::move(filename), m_properties);
    }
}


std::wstring ConnectionString::ToStringWithoutDirectory() const
{
    return ToString(PortableFunctions::PathGetFilename(m_filename));
}


std::wstring ConnectionString::ToRelativeString(const wstring_view directory_name_sv, const bool use_display_mode/* = false*/) const
{
    if( !IsFilenamePresent() )
        return ToString();

    const std::wstring adjusted_directory_name = PortableFunctions::PathEnsureTrailingSlash<std::wstring>(directory_name_sv);

    return use_display_mode ? ToString(GetRelativeFNameForDisplay(adjusted_directory_name, m_filename)) :
                              ToString(GetRelativeFName(adjusted_directory_name, m_filename));
}


std::wstring ConnectionString::ToDisplayString() const
{
    ASSERT(m_defined);

    return IsFilenamePresent() ? PortableFunctions::PathGetFilename(m_filename) :
                                 DataRepositoryTypeNames[static_cast<size_t>(m_dataRepositoryType)];
}


void ConnectionString::AdjustRelativePath(const wstring_view directory_name_sv)
{
    if( !m_filename.empty() )
        m_filename = MakeFullPath(PortableFunctions::PathEnsureTrailingSlash<std::wstring>(directory_name_sv), m_filename);
}


std::wstring ConnectionString::GetName(const DataRepositoryNameType name_type) const
{
    ASSERT(m_defined);

    if( name_type == DataRepositoryNameType::Full )
    {
        return m_filename;
    }

    else if( name_type == DataRepositoryNameType::Concise )
    {
        return PortableFunctions::PathGetFilename(m_filename);
    }

    else
    {
        const TCHAR* repository_prefix =
            ( m_dataRepositoryType == DataRepositoryType::Null )   ? _T("Empty") :
            ( m_dataRepositoryType == DataRepositoryType::Text )   ? _T("Text File") :
            ( m_dataRepositoryType == DataRepositoryType::Memory ) ? _T("Memory") :
                                                                     ::ToString(m_dataRepositoryType);

        return FormatTextCS2WS(_T("<<%s>> %s"), repository_prefix, m_filename.c_str());
    }
}


void ConnectionString::WriteJson(JsonWriter& json_writer, const bool write_relative_path/* = true*/) const
{
    ASSERT(m_defined);

    json_writer.BeginObject();

    if( !m_filename.empty() )
    {
        if( write_relative_path )
        {
            json_writer.WriteRelativePath(JK::path, m_filename);
        }

        else
        {
            json_writer.WritePath(JK::path, m_filename);
        }
    }

    json_writer.Write(JK::type, ::ToString(m_dataRepositoryType));

    PropertyString::WriteJson(json_writer, false);

    json_writer.EndObject();
}
