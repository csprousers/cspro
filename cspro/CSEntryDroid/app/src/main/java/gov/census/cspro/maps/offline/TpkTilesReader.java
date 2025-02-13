package gov.census.cspro.maps.offline;

import android.annotation.SuppressLint;
import androidx.annotation.Nullable;

import com.google.android.gms.maps.model.LatLng;
import com.google.android.gms.maps.model.LatLngBounds;
import com.jayway.jsonpath.DocumentContext;
import com.jayway.jsonpath.JsonPath;

import org.w3c.dom.Document;
import org.w3c.dom.Node;
import org.w3c.dom.NodeList;
import org.xml.sax.SAXException;

import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.util.Collections;
import java.util.Comparator;
import java.util.Enumeration;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Locale;
import java.util.Map;
import java.util.Set;
import java.util.TreeMap;
import java.util.regex.Matcher;
import java.util.regex.Pattern;
import java.util.zip.ZipEntry;
import java.util.zip.ZipFile;

import javax.xml.parsers.DocumentBuilder;
import javax.xml.parsers.DocumentBuilderFactory;
import javax.xml.parsers.ParserConfigurationException;
import javax.xml.xpath.XPath;
import javax.xml.xpath.XPathConstants;
import javax.xml.xpath.XPathExpressionException;
import javax.xml.xpath.XPathFactory;

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
    private final ZipFile m_zipFile;
    private final String m_allLayersPath;
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
    private final Map<Integer, Integer> m_zoomLevelToLod = new HashMap<>();

    private interface BundleReader {
        byte[] getTile(String bundlePath, int row, int column);
    }

    private final BundleReader m_bundleReader;

    public TpkTilesReader(String tilePackageFilePath) throws IOException
    {
        m_zipFile = new ZipFile(tilePackageFilePath);

        // Find the conf.xml that has the package metadata. Since this
        // file is inside a folder based on layer name just search for any
        // file named conf.xml. There should only be one.
        ZipEntry confEntry = findConfFileInZip();
        if (confEntry == null)
        {
            throw new IOException("Tile package missing conf.xml");
        }

        // Save off path to _alllayers folder that we will need to retreive tiles
        // later on. This is always the same directory as the conf.xml.
        m_allLayersPath = new File(confEntry.getName()).getParent() + "/_alllayers/";

        try
        {
            DocumentBuilderFactory docBuilderFactory = DocumentBuilderFactory.newInstance();
            DocumentBuilder docBuilder = docBuilderFactory.newDocumentBuilder();
            Document document = docBuilder.parse(m_zipFile.getInputStream(confEntry));

            XPathFactory xPathFactory = XPathFactory.newInstance();
            XPath xpath = xPathFactory.newXPath();

            m_tileWidth = (int) (double) xpath.evaluate("/CacheInfo/TileCacheInfo/TileCols/text()", document, XPathConstants.NUMBER);
            m_tileHeight = (int) (double) xpath.evaluate("/CacheInfo/TileCacheInfo/TileRows/text()", document, XPathConstants.NUMBER);
            String tileFormat = xpath.evaluate("/CacheInfo/TileImageInfo/CacheTileFormat/text()", document).toLowerCase();
            if (tileFormat.startsWith("png")) {
                m_tileFormat = PNG;
            } else if (tileFormat.compareTo("jpg") == 0 || tileFormat.compareTo("jpeg") == 0) {
                m_tileFormat = JPG;
            } else {
                throw new IOException("Invalid tile format " + tileFormat);
            }

            m_packetSize = (int) (double) xpath.evaluate("/CacheInfo/CacheStorageInfo/PacketSize/text()", document, XPathConstants.NUMBER);
            String storageFormat = xpath.evaluate("/CacheInfo/CacheStorageInfo/StorageFormat/text()", document);
            switch (storageFormat) {
                case "esriMapCacheStorageModeCompact":
                    m_bundleReader = new V1BundleReader();
                    break;
                case "esriMapCacheStorageModeCompactV2":
                    m_bundleReader = new V2BundleReader();
                    break;
                default:
                    throw new IOException("Invalid storage format: " + storageFormat);
            }

            // Extract the level of detail nodes and match the scales to the scales for google maps
            // zoom levels
            NodeList levelsOfDetail = (NodeList) xpath.evaluate("/CacheInfo/TileCacheInfo/LODInfos/*", document, XPathConstants.NODESET);
            Set<Integer> levelsWithBundles = findLevelsWithBundles();

            Map<Double, Integer> scaleToZoomLevel = createScaleToZoomLevelMap();
            for (int i = 0; i < levelsOfDetail.getLength(); ++i)
            {
                Node lodNode = levelsOfDetail.item(i);
                int id = (int) (double) xpath.evaluate("LevelID/text()", lodNode, XPathConstants.NUMBER);
                double scale = (double) xpath.evaluate("Scale/text()", lodNode, XPathConstants.NUMBER);

                // Check to make sure that the folder for the level exists and contains at least one tile
                if (levelsWithBundles.contains(id))
                {
                    Integer zoomLevel = scaleToZoomLevel.get(scale);
                    if (zoomLevel == null)
                    {
                        throw new IOException("Invalid scale " + scale + " in tiling scheme for LevelID " + id + ". Only scales from ArcGIS Online/Bing Maps/Google Maps scheme are supported.");
                    }
                    m_zoomLevelToLod.put(zoomLevel, id);
                }
            }

            if (m_zoomLevelToLod.isEmpty())
                throw new IOException("Tile package doesn't contain any valid levels of detail");

            // Get bounds from mapserver.json file
            ZipEntry mapserverEntry = m_zipFile.getEntry("servicedescriptions/mapserver/mapserver.json");
            LatLngBounds initialExtent = null, fullExtent = null;
            if (mapserverEntry != null) {
                try
                {
                    DocumentContext jsonContext = JsonPath.parse(m_zipFile.getInputStream(mapserverEntry));
                    initialExtent = getBoundsFromJson(jsonContext, "$.resourceInfo.geoInitialExtent");
                    fullExtent = getBoundsFromJson(jsonContext, "$.resourceInfo.geoFullExtent");
                } catch (IOException e) {
                    Timber.e(e, "Error reading extents from mapserver.json");
                }
            }
            m_initialExtent = initialExtent;
            m_fullExtent = fullExtent;

        } catch (ParserConfigurationException e)
        {
            throw new IOException("Failed to parse conf.xml", e);
        } catch (SAXException e)
        {
            throw new IOException("Failed to parse conf.xml", e);
        } catch (XPathExpressionException e)
        {
            throw new IOException("Failed to parse conf.xml", e);
        }
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
        return Collections.max(m_zoomLevelToLod.keySet());
    }

    @Override
    public int getMinZoom()
    {
        return Collections.min(m_zoomLevelToLod.keySet());
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
        Integer lod = m_zoomLevelToLod.get(z);
        if (lod == null)
        {
            Timber.d("No tile at (" + x + "," + y + "," + z + "): no tiles at this zoom level");
            return null;
        }

        String bundlePath = m_allLayersPath + String.format(Locale.ENGLISH, "L%02d/R%04xC%04x.bundle", lod, bundleRow, bundleColumn);
        return m_bundleReader.getTile(bundlePath, x, y);
    }

    @Override
    public void close() throws IOException
    {
        m_zipFile.close();
    }

    private ZipEntry findConfFileInZip()
    {
        Enumeration<? extends ZipEntry> entries = m_zipFile.entries();
        while (entries.hasMoreElements())
        {
            ZipEntry entry = entries.nextElement();
            if (!entry.isDirectory() && entry.getName().endsWith("conf.xml"))
            {
                return entry;
            }
        }
        return null;
    }

    private Set<Integer> findLevelsWithBundles()
    {
        Set<Integer> levels = new HashSet<>();
        Pattern pattern = Pattern.compile("^" + m_allLayersPath + "L(\\d{2})/.*\\.bundle$");

        Enumeration<? extends ZipEntry> entries = m_zipFile.entries();
        while (entries.hasMoreElements())
        {
            ZipEntry entry = entries.nextElement();
            if (!entry.isDirectory())
            {
                Matcher m = pattern.matcher(entry.getName());
                if (m.matches())
                {
                    int level = Integer.parseInt(m.group(1));
                    levels.add(level);
                }
            }
        }
        return levels;
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

    // Create map from ArcGIS scale that is found in LOD nodes in tpk conf.xml
    // to Google Maps zoom levels.
    private static Map<Double, Integer> createScaleToZoomLevelMap()
    {
        final Map<Double, Integer> map = new TreeMap<>(new Comparator<Double>()
        {
            // Since the scale levels are floating point it seems that they get rounded
            // differently in different tpk files so need to do a fuzzy compare.
            private final static double EPSILON = 1e-5;

            @Override
            public int compare(Double a, Double b)
            {
                if (Math.abs(a - b) < EPSILON)
                    return 0; // Equals
                else
                    return (a < b) ? -1 : +1;
            }
        });

        // These are copied from the ArcGIS_Online_Bing_Maps_Google_Maps.xml
        // tiling scheme that comes with ArcGIS.
        map.put(591657527.591555, 0);
        map.put(295828763.795777, 1);
        map.put(147914381.897889, 2);
        map.put(73957190.948944, 3);
        map.put(36978595.474472, 4);
        map.put(18489297.737236, 5);
        map.put(9244648.868618, 6);
        map.put(4622324.434309, 7);
        map.put(2311162.217155, 8);
        map.put(1155581.108577, 9);
        map.put(577790.554289, 10);
        map.put(288895.277144, 11);
        map.put(144447.638572, 12);
        map.put(72223.819286, 13);
        map.put(36111.909643, 14);
        map.put(18055.954822, 15);
        map.put(9027.977411, 16);
        map.put(4513.988705, 17);
        map.put(2256.994353, 18);
        map.put(1128.497176, 19);
        map.put(564.248588, 20);
        map.put(282.124294, 21);
        map.put(141.062147, 22);
        map.put(70.531074, 23);

        return map;
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
