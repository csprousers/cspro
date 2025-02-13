package gov.census.cspro.engine.functions;

import gov.census.cspro.engine.functions.fragments.EngineErrmsgDialogFragment;
import android.app.Activity;
import androidx.fragment.app.DialogFragment;
import androidx.appcompat.app.AppCompatActivity;

// GHM 20140109

public class ErrmsgFunction implements EngineFunction {

	private String m_title;
	private String m_message;
	private String[] m_buttons;


	public ErrmsgFunction(String title,String message,String[] buttons)
	{
		m_title = title;
		m_message = message;
		m_buttons = buttons;
	}

	public void runEngineFunction(Activity activity)
	{
		DialogFragment dlgFragment = EngineErrmsgDialogFragment.newInstance(m_title, m_message, m_buttons);
		dlgFragment.show(((AppCompatActivity) activity).getSupportFragmentManager(), "ModalDialogFragment");
	}

}
