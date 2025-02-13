package gov.census.cspro.maps.offline;

import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Matrix;
import androidx.annotation.NonNull;

import com.google.android.gms.maps.model.Tile;
import com.google.android.gms.maps.model.TileProvider;

import java.io.ByteArrayOutputStream;
import java.io.Closeable;
import java.io.IOException;

/**
 * Custom tile provider for Google Maps that loads tiles
 * from offline tiles file via IOfflineTileReader.
 */
public class OfflineTileProvider implements TileProvider, Closeable
{
    private final IOfflineTileReader m_reader;

    public OfflineTileProvider(@NonNull IOfflineTileReader reader)
    {
        m_reader = reader;
    }

    @Override
    public @NonNull Tile getTile(int x, int y, int z)
    {
        byte[] tileImage = getTileImage(x, y, z);
        if (tileImage == null)
            return NO_TILE;
        else
            return new Tile(m_reader.getTileWidth(), m_reader.getTileHeight(), tileImage);
    }

    private byte[] getTileImage(int x, int y, int z)
    {
        if (z <= m_reader.getMaxZoom())
        {
            return m_reader.getTile(x, y, z);
        } else {
            // Beyond max zoom try to find a tile at max zoom and scale it up
            final int srcZ = m_reader.getMaxZoom();
            final int zoomShift = 1 << (z - srcZ);
            final int srcX = x/zoomShift;
            final int srcY = y/zoomShift;
            byte[] srcData = m_reader.getTile(srcX, srcY, srcZ);
            if (srcData != null)
            {
                return createTileByCroppingHigherLevelTile(srcData, srcX, srcY, srcZ, x, y, z);
            } else {
                return null;
            }
        }
    }

    private byte[] createTileByCroppingHigherLevelTile(byte[] srcData, int srcX, int srcY, int srcZ, int dstX, int dstY, int dstZ)
    {
        Bitmap bitmap = BitmapFactory.decodeByteArray(srcData, 0, srcData.length);

        // Scale between src and dest (2 if one level different, 4 for two levels...) in general 2^(dstZ-srcZ)
        int scale = 1 << (dstZ - srcZ);

        // Find size in px of subregion of source tile to use as destination tile - don't let it be less than 1px
        int dstWidthPx = Math.max(bitmap.getWidth()/scale, 1);
        int dstHeightPx = Math.max(bitmap.getHeight()/scale, 1);

        // Find corner of subregion in src to use as dest - y is flipped since bitmap y coords start at bottom
        float dstStartXPx = (dstX - srcX * scale)/((float) scale);
        float dstStartYPx = (dstY - srcY * scale)/((float) scale);

        // Scaled the cropped portion of the bitmap back to original tile size
        Matrix matrix = new Matrix();
        matrix.postScale((float) bitmap.getWidth()/dstWidthPx, (float) bitmap.getHeight()/dstHeightPx);

        bitmap = Bitmap.createBitmap(
            bitmap,
            (int) (dstStartXPx * bitmap.getWidth()),
            (int) (dstStartYPx * bitmap.getHeight()),
            dstWidthPx,
            dstHeightPx,
            matrix, true
        );

        final ByteArrayOutputStream stream = new ByteArrayOutputStream();
        bitmap.compress(getBitmapFormat(), 100, stream);
        bitmap.recycle();

        return stream.toByteArray();
    }

    private Bitmap.CompressFormat getBitmapFormat()
    {
        if (m_reader.getFormat() == IOfflineTileReader.JPG)
            return Bitmap.CompressFormat.JPEG;
        else
            return Bitmap.CompressFormat.PNG;
    }

    @Override
    public void close()
    {
        try
        {
            m_reader.close();
        } catch (IOException ignore)
        {
        }
    }
}
