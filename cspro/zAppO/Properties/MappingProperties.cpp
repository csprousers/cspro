#include "stdafx.h"
#include "MappingProperties.h"


MappingProperties::MappingProperties()
    :   m_coordinateDisplay(CoordinateDisplay::Decimal),
        m_defaultBaseMap(BaseMap::Normal),
        m_windowsMappingTileProvider(MappingTileProvider::Esri),
        m_esriMappingTileProviderProperties(MappingTileProvider::Esri),
        m_mapboxMappingTileProviderProperties(MappingTileProvider::Mapbox)
{
}


bool MappingProperties::operator==(const MappingProperties& rhs) const
{
    return ( m_coordinateDisplay == rhs.m_coordinateDisplay &&
             m_defaultBaseMap == rhs.m_defaultBaseMap &&
             m_windowsMappingTileProvider == rhs.m_windowsMappingTileProvider &&
             m_esriMappingTileProviderProperties == rhs.m_esriMappingTileProviderProperties &&
             m_mapboxMappingTileProviderProperties == rhs.m_mapboxMappingTileProviderProperties );
}


const MappingTileProviderProperties& MappingProperties::GetWindowsMappingTileProviderProperties() const
{
    return ( m_windowsMappingTileProvider == MappingTileProvider::Esri ) ? m_esriMappingTileProviderProperties :
                                                                           m_mapboxMappingTileProviderProperties;
}



// -----------------------------------------------------
// serialization
// -----------------------------------------------------

CREATE_ENUM_JSON_SERIALIZER(CoordinateDisplay,
    { CoordinateDisplay::Decimal, _T("decimal") },
    { CoordinateDisplay::DMS,     _T("DMS") })


MappingProperties MappingProperties::CreateFromJson(const JsonNode<wchar_t>& json_node)
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

    mapping_properties.m_windowsMappingTileProvider = json_node.GetOrDefault(JK::windowsMappingTileProvider, mapping_properties.m_windowsMappingTileProvider);

    for( const auto& tile_provider_node : json_node.GetArrayOrEmpty(JK::tileProviders) )
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
        json_writer.WriteRelativePath(JK::defaultBaseMap, std::get<std::wstring>(m_defaultBaseMap));
    }
    
    json_writer.Write(JK::windowsMappingTileProvider, m_windowsMappingTileProvider);

    json_writer.BeginArray(JK::tileProviders)
               .Write(m_esriMappingTileProviderProperties)
               .Write(m_mapboxMappingTileProviderProperties)
               .EndArray();

    json_writer.EndObject();
}


void MappingProperties::serialize(Serializer& ar)
{
    ar.SerializeEnum(m_coordinateDisplay);
    ar.SerializeEnum(m_windowsMappingTileProvider);

    if( ar.IsSaving() )
    {
        ar.Write(ToString(m_defaultBaseMap, ar.GetArchiveFilename().c_str()));
    }

    else
    {
        m_defaultBaseMap = FromString(ar.Read<std::wstring>(), ar.GetArchiveFilename().c_str());
    }

    ar & m_esriMappingTileProviderProperties;
    ar & m_mapboxMappingTileProviderProperties;
}
