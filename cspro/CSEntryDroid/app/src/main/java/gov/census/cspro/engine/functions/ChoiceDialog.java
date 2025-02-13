package gov.census.cspro.engine.functions;

import gov.census.cspro.engine.functions.fragments.EngineChoiceDialogFragment;
import android.app.Activity;
import androidx.fragment.app.DialogFragment;
import androidx.appcompat.app.AppCompatActivity;

public class ChoiceDialog implements EngineFunction {

	private String m_title;
	private String[] m_list;

	public ChoiceDialog(String title,String[] list)
	{
		m_title = title;
		m_list = list;
	}

	public void runEngineFunction(Activity activity)
	{
		DialogFragment dlgFragment = EngineChoiceDialogFragment.newInstance(m_title, m_list);
		dlgFragment.show(((AppCompatActivity) activity).getSupportFragmentManager(), "ModalDialogFragment");
	}
}
