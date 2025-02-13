package gov.census.cspro.maps.offline;

import androidx.annotation.Nullable;

import com.google.android.gms.maps.model.LatLngBounds;

import java.io.Closeable;

public interface IOfflineTileReader extends Closeable
{
    int PNG = 1;
    int JPG = 2;

    int getFormat();

    int getTileWidth();

    int getTileHeight();
    
    int getMaxZoom();

    int getMinZoom();

    @Nullable
    LatLngBounds getFullExtent();

    @Nullable
    LatLngBounds getInitialExtent();

    @Nullable byte[] getTile(int x, int y, int z);
}
