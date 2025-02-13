package gov.census.cspro.engine.functions;

import android.app.Activity;
import androidx.fragment.app.DialogFragment;
import androidx.appcompat.app.AppCompatActivity;

import gov.census.cspro.engine.functions.fragments.EngineEditNoteDialogFragment;

// GHM 20140114 + 20140122

public class EditNoteFunction implements EngineFunction {

	private String m_fieldNote;
	private String m_title;
	private boolean m_caseNote;

	public EditNoteFunction(String fieldNote,String title,boolean caseNote)
	{
		m_fieldNote = fieldNote;
		m_title = title;
		m_caseNote = caseNote;
	}

	public void runEngineFunction(Activity activity)
	{
		DialogFragment dlgFragment = EngineEditNoteDialogFragment.newInstance(m_fieldNote, m_title, m_caseNote);
		dlgFragment.show(((AppCompatActivity) activity).getSupportFragmentManager(), "ModalDialogFragment");
	}
}
