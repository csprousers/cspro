package gov.census.cspro.engine.functions.fragments;

import android.annotation.SuppressLint;
import android.app.Dialog;
import android.content.DialogInterface;
import android.os.Bundle;
import androidx.annotation.NonNull;
import androidx.fragment.app.DialogFragment;
import android.text.InputType;
import android.view.Gravity;
import android.view.KeyEvent;
import android.view.View;
import android.view.WindowManager;
import android.view.inputmethod.EditorInfo;
import android.widget.EditText;
import android.widget.TextView;

import gov.census.cspro.csentry.R;
import gov.census.cspro.engine.Messenger;
import gov.census.cspro.engine.Util;

/**
 * DialogFragment to use with PromptFunction engine function.
 *
 * Calls Messenger.engineFunctionComplete when dialog dismissed.
 */
public class EnginePromptDialogFragment extends DialogFragment
{
    private static final int MULTILINE_NUMBER_LINES	= 3;

    private final static String TITLE = "TITLE";
    private final static String INITIAL_VALUE = "INITIAL_VALUE";
    private final static String NUMERIC = "NUMERIC";
    private final static String PASSWORD = "PASSWORD";
    private final static String UPPER_CASE = "UPPER_CASE";
    private final static String MULTILINE = "MULTILINE";

    public static @NonNull
    EnginePromptDialogFragment newInstance(String title, String initialValue, boolean numeric,
                                           boolean password, boolean upperCase, boolean multiline)
    {
        EnginePromptDialogFragment frag = new EnginePromptDialogFragment();
        Bundle args = new Bundle();
        args.putString(TITLE, title);
        args.putString(INITIAL_VALUE, initialValue);
        args.putBoolean(NUMERIC, numeric);
        args.putBoolean(PASSWORD, password);
        args.putBoolean(UPPER_CASE, upperCase);
        args.putBoolean(MULTILINE, multiline);
        frag.setArguments(args);

        return frag;
    }

    @Override
    public @NonNull
    Dialog onCreateDialog(Bundle savedInstanceState)
    {
        //noinspection ConstantConditions
        String title = getArguments().getString(TITLE);
        String initialValue = getArguments().getString(INITIAL_VALUE);

        @SuppressWarnings("ConstantConditions") @SuppressLint("InflateParams") View promptView = getActivity().getLayoutInflater().inflate(R.layout.dialog_prompt,null);

        final @NonNull TextView titleText = promptView.findViewById(R.id.textview_prompt_title);
        titleText.setText(title);

        final EditText editText = promptView.findViewById(R.id.edittext_prompt);
        editText.setText(initialValue);

        setEditTextAttributes(editText);

        android.app.AlertDialog.Builder builder = new android.app.AlertDialog.Builder(getActivity());
        builder.setView(promptView)
                   .setPositiveButton(getActivity().getString(R.string.modal_dialog_helper_ok_text),null);

        final android.app.AlertDialog promptDialog = builder.create();
        promptDialog.setCanceledOnTouchOutside(false);

        promptDialog.setOnShowListener(new DialogInterface.OnShowListener() {
            @Override
            public void onShow(DialogInterface dialog) {
                Util.setFocus(editText);
                promptDialog.getButton(android.app.AlertDialog.BUTTON_POSITIVE).setOnClickListener(new View.OnClickListener() {
                    @Override
                    public void onClick(View view) {
                        String promptText = editText.getText().toString().trim();
                        promptDialog.dismiss();
                        endFunction(promptText);
                    }
                });
            }
        });

        // pressing the Done key will act as if they clicked on OK
        editText.setOnEditorActionListener(new TextView.OnEditorActionListener() {
            @Override
            public boolean onEditorAction(TextView v, int actionId, KeyEvent event) {
                if( ( actionId & EditorInfo.IME_MASK_ACTION ) == EditorInfo.IME_ACTION_DONE ) {
                    String promptText = editText.getText().toString().trim();
                    promptDialog.dismiss();
                    endFunction(promptText);
                    return true;
                }

                return false;
            }
        });

       return promptDialog;
    }


    private void setEditTextAttributes(EditText editText)
    {
        //noinspection ConstantConditions
        boolean numeric = getArguments().getBoolean(NUMERIC);
        boolean password = getArguments().getBoolean(PASSWORD);
        boolean upperCase = getArguments().getBoolean(UPPER_CASE);
        boolean multiline = getArguments().getBoolean(MULTILINE);
        int inputTypeFlags = 0;

        if( numeric )
        {
            inputTypeFlags |= InputType.TYPE_CLASS_NUMBER | InputType.TYPE_NUMBER_FLAG_DECIMAL | InputType.TYPE_NUMBER_FLAG_SIGNED;

            if( password )
                inputTypeFlags |= InputType.TYPE_NUMBER_VARIATION_PASSWORD;
        }

        else
        {
            inputTypeFlags |= InputType.TYPE_CLASS_TEXT | InputType.TYPE_TEXT_FLAG_NO_SUGGESTIONS;

            if( password )
                inputTypeFlags |= InputType.TYPE_TEXT_VARIATION_PASSWORD;

            if( upperCase )
                inputTypeFlags |= InputType.TYPE_TEXT_FLAG_CAP_CHARACTERS;
        }

        if( multiline )
        {
            inputTypeFlags |= InputType.TYPE_TEXT_FLAG_MULTI_LINE;
            editText.setGravity(Gravity.TOP | Gravity.START);
            editText.setMinLines(MULTILINE_NUMBER_LINES);
        }

        else
        {
            editText.setGravity(Gravity.CENTER);
            editText.setImeOptions(EditorInfo.IME_ACTION_DONE);
        }

        editText.setInputType(inputTypeFlags);
    }

    @Override
    public void onCancel(DialogInterface dlg)
    {
        super.onCancel(dlg);
        endFunction("");
    }

    private void endFunction(String retVal)
    {
        Messenger.getInstance().engineFunctionComplete(retVal);
    }

}
