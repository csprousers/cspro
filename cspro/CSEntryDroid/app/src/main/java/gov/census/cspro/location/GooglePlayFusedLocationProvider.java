package gov.census.cspro.location;

import android.annotation.SuppressLint;
import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;
import android.os.Looper;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import com.google.android.gms.location.FusedLocationProviderClient;
import com.google.android.gms.location.LocationCallback;
import com.google.android.gms.location.LocationRequest;
import com.google.android.gms.location.LocationResult;
import com.google.android.gms.location.LocationServices;
import com.google.android.gms.tasks.Task;

public class GooglePlayFusedLocationProvider implements ILocationProvider
{
    private FusedLocationProviderClient m_fusedLocationClient;

    GooglePlayFusedLocationProvider(Context context)
    {
        m_fusedLocationClient = LocationServices.getFusedLocationProviderClient(context);
    }

    @SuppressLint("MissingPermission")
    @Override
    public Task<Void> requestLocationUpdates(@NonNull LocationRequest request, @NonNull LocationCallback callback, @Nullable Looper looper)
    {
        return m_fusedLocationClient.requestLocationUpdates(request, callback, looper);
    }

    @SuppressLint("MissingPermission")
    @Override
    public Task<Void> requestLocationUpdates(@NonNull LocationRequest request, @NonNull PendingIntent intent)
    {
        return m_fusedLocationClient.requestLocationUpdates(request, intent);
    }

    @Override
    public Task<Void> removeLocationUpdates(@NonNull LocationCallback callback)
    {
        return m_fusedLocationClient.removeLocationUpdates(callback);
    }

    @Override
    public Task<Void> removeLocationUpdates(@NonNull PendingIntent intent)
    {
        return m_fusedLocationClient.removeLocationUpdates(intent);
    }

    @Override
    public Task<Void> flushLocations()
    {
        return m_fusedLocationClient.flushLocations();
    }

    @Override
    public LocationResult extractResultFromIntent(Intent intent)
    {
        return LocationResult.extractResult(intent);
    }
}
