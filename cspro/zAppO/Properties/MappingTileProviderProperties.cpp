#include "stdafx.h"
#include "MappingTileProviderProperties.h"


MappingTileProviderProperties::MappingTileProviderProperties(const MappingTileProvider mapping_tile_provider)
    :   m_mappingTileProvider(mapping_tile_provider)
{
    if( m_mappingTileProvider == MappingTileProvider::Esri )
    {
        m_tileLayers.try_emplace(BaseMap::Normal, "Streets");
        m_tileLayers.try_emplace(BaseMap::Hybrid, "NationalGeographic");
        m_tileLayers.try_emplace(BaseMap::Satellite, "Imagery");
        m_tileLayers.try_emplace(BaseMap::Terrain, "Topographic");
    }

    else
    {
        m_tileLayers.try_emplace(BaseMap::Normal, "mapbox/streets-v11");
        m_tileLayers.try_emplace(BaseMap::Hybrid, "mapbox/satellite-streets-v11");
        m_tileLayers.try_emplace(BaseMap::Satellite, "mapbox/satellite-v9");
        m_tileLayers.try_emplace(BaseMap::Terrain, "mapbox/outdoors-v11");
    }
}


MappingTileProviderProperties& MappingTileProviderProperties::operator=(MappingTileProviderProperties&& rhs) noexcept
{
    ASSERT(m_mappingTileProvider == rhs.m_mappingTileProvider);

    m_accessToken = std::move(rhs.m_accessToken);
    m_tileLayers = std::move(rhs.m_tileLayers);

    return *this;
}


bool MappingTileProviderProperties::operator==(const MappingTileProviderProperties& rhs) const
{
    return ( m_mappingTileProvider == rhs.m_mappingTileProvider &&
             m_accessToken == rhs.m_accessToken &&
             m_tileLayers == rhs.m_tileLayers );
}


const std::string& MappingTileProviderProperties::GetTileLayer(const BaseMap base_map) const
{
    const auto& tile_layer_search = m_tileLayers.find(base_map);
    ASSERT(tile_layer_search != m_tileLayers.cend());
    return tile_layer_search->second;
}


void MappingTileProviderProperties::SetTileLayer(const BaseMap base_map, std::string tile_layer)
{
    ASSERT(m_tileLayers.find(base_map) != m_tileLayers.cend());
    m_tileLayers[base_map] = std::move(tile_layer);
}



// --------------------------------------------------------------------------
// serialization
// --------------------------------------------------------------------------

MappingTileProviderProperties MappingTileProviderProperties::CreateFromJson(const JsonNode& json_node)
{
    MappingTileProviderProperties mapping_properties(json_node.Get<MappingTileProvider>(JK::name));

    mapping_properties.m_accessToken = json_node.GetOrConstruct<std::string>(JK::accessToken);

    for( const JsonNode& tile_layers_node : json_node.GetArrayOrEmpty(JK::tileLayers) )
    {
        const std::optional<BaseMap> base_map = tile_layers_node.GetOptional<BaseMap>(JK::name);

        if( base_map.has_value() )
            mapping_properties.m_tileLayers[*base_map] = tile_layers_node.Get<std::string>(JK::tileLayer);
    }

    return mapping_properties;
}


void MappingTileProviderProperties::WriteJson(JsonWriter& json_writer) const
{
    json_writer.BeginObject();

    json_writer.Write(JK::name, m_mappingTileProvider);

    if( json_writer.Verbose() || !m_accessToken.empty() )
        json_writer.Write(JK::accessToken, m_accessToken);

    json_writer.BeginArray(JK::tileLayers);

    for( const auto& [base_map, tile_layer] : m_tileLayers )
    {
        json_writer.BeginObject()
                   .Write(JK::name, base_map)
                   .Write(JK::tileLayer, tile_layer)
                   .EndObject();
    };

    json_writer.EndArray();

    json_writer.EndObject();
}


void MappingTileProviderProperties::serialize(Serializer& ar)
{
    ar & m_accessToken;

    map_serialize(ar, m_tileLayers,
        [&](BaseMap& key, std::string& value)
        {
            ar.SerializeEnum(key);
            ar & value;
        });
}
