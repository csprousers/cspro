package gov.census.cspro.engine.functions;

import android.app.Activity;
import androidx.fragment.app.DialogFragment;
import androidx.appcompat.app.AppCompatActivity;

import gov.census.cspro.engine.functions.fragments.EngineShowListDialogFragment;

public class ShowListFunction implements EngineFunction
{
	private final boolean 					m_SelectFunction;
	private final boolean 					m_MultipleSelection;
	private final String[] 					m_headers;
	private final int[] 					m_rowTextColors;
	private final String[] 					m_lines;
	private final String					m_headingText;

	public ShowListFunction(String[] headers, int[] rowTextColors, String[] lines, String headingText)
	{
		m_headers 			= headers;
		m_rowTextColors     = rowTextColors;
		m_lines 			= lines;
		m_headingText		= headingText;
		m_SelectFunction = false;
		m_MultipleSelection = false;
	}

	public ShowListFunction(String[] headers, String[] lines, String headingText, boolean multipleSelection)
	{
		m_headers 			= headers;
		m_rowTextColors 	= null;
		m_lines 			= lines;
		m_headingText		= headingText;
		m_SelectFunction 	= true;
		m_MultipleSelection = multipleSelection;
	}

	public void runEngineFunction(Activity activity)
	{

		DialogFragment dlgFragment = EngineShowListDialogFragment.newInstance(m_headers, m_rowTextColors,
            m_lines, m_headingText, m_MultipleSelection, m_SelectFunction);
		dlgFragment.show(((AppCompatActivity) activity).getSupportFragmentManager(), "ModalDialogFragment");
	}
}
