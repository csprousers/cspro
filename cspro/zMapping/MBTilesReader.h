#pragma once

#include <zMapping/OfflineTileReader.h>
#include <zSql/DB.h>
#include <mutex>


// --------------------------------------------------------------------------
// MBTilesReader
//
// The constructor will throw a CSProException if there are problems reading
// the MBTiles file.
//
// Details on the format of MBtiles are available at:
//     https://github.com/mapbox/mbtiles-spec/blob/master/1.3/spec.md
// --------------------------------------------------------------------------

class MBTilesReader : public OfflineTileReader
{
public:
    MBTilesReader(const std::string& file_path);

    int GetTileWidth() override  { return 256; }
    int GetTileHeight() override { return 256; }

    std::optional<int> GetMinNativeZoom() override { return m_minNativeZoom; }
    std::optional<int> GetMaxNativeZoom() override { return m_maxNativeZoom; }

    std::optional<Bounds> GetBounds() override { return m_bounds; };

    const std::string& GetAttribution() override { return m_attribution; }

    std::optional<Tile> GetTile(int z, int x, int y) override;

private:
    void ReadMetadata();

private:
    Sqlite::DB m_db;
    Sqlite::Statement m_stmtTileQuery;
    std::mutex m_tileQueryMutex;

    SharableString m_tileMimeType;
    std::optional<int> m_minNativeZoom;
    std::optional<int> m_maxNativeZoom;
    std::optional<Bounds> m_bounds;
    std::string m_attribution;
};
