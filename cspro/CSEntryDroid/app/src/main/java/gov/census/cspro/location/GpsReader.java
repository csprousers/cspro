package gov.census.cspro.location;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.location.Location;
import android.location.LocationManager;
import android.os.Looper;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.app.AlertDialog;

import com.google.android.gms.location.LocationCallback;
import com.google.android.gms.location.LocationRequest;
import com.google.android.gms.location.LocationResult;

import gov.census.cspro.csentry.R;
import gov.census.cspro.csentry.ui.EntryActivity;
import timber.log.Timber;

public class GpsReader
{
    private ILocationProvider m_locationClient;
    private LocationCallback m_locationCallback;

    private static final long UPDATE_INTERVAL_IN_MILLISECONDS = 200;
    private static final long FASTEST_UPDATE_INTERVAL_IN_MILLISECONDS = 200;

    private Location m_lastLocation;
    private Location m_mostAccurateLocation;
    private boolean m_hasReceivedValue;
    private boolean m_hasReceivedNewValue;
    private EnableListener m_enableListener;

    public boolean isRunning()
    {
        return m_locationClient != null;
    }

    public String readLast()
    {
        return LocationUtils.locationString(m_lastLocation);
    }


    @SuppressLint("MissingPermission")
    public void start(@NonNull Activity activity, @NonNull EnableListener enableListener)
    {
        m_hasReceivedValue = false;
        m_hasReceivedNewValue = false;
        m_mostAccurateLocation = null;

        LocationManager locationManager = (LocationManager) activity.getSystemService(Context.LOCATION_SERVICE);
        if (locationManager == null)
        {
            enableListener.onFailure();
            return;
        }

        if (!locationManager.isProviderEnabled(LocationManager.GPS_PROVIDER))
        {
            askToEnableGpsInSettings(activity, enableListener);
        } else
        {
            startCallback(activity, enableListener);
        }
    }

    public void onSettingsResult(Activity activity)
    {
        LocationManager locationManager = (LocationManager) activity.getSystemService(Context.LOCATION_SERVICE);
        if (locationManager == null || !locationManager.isProviderEnabled(LocationManager.GPS_PROVIDER))
        {
            if (m_enableListener != null)
                m_enableListener.onFailure();
        } else
        {
            startCallback(activity, m_enableListener);
        }
        m_enableListener = null;
    }

    private void startCallback(@NonNull Context context, @Nullable EnableListener enableListener)
    {
        Timber.d("start receiving location updates");

        final LocationRequest locationRequest = new LocationRequest();
        locationRequest.setInterval(UPDATE_INTERVAL_IN_MILLISECONDS);
        locationRequest.setFastestInterval(FASTEST_UPDATE_INTERVAL_IN_MILLISECONDS);
        locationRequest.setPriority(LocationRequest.PRIORITY_HIGH_ACCURACY);

        m_locationCallback = new LocationCallback()
        {
            @Override
            public void onLocationResult(LocationResult locationResult)
            {
                super.onLocationResult(locationResult);
                Timber.d("Got new location result with %d results ",locationResult.getLocations().size());
                Timber.d("Last location %s", locationResult.getLastLocation().toString());

                m_hasReceivedValue = true;
                m_hasReceivedNewValue = true;
                m_lastLocation = locationResult.getLastLocation();

                if (isMoreAccurate(m_lastLocation, m_mostAccurateLocation)) {
                    m_mostAccurateLocation = m_lastLocation;
                    Timber.d("Most accurate = %s", m_mostAccurateLocation.toString());
                }
            }
        };

        m_locationClient = LocationProviderFactory.getLocationProvider(context);
        m_locationClient.requestLocationUpdates(locationRequest, m_locationCallback, Looper.myLooper());

        if (enableListener != null)
            enableListener.onSuccess();
    }

    public boolean hasNewGPSReading(int desiredAccuracy)
    {
        if (desiredAccuracy == 0)
            return m_hasReceivedNewValue;
        else
            return m_hasReceivedNewValue && m_mostAccurateLocation.hasAccuracy() && m_mostAccurateLocation.getAccuracy() <= desiredAccuracy;
    }

    public String readMostAccurateGPS()
    {
        if (!m_hasReceivedValue)
            return "";

        return LocationUtils.locationString(m_mostAccurateLocation);
    }

    public interface EnableListener
    {
        void onSuccess();

        void onFailure();
    }

    public void stop()
    {
        Timber.d("stop receiving location updates");
        if (m_locationClient != null)
        {
            m_locationClient.removeLocationUpdates(m_locationCallback);
            m_locationClient = null;
        }
    }

    private boolean isMoreAccurate(@Nullable Location a, @Nullable Location b)
    {
        if (a == null)
            return false;
        if (b == null)
            return true;
        if (!a.hasAccuracy())
            return false;
        if (!b.hasAccuracy())
            return true;
        return a.getAccuracy() < b.getAccuracy();
    }

    private void askToEnableGpsInSettings(final @NonNull Activity activity, final @Nullable EnableListener enableListener)
    {
        final AlertDialog.Builder builder = new AlertDialog.Builder(activity);
        builder.setMessage(R.string.ask_enable_gps)
               .setCancelable(false)
               .setPositiveButton(R.string.modal_dialog_helper_yes_text, new DialogInterface.OnClickListener() {
                   public void onClick(final DialogInterface dialog, final int id) {
                       m_enableListener = enableListener;
                       activity.startActivityForResult(new Intent(android.provider.Settings.ACTION_LOCATION_SOURCE_SETTINGS), EntryActivity.EnableLocationInSettings );
                   }
               })
               .setNegativeButton(R.string.modal_dialog_helper_no_text, new DialogInterface.OnClickListener() {
                   public void onClick(final DialogInterface dialog, final int id) {
                       dialog.cancel();
                       if (enableListener != null)
                           enableListener.onFailure();
                   }
               });
        final AlertDialog alert = builder.create();
        alert.show();
    }

}
