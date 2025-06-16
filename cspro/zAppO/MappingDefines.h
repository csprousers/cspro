#pragma once

#include <zAppO/zAppO.h>
#include <zUtilO/FromString.h>
#include <zJson/JsonSerializer.h>


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

using BaseMapSelection = std::variant<BaseMap, std::string>;

ZAPPO_API const std::vector<const char*>& GetBaseMapStrings();

ZAPPO_API const char* ToString(BaseMap base_map);

template<> ZAPPO_API std::optional<BaseMap> FromString<BaseMap>(std::string_view text_sv);

ZAPPO_API std::string ToString(const BaseMapSelection& base_map_selection, cs::cref_optional<std::string> relative_to_path = std::nullopt);

ZAPPO_API BaseMapSelection FromString(std::string_view text_sv, std::string_view relative_to_path_sv);

DECLARE_ENUM_JSON_SERIALIZER_CLASS(BaseMap, ZAPPO_API)



// --------------------------------------------------
// MappingEngine
// --------------------------------------------------

enum class MappingEngine : int
{
    Default,
    Leaflet
};

DECLARE_ENUM_JSON_SERIALIZER_CLASS(MappingEngine, ZAPPO_API)



// --------------------------------------------------
// MappingTileProvider
// --------------------------------------------------

enum class MappingTileProvider : int
{
    Esri,
    Mapbox
};

ZAPPO_API const std::vector<const char*>& GetMappingTileProviderStrings();

ZAPPO_API const char* ToString(MappingTileProvider mapping_tile_provider);

DECLARE_ENUM_JSON_SERIALIZER_CLASS(MappingTileProvider, ZAPPO_API)



// --------------------------------------------------
// AppMappingOptions
// --------------------------------------------------

struct ZAPPO_API AppMappingOptions
{
    std::string latitude_item;
    std::string longitude_item;

    bool operator==(const AppMappingOptions& rhs) const;
    bool operator!=(const AppMappingOptions& rhs) const { return !operator==(rhs); }

    bool IsDefined() const { return ( !latitude_item.empty() && !longitude_item.empty() ); }

    // serialization
    static AppMappingOptions CreateFromJson(const JsonNode& json_node);
    void WriteJson(JsonWriter& json_writer, bool write_to_new_json_object = true) const;
    void serialize(Serializer& ar);
};
