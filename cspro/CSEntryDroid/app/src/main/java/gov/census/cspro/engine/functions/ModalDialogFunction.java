package gov.census.cspro.engine.functions;

import android.app.Activity;
import androidx.fragment.app.DialogFragment;
import androidx.appcompat.app.AppCompatActivity;

import gov.census.cspro.engine.functions.fragments.EngineModalDialogFragment;

public class ModalDialogFunction implements EngineFunction {

	private final String m_title;
	private final String m_message;
	private final int m_type;

	// These match the Windows C++ message box constants
	public static final int	MB_OK =  0;
	@SuppressWarnings("unused")
	public static final int	MB_OKCANCEL =  1;
	public static final int	MB_YESNO = 4;

	public ModalDialogFunction(String title, String message, int type) {
		m_title = title;
		m_message = message;
		m_type = type;
	}

	@Override
	public void runEngineFunction(Activity activity) {

        DialogFragment dlgFragment = EngineModalDialogFragment.newInstance(m_title, m_message, m_type);
        try {
            dlgFragment.show(((AppCompatActivity) activity).getSupportFragmentManager(), "ModalDialogFragment");
        } catch (IllegalStateException e) {
            //do nothing as this is trying to show dialog when state has changed.
        }
    }
}
