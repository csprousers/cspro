#include "StdAfx.h"
#include "CustomUri.h"
#include <zToolsO/Hash.h>
#include <zToolsO/UniqueId.h>
#include <zDataO/ConnectionStringProperties.h>
#include <zDataO/DataRepositoryHelpers.h>


namespace
{
    constexpr static std::string_view HttpScheme_sv  = "http://";
    constexpr static std::string_view HttpsScheme_sv = "https://";

    constexpr static std::string_view CSProTextPrefix_sv  = "cspro:///text/";
    constexpr static std::string_view CSProDataPrefix_sv  = "cspro:///data/";
    constexpr static std::string_view CSProSyncPrefix_sv  = "cspro:///sync/";
    constexpr static std::string_view CSProCachePrefix_sv = "cspro:///cache/";
}


struct CustomUri::UriBuilder
{
    std::string uri;
    std::optional<bool> query_started;

    template<typename AT, typename VT>
    void AddToQuery(AT&& attribute, VT&& value)
    {
        if( !query_started.has_value() )
            query_started = uri.find('?');

        if( *query_started )
        {
            uri.push_back('&');
        }

        else
        {
            uri.push_back('?');
            query_started = true;
        }

        uri.append(Encoders::ToUriComponent(std::forward<AT>(attribute)));
        uri.push_back('=');
        uri.append(Encoders::ToUriComponent(std::forward<VT>(value)));
    }
};


bool CustomUri::UsesHttpScheme(std::string_view uri_sv)
{
    constexpr std::string_view Http_sv = HttpScheme_sv.substr(0, 4);

    if( SO::StartsWith(uri_sv, Http_sv) )
    {
        uri_sv.remove_prefix(Http_sv.length());

        if( SO::StartsWith(uri_sv, HttpScheme_sv.substr(Http_sv.length())) ||
            SO::StartsWith(uri_sv, HttpsScheme_sv.substr(Http_sv.length())) )
        {
            return true;
        }
    }

    return false;
}


bool CustomUri::UsesHttpOrCSProScheme(const std::string_view uri_sv)
{
    return ( UsesHttpScheme(uri_sv) ||
             UsesCSProScheme(uri_sv) );
}


std::optional<CustomUri::UriType> CustomUri::GetUriType(const std::string_view uri_sv)
{
    return SO::StartsWith(uri_sv, CSProTextPrefix_sv)  ? std::make_optional(UriType::Text) :
           SO::StartsWith(uri_sv, CSProDataPrefix_sv)  ? std::make_optional(UriType::Data) :
           SO::StartsWith(uri_sv, CSProSyncPrefix_sv)  ? std::make_optional(UriType::Sync) :
           SO::StartsWith(uri_sv, CSProCachePrefix_sv) ? std::make_optional(UriType::Cache) :
                                                         std::nullopt;
}


std::string CustomUri::EvaluatePathAndQueryString(const std::string_view path_sv, std::vector<std::tuple<std::string, std::string>>* const query_string_properties)
{
    const size_t query_string_start_pos = path_sv.find('?');

    if( query_string_start_pos != std::string_view::npos && query_string_properties != nullptr )
    {
        SO::ForeachSection(path_sv.substr(query_string_start_pos + 1), '&',
            [&](const std::string_view attribute_and_value_sv)
            {
                const auto [attribute_sv, value_sv] = SO::GetTextOnEitherSideOfCharacter(attribute_and_value_sv, '=');

                query_string_properties->emplace_back(Encoders::FromUrlQueryString(attribute_sv),
                                                      Encoders::FromUrlQueryString(value_sv));
            });
    }

    return Encoders::FromPercentEncoding(path_sv.substr(0, query_string_start_pos));
}


std::string CustomUri::EncodeFilePath(std::string file_path)
{
    if( file_path.empty() )
        return file_path;

    PortableFunctions::MakePathToForwardSlash(file_path);

    std::string_view file_path_sv = file_path;

    // remove a leading forward slash
    if( file_path_sv.front() == '/' )
        file_path_sv.remove_prefix(1);

    return Encoders::ToUriPath(file_path_sv);
}


std::string CustomUri::ConvertUriToPropertyString(const std::string_view cspro_scheme_prefix_sv, const std::string_view uri_sv)
{
    ASSERT(SO::StartsWith(uri_sv, cspro_scheme_prefix_sv));

    std::vector<std::tuple<std::string, std::string>> properties;
    const std::string path = EvaluatePathAndQueryString(uri_sv.substr(cspro_scheme_prefix_sv.length()), &properties);

    // this object exists only to call PropertyString's protected method
    struct PropertyStringCreator : public PropertyString
    {
        static std::string CreateString(const std::string& path, const std::vector<std::tuple<std::string, std::string>>& properties)
        {
            return ToString(path, properties);
        }
    };

    return PropertyStringCreator::CreateString(path, properties);
}


std::string CustomUri::CreateTextUri(std::string file_path)
{
    return SO::Concatenate(CSProTextPrefix_sv, EncodeFilePath(std::move(file_path)));
}


std::string CustomUri::ConvertTextUriToFilePath(const std::string_view uri_sv)
{
    ASSERT(SO::StartsWith(uri_sv, CSProTextPrefix_sv));

    std::string file_path = EvaluatePathAndQueryString(uri_sv.substr(CSProTextPrefix_sv.length()), nullptr);

    return PortableFunctions::MakePathToNativeSlash(file_path);
}


CustomUri::UriBuilder CustomUri::CreateUriForConnectionString(const ConnectionString& connection_string)
{
    UriBuilder uri_builder { std::string(CSProDataPrefix_sv), false };

    if( connection_string.HasFilePath() )
    {
        uri_builder.uri.append(EncodeFilePath(connection_string.GetFilePath()));
    }

    else if( connection_string.HasUrl() )
    {
        uri_builder.uri.append(Encoders::ToUriPath(connection_string.GetUrl()));
    }

    // add the type when necessary
    if( !connection_string.TypeCanBeCalculatedImplicitly() )
    {
        uri_builder.AddToQuery(ConnectionStringDataRepositoryPropertyType,
                               DataRepositoryTypeNames[static_cast<size_t>(connection_string.GetType())]);
    }

    // add any additional properties
    for( const auto& [attribute, value] : connection_string.GetProperties() )
        uri_builder.AddToQuery(attribute, value);

    return uri_builder;
}


std::string CustomUri::CreateDataUri(const ConnectionString& connection_string, const std::string* const dictionary_file_path)
{
    UriBuilder uri_builder = CreateUriForConnectionString(connection_string);

    // add the dictionary information when possible (and not using an embedded dicitionary)
    if( dictionary_file_path != nullptr &&
        !DataRepositoryHelpers::IsTypeFileBasedWithAnEmbeddedDictionary(connection_string.GetType()) &&
        PortableFunctions::FileIsRegular(*dictionary_file_path) )
    {
        // use a relative path when possible
        std::string serialized_dictionary_file_path =
            connection_string.HasFilePath() ? GetRelativePathForDisplay(connection_string.GetFilePath(), *dictionary_file_path) :
                                              *dictionary_file_path;

        PortableFunctions::MakePathToForwardSlash(serialized_dictionary_file_path);

        uri_builder.AddToQuery(CSProperty::dictionaryPath, serialized_dictionary_file_path);
    }

    return std::move(uri_builder.uri);
}


std::string CustomUri::AddCaseToDataUri(std::string data_uri, const std::string& key, const std::string* const uuid)
{
    UriBuilder uri_builder { std::move(data_uri) };

    uri_builder.AddToQuery(CSProperty::key, key);

    if( uuid != nullptr )
        uri_builder.AddToQuery(CSProperty::uuid, *uuid);

    return std::move(uri_builder.uri);
}


std::string CustomUri::ConvertDataUriToConnectionStringText(const std::string_view uri_sv)
{
    return ConvertUriToPropertyString(CSProDataPrefix_sv, uri_sv);
}


std::string CustomUri::ConvertSyncUriToSyncConnectionStringText(const std::string_view uri_sv)
{
    return ConvertUriToPropertyString(CSProSyncPrefix_sv, uri_sv);
}


std::string CustomUri::CreateCacheUri()
{
    // create a cache ID that is not easily guessable by hashing it
    // https://stackoverflow.com/questions/664014/what-integer-hash-function-are-good-that-accepts-an-integer-hash-key
    int cache_id = UniqueId::CreateInt();

    cache_id = ( ( cache_id >> 16 ) ^ cache_id) * 0x45d9f3b;
    cache_id = ( ( cache_id >> 16 ) ^ cache_id) * 0x45d9f3b;
    cache_id = ( ( cache_id >> 16 ) ^ cache_id);

    return SO::Concatenate(
        CSProCachePrefix_sv,
        Hash::BytesToHexString(&cache_id, sizeof(cache_id))
    );
}
