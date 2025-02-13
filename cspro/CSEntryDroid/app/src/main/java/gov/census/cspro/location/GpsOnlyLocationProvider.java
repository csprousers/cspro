package gov.census.cspro.location;

import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;
import android.location.Location;
import android.location.LocationListener;
import android.location.LocationManager;
import android.os.Bundle;
import android.os.Looper;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import com.google.android.gms.location.LocationCallback;
import com.google.android.gms.location.LocationRequest;
import com.google.android.gms.location.LocationResult;
import com.google.android.gms.tasks.Task;
import com.google.android.gms.tasks.TaskCompletionSource;

import java.util.Collections;

public class GpsOnlyLocationProvider implements ILocationProvider
{
    private LocationManager m_locationManager;
    private LocationListener m_locationListener;

    GpsOnlyLocationProvider(@NonNull Context context)
    {
        m_locationManager = (LocationManager) context.getSystemService(Context.LOCATION_SERVICE);
    }

    @Override
    public Task<Void> requestLocationUpdates(@NonNull LocationRequest request, @NonNull final LocationCallback callback, @Nullable Looper looper)
    {
        TaskCompletionSource<Void> taskCompletionSource = new TaskCompletionSource<>();

        m_locationListener = new LocationListener()
        {
            @Override
            public void onLocationChanged(Location location)
            {
                callback.onLocationResult(LocationResult.create(Collections.singletonList(location)));
            }

            @Override
            public void onStatusChanged(String s, int i, Bundle bundle)
            {

            }

            @Override
            public void onProviderEnabled(String s)
            {

            }

            @Override
            public void onProviderDisabled(String s)
            {

            }
        };
        try
        {
            m_locationManager.requestLocationUpdates(LocationManager.GPS_PROVIDER, request.getInterval(), request.getSmallestDisplacement(), m_locationListener, looper);

            taskCompletionSource.setResult(null);

        } catch (SecurityException ex)
        {
            taskCompletionSource.setException(ex);
        }

        return taskCompletionSource.getTask();
    }

    @Override
    public Task<Void> requestLocationUpdates(@NonNull LocationRequest request, @NonNull PendingIntent intent)
    {
        TaskCompletionSource<Void> taskCompletionSource = new TaskCompletionSource<>();
        try
        {
            m_locationManager.requestLocationUpdates(LocationManager.GPS_PROVIDER, request.getInterval(), request.getSmallestDisplacement(), intent);
            taskCompletionSource.setResult(null);
        } catch (SecurityException ex)
        {
            taskCompletionSource.setException(ex);
        }
        return taskCompletionSource.getTask();
    }

    @Override
    public Task<Void> removeLocationUpdates(@NonNull LocationCallback callback)
    {
        if (m_locationListener != null)
        {
            m_locationManager.removeUpdates(m_locationListener);
            m_locationListener = null;
        }

        TaskCompletionSource<Void> taskCompletionSource = new TaskCompletionSource<>();
        taskCompletionSource.setResult(null);
        return taskCompletionSource.getTask();
    }

    @Override
    public Task<Void> removeLocationUpdates(@NonNull PendingIntent intent)
    {
        m_locationManager.removeUpdates(intent);
        TaskCompletionSource<Void> taskCompletionSource = new TaskCompletionSource<>();
        taskCompletionSource.setResult(null);
        return taskCompletionSource.getTask();
    }

    @Override
    public Task<Void> flushLocations()
    {
        TaskCompletionSource<Void> taskCompletionSource = new TaskCompletionSource<>();
        taskCompletionSource.setResult(null);
        return taskCompletionSource.getTask();
    }

    @Override
    public LocationResult extractResultFromIntent(Intent intent)
    {
        String locationKey = LocationManager.KEY_LOCATION_CHANGED;
        if (intent.hasExtra(locationKey))
            return LocationResult.create(Collections.singletonList((Location) intent.getExtras().get(locationKey)));
        else
            return null;
    }
}
