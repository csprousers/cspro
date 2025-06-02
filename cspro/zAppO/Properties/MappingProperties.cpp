#include "stdafx.h"
#include "MappingProperties.h"


CREATE_JSON_KEY(windowsMappingTileProvider) // used by CSPro 8.0 only


MappingProperties::MappingProperties()
    :   m_coordinateDisplay(CoordinateDisplay::Decimal),
        m_defaultBaseMap(BaseMap::Normal),
        m_mappingEngine(MappingEngine::Default),
        m_mappingTileProvider(MappingTileProvider::Esri),
        m_esriMappingTileProviderProperties(MappingTileProvider::Esri),
        m_mapboxMappingTileProviderProperties(MappingTileProvider::Mapbox)
{
}


bool MappingProperties::operator==(const MappingProperties& rhs) const
{
    return ( m_coordinateDisplay == rhs.m_coordinateDisplay &&
             m_defaultBaseMap == rhs.m_defaultBaseMap &&
             m_mappingEngine == rhs.m_mappingEngine &&
             m_mappingTileProvider == rhs.m_mappingTileProvider &&
             m_esriMappingTileProviderProperties == rhs.m_esriMappingTileProviderProperties &&
             m_mapboxMappingTileProviderProperties == rhs.m_mapboxMappingTileProviderProperties );
}


const MappingTileProviderProperties& MappingProperties::GetMappingTileProviderProperties() const
{
    return ( m_mappingTileProvider == MappingTileProvider::Esri ) ? m_esriMappingTileProviderProperties :
                                                                    m_mapboxMappingTileProviderProperties;
}



// --------------------------------------------------------------------------
// serialization
// --------------------------------------------------------------------------

CREATE_ENUM_JSON_SERIALIZER(CoordinateDisplay,
    { CoordinateDisplay::Decimal, "decimal" },
    { CoordinateDisplay::DMS,     "DMS" })


MappingProperties MappingProperties::CreateFromJson(const JsonNode& json_node)
{
    MappingProperties mapping_properties;

    mapping_properties.m_coordinateDisplay = json_node.GetOrDefault(JK::coordinateDisplay, mapping_properties.m_coordinateDisplay);

    if( json_node.Contains(JK::defaultBaseMap) )
    {
        try
        {
            mapping_properties.m_defaultBaseMap = json_node.Get(JK::defaultBaseMap).Get<BaseMap>();
        }

        catch( const JsonParseException& )
        {
            mapping_properties.m_defaultBaseMap = json_node.GetAbsolutePath(JK::defaultBaseMap);
        }
    }

    mapping_properties.m_mappingEngine = json_node.GetOrDefault(JK::engine, mapping_properties.m_mappingEngine);

    const char* const tile_provider_key = ( json_node.Contains(JK::tileProvider) ||
                                            !json_node.Contains(JK::windowsMappingTileProvider) ) ? JK::tileProvider : JK::windowsMappingTileProvider;
    mapping_properties.m_mappingTileProvider = json_node.GetOrDefault(tile_provider_key, mapping_properties.m_mappingTileProvider);

    for( const JsonNode& tile_provider_node : json_node.GetArrayOrEmpty(JK::tileProviders) )
    {
        MappingTileProviderProperties mapping_tile_provider_properties = tile_provider_node.Get<MappingTileProviderProperties>();

        MappingTileProviderProperties& this_mapping_tile_provider_properties =
            ( mapping_tile_provider_properties.GetMappingTileProvider() == MappingTileProvider::Esri ) ? mapping_properties.m_esriMappingTileProviderProperties :
                                                                                                         mapping_properties.m_mapboxMappingTileProviderProperties;
        this_mapping_tile_provider_properties = std::move(mapping_tile_provider_properties);
    }

    return mapping_properties;
}


void MappingProperties::WriteJson(JsonWriter& json_writer) const
{
    json_writer.BeginObject();

    json_writer.Write(JK::coordinateDisplay, m_coordinateDisplay);

    if( std::holds_alternative<BaseMap>(m_defaultBaseMap) )
    {
        json_writer.Write(JK::defaultBaseMap, std::get<BaseMap>(m_defaultBaseMap));
    }

    else
    {
        json_writer.WriteRelativePath(JK::defaultBaseMap, std::get<std::string>(m_defaultBaseMap));
    }

    json_writer.Write(JK::engine, m_mappingEngine)
               .Write(JK::tileProvider, m_mappingTileProvider);

    json_writer.BeginArray(JK::tileProviders)
               .Write(m_esriMappingTileProviderProperties)
               .Write(m_mapboxMappingTileProviderProperties)
               .EndArray();

    json_writer.EndObject();
}


void MappingProperties::serialize(Serializer& ar)
{
    ar.SerializeEnum(m_coordinateDisplay);

    if( ar.MeetsVersionIteration(Serializer::Iteration_8_1_000_1) )
        ar.SerializeEnum(m_mappingEngine);

    ar.SerializeEnum(m_mappingTileProvider);

    if( ar.IsSaving() )
    {
        ar.Write(ToString(m_defaultBaseMap, ar.GetArchiveFilePath()));
    }

    else
    {
        m_defaultBaseMap = FromString(ar.Read<std::string>(), ar.GetArchiveFilePath());
    }

    ar & m_esriMappingTileProviderProperties;
    ar & m_mapboxMappingTileProviderProperties;
}
