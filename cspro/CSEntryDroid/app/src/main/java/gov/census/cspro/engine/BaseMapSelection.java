package gov.census.cspro.engine;

import com.google.android.gms.maps.GoogleMap;

import gov.census.cspro.maps.MapData;

public class BaseMapSelection
{
    private final int mType;
    private final String mFilename;

    public BaseMapSelection(int type, String filename)
    {
        assert ( type == MapData.BASE_MAP_CUSTOM ) == ( filename != null );

        mType = type;
        mFilename = filename;
    }

    public BaseMapSelection(int type)
    {
        this(type, null);
    }

    public int getType()
    {
        return mType;
    }

    public String getFilename()
    {
        return mFilename;
    }

    public int getGoogleMapType()
    {
        switch (mType)
        {
            case MapData.BASE_MAP_NORMAL:
                return GoogleMap.MAP_TYPE_NORMAL;

            case MapData.BASE_MAP_HYBRID:
                return GoogleMap.MAP_TYPE_HYBRID;

            case MapData.BASE_MAP_SATELLITE:
                return GoogleMap.MAP_TYPE_SATELLITE;

            case MapData.BASE_MAP_TERRAIN:
                return GoogleMap.MAP_TYPE_TERRAIN;

            case MapData.BASE_MAP_NONE:
            case MapData.BASE_MAP_CUSTOM:
                return GoogleMap.MAP_TYPE_NONE;
        }

        return GoogleMap.MAP_TYPE_NORMAL;
    }
}
