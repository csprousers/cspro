package gov.census.cspro.commonui;

import android.annotation.TargetApi;
import android.app.Activity;
import android.app.Dialog;
import android.content.Context;
import android.content.DialogInterface;
import android.os.Bundle;
import androidx.annotation.NonNull;
import androidx.fragment.app.DialogFragment;
import androidx.appcompat.app.AlertDialog;

import gov.census.cspro.csentry.R;

public class ErrorDialogFragment extends DialogFragment
{
    private final static String ARG_TITLE = "title";
    private final static String ARG_MESSAGE = "message";
    private final static String ARG_REQUEST_CODE = "code";

    /**
     * Class to show an error message dialog in a fragment.
     * <p>
     * Using AlertDialog.show() does not work well with the Activity Lifecyle.
     * If you rotate the screen while the dialog is showing the dialog is destroyed without
     * the onClick being called. This can lead to hangs while calling code waits for the onClick.
     * By putting the AlertDialog in a fragment we avoid this problem. When the device is rotated
     * the fragment redisplays the dialog so it will only go away when it is dismissed by the user.
     * <p>
     * To use this fragment the calling Activity must implement
     * {@link ErrorDialogFragment.OnErrorFragmentDismissed}.
     * OnErrorFragmentDismissed.errorDialogDismissed(int requestCode) will be called when the dialog
     * is dismissed by the user. The code that this method is passed is the same code that
     * is passed to the fragment. This allows the calling activity to distinguish between multiple
     * error messages when all the dismissals all come to the same errorDialogDismissed
     * implementation.
     * <p>
     * To create a new fragment use the static newInstance method:
     * <p>
     * ErrorDialogFragment errorDialog = ErrorDialogFragment.newInstance(
     * getString(R.string.my_error_title),
     * getString(R.string.my_error_msg),
     * MY_ERROR_CODE);
     * <p>
     * To display the dialog call show:
     * <p>
     * errorDialog.show(getSupportFragmentManager(), "errorDialog");
     * <p>
     * Then in the calling activity handle the dismissal:
     * <p>
     * public void errorDialogDismissed(int requestCode)
     * {
     * if (requestCode == MY_ERROR_CODE) {
     * // Handle dismissal of the error message here
     * }
     * }
     */
    private OnErrorFragmentDismissed m_listener;

    /**
     * Create a new fragment
     *
     * @param title      Title to display in dialog box
     * @param message    Error message to display
     * @param resultCode Client code that will be passed to errorDialogDismissed
     * @return new fragment
     */
    public static ErrorDialogFragment newInstance(String title, String message, int resultCode)
    {
        ErrorDialogFragment frag = new ErrorDialogFragment();
        frag.setCancelable(false);
        Bundle args = new Bundle();
        args.putString(ARG_TITLE, title);
        args.putString(ARG_MESSAGE, message);
        args.putInt(ARG_REQUEST_CODE, resultCode);
        frag.setArguments(args);
        return frag;
    }

    public interface OnErrorFragmentDismissed
    {
        void errorDialogDismissed(int code);
    }

    @TargetApi(23)
    @Override
    public void onAttach(Context context)
    {
        super.onAttach(context);
        attachToContext(context);
    }

    @SuppressWarnings("deprecation")
    @Override
    public void onAttach(Activity activity)
    {
        super.onAttach(activity);
        attachToContext(activity);
    }

    private void attachToContext(Context context)
    {
        if (context instanceof ErrorDialogFragment.OnErrorFragmentDismissed)
        {
            m_listener = (ErrorDialogFragment.OnErrorFragmentDismissed) context;
        } else
        {
            throw new RuntimeException(context.toString()
                + " must implement ErrorDialogFragment.OnErrorFragmentDismissed");
        }
    }

    @NonNull
    @Override
    public Dialog onCreateDialog(Bundle savedInstanceState)
    {
        //noinspection ConstantConditions
        final String title = getArguments().getString(ARG_TITLE);
        final String message = getArguments().getString(ARG_MESSAGE);
        final int code = getArguments().getInt(ARG_REQUEST_CODE);

        return new AlertDialog.Builder(requireActivity())
            .setMessage(message)
            .setTitle(title)
            .setPositiveButton(getString(R.string.modal_dialog_helper_ok_text),
                new DialogInterface.OnClickListener()
                {
                    public void onClick(DialogInterface dialog, int whichButton)
                    {
                        if (m_listener != null)
                            m_listener.errorDialogDismissed(code);
                    }
                }
            )
            .create();
    }
}
