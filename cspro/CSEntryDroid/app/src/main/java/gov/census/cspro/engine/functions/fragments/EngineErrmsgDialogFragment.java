package gov.census.cspro.engine.functions.fragments;

import android.annotation.SuppressLint;
import android.app.Dialog;
import androidx.fragment.app.DialogFragment;
import android.content.DialogInterface;
import android.os.Bundle;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.app.AlertDialog;
import android.view.View;
import android.widget.TextView;

import gov.census.cspro.csentry.R;
import gov.census.cspro.engine.Messenger;
import gov.census.cspro.engine.Util;

/**
 * DialogFragment to use with ErrorMessageFunction engine function.
 *
 * Calls Messenger.engineFunctionComplete when dialog dismissed.
 * If you want a dialog fragment for an error message shown from
 * an Activity or Fragment use {@link gov.census.cspro.commonui.ErrorDialogFragment}
 */
public class EngineErrmsgDialogFragment extends DialogFragment
{
    public static @NonNull EngineErrmsgDialogFragment newInstance(@Nullable String title, @Nullable String message, @Nullable String[] buttons)
    {
        EngineErrmsgDialogFragment frag = new EngineErrmsgDialogFragment();
        Bundle args = new Bundle();
        args.putString("title", title);
        args.putString("message", message);
        args.putStringArray("buttons", buttons);
        frag.setArguments(args);
        return frag;
    }

    @Override
    public @NonNull Dialog onCreateDialog(Bundle savedInstanceState)
    {
        //noinspection ConstantConditions
        String title = getArguments().getString("title");
        String message = getArguments().getString("message");
        String[] buttons = getArguments().getStringArray("buttons");

        boolean noButtons = (buttons == null || buttons.length == 0);

        //noinspection ConstantConditions
        AlertDialog.Builder builder = new AlertDialog.Builder(getActivity());
        builder.setCancelable(false);
        setCancelable(false);

        if (noButtons) {
            if (!Util.stringIsNullOrEmpty(title))
                builder.setTitle(title);

            builder.setMessage(message)
                   .setPositiveButton(getString(R.string.modal_dialog_helper_ok_text),new DialogInterface.OnClickListener() {
                       @Override
                       public void onClick(DialogInterface dialog, int which) {
                           Messenger.getInstance().engineFunctionComplete(0); // 0 means success
                       }
                   });
        } else {
            // GHM 20140710 this custom view is used so that we don't run into the two-line limit
            // that applies to the default dialog box title
            @SuppressLint("InflateParams") final View titleView = getActivity().getLayoutInflater().inflate(R.layout.dialog_alert_title,null);
            final TextView messageText = titleView.findViewById(R.id.textview_alert_title);
            messageText.setText(message);
            builder.setCustomTitle(titleView);

            // TODO: if the title is ridiculously long, the buttons will not appear on the screen
            // is there a way to limit the height of the title dynamically based on the screen resolution?

            builder.setItems(buttons, new DialogInterface.OnClickListener() {
                public void onClick(DialogInterface dialog, int which) {
                    Messenger.getInstance().engineFunctionComplete(which);
                }
            });
        }

        return builder.create();
    }
}

