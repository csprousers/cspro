package gov.census.cspro.maps.offline;

import android.database.Cursor;
import android.database.SQLException;
import android.database.sqlite.SQLiteDatabase;
import android.util.Log;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import com.google.android.gms.maps.model.LatLng;
import com.google.android.gms.maps.model.LatLngBounds;

import java.io.IOException;
import java.util.HashMap;
import java.util.Map;

import timber.log.Timber;

/**
 * Read tiles from mbtiles file
 *
 * https://github.com/mapbox/mbtiles-spec
 */
@SuppressWarnings("TryFinallyCanBeTryWithResources")
public class MBTilesReader implements IOfflineTileReader
{
    private SQLiteDatabase m_database;
    private int m_minZoom;
    private int m_maxZoom;
    private int m_format;
    private LatLngBounds m_bounds;
    private Map<String, String> m_metadata = new HashMap<>();

    public MBTilesReader(@NonNull String mbTilesFilePath) throws IOException
    {
        try
        {
            m_database = SQLiteDatabase.openDatabase(mbTilesFilePath, null, SQLiteDatabase.OPEN_READONLY);

            readMetadata();

            String format = m_metadata.get("format");
            if (format == null)
                format = "png";

            switch (format)
            {
                case "png":
                    m_format = PNG;
                    break;
                case "jpg":
                case "jpeg":
                    m_format = JPG;
                    break;
                default:
                   throw new IOException("Invalid tile bitmap format " + format);
            }

            // Use the min zoom from metadata if it exists
            Integer minFromMetada = getIntegerFromMetadata("minzoom");
            if (minFromMetada != null) {
                m_minZoom = minFromMetada;
            } else {
                // Not in metadata, get from tiles table
                Integer minFromTable = queryMinZoom();
                if (minFromTable != null)
                    m_minZoom = minFromTable;
                else
                    m_minZoom = 0;
            }
            
            // Get max zoom from metadata
            Integer maxFromMetada = getIntegerFromMetadata("maxzoom");
            if (maxFromMetada != null) {
                m_maxZoom = maxFromMetada;
            } else {
                // Not in metadata, get from tiles table
                Integer maxFromTable = queryMaxZoom();
                if (maxFromTable != null)
                    m_maxZoom = maxFromTable;
                else
                    m_maxZoom = Integer.MAX_VALUE;
            }

        }
        catch (SQLException e) {
            throw new IOException(e);
        }
    }

    private Integer getIntegerFromMetadata(String key)
    {
        String valueString = m_metadata.get(key);
        if (valueString == null)
            return null;
        try
        {
            return Integer.parseInt(valueString);
        } catch (NumberFormatException ignored)
        {
            return null;
        }
    }

    @Override
    public int getFormat()
    {
        return m_format;
    }

    @Override
    public int getTileWidth()
    {
        return 256;
    }

    @Override
    public int getTileHeight()
    {
        return 256;
    }

    @Override
    public @Nullable byte[] getTile(int x, int y, int z)
    {
        if (m_database == null || !m_database.isOpen())
            return null;

        // mbtiles has y coordinate flipped
        y = (1 << z) - y - 1;

        Cursor cursor = null;

        try {
            cursor = m_database.query("tiles",
                new String[]{"tile_data"},
                "tile_column=? and tile_row=? and zoom_level=?",
                new String[]{String.valueOf(x), String.valueOf(y), String.valueOf(z)},
                null,
                null,
                null);
            if (cursor != null && cursor.moveToFirst())
            {
                return cursor.getBlob(0);
            }
        }
        finally {
            if (cursor != null)
                cursor.close();
        }

        return null;
    }

    private @Nullable Integer queryInteger(String query)
    {
        Cursor cursor = null;

        try {
            cursor = m_database.rawQuery(query, null);
            if (cursor != null && cursor.moveToFirst()) {
                return cursor.getInt(0);
            }
        }
        finally
        {
            if (cursor != null)
                cursor.close();
        }

        return null;
    }

    private Integer queryMaxZoom()
    {
        return queryInteger("SELECT MAX(zoom_level), MIN(zoom_level) FROM tiles");
    }

    private Integer queryMinZoom()
    {
        return queryInteger("SELECT MIN(zoom_level), MIN(zoom_level) FROM tiles");
    }

    @Override
    public int getMaxZoom()
    {
        return m_maxZoom;
    }

    @Override
    public int getMinZoom()
    {
        return m_minZoom;
    }

    @Nullable
    @Override
    public LatLngBounds getFullExtent()
    {
        return m_bounds;
    }

    @Nullable
    @Override
    public LatLngBounds getInitialExtent()
    {
        return m_bounds;
    }

    private void readMetadata()
    {
        Cursor cursor = null;

        try {
            cursor = m_database.query("metadata",
                null,
                null,
                null,
                null,
                null,
                null);

            while(cursor != null && cursor.moveToNext()) {
                String name = cursor.getString(0);
                String value = cursor.getString(1);
                m_metadata.put(name.toLowerCase(), value);
            }
        } finally
        {
            if (cursor != null)
                cursor.close();
        }

        if (m_metadata.containsKey("bounds"))
        {
            try
            {
                @SuppressWarnings("ConstantConditions") String[] split = m_metadata.get("bounds").split(",");
                double left = Math.max(Double.parseDouble(split[0]), -180);
                double bottom = Math.max(Double.parseDouble(split[1]), -85);
                double right = Math.min(Double.parseDouble(split[2]), 180);
                double top = Math.min(Double.parseDouble(split[3]), 85);

                m_bounds = new LatLngBounds(new LatLng(bottom, left), new LatLng(top, right));
            } catch (IllegalArgumentException e)
            {
                Timber.e(e, "Error reading extents from mbtiles file");
            }
        }
    }

    @Override
    public void close()
    {
        if (m_database != null) {
            m_database.close();
            m_database = null;
        }
    }
}
