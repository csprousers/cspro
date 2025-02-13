package gov.census.cspro.engine.functions;

import android.app.Activity;

import androidx.appcompat.app.AppCompatActivity;
import androidx.fragment.app.DialogFragment;

import gov.census.cspro.engine.functions.fragments.PasswordQueryFunctionFragment;

public class PasswordQueryFunction implements EngineFunction
{
    private String _title;
    private String _description;
    private boolean _hideReenter;

	public PasswordQueryFunction(String title, String description, boolean hideReenter)
	{
        _title = title;
        _description = description;
        _hideReenter = hideReenter;
	}
	
	@Override
	public void runEngineFunction(Activity activity)
	{
		DialogFragment dlgFragment = PasswordQueryFunctionFragment.newInstance(_title, _description, _hideReenter);
		dlgFragment.show(((AppCompatActivity)activity).getSupportFragmentManager(), "ModalDialogFragment");
	}
}
