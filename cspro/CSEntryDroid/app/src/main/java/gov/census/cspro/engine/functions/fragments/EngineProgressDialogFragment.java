package gov.census.cspro.engine.functions.fragments;

import android.app.Dialog;
import android.app.ProgressDialog;
import android.content.DialogInterface;
import android.os.Bundle;
import androidx.annotation.NonNull;
import androidx.fragment.app.DialogFragment;
import androidx.appcompat.app.AlertDialog;
import android.widget.Button;

import gov.census.cspro.csentry.CSEntry;
import gov.census.cspro.csentry.R;
import gov.census.cspro.engine.ApplicationInterface;
import gov.census.cspro.engine.EngineInterface;

public class EngineProgressDialogFragment extends DialogFragment
{
    private static final String DIALOG_TEXT = "DIALOG_TEXT";

    public static @NonNull EngineProgressDialogFragment newInstance(String dialogText)
    {
        EngineProgressDialogFragment frag = new EngineProgressDialogFragment();
        Bundle args = new Bundle();
        args.putString(DIALOG_TEXT, dialogText);
        frag.setArguments(args);
        return frag;
    }

    @Override
    public @NonNull
    Dialog onCreateDialog(Bundle savedInstanceState)
    {
        //noinspection ConstantConditions
        String dialogText = getArguments().getString(DIALOG_TEXT);

        final ProgressDialog progressDialog = new ProgressDialog(getContext());
        progressDialog.setCanceledOnTouchOutside(false);
        progressDialog.setMessage(dialogText);
        progressDialog.setIndeterminate(true);
        progressDialog.setMax(10000);
        progressDialog.setCancelable(false);
        progressDialog.setProgressStyle(ProgressDialog.STYLE_HORIZONTAL);
        progressDialog.setProgressPercentFormat(null); // don't show %
        progressDialog.setProgressNumberFormat(null); // don't show fraction
        progressDialog.setButton(DialogInterface.BUTTON_POSITIVE,
            getString(R.string.modal_dialog_helper_cancel_text),
            (DialogInterface.OnClickListener) null);
        progressDialog.setOnShowListener(dialogInterface -> {
            Button b = progressDialog.getButton(AlertDialog.BUTTON_POSITIVE);
            b.setOnClickListener(view -> {
                progressDialog.setMessage(CSEntry.Companion.getContext().getString(R.string.sync_progress_canceling));
                progressDialog.setIndeterminate(true);
                onProgressDialogCancel();
            });

        });
        ApplicationInterface.getProgressUpdateLiveData().observe(this, update -> updateProgress(update.getProgress(), update.getDialogText()));
        return progressDialog;
    }

    private void onProgressDialogCancel()
    {
        EngineInterface.getInstance().onProgressDialogCancel();
    }

    private void setIndeterminate(boolean indeterminate)
    {
        ProgressDialog progressDialog = (ProgressDialog) getDialog();
        if (progressDialog != null)
            progressDialog.setIndeterminate(indeterminate);
    }

    public void setProgress(int progress)
    {
        ProgressDialog progressDialog = (ProgressDialog) getDialog();
        if (progressDialog != null)
            progressDialog.setProgress(progress);
    }

    public void setMessage(String dialogText)
    {
        ProgressDialog progressDialog = (ProgressDialog) getDialog();
        if (progressDialog != null)
            progressDialog.setMessage(dialogText);
    }

    // Update progress bar
    // Progress should be number between 0 and 10,000 or a negative
    // number for indeterminate
    private void updateProgress(int progress, String dialogText)
    {
        if (progress == -1) {
            setIndeterminate(true);
            setProgress(0);
        } else if (progress >= 0){
            setIndeterminate(false);
            setProgress(progress);
        }
        if (dialogText != null)
            setMessage(dialogText);
    }
}
