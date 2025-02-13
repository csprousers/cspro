package gov.census.cspro.engine.functions.fragments;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.content.DialogInterface;
import android.os.Bundle;
import androidx.annotation.NonNull;
import androidx.fragment.app.DialogFragment;
import android.view.LayoutInflater;
import android.view.View;
import android.view.WindowManager;
import android.widget.EditText;

import gov.census.cspro.csentry.R;
import gov.census.cspro.csentry.ui.FieldNoteUpdateListener;
import gov.census.cspro.engine.Messenger;
import gov.census.cspro.engine.Util;

public class EngineEditNoteDialogFragment extends DialogFragment
{

    private static final String FIELD_NOTE = "FIELD_NOTE";
    private static final String TITLE = "TITLE";
    private static final String IS_CASE_NOTE = "IS_CASE_NOTE";

    public static EngineEditNoteDialogFragment newInstance(String fieldNote, String title, boolean isCaseNote)
    {
        EngineEditNoteDialogFragment frag = new EngineEditNoteDialogFragment();
        Bundle args = new Bundle();
        args.putString(FIELD_NOTE, fieldNote);
        args.putString(TITLE, title);
        args.putBoolean(IS_CASE_NOTE, isCaseNote);
        frag.setArguments(args);
        return frag;
    }

    @Override
    public @NonNull Dialog onCreateDialog(Bundle savedInstanceState)
    {
        //noinspection ConstantConditions
        final String fieldNote = getArguments().getString(FIELD_NOTE);
        final String title = getArguments().getString(TITLE);

        //noinspection ConstantConditions
        LayoutInflater inflator = getActivity().getLayoutInflater();
        @SuppressLint("InflateParams") View view = inflator.inflate(R.layout.dialog_field_note,null);

        AlertDialog.Builder builder = new AlertDialog.Builder(getContext());
        builder	.setTitle(title)
                   .setView(view)
                   .setPositiveButton(getString(R.string.modal_dialog_helper_ok_text),null)
                   .setNeutralButton(getString(R.string.modal_dialog_helper_clear_text),null);

        final AlertDialog noteDialog = builder.create();

        final EditText editText = view.findViewById(R.id.edittext_field_note_id);
        editText.setText(fieldNote);

        noteDialog.setOnShowListener(new DialogInterface.OnShowListener() {
            @Override
            public void onShow(DialogInterface dialog) {
                Util.setFocus(editText);

                noteDialog.getButton(AlertDialog.BUTTON_POSITIVE).setOnClickListener(new View.OnClickListener() {
                    @Override
                    public void onClick(View view) {
                        String fieldNote = editText.getText().toString().trim();
                        noteDialog.dismiss();
                        endFunction(fieldNote);
                    }
                });
                noteDialog.getButton(AlertDialog.BUTTON_NEUTRAL).setOnClickListener(new View.OnClickListener() {
                    @Override
                    public void onClick(View view) {
                        editText.setText("");
                    }
                });
            }
        });
        noteDialog.setCanceledOnTouchOutside(false);
        return noteDialog;
    }

    @Override
    public void onCancel(DialogInterface dlg)
    {
        super.onCancel(dlg);
        endFunction(null);
    }

    private void endFunction(String retVal)
    {
        //noinspection ConstantConditions
        final boolean isCaseNote = getArguments().getBoolean(IS_CASE_NOTE);

        if( retVal != null && !isCaseNote ) {
            Activity activity = getActivity();
            if (activity instanceof FieldNoteUpdateListener) {
                ((FieldNoteUpdateListener) activity).noteStateChanged();
            }
        }

        Messenger.getInstance().engineFunctionComplete(retVal);
    }
}
