#pragma once

#include <zMapping/OfflineTileReader.h>

class SimpleServer;


class OfflineTileProvider
{
public:
    OfflineTileProvider(std::shared_ptr<OfflineTileReader> offline_tile_reader);
    ~OfflineTileProvider();

    std::string GetTileLayerUrl() const;

    void WriteJsonLeafletTileLayerOptions(JsonWriter& json_writer) const;

private:
    std::shared_ptr<OfflineTileReader> m_offlineTileReader;

    std::unique_ptr<SimpleServer> m_server;
};
