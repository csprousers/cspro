#include "stdafx.h"
#include "MappingDefines.h"


// --------------------------------------------------
// BaseMap
// --------------------------------------------------

namespace
{
    const std::vector<std::wstring> BaseMapStrings =
    {
        _T("Normal"),
        _T("Hybrid"),
        _T("Satellite"),
        _T("Terrain"),
        _T("None")
    };
}

const std::vector<std::wstring>& GetBaseMapStrings()
{
    return BaseMapStrings;
}


const TCHAR* ToString(BaseMap base_map)
{
    const size_t index = static_cast<size_t>(base_map);
    ASSERT(index >= 1 && index <= BaseMapStrings.size());
    return BaseMapStrings[index - 1].c_str();
}


template<> std::optional<BaseMap> FromString<BaseMap>(wstring_view text)
{
    size_t index = 1;

    for( const std::wstring& base_map_string : BaseMapStrings )
    {
        if( SO::EqualsNoCase(base_map_string, text) )
            return static_cast<BaseMap>(index);

        ++index;
    }

    return std::nullopt;
}


std::wstring ToString(const BaseMapSelection& base_map_selection, std::optional<NullTerminatedString> relative_to_path/* = std::nullopt*/)
{
    if( std::holds_alternative<BaseMap>(base_map_selection) )
    {
        return ToString(std::get<BaseMap>(base_map_selection));
    }

    else if( relative_to_path.has_value() )
    {
        return GetRelativeFName(*relative_to_path, std::get<std::wstring>(base_map_selection));
    }

    else
    {
        return std::get<std::wstring>(base_map_selection);
    }
}


BaseMapSelection FromString(wstring_view text, NullTerminatedString relative_to_path)
{
    std::optional<BaseMap> base_map = FromString<BaseMap>(text);

    if( base_map.has_value() )
    {
        return *base_map;
    }

    else
    {
        return MakeFullPath(GetWorkingFolder(relative_to_path.c_str()), std::wstring(text));
    }
}


DEFINE_ENUM_JSON_SERIALIZER_CLASS(BaseMap,
    { BaseMap::Normal,    _T("Normal") },
    { BaseMap::Hybrid,    _T("Hybrid") },
    { BaseMap::Satellite, _T("Satellite") },
    { BaseMap::Terrain,   _T("Terrain") },
    { BaseMap::None,      _T("None") })



// --------------------------------------------------
// MappingTileProvider
// --------------------------------------------------

namespace
{
    const std::vector<std::wstring> TileProviderStrings = 
    {
        _T("Esri"),
        _T("Mapbox")
    };
}

const std::vector<std::wstring>& GetMappingTileProviderStrings()
{
    return TileProviderStrings;
}


const TCHAR* ToString(MappingTileProvider mapping_tile_provider)
{
    const size_t index = static_cast<size_t>(mapping_tile_provider);
    ASSERT(index < TileProviderStrings.size());
    return TileProviderStrings[index].c_str();
}


DEFINE_ENUM_JSON_SERIALIZER_CLASS(MappingTileProvider,
    { MappingTileProvider::Esri,   _T("Esri") },
    { MappingTileProvider::Mapbox, _T("Mapbox") })



// --------------------------------------------------
// AppMappingOptions
// --------------------------------------------------

bool AppMappingOptions::operator==(const AppMappingOptions& rhs) const
{
    return ( latitude_item == rhs.latitude_item &&
             longitude_item == rhs.longitude_item );
}


AppMappingOptions AppMappingOptions::CreateFromJson(const JsonNode<wchar_t>& json_node)
{
    AppMappingOptions app_mapping_options
    {
        json_node.GetOrDefault(JK::latitude, SO::EmptyString),
        json_node.GetOrDefault(JK::longitude, SO::EmptyString)
    };

    if( app_mapping_options.latitude_item.empty() != app_mapping_options.longitude_item.empty() )
    {
        json_node.LogWarning(_T("Both latitude and longitude items must be specified to use a map as a case listing"));
        return AppMappingOptions();
    }

    return app_mapping_options;
}


void AppMappingOptions::WriteJson(JsonWriter& json_writer, bool write_to_new_json_object/* = true*/) const
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
