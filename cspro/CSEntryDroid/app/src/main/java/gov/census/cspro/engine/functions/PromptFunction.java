package gov.census.cspro.engine.functions;

import android.app.Activity;
import androidx.fragment.app.DialogFragment;
import androidx.appcompat.app.AppCompatActivity;

import gov.census.cspro.engine.functions.fragments.EnginePromptDialogFragment;


public class PromptFunction implements EngineFunction {

	private String m_title;
	private String m_initialValue;
	private boolean m_numeric;
	private boolean m_password;
	private boolean m_upperCase;
	private boolean m_multiline;

	public PromptFunction(String title,String initialValue,boolean numeric,boolean password,boolean upperCase,boolean multiline)
	{
		m_title = title;
		m_initialValue = initialValue;
		m_numeric = numeric;
		m_password = password;
		m_upperCase = upperCase;
		m_multiline = multiline;
	}

	public void runEngineFunction(Activity activity)
	{
		DialogFragment dlgFragment = EnginePromptDialogFragment.newInstance(m_title,
			m_initialValue, m_numeric, m_password, m_upperCase, m_multiline);

		dlgFragment.show(((AppCompatActivity) activity).getSupportFragmentManager(), "ModalDialogFragment");
	}

}
