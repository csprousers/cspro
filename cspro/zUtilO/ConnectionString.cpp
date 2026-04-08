#include "StdAfx.h"
#include "ConnectionString.h"
#include "CustomUri.h"
#include <zDataO/ConnectionStringProperties.h>
#include <zDataO/DataRepositoryHelpers.h>


const char* const DataRepositoryTypeNames[] =
{
    "None",
    "Text",
    "CSProDB",
    "EncryptedCSProDB",
    "Memory",
    "JSON",
    "CSWeb",
    "CSV",
    "Semicolon",
    "Tab",
    "Excel",
    "CSProExport",
    "R",
    "SAS",
    "SPSS",
    "Stata",
};


DEFINE_ENUM_JSON_SERIALIZER_CLASS(DataRepositoryType,
    { DataRepositoryType::Null, DataRepositoryTypeNames[static_cast<size_t>(DataRepositoryType::Null)] },
    { DataRepositoryType::Text, DataRepositoryTypeNames[static_cast<size_t>(DataRepositoryType::Text)] },
    { DataRepositoryType::SQLite, DataRepositoryTypeNames[static_cast<size_t>(DataRepositoryType::SQLite)] },
    { DataRepositoryType::EncryptedSQLite, DataRepositoryTypeNames[static_cast<size_t>(DataRepositoryType::EncryptedSQLite)] },
    { DataRepositoryType::Memory, DataRepositoryTypeNames[static_cast<size_t>(DataRepositoryType::Memory)] },
    { DataRepositoryType::Json, DataRepositoryTypeNames[static_cast<size_t>(DataRepositoryType::Json)] },
    { DataRepositoryType::CSWeb, DataRepositoryTypeNames[static_cast<size_t>(DataRepositoryType::CSWeb)] },
    { DataRepositoryType::CommaDelimited, DataRepositoryTypeNames[static_cast<size_t>(DataRepositoryType::CommaDelimited)] },
    { DataRepositoryType::SemicolonDelimited, DataRepositoryTypeNames[static_cast<size_t>(DataRepositoryType::SemicolonDelimited)] },
    { DataRepositoryType::TabDelimited, DataRepositoryTypeNames[static_cast<size_t>(DataRepositoryType::TabDelimited)] },
    { DataRepositoryType::Excel, DataRepositoryTypeNames[static_cast<size_t>(DataRepositoryType::Excel)] },
    { DataRepositoryType::CSProExport, DataRepositoryTypeNames[static_cast<size_t>(DataRepositoryType::CSProExport)] },
    { DataRepositoryType::R, DataRepositoryTypeNames[static_cast<size_t>(DataRepositoryType::R)] },
    { DataRepositoryType::SAS, DataRepositoryTypeNames[static_cast<size_t>(DataRepositoryType::SAS)] },
    { DataRepositoryType::SPSS, DataRepositoryTypeNames[static_cast<size_t>(DataRepositoryType::SPSS)] },
    { DataRepositoryType::Stata, DataRepositoryTypeNames[static_cast<size_t>(DataRepositoryType::Stata)] })


const char* const DataRepositoryTypeDefaultExtensions[] =
{
    "", // Null
    FileExtensions::Data::TextDataDefault,
    FileExtensions::Data::CSProDB,
    FileExtensions::Data::EncryptedCSProDB,
    "", // Memory
    FileExtensions::Data::Json,
    "", // CSWeb
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


const char* ToString(const DataRepositoryType type)
{
    static const char* names[] =
    {
        DataRepositoryTypeNames[static_cast<size_t>(DataRepositoryType::Null)],
        DataRepositoryTypeNames[static_cast<size_t>(DataRepositoryType::Text)],
        "CSPro DB",
        "Encrypted CSPro DB",
        "In-Memory",
        DataRepositoryTypeNames[static_cast<size_t>(DataRepositoryType::Json)],
        DataRepositoryTypeNames[static_cast<size_t>(DataRepositoryType::CSWeb)],
        "Comma Delimited (CSV)",
        "Semicolon Delimited",
        "Tab Delimited",
        DataRepositoryTypeNames[static_cast<size_t>(DataRepositoryType::Excel)],
        "CSPro (Exported)",
        DataRepositoryTypeNames[static_cast<size_t>(DataRepositoryType::R)],
        DataRepositoryTypeNames[static_cast<size_t>(DataRepositoryType::SAS)],
        DataRepositoryTypeNames[static_cast<size_t>(DataRepositoryType::SPSS)],
        DataRepositoryTypeNames[static_cast<size_t>(DataRepositoryType::Stata)],
    };

    static_assert(_countof(names) == _countof(DataRepositoryTypeNames));
    static_assert(_countof(names) == ( static_cast<size_t>(DataRepositoryType::Stata) + 1 ));

    return names[static_cast<size_t>(type)];
}


DataRepositoryType ConnectionString::GetDefaultDataRepositoryTypeFromText(std::string_view text_sv)
{
    text_sv = SO::Trim(text_sv);

    if( text_sv.empty() )
        return DataRepositoryType::Null;

    if( CustomUri::UsesHttpScheme(text_sv) )
        return DataRepositoryType::CSWeb;

    const std::string extension = PortableFunctions::PathGetFileExtension(text_sv);

    return ( SO::EqualsNoCase(extension, FileExtensions::Data::CSProDB) )          ? DataRepositoryType::SQLite :
           ( SO::EqualsNoCase(extension, FileExtensions::Data::EncryptedCSProDB) ) ? DataRepositoryType::EncryptedSQLite :
           ( SO::EqualsNoCase(extension, FileExtensions::Data::Json) )             ? DataRepositoryType::Json :
           ( SO::EqualsNoCase(extension, FileExtensions::CSV) )                    ? DataRepositoryType::CommaDelimited :
           ( SO::EqualsNoCase(extension, FileExtensions::SemicolonDelimited) )     ? DataRepositoryType::SemicolonDelimited :
           ( SO::EqualsNoCase(extension, FileExtensions::TabDelimited) )           ? DataRepositoryType::TabDelimited :
           ( SO::EqualsNoCase(extension, FileExtensions::Excel) )                  ? DataRepositoryType::Excel :
           ( SO::EqualsOneOfNoCase(extension, FileExtensions::RData, "rda") )      ? DataRepositoryType::R :
           ( SO::EqualsNoCase(extension, FileExtensions::SasData) )                ? DataRepositoryType::SAS :
           ( SO::EqualsNoCase(extension, FileExtensions::SpssData) )               ? DataRepositoryType::SPSS :
           ( SO::EqualsNoCase(extension, FileExtensions::StataData) )              ? DataRepositoryType::Stata :
                                                                                     DataRepositoryType::Text;
}


ConnectionString::ConnectionString(const std::string_view connection_string_text_sv)
    :   ConnectionString(ResourceType::None, DataRepositoryType::Null)
{
    if( CustomUri::UsesCSProScheme(connection_string_text_sv, CustomUri::UriType::Data) )
    {
        InitializeFromString(CustomUri::ConvertDataUriToConnectionStringText(connection_string_text_sv));
    }

    else
    {
        InitializeFromString(connection_string_text_sv);
    }

    InitializeDataRepositoryType();

    // blank connection strings will be undefined
    if( m_dataRepositoryType == DataRepositoryType::Null &&
        connection_string_text_sv.find(PropertySeparatorInitial) == std::string_view::npos )
    {
        Clear();
    }
}


ConnectionString ConnectionString::CreateNullRepositoryConnectionString()
{
    return ConnectionString(ResourceType::None, DataRepositoryType::Null);
}


ConnectionString ConnectionString::CreateMemoryRepositoryConnectionString()
{
    return ConnectionString(ResourceType::None, DataRepositoryType::Memory);
}


void ConnectionString::SetMainValue(const std::string_view main_value_sv)
{
    m_resource = main_value_sv;
}


void ConnectionString::InitializeDataRepositoryType()
{
    // prior to CSPro 7.7, the file extension always overrode any type specified in the connection string;
    // now, to support the ability to write to files with an extension that that might have defaulted
    // to an export format, the extension will only sometimes override the type
    auto adjust = [&](const DataRepositoryType default_data_repository_type)
    {
        if( m_dataRepositoryType == default_data_repository_type )
            return;

        // if exporting to CSPro format, let the file path be handled by the repository
        if( m_dataRepositoryType == DataRepositoryType::CSProExport )
            return;

        // text-based files can be written to any extension
        if( DataRepositoryHelpers::TypeWritesToText(m_dataRepositoryType) && DataRepositoryHelpers::TypeWritesToText(default_data_repository_type) )
            return;

        // some connection strings without file paths are allowed
        if( default_data_repository_type == DataRepositoryType::Null && !DataRepositoryHelpers::TypeUsesResource(m_dataRepositoryType) )
            return;

        // if here, then the type will be changed to default type
        m_dataRepositoryType = default_data_repository_type;
    };

    adjust(GetDefaultDataRepositoryTypeFromText(m_resource));

    // adjust the source type based on the data repository type
    if( DataRepositoryHelpers::TypeUsesFileResource(m_dataRepositoryType) )
    {
        m_resourceType = ResourceType::FilePath;
        PortableFunctions::MakePathToNativeSlash(m_resource);
    }

    else if( DataRepositoryHelpers::TypeUsesUrlResource(m_dataRepositoryType) )
    {
        m_resourceType = ResourceType::Url;
    }

    else
    {
        ASSERT(!DataRepositoryHelpers::TypeUsesResource(m_dataRepositoryType));
        m_resourceType = ResourceType::None;
    }
}


bool ConnectionString::TextToType(DataRepositoryType& type, const std::string_view text_sv)
{
    for( size_t i = 0; i < _countof(DataRepositoryTypeNames); ++i )
    {
        if( SO::EqualsNoCase(text_sv, DataRepositoryTypeNames[i]) )
        {
            type = static_cast<DataRepositoryType>(i);
            return true;
        }
    }

    return false;
}


bool ConnectionString::PreprocessProperty(const std::string_view attribute_sv, const std::string_view value_sv)
{
    // special handling for the type
    if( SO::EqualsNoCase(attribute_sv, ConnectionStringDataRepositoryPropertyType) )
    {
        TextToType(m_dataRepositoryType, value_sv);
        return true;
    }

    // prior to CSPro 7.3, only the repository type was specified (without an =),
    // so for backwards compatability, we will still process these
    else if( TextToType(m_dataRepositoryType, attribute_sv) )
    {
        return true;
    }

    else
    {
        return false;
    }
}


template<bool return_value_for_no_resource>
bool ConnectionString::ResourceMatches(const ConnectionString& rhs_connection_string) const
{
    // compare file paths as case-insensitive
    if( m_resourceType == ResourceType::FilePath ||
        rhs_connection_string.m_resourceType == ResourceType::FilePath )
    {
        return SO::EqualsNoCase(m_resource, rhs_connection_string.m_resource);
    }

    // compare URLs as case-sensitive
    else if( m_resourceType == ResourceType::Url ||
             rhs_connection_string.m_resourceType == ResourceType::Url )
    {
        if( m_resource != rhs_connection_string.m_resource )
            return false;

        // for CSWeb data sources, check if there is a dictionaryName override, treating that as a resource
        if( m_dataRepositoryType == DataRepositoryType::CSWeb &&
            rhs_connection_string.m_dataRepositoryType == DataRepositoryType::CSWeb )
        {
            const std::string* const dictionary_name_override = GetProperty(CSProperty::dictionaryName);
            const std::string* const rhs_dictionary_name_override = rhs_connection_string.GetProperty(CSProperty::dictionaryName);

            if( ( dictionary_name_override != nullptr ) != ( rhs_dictionary_name_override != nullptr ) )
            {
                return true;
            }

            else if( dictionary_name_override == nullptr )
            {
                // ideally if one dictionary name is specified and another is not, we could determine the
                // implicit dictionary name, but without going to the lengths to implement that, we will
                // indicate that the resource is shared; this means that users will have to explicitly
                // specify dictionary names in instances when they want to make sure this is treated as different; e.g.,:
                // http://localhost/csweb/api|dictionaryName=DICT1 + http://localhost/csweb/api|dictionaryName=DICT2
                return true;
            }

            else
            {
                return SO::EqualsNoCase(*dictionary_name_override, *rhs_dictionary_name_override);
            }
        }

        return true;
    }

    else
    {
        ASSERT(m_resourceType == ResourceType::Undefined || m_resourceType == ResourceType::None);
        ASSERT(rhs_connection_string.m_resourceType == ResourceType::Undefined || rhs_connection_string.m_resourceType == ResourceType::None);
        ASSERT(m_resource.empty() && rhs_connection_string.m_resource.empty());

        return return_value_for_no_resource;
    }
}


bool ConnectionString::Equals(const ConnectionString& rhs_connection_string, const bool compare_properties/* = false*/) const
{
    // the properties are not compared, though they could be in the future for some other repository type
    ASSERT(!compare_properties);

    return ( m_resourceType == rhs_connection_string.m_resourceType &&
             m_dataRepositoryType == rhs_connection_string.m_dataRepositoryType &&
             ResourceMatches<true>(rhs_connection_string) );
}


bool ConnectionString::SharesResource(const ConnectionString& rhs_connection_string) const
{
    return ResourceMatches<false>(rhs_connection_string);
}


bool ConnectionString::SharesResource(const std::vector<ConnectionString>& rhs_connection_strings) const
{
    for( const ConnectionString& rhs_connection_string : rhs_connection_strings )
    {
        if( SharesResource(rhs_connection_string) )
            return true;
    }

    return false;
}


bool ConnectionString::TypeCanBeCalculatedImplicitly(const std::string& resource) const
{
    ASSERT(IsDefined());

    // the type information needs to be added:
    //   - when using the null or memory repositories (because there is no file path or URL)
    //   - when the data repository type that would be calculated based on the file extension or URL differs from the actual type
    if( ( m_resourceType == ResourceType::None ) ||
        ( m_resourceType == ResourceType::FilePath && m_dataRepositoryType != GetDefaultDataRepositoryTypeFromText(resource) ) )
    {
        return false;
    }

    return true;
}


bool ConnectionString::TypeCanBeCalculatedImplicitly() const
{
    return TypeCanBeCalculatedImplicitly(m_resource);
}


std::string ConnectionString::ToString(std::string file_path) const
{
    ASSERT(IsDefined());

    // add the type when necessary
    if( !TypeCanBeCalculatedImplicitly(file_path) )
    {
        std::vector<std::tuple<std::string, std::string>> properties = m_properties;
        properties.insert(properties.begin(), std::make_tuple(ConnectionStringDataRepositoryPropertyType,
                                                              DataRepositoryTypeNames[static_cast<size_t>(m_dataRepositoryType)]));

        return PropertyString::ToString(std::move(file_path), properties);
    }

    else
    {
        return PropertyString::ToString(std::move(file_path), m_properties);
    }
}


std::string ConnectionString::ToStringWithoutDirectory() const
{
    if( !HasFilePath() )
        return ToString();

    return ToString(PortableFunctions::PathGetFilename(m_resource));
}


std::string ConnectionString::ToRelativeString(std::string directory_path, const bool use_display_mode/* = false*/) const
{
    if( !HasFilePath() )
        return ToString();

    const std::string adjusted_directory_path = PortableFunctions::PathEnsureTrailingSlash(std::move(directory_path));

    return use_display_mode ? ToString(GetRelativePathForDisplay(adjusted_directory_path, m_resource)) :
                              ToString(UTF8_TODO::GetUtf8(GetRelativeFName(UTF8_TODO::GetWide(adjusted_directory_path), UTF8_TODO::GetWide(m_resource))));
}


std::string ConnectionString::ToDisplayString(const bool use_filename_only/* = false*/) const
{
    ASSERT(IsDefined());

    return HasFilePath() ? ( use_filename_only ? PortableFunctions::PathGetFilename(m_resource) : m_resource ) :
           HasUrl() ?      m_resource :
                           DataRepositoryTypeNames[static_cast<size_t>(m_dataRepositoryType)];
}


void ConnectionString::AdjustRelativePath(const std::string& directory_path)
{
    if( HasFilePath() )
        m_resource = MakeFullPath(PortableFunctions::PathEnsureTrailingSlash(directory_path), m_resource);
}


std::string ConnectionString::GetName(const DataRepositoryNameType name_type) const
{
    ASSERT(IsDefined());

    auto get_type_description = [&]()
    {
        const char* const repository_prefix =
            ( m_dataRepositoryType == DataRepositoryType::Null )   ? "Empty" :
            ( m_dataRepositoryType == DataRepositoryType::Text )   ? "Text File" :
            ( m_dataRepositoryType == DataRepositoryType::Memory ) ? "Memory" :
                                                                     ::ToString(m_dataRepositoryType);

        return SO::Concatenate("<<", repository_prefix, ">>");
    };

    if( m_resourceType == ResourceType::None )
    {
        return get_type_description();
    }

    else if( name_type == DataRepositoryNameType::Full )
    {
        return m_resource;
    }

    else if( name_type == DataRepositoryNameType::Concise )
    {
        return HasFilePath() ? PortableFunctions::PathGetFilename(m_resource) :
                               m_resource;
    }

    else
    {
        ASSERT(name_type == DataRepositoryNameType::ForListing);
        return SO::Concatenate(get_type_description(), " ", m_resource);
    }
}


ConnectionString ConnectionString::CreateFromJson(const JsonNode& json_node)
{
    // allow the connection string to be specified as a string
    if( json_node.IsString() )
    {
        ConnectionString connection_string(json_node.Get<std::string_view>());
        connection_string.AdjustRelativePath(json_node.GetJsonReaderInterface().GetDirectory());
        return connection_string;
    }

    // otherwise the connection string is specified as an object
    ConnectionString connection_string;
    std::optional<DataRepositoryType> type;
    std::string text;

    json_node.ForeachNode(
        [&](const std::string_view key_sv, const JsonNode& attribute_value_node)
        {
            if( key_sv == JK::type )
            {
                if( !TextToType(type.emplace(), attribute_value_node.Get<std::string_view>()) )
                {
                    throw JsonParseException("'%s' is not a valid connection string type",
                                             attribute_value_node.Get<std::string>().c_str());
                }
            }

            else if( key_sv == JK::path )
            {
                text = attribute_value_node.GetAbsolutePath();
            }

            else if( key_sv == JK::url )
            {
                text = attribute_value_node.Get<std::string>();
            }

            else
            {
                // only add properties that can be represented as strings
                std::optional<std::string> attribute = attribute_value_node.GetOptional<std::string>();

                if( attribute.has_value() )
                    connection_string.SetProperty(key_sv, std::move(*attribute));
            }
        });

    if( !type.has_value() && SO::IsWhitespace(text) )
    {
        ASSERT(!connection_string.IsDefined());
        connection_string.m_properties.clear();
        return connection_string;
    }

    connection_string.m_resource = std::move(text);

    if( type.has_value() )
        connection_string.m_dataRepositoryType = *type;

    connection_string.InitializeDataRepositoryType();

    return connection_string;
}


void ConnectionString::WriteJson(JsonWriter& json_writer, const bool write_relative_path/* = true*/) const
{
    json_writer.BeginObject();

    if( IsDefined() )
    {
        if( HasFilePath() )
        {
            write_relative_path ? json_writer.WriteRelativePath(JK::path, m_resource) :
                                  json_writer.WritePath(JK::path, m_resource);
        }

        else if( HasUrl() )
        {
            json_writer.Write(JK::url, m_resource);
        }

        json_writer.Write(JK::type, m_dataRepositoryType);

        PropertyString::WriteJson(json_writer, false);
    }

    json_writer.EndObject();
}
