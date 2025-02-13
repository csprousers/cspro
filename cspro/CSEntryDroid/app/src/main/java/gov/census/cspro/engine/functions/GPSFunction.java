package gov.census.cspro.engine.functions;

import android.app.Activity;
import android.content.Intent;

import androidx.fragment.app.DialogFragment;
import androidx.appcompat.app.AppCompatActivity;


import gov.census.cspro.engine.BaseMapSelection;
import gov.census.cspro.engine.Messenger;
import gov.census.cspro.engine.functions.fragments.EngineGpsDialogFragment;
import gov.census.cspro.location.GpsReader;
import gov.census.cspro.location.LocationActivity;

public class GPSFunction implements EngineFunction {

	public static final int GPS_OPEN = 1;
	public static final int GPS_CLOSE = 2;
	public static final int GPS_READ = 3;
	public static final int GPS_READLAST = 4;
	public static final int GPS_READINTERACTIVE = 5;
	public static final int GPS_SELECT = 6;

	public static final int DIALOG_TYPE_INTERACTIVE = 0;
    public static final int DIALOG_TYPE_SELECT = 1;

	private static final String stringTrue = "1";
	private static final String stringFalse = "0";

	private int m_command;
	private int m_waitTime;
	private int m_desiredAccuracy;
	private String m_dialogText;
    private BaseMapSelection m_baseMapSelection;

	static private GpsReader m_gpsReader = new GpsReader();

	public GPSFunction(int command, int waitTime, int desiredAccuracy, String dialogText, BaseMapSelection baseMapSelection)
	{
		m_command = command;
		m_waitTime = waitTime;
		m_desiredAccuracy = desiredAccuracy;
		m_dialogText = dialogText;
        m_baseMapSelection = baseMapSelection;
    }

	public static GpsReader getReader()
	{
		return m_gpsReader;
	}

	public void runEngineFunction(Activity activity) {
        switch (m_command) {
            case GPS_OPEN:
                if (m_gpsReader.isRunning()) {
                    // Already running
                    endFunction(stringTrue);
                } else {
                    // Attempt to start
                    m_gpsReader.start(activity, new GpsReader.EnableListener() {
                        @Override
                        public void onSuccess() {
                            endFunction(stringTrue);
                        }

                        @Override
                        public void onFailure() {
                            endFunction(stringFalse);
                        }
                    });
                }
                break;

            case GPS_CLOSE:
                close();
                endFunction(stringTrue);
                break;

            case GPS_READLAST:
                endFunction(m_gpsReader.readLast());
                break;

            case GPS_READ:
				if (!m_gpsReader.isRunning()) {
					endFunction("");
				} else {
					DialogFragment dlgFragment = EngineGpsDialogFragment.newInstance(m_waitTime, m_desiredAccuracy, m_dialogText);
					dlgFragment.show(((AppCompatActivity) activity).getSupportFragmentManager(), "ModalDialogFragment");
				}
                break;

            case GPS_READINTERACTIVE:
                showInteractiveLocationDialog(activity, DIALOG_TYPE_INTERACTIVE);
                break;

            case GPS_SELECT:
                showInteractiveLocationDialog(activity, DIALOG_TYPE_SELECT);
                break;
        }
    }

    private void showInteractiveLocationDialog(Activity activity, int dialogType) {
        Intent intentSelect = new Intent(activity, LocationActivity.class);
        intentSelect.putExtra("WAIT_TIME", m_waitTime);
        intentSelect.putExtra("ACCURACY", m_desiredAccuracy);
        intentSelect.putExtra("DIALOG_TEXT", m_dialogText);
        intentSelect.putExtra("DIALOG_TYPE", dialogType);
        intentSelect.putExtra("BASE_MAP_TYPE", m_baseMapSelection.getType());
        intentSelect.putExtra("BASE_MAP_FILENAME", m_baseMapSelection.getFilename());
        activity.startActivity(intentSelect);
    }

	public static void close()
	{
		m_gpsReader.stop();
	}

	private void endFunction(String retVal)
	{
		Messenger.getInstance().engineFunctionComplete(retVal);
	}
}
