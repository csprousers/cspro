package gov.census.cspro.maps.offline;

import android.annotation.SuppressLint;
import androidx.annotation.Nullable;

import com.google.android.gms.maps.model.LatLng;
import com.google.android.gms.maps.model.LatLngBounds;
import com.jayway.jsonpath.DocumentContext;
import com.jayway.jsonpath.JsonPath;

import java.io.IOException;
import java.io.InputStream;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.util.Collections;
import java.util.HashMap;
import java.util.List;
import java.util.Locale;
import java.util.Map;
import java.util.zip.ZipEntry;
import java.util.zip.ZipFile;

import gov.census.cspro.engine.EngineInterface;
import timber.log.Timber;

/**
 * Reader for ArcGIS tile packages in Compact Cache format (.tpk files)
 *
 * Tpk files are tile packages exported from ArcGIS.
 *
 * Creating a tile package from ArcGIS is described here:
 *
 *      http://desktop.arcgis.com/en/arcmap/10.3/map/working-with-arcmap/about-tile-packages.htm
 *
 * It is important to use the "ArcGIS Online / Bing Maps / Google Maps" tiling scheme otherwise
 * the zoom levels and row/column numbers will not match up with what the Google Maps SDK expects.
 * We also need tiles that are 256x256 jpeg or png (jpeg will be smaller).
 *
 * Tpk files are zip archives that contain an xml descriptor file named conf.xml
 * and directories of tiles in cache bundles (.bundle files). The bundle files
 * are organized in folders by zoom level and named for the tile row and columns.
 * A single bundle file can contain tile images for up to 128x128 tile grid.
 *
 * The layout of the zip archive looks something like this:
 *
 * esriinfo
 *      iteminfo.xml
 *      item.pkinfo
 * servicedescriptions
 *      mapserver
 *              mapserver.json
 * v101
 *      layername
 *              conf.xml
 *              conf.cdi
 *              conf.properties
 *              _alllayers
 *                  L00
 *                      R0000C0000.bundle
 *                      R0000C0000.bundlx
 *                  L01
 *                      R0000C0000.bundle
 *                      R0000C0000.bundlx
 *
 * The folders starting with L are the levels (zoom levels) and the bundle file names
 * are of the format RxxxxCyyyy.bundle where xxxx and yyyy are the row and column number
 * of the top-left most tile in the bundle in hex.
 *
 * The bundle file format changed in ArcGIS 10.3. The new format named Compact Cache V2
 * is described here:
 *
 *      https://github.com/Esri/raster-tiles-compactcache/blob/master/CompactCacheV2.md
 *
 * The old format is described here:
 *
 *      https://gdbgeek.wordpress.com/2012/08/09/demystifying-the-esri-compact-cache/
 *
 * The difference between the two formats seems to be that in the older format
 * each .bundle file has a corresponding .bundlx file that contains the indices
 * of the tiles in the bundle file and the new format the index has been moved
 * inside the bundle file itself.
 *
 * Since the documentation is a bit scarce it is best to look at some other projects
 * that have implemented readers/writers for the format:
 *
 * TpkUtils converts tpk files to mbtiles and xyz exploded format. Currently only supports the older
 * v1 format:
 *
 *      https://github.com/consbio/tpkutils
 *
 * Geowebcache has readers for both old and new formats:
 *      https://github.com/GeoWebCache/geowebcache/tree/master/geowebcache/arcgiscache/src/main/java/org/geowebcache/arcgis/compact
 *
 * Tiler arcgis bundle has reader for old format:
 *
 *      https://github.com/fuzhenn/tiler-arcgis-bundle
 */
public class TpkTilesReader implements IOfflineTileReader
{
    private final int m_packetSize;
    private final int m_tileFormat;
    private final int m_tileWidth;
    private final int m_tileHeight;
    private final LatLngBounds m_initialExtent;
    private final LatLngBounds m_fullExtent;

    // The levels of detail in the tpk folder are used to get the folder names
    // for the level of detail. Each level of detail is in a folder named LXX where XX is
    // the level id. The level id does not necessarily correspond to a google maps zoom level.
    // This map is built based on the scales in the tpk file to map the google map zoom level
    // to the corresponding level id.
    @SuppressLint("UseSparseArrays")
    private final Map<Integer, String> m_zoomLevelBundlePathMap = new HashMap();

    private interface BundleReader {
        byte[] getTile(String bundlePath, int row, int column);
    }

    private final BundleReader m_bundleReader;

    private final ZipFile m_zipFile;

    public TpkTilesReader(String tilePackageFilePath) throws IOException
    {
        // get the TPK metadata from the C++ code as that supports both .tpk and .tpkx files
        String tpkMetadataJson = EngineInterface.GetTpkMetadataAsJson(tilePackageFilePath);
        DocumentContext jsonContext = JsonPath.parse(tpkMetadataJson);

        m_packetSize = jsonContext.read("$.packetSize", Integer.class);

        List<String> tileMimeTypes = jsonContext.read("$.tileMimeTypes", List.class);
        if( tileMimeTypes.size() != 1 ) {
            throw new IOException("Tile packages with tile type 'MIXED' are not supported.");
        }
        else if( tileMimeTypes.get(0).equals("image/png") ) {
            m_tileFormat = PNG;
        }
        else if( tileMimeTypes.get(0).equals("image/jpeg") ) {
            m_tileFormat = JPG;
        }
        else {
            throw new IOException("Invalid tile format " + tileMimeTypes.get(0));
        }

        m_tileWidth = jsonContext.read("$.tileWidth", Integer.class);
        m_tileHeight = jsonContext.read("$.tileHeight", Integer.class);

        m_initialExtent = getBoundsFromJson(jsonContext, "$.initialExtent");
        m_fullExtent = getBoundsFromJson(jsonContext, "$.fullExtent");

        Map<String, String> zoomLevelBundlePathMapWithStrings = jsonContext.read("$.zoomLevelBundlePathMap", Map.class);
        for( Map.Entry<String, String> entry : zoomLevelBundlePathMapWithStrings.entrySet() ) {
            m_zoomLevelBundlePathMap.put(Integer.parseInt(entry.getKey()), entry.getValue());
        }

        if( jsonContext.read("$.v1BundleType", Boolean.class) ) {
            m_bundleReader = new V1BundleReader();
        }
        else {
            m_bundleReader = new V2BundleReader();
        }

        m_zipFile = new ZipFile(tilePackageFilePath);
    }

    @Override
    public int getFormat()
    {
        return m_tileFormat;
    }

    @Override
    public int getTileWidth()
    {
        return m_tileWidth;
    }

    @Override
    public int getTileHeight()
    {
        return m_tileHeight;
    }

    @Override
    public int getMaxZoom()
    {
        return Collections.max(m_zoomLevelBundlePathMap.keySet());
    }

    @Override
    public int getMinZoom()
    {
        return Collections.min(m_zoomLevelBundlePathMap.keySet());
    }

    @Nullable
    @Override
    public LatLngBounds getFullExtent()
    {
        return m_fullExtent;
    }

    @Nullable
    @Override
    public LatLngBounds getInitialExtent()
    {
        return m_initialExtent;
    }

    @Nullable
    @Override
    public byte[] getTile(int x, int y, int z)
    {
        Timber.d("getTile(" + x + "," + y + "," + z + ")");

        int bundleRow = (y/m_packetSize) * m_packetSize;
        int bundleColumn = (x/m_packetSize) * m_packetSize;
        String path = m_zoomLevelBundlePathMap.get(z);
        if (path == null)
        {
            Timber.d("No tile at (" + x + "," + y + "," + z + "): no tiles at this zoom level");
            return null;
        }

        String bundlePath = path + String.format(Locale.ENGLISH, "/R%04xC%04x.bundle", bundleRow, bundleColumn);
        return m_bundleReader.getTile(bundlePath, x, y);
    }

    @Override
    public void close() throws IOException
    {
        m_zipFile.close();
    }

    private class V1BundleReader implements BundleReader
    {
        @Override
        public byte[] getTile(String bundlePath, int row, int column)
        {
            try
            {
                // Load the bundlx file that contains the index
                String bundleIndexPath = bundlePath.substring(0,bundlePath.length() - 1) + "x";
                ZipEntry bundleIndexEntry = m_zipFile.getEntry(bundleIndexPath);
                if (bundleIndexEntry == null)
                {
                    Timber.d("No tile at (" + row + "," + column + "): bundle index " + bundleIndexPath + " not found");
                    return null;
                }

                InputStream bundleIndexStream = m_zipFile.getInputStream(bundleIndexEntry);

                final int headerSize = 16;
                final int indexSize = 5;

                int tileIndexOffset = headerSize + indexSize * (m_packetSize * (row % m_packetSize) + (column % m_packetSize));
                if (skipNBytes(bundleIndexStream, tileIndexOffset) != tileIndexOffset)
                {
                    Timber.e("Error reading tile (" + row + "," + column + "): bundle index " + bundleIndexPath + " failed to skip to index tileOffset " + tileIndexOffset);
                    return null;
                }

                byte[] tileIndexBytes = new byte[8];

                if (readNBytes(bundleIndexStream, tileIndexBytes, indexSize) != indexSize)
                {
                    Timber.e("Error reading tile (" + row + "," + column + "): bundle index " + bundleIndexPath + " failed to read index at " + tileIndexOffset);
                    return null;
                }

                ByteBuffer tileIndexBuff = ByteBuffer.wrap(tileIndexBytes).order(ByteOrder.LITTLE_ENDIAN);
                long tileIndex = tileIndexBuff.getLong();

                // Load the bundle file that contains the tile image itself
                ZipEntry bundleEntry = m_zipFile.getEntry(bundlePath);
                if (bundleEntry == null)
                {
                    Timber.e("Error reading tile at (" + row + "," + column + "): bundle " + bundlePath + " bundle file not found");
                    return null;
                }

                InputStream bundleStream = m_zipFile.getInputStream(bundleEntry);

                if (skipNBytes(bundleStream, tileIndex) != tileIndex) {
                    Timber.e("Error reading tile (" + row + "," + column + "): bundle " + bundlePath + " failed skip to tile offset " + tileIndex);
                    return null;
                }

                final int tileSizeSize = 4;
                byte[] tileSizeBytes = new byte[tileSizeSize];

                if (readNBytes(bundleStream, tileSizeBytes, tileSizeSize) != tileSizeSize)
                {
                    Timber.e("Error reading tile (" + row + "," + column + "): bundle " + bundlePath + " failed to read tile size at " + tileIndex);
                    return null;
                }
                ByteBuffer tileSizeBuff = ByteBuffer.wrap(tileSizeBytes).order(ByteOrder.LITTLE_ENDIAN);
                final int tileSize = tileSizeBuff.getInt();

                byte[] tileData = new byte[tileSize];

                if (tileSize == 0) {
                    Timber.e("Error reading tile (" + row + "," + column + "): bundle " + bundlePath + " tile size is zero");
                    return null;
                }

                if (readNBytes(bundleStream, tileData, tileSize) != tileSize)
                {
                    Timber.e("Error reading tile (" + row + "," + column + "): bundle " + bundlePath + " failed to read " + tileSize + " bytes at offset " + tileIndex);
                    return null;
                }

                return tileData;
            } catch (IOException e)
            {
                Timber.e(e, "Error reading tile (" + row + "," + column + "): bundle " + bundlePath);
                return null;
            }
        }
    }

    private class V2BundleReader implements BundleReader
    {
        @Override
        public byte[] getTile(String bundlePath, int row, int column)
        {
            ZipEntry bundleEntry = m_zipFile.getEntry(bundlePath);
            if (bundleEntry == null) {
                Timber.d("No tile at (" + row + "," + column + "): bundle " + bundlePath + " not found");
                return null;
            }

            try
            {
                InputStream bundleStream = m_zipFile.getInputStream(bundleEntry);

                final int headerSize = 64;
                final int indexSize = 8;

                // V2 appears to reverse the order of row/column from the V1 version
                int temp_row = row;
                row = column;
                column = temp_row;

                // Skip 64 byte header plus 8 bytes for previous tile indices
                int tileIndexOffset = headerSize + indexSize * (m_packetSize * (row % m_packetSize) + (column % m_packetSize));
                if (skipNBytes(bundleStream, tileIndexOffset) != tileIndexOffset) {
                    Timber.e("Error reading tile (" + row + "," + column + "): bundle " + bundlePath + " failed to skip to index tileOffset " + tileIndexOffset);
                    return null;
                }

                byte[] tileIndexBytes = new byte[indexSize];

                if (readNBytes(bundleStream, tileIndexBytes, indexSize) != indexSize)
                {
                    Timber.e("Error reading tile (" + row + "," + column + "): bundle " + bundlePath + " failed to read index at " + tileIndexOffset);
                    return null;
                }

                ByteBuffer tileIndexBuff = ByteBuffer.wrap(tileIndexBytes).order(ByteOrder.LITTLE_ENDIAN);
                long tileIndex = tileIndexBuff.getLong();
                final long M = 0x10000000000L; // 2 to the power of 40
                int tileOffset = (int) (tileIndex % M);
                int tileSize = (int) (tileIndex / M);

                if (tileSize == 0)
                {
                    Timber.e("Error reading tile (" + row + "," + column + "): bundle " + bundlePath + " index " + tileIndex + " size is 0");
                    return null;
                }

                long bytesToTileStart = tileOffset - tileIndexOffset - indexSize;
                if (skipNBytes(bundleStream, bytesToTileStart) != bytesToTileStart) {
                    Timber.e("Error reading tile (" + row + "," + column + "): bundle " + bundlePath + " failed to skip to tile tileOffset " + tileOffset);
                    return null;
                }

                byte[] tileData = new byte[tileSize];

                if (readNBytes(bundleStream, tileData, tileSize) != tileSize)
                {
                    Timber.e("Error reading tile (" + row + "," + column + "): bundle " + bundlePath + " index " + tileIndex + " failed to read " + tileSize + " bytes at tileOffset " + tileOffset);
                    return null;
                }

                return tileData;
            } catch (IOException e) {
                Timber.d(e, "Error reading tile (" + row + "," + column + "): bundle " + bundlePath);
                return null;
            }
        }
    }

    private static int readNBytes(InputStream src, byte[] dst, int length) throws IOException
    {
        int totalRead = 0;
        while (totalRead < length) {
            int read = src.read(dst, totalRead, length - totalRead);
            if (read < 0)
                return totalRead;
            totalRead += read;
        }
        return totalRead;
    }

    private static long skipNBytes(InputStream is, long bytesToSkip) throws IOException
    {
        long totalSkipped = 0;
        while (totalSkipped < bytesToSkip) {
            long skipped = is.skip(bytesToSkip - totalSkipped);
            if (skipped < 0)
                return totalSkipped;
            totalSkipped += skipped;
        }
        return totalSkipped;
    }

    private LatLngBounds getBoundsFromJson(DocumentContext jsonContext, String jsonPath) throws IOException
    {
        try {
            double xmin = jsonContext.read(jsonPath + ".xmin", Double.class);
            double ymin = jsonContext.read(jsonPath + ".ymin", Double.class);
            double xmax = jsonContext.read(jsonPath + ".xmax", Double.class);
            double ymax = jsonContext.read(jsonPath + ".ymax", Double.class);
            return new LatLngBounds(new LatLng(ymin, xmin), new LatLng(ymax, xmax));
        } catch (Exception e) {
            throw new IOException("Failed to extent from mapserver.json");
        }
    }
}
