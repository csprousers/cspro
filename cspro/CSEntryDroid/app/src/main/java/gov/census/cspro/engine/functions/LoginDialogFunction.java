package gov.census.cspro.engine.functions;

import android.app.Activity;
import androidx.fragment.app.DialogFragment;
import androidx.appcompat.app.AppCompatActivity;

import gov.census.cspro.engine.functions.fragments.EngineLoginDialogFragment;

public class LoginDialogFunction implements EngineFunction
{
	private boolean showInvalidLoginError;
	private String server;

	public LoginDialogFunction(String server, boolean showInvalidLoginError)
	{
		this.showInvalidLoginError = showInvalidLoginError;
		this.server = server;
	}
	
	@Override
	public void runEngineFunction(Activity activity)
	{
		DialogFragment dlgFragment = EngineLoginDialogFragment.newInstance(server, showInvalidLoginError);
		try {
		    dlgFragment.show(((AppCompatActivity) activity).getSupportFragmentManager(), "ModalDialogFragment");
        } catch (IllegalStateException e) {
            //do nothing as this is trying to show dialog when state has changed.
        }
	}
}
