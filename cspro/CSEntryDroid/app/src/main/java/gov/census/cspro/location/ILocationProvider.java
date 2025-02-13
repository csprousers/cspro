package gov.census.cspro.location;

import android.app.PendingIntent;
import android.content.Intent;
import android.os.Looper;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import com.google.android.gms.location.LocationCallback;
import com.google.android.gms.location.LocationRequest;
import com.google.android.gms.location.LocationResult;
import com.google.android.gms.tasks.Task;

/**
 * Interface for generic location provider that could
 * be implemented with Play Services FusedLocationProvider
 * or an implementation that doesn't require Play Services.
 */
public interface ILocationProvider
{
    Task<Void> requestLocationUpdates(@NonNull LocationRequest request, @NonNull LocationCallback callback, @Nullable Looper looper);

    Task<Void> requestLocationUpdates(@NonNull LocationRequest request, @NonNull PendingIntent intent);

    Task<Void> removeLocationUpdates(@NonNull LocationCallback callback);

    Task<Void> removeLocationUpdates(@NonNull PendingIntent intent);

    Task<Void> flushLocations();

    LocationResult extractResultFromIntent(Intent intent);
}
