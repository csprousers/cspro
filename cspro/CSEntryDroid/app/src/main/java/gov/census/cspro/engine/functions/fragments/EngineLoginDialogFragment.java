package gov.census.cspro.engine.functions.fragments;

import android.annotation.SuppressLint;
import android.app.AlertDialog;
import android.app.Dialog;
import android.content.DialogInterface;
import android.os.Bundle;
import androidx.annotation.NonNull;
import androidx.fragment.app.DialogFragment;
import android.view.View;
import android.widget.EditText;
import android.widget.TextView;

import org.jetbrains.annotations.NotNull;

import gov.census.cspro.csentry.R;
import gov.census.cspro.engine.Messenger;
import gov.census.cspro.engine.Util;

public class EngineLoginDialogFragment extends DialogFragment
{
    private static final String SHOW_ERROR = "SHOW_ERROR";
    private static final String SERVER = "SERVER";

    public static EngineLoginDialogFragment newInstance(String server, boolean showInvalidLoginError)
    {
        EngineLoginDialogFragment frag = new EngineLoginDialogFragment();
        Bundle args = new Bundle();
        args.putBoolean(SHOW_ERROR, showInvalidLoginError);
        args.putString(SERVER, server);
        frag.setArguments(args);
        return frag;
    }

    @Override
    public @NonNull Dialog onCreateDialog(Bundle savedInstanceState)
    {
        //noinspection ConstantConditions
        final boolean showInvalidLoginError = getArguments().getBoolean(SHOW_ERROR);
        final String serverName = getArguments().getString(SERVER);

        @SuppressWarnings("ConstantConditions") @SuppressLint("InflateParams") View usernamePasswordView = getActivity().getLayoutInflater().inflate(R.layout.dialog_username_password, null);
        final TextView titleText = usernamePasswordView.findViewById(R.id.textview_title);
        titleText.setText(getString(R.string.sync_login_dialog_prompt, serverName));
        final EditText usernameText = usernamePasswordView.findViewById(R.id.edittext_username);
        final EditText passwordText = usernamePasswordView.findViewById(R.id.edittext_password);
        final TextView invalidMessage = usernamePasswordView.findViewById(R.id.invalid_username_password_message);
        invalidMessage.setVisibility(showInvalidLoginError ? View.VISIBLE : View.GONE);
        AlertDialog.Builder builder = new AlertDialog.Builder(getContext());
        builder.setView(usernamePasswordView).setPositiveButton(getString(R.string.modal_dialog_helper_ok_text),
                (dialog, whichButton) -> {
                    final String username = usernameText.getText().toString().trim();
                    final String password = passwordText.getText().toString().trim();
                    Messenger.getInstance().engineFunctionComplete(username + "\n" + password);
                });

        final AlertDialog dialog = builder.create();
        dialog.setCanceledOnTouchOutside(false);

        dialog.setOnShowListener(dialogInterface -> Util.setFocus(usernameText));
        return dialog;
    }

    @Override
    public void onCancel(@NotNull DialogInterface dlg)
    {
        super.onCancel(dlg);
        Messenger.getInstance().engineFunctionComplete(null);
    }

}
