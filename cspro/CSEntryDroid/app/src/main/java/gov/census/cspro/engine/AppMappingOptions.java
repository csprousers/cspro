package gov.census.cspro.engine;

import android.text.TextUtils;

public class AppMappingOptions
{
    @SuppressWarnings("WeakerAccess")
    public final String latitudeItem;
    @SuppressWarnings("WeakerAccess")
    public final String longitudeItem;

    AppMappingOptions(String latitudeItem, String longitudeItem)
    {
        this.latitudeItem = latitudeItem;
        this.longitudeItem = longitudeItem;
    }

    public boolean isMappingEnabled()
    {
        return !TextUtils.isEmpty(longitudeItem) && !TextUtils.isEmpty(latitudeItem);
    }
}
