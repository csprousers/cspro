#pragma once

#include <zMapping/OfflineTileReader.h>
#include <zZipo/ZipReader.h>
#include <mutex>


// the constructor will throw a CSProException if there are problems reading the Tile Package file;
// this implementation is a C++ version of the one added in Java for CSPro 7.3 and comments can
// be found in that file: ..\CSEntryDroid\app\src\main\java\gov\census\cspro\maps\offline\TpkTilesReader.java

class TPKReader : public OfflineTileReader
{
public:
    TPKReader(NullTerminatedString filename);

    std::wstring GetTileMimeType() override { return m_tileMimeType; }

    int GetTileWidth() override  { return m_tileWidth; }
    int GetTileHeight() override { return m_tileHeight; }

    std::optional<int> GetMinNativeZoom() override { return m_zoomLevelBundlePathMap.cbegin()->first; }
    std::optional<int> GetMaxNativeZoom() override { return m_zoomLevelBundlePathMap.crbegin()->first; }

    std::optional<Bounds> GetBounds() override { return m_bounds; };

    std::optional<std::wstring> GetAttribution() override { return std::nullopt; }

    std::unique_ptr<std::vector<std::byte>> GetTile(int z, int x, int y) override;

private:
    void ReadConfXml();

    std::map<int, std::wstring> GetLevelBundlePaths(const std::wstring& conf_xml_filename);
    std::optional<int> ScaleToZoomLevel(double scale);

    void ReadBounds();

    std::shared_ptr<const std::vector<std::byte>> ReadZipFileAndCache(const std::wstring& path_in_zip);

    std::unique_ptr<std::vector<std::byte>> GetTileV1(const std::wstring& bundle_path, int x, int y);
    std::unique_ptr<std::vector<std::byte>> GetTileV2(const std::wstring& bundle_path, int x, int y);

private:
    ZipReader m_zipReader;
    std::mutex m_tileQueryMutex;
    std::vector<std::tuple<std::wstring, std::shared_ptr<const std::vector<std::byte>>>> m_cachedZipFiles;

    std::wstring m_tileMimeType;
    int m_tileWidth;
    int m_tileHeight;
    int m_packetSize;
    bool m_v1BundleType;
    std::map<int, std::wstring> m_zoomLevelBundlePathMap;
    std::optional<Bounds> m_bounds;
};
