#include "stdafx.h"
#include "MappingTileProviderProperties.h"


MappingTileProviderProperties::MappingTileProviderProperties(MappingTileProvider mapping_tile_provider)
    :   m_mappingTileProvider(mapping_tile_provider)
{
    if( m_mappingTileProvider == MappingTileProvider::Esri )
    {
        m_tileLayers.try_emplace(BaseMap::Normal, _T("Streets"));
        m_tileLayers.try_emplace(BaseMap::Hybrid, _T("NationalGeographic"));
        m_tileLayers.try_emplace(BaseMap::Satellite, _T("Imagery"));
        m_tileLayers.try_emplace(BaseMap::Terrain, _T("Topographic"));
    }

    else
    {
        m_tileLayers.try_emplace(BaseMap::Normal, _T("mapbox/streets-v11"));
        m_tileLayers.try_emplace(BaseMap::Hybrid, _T("mapbox/satellite-streets-v11"));
        m_tileLayers.try_emplace(BaseMap::Satellite, _T("mapbox/satellite-v9"));
        m_tileLayers.try_emplace(BaseMap::Terrain, _T("mapbox/outdoors-v11"));
    }
}


MappingTileProviderProperties& MappingTileProviderProperties::operator=(MappingTileProviderProperties&& rhs)
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


const std::wstring& MappingTileProviderProperties::GetTileLayer(BaseMap base_map) const
{
    const auto& tile_layer_search = m_tileLayers.find(base_map);
    ASSERT(tile_layer_search != m_tileLayers.cend());
    return tile_layer_search->second;
}


void MappingTileProviderProperties::SetTileLayer(BaseMap base_map, std::wstring tile_layer)
{
    ASSERT(m_tileLayers.find(base_map) != m_tileLayers.cend());
    m_tileLayers[base_map] = std::move(tile_layer);
}



// -----------------------------------------------------
// serialization
// -----------------------------------------------------

MappingTileProviderProperties MappingTileProviderProperties::CreateFromJson(const JsonNode<wchar_t>& json_node)
{
    MappingTileProviderProperties mapping_properties(json_node.Get<MappingTileProvider>(JK::name));

    mapping_properties.m_accessToken = json_node.GetOrDefault(JK::accessToken, SO::EmptyString);

    for( const auto& tile_layers_node : json_node.GetArrayOrEmpty(JK::tileLayers) )
    {
        std::optional<BaseMap> base_map = tile_layers_node.GetOptional<BaseMap>(JK::name);

        if( base_map.has_value() )
            mapping_properties.m_tileLayers[*base_map] = tile_layers_node.Get<std::wstring>(JK::tileLayer);
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
        [&](BaseMap& key, std::wstring& value)
        {
            ar.SerializeEnum(key);
            ar & value;
        });
}
