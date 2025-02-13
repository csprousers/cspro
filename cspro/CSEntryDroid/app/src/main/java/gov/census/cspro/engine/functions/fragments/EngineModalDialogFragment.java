package gov.census.cspro.engine.functions.fragments;

import android.app.AlertDialog;
import android.app.Dialog;
import androidx.annotation.NonNull;
import androidx.fragment.app.DialogFragment;
import android.content.DialogInterface;
import android.os.Bundle;

import gov.census.cspro.csentry.R;
import gov.census.cspro.engine.Messenger;
import gov.census.cspro.engine.functions.ModalDialogFunction;

/**
 * DialogFragment to use with ModalDialogFunction engine function.
 *
 * Calls Messenger.engineFunctionComplete when dismissed.
 * If you want a dialog fragment for an error message shown from
 * an Activity or Fragment use {@link gov.census.cspro.commonui.ErrorDialogFragment}
 */
public class EngineModalDialogFragment extends DialogFragment {

    private int	m_clickedButtonId = -1;

    private DialogInterface.OnClickListener m_onClickListener =
        new DialogInterface.OnClickListener() {

            @Override
            public void onClick(DialogInterface dialog, int which) {
                m_clickedButtonId = which;
            }
        };

    @Override
    public void onDismiss(final DialogInterface dialog) {
        super.onDismiss(dialog);
        Messenger.getInstance().engineFunctionComplete(m_clickedButtonId);
    }

    public static EngineModalDialogFragment newInstance(String title, String message, int buttons)
    {
        EngineModalDialogFragment frag = new EngineModalDialogFragment();
        Bundle args = new Bundle();
        args.putString("title", title);
        args.putString("message", message);
        args.putInt("buttons", buttons);
        frag.setArguments(args);
        return frag;
    }

    @NonNull
    @Override
    public Dialog onCreateDialog(Bundle savedInstanceState)
    {
        //noinspection ConstantConditions
        String title = getArguments().getString("title");
        String message = getArguments().getString("message");
        int buttons = getArguments().getInt("buttons");

        AlertDialog.Builder builder = new AlertDialog.Builder(getActivity());

        builder.setTitle(title)
               .setMessage(message);

        switch(buttons){
            case ModalDialogFunction.MB_OKCANCEL:
                builder.setPositiveButton(getResources().getString(R.string.modal_dialog_helper_ok_text), m_onClickListener);
                builder.setNegativeButton(getResources().getString(R.string.modal_dialog_helper_cancel_text), m_onClickListener);
                break;
            case ModalDialogFunction.MB_OK:
                builder.setPositiveButton(getResources().getString(R.string.modal_dialog_helper_ok_text), m_onClickListener);
                break;
            case ModalDialogFunction.MB_YESNO:
                builder.setPositiveButton(getResources().getString(R.string.modal_dialog_helper_yes_text), m_onClickListener);
                builder.setNegativeButton(getResources().getString(R.string.modal_dialog_helper_no_text), m_onClickListener);
                break;
            default:
                builder.setPositiveButton(getResources().getString(R.string.modal_dialog_helper_ok_text), m_onClickListener);
                builder.setNegativeButton(getResources().getString(R.string.modal_dialog_helper_cancel_text), m_onClickListener);
                break;
        }

        builder.setCancelable(false);
        setCancelable(false);

        return builder.create();
    }
}
