#include "stdafx.h"
#include "MappingDefines.h"


// --------------------------------------------------------------------------
// BaseMap
// --------------------------------------------------------------------------

const std::vector<const char*>& GetBaseMapStrings()
{
    static const std::vector<const char*> base_map_strings =
    {
        "Normal",
        "Hybrid",
        "Satellite",
        "Terrain",
        "None"
    };

    return base_map_strings;
}


const char* ToString(const BaseMap base_map)
{
    const std::vector<const char*>& base_map_strings = GetBaseMapStrings();
    const size_t index = static_cast<size_t>(base_map);
    ASSERT(index >= 1 && index <= base_map_strings.size());
    return base_map_strings[index - 1];
}


template<> std::optional<BaseMap> FromString<BaseMap>(const std::string_view text_sv)
{
    size_t index = 1;

    for( const char* const base_map_string : GetBaseMapStrings() )
    {
        if( SO::EqualsNoCase(base_map_string, text_sv) )
            return static_cast<BaseMap>(index);

        ++index;
    }

    return std::nullopt;
}


std::string ToString(const BaseMapSelection& base_map_selection, const cs::cref_optional<std::string> relative_to_path/* = std::nullopt*/)
{
    if( std::holds_alternative<BaseMap>(base_map_selection) )
    {
        return ToString(std::get<BaseMap>(base_map_selection));
    }

    else if( relative_to_path.has_value() )
    {
        return GetRelativePathForDisplay(*relative_to_path, std::get<std::string>(base_map_selection));
    }

    else
    {
        return std::get<std::string>(base_map_selection);
    }
}


BaseMapSelection FromString(const std::string_view text_sv, const std::string_view relative_to_path_sv)
{
    std::optional<BaseMap> base_map = FromString<BaseMap>(text_sv);

    if( base_map.has_value() )
    {
        return *base_map;
    }

    else
    {
        return MakeFullPath(GetWorkingDirectory(relative_to_path_sv), std::string(text_sv));
    }
}


DEFINE_ENUM_JSON_SERIALIZER_CLASS(BaseMap,
    { BaseMap::Normal,    "Normal" },
    { BaseMap::Hybrid,    "Hybrid" },
    { BaseMap::Satellite, "Satellite" },
    { BaseMap::Terrain,   "Terrain" },
    { BaseMap::None,      "None" })



// --------------------------------------------------------------------------
// MappingTileProvider
// --------------------------------------------------------------------------

const std::vector<const char*>& GetMappingTileProviderStrings()
{
    static const std::vector<const char*> tile_provider_strings =
    {
        "Esri",
        "Mapbox"
    };

    return tile_provider_strings;
}


const char* ToString(const MappingTileProvider mapping_tile_provider)
{
    const std::vector<const char*>& tile_provider_strings = GetMappingTileProviderStrings();
    const size_t index = static_cast<size_t>(mapping_tile_provider);
    ASSERT(index < tile_provider_strings.size());
    return tile_provider_strings[index];
}


DEFINE_ENUM_JSON_SERIALIZER_CLASS(MappingTileProvider,
    { MappingTileProvider::Esri,   "Esri" },
    { MappingTileProvider::Mapbox, "Mapbox" })



// --------------------------------------------------------------------------
// AppMappingOptions
// --------------------------------------------------------------------------

bool AppMappingOptions::operator==(const AppMappingOptions& rhs) const
{
    return ( latitude_item == rhs.latitude_item &&
             longitude_item == rhs.longitude_item );
}


AppMappingOptions AppMappingOptions::CreateFromJson(const JsonNode& json_node)
{
    AppMappingOptions app_mapping_options
    {
        json_node.GetOrConstruct<std::string>(JK::latitude),
        json_node.GetOrConstruct<std::string>(JK::longitude)
    };

    if( app_mapping_options.latitude_item.empty() != app_mapping_options.longitude_item.empty() )
    {
        json_node.LogWarning("Both latitude and longitude items must be specified to use a map as a case listing");
        return AppMappingOptions();
    }

    return app_mapping_options;
}


void AppMappingOptions::WriteJson(JsonWriter& json_writer, const bool write_to_new_json_object/* = true*/) const
{
    if( write_to_new_json_object )
        json_writer.BeginObject();

    json_writer.Write(JK::latitude, latitude_item)
               .Write(JK::longitude, longitude_item);

    if( write_to_new_json_object )
        json_writer.EndObject();
}


void AppMappingOptions::serialize(Serializer& ar)
{
    ar & latitude_item
       & longitude_item;
}
