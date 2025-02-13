package gov.census.cspro.engine.functions;

import android.app.Activity;
import android.content.Intent;
import gov.census.cspro.csentry.ui.ChooseBluetoothDeviceActivity;
import gov.census.cspro.csentry.ui.EntryActivity;
import gov.census.cspro.engine.Messenger;

public class ChooseBluetoothDeviceFunction implements EngineFunction {

	@Override
	public void runEngineFunction(Activity activity) {
		try {
			Intent intent = new Intent(activity, ChooseBluetoothDeviceActivity.class);
			activity.startActivityForResult(intent, EntryActivity.BluetoothChooseDeviceCode);
		} catch (Exception e) {
			Messenger.getInstance().engineFunctionComplete(null);
		}	
	}
}
