package gov.census.cspro.location;

import android.content.Context;
import androidx.annotation.NonNull;

import com.google.android.gms.common.ConnectionResult;
import com.google.android.gms.common.GoogleApiAvailability;

public class LocationProviderFactory
{
    @NonNull
    public static ILocationProvider getLocationProvider(@NonNull Context context)
    {
        if (isPlayServicesAvailable(context)) {
            return new GooglePlayFusedLocationProvider(context);
        } else {
            return new GpsOnlyLocationProvider(context);
        }
    }

    private static boolean isPlayServicesAvailable(Context context)
    {
        GoogleApiAvailability apiAvailability = GoogleApiAvailability.getInstance();
        int resultCode = apiAvailability.isGooglePlayServicesAvailable(context);
        return resultCode == ConnectionResult.SUCCESS;
    }
}
