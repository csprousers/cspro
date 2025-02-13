package gov.census.cspro.engine.functions;

import android.app.Activity;
import android.app.ProgressDialog;
import androidx.fragment.app.DialogFragment;
import androidx.appcompat.app.AppCompatActivity;

import gov.census.cspro.engine.Messenger;
import gov.census.cspro.engine.functions.fragments.EngineProgressDialogFragment;

public class ProgressDialogFunction implements EngineFunction {

	public static final int COMMAND_SHOW = 1;
	public static final int COMMAND_HIDE = 2;
	public static final int PROGRESS_INDETERMINATE = -1;
    private static final String FRAGMENT_TAG = "ProgressDialogFragment";
	
	private final int progress;
	private final int command;
	private final String dialogText;
	
	public ProgressDialogFunction(int command, int progress, String dialogText)
	{
		this.command = command;
		this.progress = progress;
		this.dialogText = dialogText;
	}
	
	public ProgressDialogFunction(int command)
	{
		this.command = command;
		this.progress = PROGRESS_INDETERMINATE;
		this.dialogText = null;		
	}
	
	@Override
	public void runEngineFunction(Activity activity) {
		switch (command) {
		case COMMAND_SHOW:
			showProgressDialog(activity, dialogText);
			break;
		case COMMAND_HIDE:
			hideProgressDialog(activity);
			break;
		}
		Messenger.getInstance().engineFunctionComplete(0);
	}

	private void showProgressDialog(Activity activity, String dialogText)
	{
		DialogFragment dlgFragment = EngineProgressDialogFragment.newInstance(dialogText);
		try {
            dlgFragment.show(((AppCompatActivity) activity).getSupportFragmentManager(), FRAGMENT_TAG);
        } catch (IllegalStateException e) {
		    //do nothing as this is trying to show dialog when state has changed.
        }
	}


	private void hideProgressDialog(Activity activity)
	{
        EngineProgressDialogFragment frag = getDialogFragment(activity);
        if (frag != null) {
			frag.dismiss();
		}
	}

    private EngineProgressDialogFragment getDialogFragment(Activity activity)
    {
        if (activity instanceof AppCompatActivity)
            return (EngineProgressDialogFragment) ((AppCompatActivity) activity).getSupportFragmentManager().findFragmentByTag(FRAGMENT_TAG);
        else
            return null;
    }
}
