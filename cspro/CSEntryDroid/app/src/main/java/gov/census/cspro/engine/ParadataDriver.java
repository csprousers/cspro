package gov.census.cspro.engine;

import android.app.PendingIntent;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.location.Location;

import androidx.annotation.NonNull;

import com.google.android.gms.location.LocationRequest;
import com.google.android.gms.location.LocationResult;
import com.google.android.gms.tasks.OnFailureListener;
import com.google.android.gms.tasks.OnSuccessListener;

import java.util.ArrayList;

import gov.census.cspro.csentry.CSEntry;
import gov.census.cspro.location.ILocationProvider;
import gov.census.cspro.location.LocationProviderFactory;
import gov.census.cspro.location.LocationUtils;
import timber.log.Timber;

public class ParadataDriver
{
	private ArrayList<String> m_cachedEvents = new ArrayList<>();
	private ILocationProvider m_locationClient;
	private PendingIntent m_locationUpdatePendingIntent;

	class ParadataDriverMessage extends EngineMessage
	{
		@Override
		public void run()
		{
			EngineInterface.getInstance().getParadataCachedEvents();
		}
	}

	private void cacheEvent(String eventType, String eventContents)
	{
		boolean firstCachedEvent = m_cachedEvents.isEmpty();

		// add the timestamp and then store the string
		String eventString = String.format(CSEntry.CS_LOCALE, "%s;%.3f;%s", eventType, System.currentTimeMillis() / 1000.0, eventContents);
		m_cachedEvents.add(eventString);

		// post a message for the JNI layer to get these cached events
		if (firstCachedEvent)
			Messenger.getInstance().postMessage(new ParadataDriverMessage());
	}

	Object getCachedEvents()
	{
		Object currentCachedEvents = m_cachedEvents;
		m_cachedEvents = new ArrayList<>();
		return currentCachedEvents;
	}

	void startGpsLocationRequest(int minutesBetweenReadings, Context context)
	{
		try
		{
			Timber.d("gps_background_open");

			LocationRequest locationRequest = new LocationRequest();
			final long updateInterval = minutesBetweenReadings * 60 * 1000;
			locationRequest.setInterval(updateInterval);
			locationRequest.setFastestInterval(updateInterval);
			locationRequest.setMaxWaitTime(10 * 60 * 1000); // batch updates every 10 minutes to save battery
			locationRequest.setPriority(LocationRequest.PRIORITY_HIGH_ACCURACY);

			m_locationUpdatePendingIntent = getLocationUpdatePendingIntent(context);
			m_locationClient = LocationProviderFactory.getLocationProvider(context);
			m_locationClient.requestLocationUpdates(locationRequest, m_locationUpdatePendingIntent).addOnSuccessListener(new OnSuccessListener<Void>()
			{
				@Override
				public void onSuccess(Void aVoid)
				{
					cacheEvent("gps_background_open", "1");
				}
			}).addOnFailureListener(new OnFailureListener()
			{
				@Override
				public void onFailure(@NonNull Exception e)
				{
					cacheEvent("gps_background_open", "0");
					Timber.e(e, "Start background GPS failed");
				}
			});

			cacheEvent("gps_background_open", "1");
		} catch (SecurityException e) {
			cacheEvent("gps_background_open", "0");
			Timber.e(e, "Start background GPS failed");
		}
	}

	private PendingIntent getLocationUpdatePendingIntent(Context context)
	{
		Intent intent = new Intent(context, LocationUpdatesBroadcastReceiver.class);
		intent.setAction(LocationUpdatesBroadcastReceiver.ACTION_PROCESS_UPDATES);
		return PendingIntent.getBroadcast(context, 0, intent, PendingIntent.FLAG_UPDATE_CURRENT | PendingIntent.FLAG_IMMUTABLE);
	}

	public void stopGpsLocationUpdates()
	{
		if (m_locationClient != null)
		{
			m_locationClient.flushLocations();
			m_locationClient.removeLocationUpdates(m_locationUpdatePendingIntent);
			m_locationUpdatePendingIntent = null;
			m_locationClient = null;
            Timber.d("gps_background_close");
            cacheEvent("gps_background_close","");
		}
	}

	public static class LocationUpdatesBroadcastReceiver extends BroadcastReceiver
	{
		static final String ACTION_PROCESS_UPDATES =
			"gov.census.cspro.engine.ParadataDriver.LocationUpdatesBroadcastReceiver.action.PROCESS_UPDATES";

		public LocationUpdatesBroadcastReceiver()
		{
		}

		@Override
		public void onReceive(Context context, Intent intent) {
			if (intent != null) {
				final String action = intent.getAction();
				if (ACTION_PROCESS_UPDATES.equals(action)) {
					LocationResult result = LocationProviderFactory.getLocationProvider(context).extractResultFromIntent(intent);
					if (result != null) {
						ParadataDriver paradataDriver = EngineInterface.getInstance().getParadataDriver();
						if (paradataDriver != null)
						{
							for (Location loc : result.getLocations())
                            {
								Timber.d("gps_background_read");
                                paradataDriver.cacheEvent("gps_background_read", LocationUtils.locationString(loc));
                            }
						}
					}
				}
			}
		}
	}
}
