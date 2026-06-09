#include "StdAfx.h"
#include "SyncConnectionString.h"
#include "CustomUri.h"
#include <zNetwork/ParsedUri.h>
#include <zNetwork/SyncConnectionStringProperties.h>


namespace
{
    constexpr std::string_view Bluetooth_sv = "Bluetooth";
    constexpr std::string_view Dropbox_sv   = "Dropbox";
    constexpr std::string_view File_sv      = "file";
    constexpr std::string_view Ftp_sv       = "ftp";
    constexpr std::string_view Http_sv      = "http";

    constexpr const char* SyncServiceTypeNames[] =
    {
        "Bluetooth",
        "CSWeb",
        "Dropbox",
        "FTP",
        "LocalFiles"
    };
}


const char* ToString(const SyncServiceType sync_service_type)
{
    static_assert(_countof(SyncServiceTypeNames) == ( static_cast<size_t>(SyncServiceType::LocalFiles) + 1 ));
    return SyncServiceTypeNames[static_cast<size_t>(sync_service_type)];
}


SyncConnectionString::SyncConnectionString(const std::string_view sync_connection_string_text_sv)
    :   SyncConnectionString()
{
    if( CustomUri::UsesCSProScheme(sync_connection_string_text_sv,CustomUri::UriType::Sync) )
    {
        InitializeFromString(CustomUri::ConvertSyncUriToSyncConnectionStringText(sync_connection_string_text_sv));
    }

    else
    {
        InitializeFromString(sync_connection_string_text_sv);
    }

    Initialize(std::nullopt);

    // blank sync connection strings will be undefined
    if( m_syncServiceType == SyncServiceType::CSWeb &&
        SO::IsWhitespace(m_resource) &&
        sync_connection_string_text_sv.find(PropertySeparatorInitial) == std::string_view::npos )
    {
        Clear();
    }
}


SyncConnectionString SyncConnectionString::CreateDropboxSyncConnectionString()
{
    return SyncConnectionString(ResourceType::Text, SyncServiceType::Dropbox);
}


SyncConnectionString SyncConnectionString::CreateLocalDropboxSyncConnectionString()
{
    SyncConnectionString sync_connection_string = CreateDropboxSyncConnectionString();
    sync_connection_string.SetProperty(SCSProperty::useLocal, SCSValue::true_);
    return sync_connection_string;
}


SyncConnectionString SyncConnectionString::CreateLocalFilesSyncConnectionString(std::string directory_path)
{
    return Encoders::ToFileUrl(PortableFunctions::PathRemoveTrailingSlash(std::move(directory_path)));
}


void SyncConnectionString::SetMainValue(const std::string_view main_value_sv)
{
    m_resource = main_value_sv;
}


std::optional<SyncServiceType> SyncConnectionString::TextToSyncServiceType(const std::string_view text_sv)
{
    for( size_t i = 0; i < _countof(SyncServiceTypeNames); ++i )
    {
        if( SO::EqualsNoCase(text_sv, SyncServiceTypeNames[i]) )
            return static_cast<SyncServiceType>(i);
    }

    return std::nullopt;
}


SyncServiceType SyncConnectionString::CalculateImplicitSyncServiceType(const std::string& resource)
{
    auto [sync_service_type_sv, additional_path_info_sv] = SO::GetTextOnEitherSideOfCharacter(resource, '/');
    std::tie(sync_service_type_sv, std::ignore) = SO::GetTextOnEitherSideOfCharacter(sync_service_type_sv, ':');

    // default to CSWeb for blank strings
    if( SO::IsWhitespace(sync_service_type_sv) )
    {
        return SyncServiceType::CSWeb;
    }

    // http:// + https://
    else if( SO::StartsWith(sync_service_type_sv, Http_sv) && ( sync_service_type_sv.length() == Http_sv.length() ||
                                                                sync_service_type_sv.substr(Http_sv.length()) == "s") )
    {
        return SyncServiceType::CSWeb;
    }

    // Dropbox
    else if( SO::EqualsNoCase(sync_service_type_sv, Dropbox_sv) )
    {
        return SyncServiceType::Dropbox;
    }

    // Bluetooth
    else if( SO::EqualsNoCase(sync_service_type_sv, Bluetooth_sv) )
    {
        return SyncServiceType::Bluetooth;
    }

    // ftp:// + ftps:// + ftpes://
    else if( SO::StartsWith(sync_service_type_sv, Ftp_sv) && ( sync_service_type_sv.length() == Ftp_sv.length() ||
                                                               SO::EqualsOneOf(sync_service_type_sv.substr(Ftp_sv.length()), "s", "es") ) )
    {
        return SyncServiceType::Ftp;
    }

    // file:///
    else if( SO::StartsWith(sync_service_type_sv, File_sv) )
    {
        return SyncServiceType::LocalFiles;
    }

    // default to LocalFiles for all other types
    else
    {
        return SyncServiceType::LocalFiles;
    }
}


void SyncConnectionString::Initialize(std::optional<SyncServiceType> sync_service_type)
{
    ASSERT(m_syncServiceType == SyncServiceType::CSWeb);

    if( !sync_service_type.has_value() )
    {
        const std::string* const type = GetProperty(PropertyType);

        if( type != nullptr )
        {
            sync_service_type = TextToSyncServiceType(*type);

            // remove the type property override (it will be added later in ToString when needed)
            ClearProperty(PropertyType);
        }
    }

    // if no type is specified, calculate it implicitly
    m_syncServiceType = sync_service_type.has_value() ? *sync_service_type :
                                                        CalculateImplicitSyncServiceType(m_resource);

    // set the resource type and potentially modify the resource based on the sync service type
    ASSERT(m_resourceType == ResourceType::Undefined);

    if( m_syncServiceType == SyncServiceType::CSWeb ||
        m_syncServiceType == SyncServiceType::Ftp )
    {
        m_resourceType = ResourceType::Url;
        Path::MakeToForwardSlash(m_resource);
    }

    else if( m_syncServiceType == SyncServiceType::Dropbox ||
             m_syncServiceType == SyncServiceType::Bluetooth )
    {
        m_resourceType = ResourceType::Text;
    }

    else
    {
        ASSERT(m_syncServiceType == SyncServiceType::LocalFiles);

        if( SO::StartsWith(m_resource, Encoders::FileUrlPrefix_sv) )
        {
            m_resourceType = ResourceType::Url;
            Path::MakeToForwardSlash(m_resource);
        }

        else
        {
            m_resourceType = ResourceType::DirectoryPath;
            Path::MakeToNativeSlash(m_resource);
        }
    }
}


std::string SyncConnectionString::GetEvaluatedDirectoryPath() const
{
    ASSERT(IsDefined() && m_syncServiceType == SyncServiceType::LocalFiles);

    if( m_resourceType == ResourceType::Url )
    {
        ASSERT(SO::StartsWith(m_resource, Encoders::FileUrlPrefix_sv));

        std::optional<std::string> directory_path = Encoders::FromFileUrl(m_resource);

        return directory_path.has_value() ? std::move(*directory_path) :
                                            m_resource;
    }

    else
    {
        ASSERT(m_resource == Path::ToNativeSlash(m_resource));

        return m_resource;
    }
}


std::string SyncConnectionString::GetEvaluatedBluetoothServerDeviceName() const
{
    ASSERT(m_syncServiceType == SyncServiceType::Bluetooth && m_resourceType == ResourceType::Text);

    // extract the Bluetooth server device name from the path
    if( m_resource.length() > Bluetooth_sv.length() &&
        SO::StartsWithNoCase(m_resource, Bluetooth_sv) )
    {
        const std::string_view server_device_name_sv = std::string_view(m_resource).substr(Bluetooth_sv.length());

        if( server_device_name_sv.front() == '/' )
            return Encoders::FromPercentEncoding(server_device_name_sv.substr(1));
    }

    return std::string();
}


void SyncConnectionString::SetUsernamePasswordProperties(std::string username, std::string password)
{
    SetProperty(SCSProperty::username, std::move(username));
    SetProperty(SCSProperty::password, std::move(password));
}


template<typename CF>
void SyncConnectionString::RemoveSensitiveProperties(const SyncConnectionString& sync_connection_string, const CF& callback_function)
{
    constexpr const char* SensitiveAttributes[] =
    {
        SCSProperty::username,
        SCSProperty::password,
    };

    for( const char* const sensitive_attribute : SensitiveAttributes )
    {
        if( sync_connection_string.HasProperty(sensitive_attribute) )
            callback_function()->ClearProperty(sensitive_attribute);
    }
}


void SyncConnectionString::RemoveSensitiveProperties()
{
    RemoveSensitiveProperties(*this, [this]() { return this; });
}


std::string SyncConnectionString::ToString(std::string resource) const
{
    if( !IsDefined() )
        return std::string();

    // add the type when necessary
    if( m_syncServiceType != CalculateImplicitSyncServiceType(resource) )
    {
        std::vector<std::tuple<std::string, std::string>> properties = m_properties;
        properties.insert(properties.begin(), std::make_tuple(PropertyType,
                                                              SyncServiceTypeNames[static_cast<size_t>(m_syncServiceType)]));

        return PropertyString::ToString(std::move(resource), properties);
    }

    else
    {
        return PropertyString::ToString(std::move(resource), m_properties);
    }
}


std::string SyncConnectionString::ToDisplayString() const
{
    ASSERT(IsDefined());

    switch( m_syncServiceType )
    {
        case SyncServiceType::CSWeb:
        case SyncServiceType::Ftp:
            return SO::Concatenate(::ToString(m_syncServiceType), ": ", GetUrl());

        case SyncServiceType::Bluetooth:
        case SyncServiceType::Dropbox:
            return ::ToString(m_syncServiceType);

        case SyncServiceType::LocalFiles:
            return "Local Files: " + GetEvaluatedDirectoryPath();

        default:
            return ReturnProgrammingError(std::string());
    }
}


std::string SyncConnectionString::ToSafeString() const
{
    std::unique_ptr<SyncConnectionString> non_sensitive_sync_connection_string;

    RemoveSensitiveProperties(*this,
        [&]()
        {
            if( non_sensitive_sync_connection_string == nullptr )
                non_sensitive_sync_connection_string = std::make_unique<SyncConnectionString>(*this);

            return non_sensitive_sync_connection_string.get();
        });

    return ( non_sensitive_sync_connection_string != nullptr ) ? non_sensitive_sync_connection_string->ToString() :
                                                                 ToString();
}


std::string SyncConnectionString::ToRelativeString(const std::string& directory_path) const
{
    if( m_resourceType == ResourceType::DirectoryPath && !m_resource.empty() )
        return ToString(GetRelativePathForDisplay(PortableFunctions::PathEnsureTrailingSlash(directory_path), m_resource));

    return ToString();
}


void SyncConnectionString::AdjustRelativePath(const std::string& directory_path)
{
    if( m_resourceType == ResourceType::DirectoryPath && !m_resource.empty() )
        m_resource = MakeFullPath(PortableFunctions::PathEnsureTrailingSlash(directory_path), m_resource);
}


void SyncConnectionString::Validate(const std::optional<SyncServiceType> required_sync_service_type/* = std::nullopt*/) const
{
    auto check_evaluated_type = [&](const SyncServiceType sync_service_type)
    {
        return ( IsDefined() && sync_service_type == GetType() );
    };

    auto check_url = [&](const SyncServiceType sync_service_type)
    {
        // make sure the URL is properly escaped
        const std::string unescaped_url = Encoders::FromPercentEncoding(GetUrl());
        const std::string escaped_url = Encoders::ToUri(unescaped_url);

        if( escaped_url != GetUrl() )
        {
            throw CSProException("The %s URL contains characters that must be percent-encoded to look like: %s",
                                 ::ToString(sync_service_type), escaped_url.c_str());
        }

        const ParsedUri parsed_uri(escaped_url);

        if( parsed_uri.domain.empty() )
            throw CSProException("The %s URL must contain a domain.", ::ToString(sync_service_type));
    };

    if( required_sync_service_type == SyncServiceType::CSWeb )
    {
        if( !check_evaluated_type(SyncServiceType::CSWeb) )
            throw CSProException("CSWeb URLs must start with http:// or https://");

        check_url(SyncServiceType::CSWeb);
    }

    else if( required_sync_service_type == SyncServiceType::Ftp )
    {
        if( !check_evaluated_type(SyncServiceType::Ftp) )
            throw CSProException("FTP URLs must start with ftp:// or ftps:// or ftpes://");

        check_url(SyncServiceType::Ftp);
    }

    else if( required_sync_service_type == SyncServiceType::LocalFiles )
    {
        if( !check_evaluated_type(SyncServiceType::LocalFiles) ||
            !PortableFunctions::FileIsDirectory(GetEvaluatedDirectoryPath()) )
        {
            throw CSProException("You must specify a directory that exists to use Local Files.");
        }
    }

    else if( !IsDefined() )
    {
        if( required_sync_service_type.has_value() )
            throw CSProException("You must specify the %s URL.", ::ToString(*required_sync_service_type));

        throw CSProException("You must specify a valid sync connection string.");
    }
}


SyncConnectionString SyncConnectionString::CreateFromJson(const JsonNode& json_node)
{
    // allow the sync connection string to be specified as a string
    if( json_node.IsString() )
    {
        SyncConnectionString sync_connection_string(json_node.Get<std::string_view>());
        sync_connection_string.AdjustRelativePath(json_node.GetJsonReaderInterface().GetDirectory());
        return sync_connection_string;
    }

    // otherwise the sync connection string is specified as an object
    SyncConnectionString sync_connection_string;
    std::optional<SyncServiceType> sync_service_type;
    std::string resource;

    json_node.ForeachNode(
        [&](const std::string_view key_sv, const JsonNode& attribute_value_node)
        {
            if( key_sv == JK::type )
            {
                sync_service_type = TextToSyncServiceType(attribute_value_node.Get<std::string_view>());

                if( !sync_service_type.has_value() )
                {
                    throw JsonParseException("'%s' is not a valid sync connection string type",
                                             attribute_value_node.Get<std::string>().c_str());
                }
            }

            else if( key_sv == JK::path )
            {
                resource = attribute_value_node.GetAbsolutePath();
            }

            else if( key_sv == JK::url )
            {
                resource = attribute_value_node.Get<std::string>();
            }

            else
            {
                // only add properties that can be represented as strings
                std::optional<std::string> attribute = attribute_value_node.GetOptional<std::string>();

                if( attribute.has_value() )
                    sync_connection_string.SetProperty(key_sv, std::move(*attribute));
            }
        });

    if( !sync_service_type.has_value() && SO::IsWhitespace(resource) )
    {
        ASSERT(!sync_connection_string.IsDefined());
        sync_connection_string.m_properties.clear();
    }

    else
    {
        sync_connection_string.m_resource = std::move(resource);
        sync_connection_string.Initialize(sync_service_type);
    }

    return sync_connection_string;
}


void SyncConnectionString::WriteJson(JsonWriter& json_writer) const
{
    json_writer.BeginObject();

    if( IsDefined() )
    {
        if( m_resourceType == ResourceType::DirectoryPath )
        {
            json_writer.WriteRelativePath(JK::path, m_resource);
        }

        else
        {
            json_writer.Write(JK::url, m_resource);
        }

        json_writer.Write(JK::type, SyncServiceTypeNames[static_cast<size_t>(m_syncServiceType)]);

        PropertyString::WriteJson(json_writer, false);
    }

    json_writer.EndObject();
}


void SyncConnectionString::serialize(Serializer& ar)
{
    if( ar.IsLoading() )
    {
        *this = SyncConnectionString(ar.Read<std::string>());
    }

    else
    {
        ar.Write(ToString());
    }
}
