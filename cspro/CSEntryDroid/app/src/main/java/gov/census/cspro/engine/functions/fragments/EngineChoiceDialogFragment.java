package gov.census.cspro.engine.functions.fragments;

import android.annotation.SuppressLint;
import android.app.AlertDialog;
import android.app.Dialog;
import android.content.DialogInterface;
import android.os.Bundle;
import androidx.annotation.NonNull;
import androidx.fragment.app.DialogFragment;
import android.view.View;
import android.widget.TextView;

import gov.census.cspro.csentry.R;
import gov.census.cspro.engine.Messenger;

/**
 * DialogFragment to use with ChoiceDialog engine function.
 *
 * Calls Messenger.engineFunctionComplete when dialog dismissed.
 */
public class EngineChoiceDialogFragment extends DialogFragment
{
    private final static String TITLE = "TITLE";
    private final static String LIST = "LIST";

    public static @NonNull
    EngineChoiceDialogFragment newInstance(@NonNull String title, @NonNull String[] list)
    {
        EngineChoiceDialogFragment frag = new EngineChoiceDialogFragment();
        Bundle args = new Bundle();
        args.putString(TITLE, title);
        args.putStringArray(LIST, list);
        frag.setArguments(args);
        return frag;
    }

    @Override
    public @NonNull
    Dialog onCreateDialog(Bundle savedInstanceState)
    {
        //noinspection ConstantConditions
        String title = getArguments().getString(TITLE);
        String[] list = getArguments().getStringArray(LIST);

        // GHM 20140710 this custom view is used so that we don't run into the two-line limit
        // that applies to the default dialog box title
        //noinspection ConstantConditions
        @SuppressLint("InflateParams") View titleView = getActivity().getLayoutInflater().inflate(R.layout.dialog_alert_title, null);
        final TextView titleText = titleView.findViewById(R.id.textview_alert_title);
        titleText.setText(title);

        // TODO: if the title is ridiculously long, the buttons will not appear on the screen
        // is there a way to limit the height of the title dynamically based on the screen resolution?

        android.app.AlertDialog.Builder builder = new android.app.AlertDialog.Builder(getActivity());
        builder.setCustomTitle(titleView)
               .setItems(list, new DialogInterface.OnClickListener()
               {
                   public void onClick(DialogInterface dialog, int which)
                   {
                       endFunction(which + 1);
                   }
               });

        AlertDialog dialog = builder.create();
        dialog.setCanceledOnTouchOutside(false);
        return dialog;
    }

    @Override
    public void onCancel(DialogInterface dlg)
    {
        super.onCancel(dlg);
        endFunction(0);
    }

    private void endFunction(int retVal)
    {
        Messenger.getInstance().engineFunctionComplete(retVal);
    }
}