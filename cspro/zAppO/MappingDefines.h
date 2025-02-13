#pragma once

#include <zAppO/zAppO.h>
#include <zUtilO/FromString.h>
#include <zJson/JsonSerializer.h>

class Serializer;


// --------------------------------------------------
// BaseMap
// --------------------------------------------------

enum class BaseMap : int
{
    Normal = 1,
    Hybrid,
    Satellite,
    Terrain,
    None
};

using BaseMapSelection = std::variant<BaseMap, std::wstring>;

ZAPPO_API const std::vector<std::wstring>& GetBaseMapStrings();

ZAPPO_API const TCHAR* ToString(BaseMap base_map);

template<> ZAPPO_API std::optional<BaseMap> FromString<BaseMap>(wstring_view text);

ZAPPO_API std::wstring ToString(const BaseMapSelection& base_map_selection, std::optional<NullTerminatedString> relative_to_path = std::nullopt);

ZAPPO_API BaseMapSelection FromString(wstring_view text, NullTerminatedString relative_to_path);

DECLARE_ENUM_JSON_SERIALIZER_CLASS(BaseMap,)



// --------------------------------------------------
// MappingTileProvider
// --------------------------------------------------

enum class MappingTileProvider : int
{
    Esri,
    Mapbox
};

ZAPPO_API const std::vector<std::wstring>& GetMappingTileProviderStrings();

ZAPPO_API const TCHAR* ToString(MappingTileProvider mapping_tile_provider);

DECLARE_ENUM_JSON_SERIALIZER_CLASS(MappingTileProvider,)



// --------------------------------------------------
// AppMappingOptions
// --------------------------------------------------

struct ZAPPO_API AppMappingOptions
{
    std::wstring latitude_item;
    std::wstring longitude_item;

    bool operator==(const AppMappingOptions& rhs) const;
    bool operator!=(const AppMappingOptions& rhs) const { return !operator==(rhs); }

    bool IsDefined() const { return ( !latitude_item.empty() && !longitude_item.empty() ); }

    // serialization
    static AppMappingOptions CreateFromJson(const JsonNode<wchar_t>& json_node);
    void WriteJson(JsonWriter& json_writer, bool write_to_new_json_object = true) const;
    void serialize(Serializer& ar);
};
