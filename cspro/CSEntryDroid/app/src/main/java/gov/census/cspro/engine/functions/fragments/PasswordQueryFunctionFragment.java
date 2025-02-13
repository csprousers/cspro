package gov.census.cspro.engine.functions.fragments;

import android.annotation.SuppressLint;
import android.app.AlertDialog;
import android.app.Dialog;
import android.content.DialogInterface;
import android.os.Bundle;
import androidx.annotation.NonNull;
import androidx.fragment.app.DialogFragment;

import android.text.Editable;
import android.text.TextWatcher;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.TextView;

import gov.census.cspro.csentry.R;
import gov.census.cspro.engine.Messenger;
import gov.census.cspro.engine.Util;

public class PasswordQueryFunctionFragment extends DialogFragment
{
    private static final String TITLE = "TITLE";
    private static final String DESCRIPTION = "DESCRIPTION";
    private static final String HIDE_REENTER = "HIDE_REENTER";

    public static PasswordQueryFunctionFragment newInstance(String title, String description, boolean hideReenter)
    {
		Bundle args = new Bundle();
        args.putString(TITLE, title);
        args.putString(DESCRIPTION, description);
        args.putBoolean(HIDE_REENTER, hideReenter);
        
        PasswordQueryFunctionFragment frag = new PasswordQueryFunctionFragment();
		frag.setArguments(args);
		
		return frag;
    }

    @Override
    public @NonNull Dialog onCreateDialog(Bundle savedInstanceState)
    {
        final String title = getArguments().getString(TITLE);
        final String description = getArguments().getString(DESCRIPTION);
		final boolean hideReenter = getArguments().getBoolean(HIDE_REENTER);

        @SuppressLint("InflateParams") View passwordView = getActivity().getLayoutInflater().inflate(R.layout.dialog_password, null);
        final EditText firstPasswordText = passwordView.findViewById(R.id.edittext_password_first);
        final EditText secondPasswordText = passwordView.findViewById(R.id.edittext_password_second);

        ((TextView)passwordView.findViewById(R.id.textview_title)).setText(title);
        ((TextView)passwordView.findViewById(R.id.textview_description)).setText(description);

        if( hideReenter )
            firstPasswordText.setVisibility(View.GONE);

        AlertDialog.Builder builder = new AlertDialog.Builder(getContext());
        builder.setView(passwordView)
               .setPositiveButton(getString(R.string.modal_dialog_helper_ok_text), new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int whichButton) {
                        Messenger.getInstance().engineFunctionComplete(secondPasswordText.getText().toString());
                    }
                });

        final AlertDialog dialog = builder.create();
        dialog.setCanceledOnTouchOutside(false);

        // only enable the OK button once a password is entered, or when both passwords match
        // when the user has to enter two passwords
        class ButtonEnabler {
            public void setEnabled() {
                Button ok_button = dialog.getButton(AlertDialog.BUTTON_POSITIVE);
                String password = secondPasswordText.getText().toString();
                boolean ok_enabled = !password.isEmpty();

                if( ok_enabled && !hideReenter )
                    ok_enabled = firstPasswordText.getText().toString().equals(password);

                ok_button.setEnabled(ok_enabled);
            }
        }

        dialog.setOnShowListener(new DialogInterface.OnShowListener() {
                  @Override
                  public void onShow(DialogInterface dialog) {
                      new ButtonEnabler().setEnabled();
                      Util.setFocus(hideReenter ? secondPasswordText : firstPasswordText);
                  }
              });

        TextWatcher textWatcher = new TextWatcher() {
            @Override
            public void onTextChanged(CharSequence s, int start, int before, int count) { }

            @Override
            public void beforeTextChanged(CharSequence s, int start, int count, int after) { }

            @Override
            public void afterTextChanged(Editable s) {
                new ButtonEnabler().setEnabled();
            }
        };

        firstPasswordText.addTextChangedListener(textWatcher);
        secondPasswordText.addTextChangedListener(textWatcher);

		return dialog;
    }

    @Override
    public void onCancel(DialogInterface dlg)
    {
        super.onCancel(dlg);
        Messenger.getInstance().engineFunctionComplete(null);
    }
}
