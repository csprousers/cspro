#pragma once


class OfflineTileReader
{
public:
    struct Bounds
    {
        // latitude/longitude
        std::tuple<double, double> min;
        std::tuple<double, double> max;
    };

    virtual ~OfflineTileReader() { }

    virtual std::wstring GetTileMimeType() = 0;
    
    virtual int GetTileWidth() = 0;
    virtual int GetTileHeight() = 0;
    
    virtual std::optional<int> GetMinNativeZoom() = 0;
    virtual std::optional<int> GetMaxNativeZoom() = 0;

    virtual std::optional<Bounds> GetBounds() = 0;

    virtual std::optional<std::wstring> GetAttribution() = 0;

    virtual std::unique_ptr<std::vector<std::byte>> GetTile(int z, int x, int y) = 0;
};
