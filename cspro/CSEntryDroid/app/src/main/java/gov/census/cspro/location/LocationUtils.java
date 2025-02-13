package gov.census.cspro.location;

import android.location.Location;
import android.os.Bundle;

import java.text.SimpleDateFormat;
import java.util.Date;

import gov.census.cspro.csentry.CSEntry;

public class LocationUtils
{
    public static String locationString(Location location)
    {
        if (location == null)
            return "";

        double latitude = location.getLatitude();
        double longitude = location.getLongitude();
        double altitude = location.hasAltitude() ? location.getAltitude() : -1;

        Bundle extras = location.getExtras();
        final String satellitesKey = "satellites";
        int satellites = ( ( extras != null ) && extras.containsKey(satellitesKey) ) ? extras.getInt(satellitesKey) : 0;

        float accuracy = location.hasAccuracy() ? location.getAccuracy() : -1;

        return locationString(latitude, longitude, altitude, satellites, accuracy,
            new Date(location.getTime()));
    }

    public static String locationString(double latitude, double longitude,
                                        double altitude, int satellites,
                                        float accuracy, Date readTime)
    {
        // the date will be converted from UTC to local time
        return String.format(CSEntry.CS_LOCALE, "%.9f;%.9f;%.9f;%d;%.9f;%s",
            latitude, longitude, altitude, satellites, accuracy,
            new SimpleDateFormat("HHmmss", CSEntry.CS_LOCALE).format(readTime));
    }
}
