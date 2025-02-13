#pragma once

#include <zMapping/OfflineTileReader.h>
#include <thread>

namespace httplib { class Server; }


class OfflineTileProvider
{
public:
    OfflineTileProvider(std::shared_ptr<OfflineTileReader> offline_tile_reader);
    ~OfflineTileProvider();

    std::wstring GetTileLayerUrl() const;

    std::wstring GetLeafletTileLayerOptions() const;

private:
    std::shared_ptr<OfflineTileReader> m_offlineTileReader;
    std::string m_tileMimeType;
    std::wstring m_tileExtension;

    httplib::Server* m_server;
    std::thread m_thread;
    int m_port;
};
