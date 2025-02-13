#pragma once

#include <zAppO/zAppO.h>
#include <zAppO/MappingDefines.h>

template<typename CharType> class JsonNode;
class JsonWriter;
class Serializer;


class ZAPPO_API MappingTileProviderProperties
{
    friend class MappingProperties;

public:
    MappingTileProviderProperties(MappingTileProvider mapping_tile_provider);
    MappingTileProviderProperties(const MappingTileProviderProperties& rhs) = default;
    MappingTileProviderProperties(MappingTileProviderProperties&& rhs) = default;

    MappingTileProviderProperties& operator=(MappingTileProviderProperties&& rhs);

    bool operator==(const MappingTileProviderProperties& rhs) const;
    bool operator!=(const MappingTileProviderProperties& rhs) const { return !( *this == rhs ); }

    MappingTileProvider GetMappingTileProvider() const { return m_mappingTileProvider; }

    const std::wstring& GetAccessToken() const     { return m_accessToken; }
    void SetAccessToken(std::wstring access_token) { m_accessToken = std::move(access_token); }

    const std::map<BaseMap, std::wstring>& GetTileLayers() const { return m_tileLayers; }
    const std::wstring& GetTileLayer(BaseMap base_map) const;
    void SetTileLayer(BaseMap base_map, std::wstring tile_layer);


    // serialization
    // -----------------------------------------------------
    static MappingTileProviderProperties CreateFromJson(const JsonNode<wchar_t>& json_node);
    void WriteJson(JsonWriter& json_writer) const;

    void serialize(Serializer& ar);


private:
    const MappingTileProvider m_mappingTileProvider;
    std::wstring m_accessToken;
    std::map<BaseMap, std::wstring> m_tileLayers;
};
