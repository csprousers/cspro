package gov.census.cspro.engine.functions.fragments;

import android.annotation.SuppressLint;
import android.app.AlertDialog;
import android.app.Dialog;
import android.content.Context;
import android.content.DialogInterface;
import android.location.GnssStatus;
import android.location.GpsSatellite;
import android.location.GpsStatus;
import android.location.LocationManager;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.view.LayoutInflater;
import android.view.View;
import android.view.Window;
import android.view.WindowManager;
import android.widget.ProgressBar;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.fragment.app.DialogFragment;
import androidx.lifecycle.MutableLiveData;
import androidx.lifecycle.Observer;

import org.jetbrains.annotations.NotNull;

import gov.census.cspro.csentry.R;
import gov.census.cspro.engine.Messenger;
import gov.census.cspro.engine.Util;
import gov.census.cspro.engine.functions.GPSFunction;
import gov.census.cspro.location.GpsReader;

public class EngineGpsDialogFragment extends DialogFragment
{
    private final static String WAIT_TIME = "WAIT_TIME";
    private final static String ACCURACY = "ACCURACY";
    private final static String DIALOG_TEXT = "DIALOG_TEXT";
    private ProgressBar m_progressBarSatellites;
    private ProgressBar m_progressBarTimeRemaining;
    private int m_waitTime;
    private int m_accuracy;
    private static final int MAX_SATELLITES = 12; // (there can be more than 12 if using satellites other than GPS satellites)
    private static final int POLLING_DURATION = 200; // check the gps status every 200 milliseconds
    private LocationManager m_locationManager;
    private GpsStatus.Listener m_statusListener;
    private GnssStatus.Callback m_gnssListener;
    private MutableLiveData<String> m_gpsResultLiveData = new MutableLiveData<>();
    private int m_numSatellites;

    public static EngineGpsDialogFragment newInstance(int waitTime, int desiredAccuracy, String dialogText)
    {
        EngineGpsDialogFragment frag = new EngineGpsDialogFragment();
        Bundle args = new Bundle();
        args.putInt(WAIT_TIME, waitTime);
        args.putInt(ACCURACY, desiredAccuracy);
        args.putString(DIALOG_TEXT, dialogText);
        frag.setArguments(args);
        return frag;
    }

    @Override
    public @NonNull Dialog onCreateDialog(Bundle savedInstanceState)
    {
        if (savedInstanceState != null) {
            m_waitTime = savedInstanceState.getInt(WAIT_TIME);
        } else {
            //noinspection ConstantConditions
            m_waitTime = getArguments().getInt(WAIT_TIME);
        }

        //noinspection ConstantConditions
        m_accuracy = getArguments().getInt(ACCURACY);

        //noinspection ConstantConditions
        LayoutInflater inflater = getActivity().getLayoutInflater();
        @SuppressLint("InflateParams") View view = inflater.inflate(R.layout.dialog_gps,null);
        final TextView messageText = view.findViewById(R.id.dialog_gps_textViewWaitMessage);

        String dialogText = getArguments().getString(DIALOG_TEXT);
        if (!Util.stringIsNullOrEmpty(dialogText))
            messageText.setText(dialogText);

        m_progressBarSatellites = view.findViewById(R.id.dialog_gps_progressBarSatellites);
        m_progressBarTimeRemaining = view.findViewById(R.id.dialog_gps_progressBarTimeRemaining);

        AlertDialog.Builder builder = new AlertDialog.Builder(getContext());

        builder	.setTitle(getString(R.string.engine_gps_dialog_title))
                .setView(view);

        final AlertDialog gpsDialog = builder.create();
        gpsDialog.setCanceledOnTouchOutside(false);

        int waitTime = getArguments().getInt(WAIT_TIME);

        m_progressBarSatellites.setMax(MAX_SATELLITES);
        m_progressBarTimeRemaining.setMax(waitTime);

        //noinspection ConstantConditions
        m_locationManager = (LocationManager) getContext().getSystemService(Context.LOCATION_SERVICE);
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.N)
        {
            m_gnssListener = new GnssStatus.Callback()
            {
                @Override
                public void onSatelliteStatusChanged(GnssStatus status)
                {
                    int numberSatellites = 0;
                    for (int i = 0; i < status.getSatelliteCount(); ++i) {
                        if (status.usedInFix(i))
                            ++numberSatellites;
                    }
                    m_numSatellites = numberSatellites;
                    super.onSatelliteStatusChanged(status);
                }
            };
            try
            {
                m_locationManager.registerGnssStatusCallback(m_gnssListener);
            } catch (SecurityException ignored)
            {}
        } else
        {
            m_statusListener = new GpsStatus.Listener()
            {

                @Override
                public void onGpsStatusChanged(int event)
                {
                    try
                    {
                        GpsStatus gpsStatus = m_locationManager.getGpsStatus(null);
                        int numberSatellites = 0;

                        for( GpsSatellite satellite : gpsStatus.getSatellites() )
                        {
                            if (satellite.usedInFix())
                                numberSatellites++;
                        }

                        m_numSatellites = numberSatellites;
                    } catch (SecurityException ignored)
                    {
                    }
                }
            };

            try
            {
                m_locationManager.addGpsStatusListener(m_statusListener);
            } catch (SecurityException ignored)
            {
            }
        }

        m_progressBarSatellites.setProgress(0);
        m_progressBarTimeRemaining.setProgress(waitTime);

        // Use LiveData to signal endFunction from the gps
        // polling to avoid crash when polling code receives a reading
        // and dismisses dialog when Fragment is in background
        m_gpsResultLiveData.observe(this, new Observer<String>() {
            @Override
            public void onChanged(String s) {
                if (s != null)
                    endFunction(s);
            }
        });

        gpsHandler = new Handler();
        gpsRunnable.run();

        gpsDialog.setOnShowListener(new DialogInterface.OnShowListener()
        {
            @Override
            public void onShow(DialogInterface dialogInterface)
            {
                Window w = gpsDialog.getWindow();
                if (w != null)
                    w.addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
            }
        });

        return gpsDialog;
    }

    @Override
    public void onCancel(DialogInterface dlg)
    {
        super.onCancel(dlg);
        endFunction("cancel");
    }

    @Override
    public void onSaveInstanceState(@NotNull Bundle outState)
    {
        super.onSaveInstanceState(outState);
        outState.putInt(WAIT_TIME, m_waitTime);
    }

    private Handler gpsHandler;

    private Runnable gpsRunnable = new Runnable()
    {
        @Override
        public void run()
        {

            GpsReader reader = GPSFunction.getReader();

            if (m_waitTime < 0) // time out
            {
                // if they got a reading, even if it was not at the accuracy level, and then timed out, still return the value
                if (reader.hasNewGPSReading(0))
                    m_gpsResultLiveData.postValue(reader.readMostAccurateGPS());
                else
                    m_gpsResultLiveData.postValue("");
            } else if (reader.hasNewGPSReading(m_accuracy))
            { // a successful reading has been made
                m_gpsResultLiveData.postValue(reader.readMostAccurateGPS());
            } else
            {
                m_progressBarSatellites.setProgress(Math.min(MAX_SATELLITES, m_numSatellites));
                m_progressBarTimeRemaining.setProgress(m_waitTime);
                m_waitTime -= POLLING_DURATION;
                gpsHandler.postDelayed(gpsRunnable, POLLING_DURATION);
            }
        }
    };

    private void endFunction(String retVal)
    {
        if( gpsHandler != null )
            gpsHandler.removeCallbacks(gpsRunnable);

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.N)
        {
            if (m_locationManager != null && m_gnssListener != null)
            {
                m_locationManager.unregisterGnssStatusCallback(m_gnssListener);
                m_gnssListener = null;
            }
        } else {
            if (m_locationManager != null && m_statusListener != null) {
                m_locationManager.removeGpsStatusListener(m_statusListener);
                m_statusListener = null;
            }
        }

        dismiss(); // close dialog

        Messenger.getInstance().engineFunctionComplete(retVal);
    }
}
