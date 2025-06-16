#pragma once

#include <zAppO/zAppO.h>
#include <zAppO/MappingDefines.h>
#include <zAppO/Properties/MappingTileProviderProperties.h>


enum class CoordinateDisplay : int
{
    Decimal,
    DMS
};


class ZAPPO_API MappingProperties
{
public:
    MappingProperties();

    bool operator==(const MappingProperties& rhs) const;
    bool operator!=(const MappingProperties& rhs) const { return !( *this == rhs ); }

    CoordinateDisplay GetCoordinateDisplay() const                  { return m_coordinateDisplay; }
    void SetCoordinateDisplay(CoordinateDisplay coordinate_display) { m_coordinateDisplay = coordinate_display; }

    const BaseMapSelection& GetDefaultBaseMap() const         { return m_defaultBaseMap; }
    void SetDefaultBaseMap(BaseMapSelection default_base_map) { m_defaultBaseMap = std::move(default_base_map); }

    MappingEngine GetMappingEngine() const      { return m_mappingEngine; }
    void SetMappingEngine(MappingEngine engine) { m_mappingEngine = engine; }

    MappingTileProvider GetMappingTileProvider() const        { return m_mappingTileProvider; }
    void SetMappingTileProvider(MappingTileProvider provider) { m_mappingTileProvider = provider; }

    const MappingTileProviderProperties& GetEsriMappingTileProviderProperties() const { return m_esriMappingTileProviderProperties; }
    MappingTileProviderProperties& GetEsriMappingTileProviderProperties()             { return m_esriMappingTileProviderProperties; }

    const MappingTileProviderProperties& GetMapboxMappingTileProviderProperties() const { return m_mapboxMappingTileProviderProperties; }
    MappingTileProviderProperties& GetMapboxMappingTileProviderProperties()             { return m_mapboxMappingTileProviderProperties; }

    const MappingTileProviderProperties& GetMappingTileProviderProperties() const;


    // serialization
    // --------------------------------------------------------------------------
    static MappingProperties CreateFromJson(const JsonNode& json_node);
    void WriteJson(JsonWriter& json_writer) const;

    void serialize(Serializer& ar);


private:
    CoordinateDisplay m_coordinateDisplay;
    BaseMapSelection m_defaultBaseMap;
    MappingEngine m_mappingEngine;
    MappingTileProvider m_mappingTileProvider;
    MappingTileProviderProperties m_esriMappingTileProviderProperties;
    MappingTileProviderProperties m_mapboxMappingTileProviderProperties;
};
