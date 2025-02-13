#pragma once

#include <zMapping/OfflineTileReader.h>
#include <mutex>

struct sqlite3;
struct sqlite3_stmt;


// the constructor will throw a CSProException if there are problems reading the MBTiles file;
// details on the format of MBtiles are available at https://github.com/mapbox/mbtiles-spec/blob/master/1.3/spec.md

class MBTilesReader : public OfflineTileReader
{
public:
    MBTilesReader(NullTerminatedString filename);
    ~MBTilesReader();

    std::wstring GetTileMimeType() override { return m_tileMimeType; }

    int GetTileWidth() override  { return 256; }
    int GetTileHeight() override { return 256; }

    std::optional<int> GetMinNativeZoom() override { return m_minNativeZoom; }
    std::optional<int> GetMaxNativeZoom() override { return m_maxNativeZoom; }

    std::optional<Bounds> GetBounds() override { return m_bounds; };

    std::optional<std::wstring> GetAttribution() override { return m_attribution; }

    std::unique_ptr<std::vector<std::byte>> GetTile(int z, int x, int y) override;

private:
    void CloseDatabase();

    void ReadMetadata();

private:
    sqlite3* m_db;
    sqlite3_stmt* m_stmtTileQuery;
    std::mutex m_tileQueryMutex;

    std::wstring m_tileMimeType;
    std::optional<int> m_minNativeZoom;
    std::optional<int> m_maxNativeZoom;
    std::optional<Bounds> m_bounds;
    std::optional<std::wstring> m_attribution;
};
