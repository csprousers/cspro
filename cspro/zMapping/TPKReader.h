#pragma once

#include <zMapping/OfflineTileReader.h>
#include <mutex>

class ZipReader;


// the constructor will throw a CSProException if there are problems reading the Tile Package file;
// this implementation is a C++ version of the one added in Java for CSPro 7.3 and comments can
// be found in that file: ..\CSEntryDroid\app\src\main\java\gov\census\cspro\maps\offline\TpkTilesReader.java

class TPKReader : public OfflineTileReader
{
public:
    TPKReader(const std::string& file_path);
    ~TPKReader();

    int GetTileWidth() override  { return m_tileWidth; }
    int GetTileHeight() override { return m_tileHeight; }

    std::optional<int> GetMinNativeZoom() override { return m_zoomLevelBundlePathMap.cbegin()->first; }
    std::optional<int> GetMaxNativeZoom() override { return m_zoomLevelBundlePathMap.crbegin()->first; }

    std::optional<Bounds> GetBounds() override { return m_initialExtent; };

    const std::string& GetAttribution() override { return SO::Empty_string; }

    std::optional<Tile> GetTile(int z, int x, int y) override;

    std::string GetMetadataAsJson() const;

private:
    class MetadataReader;
    class MetadataReader_Tpk;
    class MetadataReader_Tpkx;
    void ReadMetadata(const std::string& file_path);

    std::shared_ptr<const BinaryBlock> ReadZipFileAndCache(const std::string& path_in_zip);

    static void ReadData(void* out_data, const BinaryBlock& in_data, size_t offset, size_t size);

    std::optional<BinaryBlock> GetTileV1(const std::string& bundle_path, int x, int y);
    std::optional<BinaryBlock> GetTileV2(const std::string& bundle_path, int x, int y);

private:
    std::unique_ptr<ZipReader> m_zipReader;
    std::mutex m_tileQueryMutex;
    std::vector<std::tuple<std::string, std::shared_ptr<BinaryBlock>>> m_cachedZipFiles;

    int m_tileWidth;
    int m_tileHeight;
    std::vector<SharableString> m_tileMimeTypes;
    int m_packetSize;
    bool m_v1BundleType;
    std::map<int, std::string> m_zoomLevelBundlePathMap;
    std::optional<Bounds> m_initialExtent;
    std::optional<Bounds> m_fullExtent;
};
