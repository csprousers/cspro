package gov.census.cspro.engine.functions;

import android.app.Activity;
import android.content.Intent;

import gov.census.cspro.camera.BarcodeCaptureActivity;
import gov.census.cspro.engine.Messenger;
import gov.census.cspro.util.Constants;

public class BarcodeReadFunction implements EngineFunction {
    private final String _message;

    public BarcodeReadFunction(String message) {
        _message = message;
    }

    @Override
    public void runEngineFunction(Activity activity) {
        try {
            //open Camera with Barcode
            Intent intent = new Intent(activity, BarcodeCaptureActivity.class);
            intent.putExtra(Constants.EXTRA_BARCODE_KEY, Constants.EXTRA_BARCODE_VALUE);
            intent.putExtra(Constants.EXTRA_USER_MESSAGE_KEY, _message);
            activity.startActivityForResult(intent, Constants.INTENT_STARTACTIVITY_BARCODE);
        } catch (Exception e) {
            Messenger.getInstance().engineFunctionComplete(null);
        }
    }
}
