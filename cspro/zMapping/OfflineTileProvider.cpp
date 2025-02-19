#include "stdafx.h"
#include "OfflineTileProvider.h"
#include "OfflineTileReader.h"
#include <zHtml/SimpleServer.h>


OfflineTileProvider::OfflineTileProvider(std::shared_ptr<OfflineTileReader> offline_tile_reader)
    :   m_offlineTileReader(std::move(offline_tile_reader)),
        m_server(std::make_unique<SimpleServer>())
{
    ASSERT(m_offlineTileReader != nullptr);

    m_server->AddMapping("(\\/\\d+\\/\\d+\\/\\d+)",
        [&](SimpleServer::Handler& handler)
        {
            const std::vector<std::string> matches = SO::SplitString(handler.GetRequestMatches()[1].str(), '/', true, false);

            if( matches.size() != 3 )
            {
                ASSERT(false);
                return;
            }

            const std::optional<OfflineTileReader::Tile> tile = m_offlineTileReader->GetTile(atoi(matches[0].c_str()),
                                                                                             atoi(matches[1].c_str()),
                                                                                             atoi(matches[2].c_str()));

            if( tile.has_value() )
                handler.SetResponseContent(tile->image.data<char>(), tile->image.size(), tile->mime_type.GetString());
        });
}


OfflineTileProvider::~OfflineTileProvider()
{
}


std::string OfflineTileProvider::GetTileLayerUrl() const
{
    return m_server->GetBaseUrl() + "{z}/{x}/{y}";
}


CREATE_JSON_KEY(attribution)
CREATE_JSON_KEY(bounds)
CREATE_JSON_KEY(maxNativeZoom)
CREATE_JSON_KEY(minNativeZoom)
CREATE_JSON_KEY(tileSize)


void OfflineTileProvider::WriteJsonLeafletTileLayerOptions(JsonWriter& json_writer) const
{
    json_writer.BeginObject();

    // add any defined metadata
    ASSERT(m_offlineTileReader->GetTileWidth() == m_offlineTileReader->GetTileHeight());
    json_writer.Write(JK::tileSize, m_offlineTileReader->GetTileWidth());

    json_writer.WriteIfHasValue(JK::minNativeZoom, m_offlineTileReader->GetMinNativeZoom())
               .WriteIfHasValue(JK::maxNativeZoom, m_offlineTileReader->GetMaxNativeZoom());

    const std::optional<OfflineTileReader::Bounds> bounds = m_offlineTileReader->GetBounds();

    if( bounds.has_value() )
    {
        json_writer.BeginArray(JK::bounds)
                   .Write(std::vector<double>{ std::get<0>(bounds->min), std::get<1>(bounds->min) })
                   .Write(std::vector<double>{ std::get<0>(bounds->max), std::get<1>(bounds->max) })
                   .EndArray();
    }

    json_writer.WriteIfNotBlank(JK::attribution, m_offlineTileReader->GetAttribution());

    json_writer.EndObject();
}
