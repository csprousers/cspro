#pragma once

#include <zToolsO/BinaryBlock.h>


class OfflineTileReader
{
public:
    struct Bounds
    {
        // latitude/longitude
        std::tuple<double, double> min;
        std::tuple<double, double> max;
    };

    struct Tile
    {
        BinaryBlock image;
        SharableString mime_type;
    };

    virtual ~OfflineTileReader() { }

    virtual int GetTileWidth() = 0;
    virtual int GetTileHeight() = 0;

    virtual std::optional<int> GetMinNativeZoom() = 0;
    virtual std::optional<int> GetMaxNativeZoom() = 0;

    virtual std::optional<Bounds> GetBounds() = 0;

    virtual const std::string& GetAttribution() = 0;

    virtual std::optional<Tile> GetTile(int z, int x, int y) = 0;
};
