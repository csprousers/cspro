package gov.census.cspro.smartsync.p2p;

import java.io.IOException;
import java.util.UUID;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.Executors;
import java.util.concurrent.TimeUnit;

import android.app.Activity;
import android.app.Dialog;
import android.app.DialogFragment;
import android.app.FragmentManager;
import android.app.ProgressDialog;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothServerSocket;
import android.bluetooth.BluetoothSocket;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.os.SystemClock;

import gov.census.cspro.csentry.CSEntry;
import gov.census.cspro.csentry.R;
import gov.census.cspro.csentry.ui.EntryActivity;
import gov.census.cspro.engine.Messenger;
import gov.census.cspro.smartsync.SyncError;
import gov.census.cspro.util.CancelableTaskRunner;
import timber.log.Timber;

@SuppressWarnings("unused")
public class AndroidBluetoothAdapter {

	private final BluetoothAdapter m_adapter;
	private final Context m_context = CSEntry.Companion.getContext();
	private final CancelableTaskRunner cancelableTaskRunner = new CancelableTaskRunner();
	private final int m_bluetooth_name_change_timeout = 5000;
	private final CancelableTaskRunner.CancelChecker cancelChecker = new CancelableTaskRunner.CancelChecker() {
		
		@Override
		public boolean isCancelled() {
			return AndroidBluetoothAdapter.this.isCancelled();
		}
	};

	public static AndroidBluetoothAdapter create()
	{
 		BluetoothAdapter adapter = BluetoothAdapter.getDefaultAdapter();
 		if (adapter == null) {
 			// Bluetooth not supported on device
			return null;
		} else {
 			return new AndroidBluetoothAdapter(adapter);
		}
	}

	private AndroidBluetoothAdapter(BluetoothAdapter bluetoothAdapter)
	{
		m_adapter = bluetoothAdapter;
	}

	public void enable()
	{
		Timber.d("Enabling bluetooth");
		if (!m_adapter.isEnabled()) {
			
			// Need to enable in a separate thread so we can use
			// a broadcast receiver and wait synchronously.
			// 20 seconds is pretty long but in theory the EnableTask 
			// should always terminate by itself
			try {
				cancelableTaskRunner.run(Executors.newSingleThreadExecutor(), 
						new EnableTask(), cancelChecker, 
						20, TimeUnit.SECONDS);
			} catch (ExecutionException e) {
				// EnableTask doesn't really throw any checked exceptions
				// but ExecutionException has to be in the signature since 
				// other tasks can.
				Timber.d(e, "Error enabling bluetooth");
			}
		}
	}
	
	/**
	 * Task for enabling bluetooth device.
	 *
	 * Bluetooth enable is asynchronous. In order to do this synchronously
	 * so that it fits into the existing sync framework we use a background
	 * task with its own Looper/Handler that runs an event loop in order to
	 * get the BroadcastReceiver notifications and exit when it is finished.
	 */
	private final class EnableTask implements CancelableTaskRunner.CancelableTask<Void> {
		
		private Handler m_handler;
		boolean aborted = false;

		@Override
		public Void run() {
			Timber.d("Start bluetooth enable thread...");
			Looper.prepare();
			m_handler = new Handler(Looper.myLooper());
			if (!aborted) {
				final BroadcastReceiver bluetoothBroadcastReceiver = new BroadcastReceiver() {

					public void onReceive(Context c, Intent intent) {
						final String action = intent.getAction();
						if (BluetoothAdapter.ACTION_STATE_CHANGED.equals(action)) {
							if (intent.getExtras() != null && intent.getExtras().getInt(BluetoothAdapter.EXTRA_STATE) == BluetoothAdapter.STATE_ON) {
								abort();
							}
						}
					}
				};

				IntentFilter filter = new IntentFilter(BluetoothAdapter.ACTION_STATE_CHANGED);
				m_context.registerReceiver(bluetoothBroadcastReceiver, filter,
						null, m_handler);

				m_adapter.enable();

				Looper.loop();
				m_context.unregisterReceiver(bluetoothBroadcastReceiver);
			}
			Timber.d("Bluetooth enable thread exiting.");
			return null;
		}
		
		@Override
		public void abort()
		{
			aborted = true;
			if (m_handler != null)
				m_handler.getLooper().quit();
		}
	}

	public void disable()
	{

		m_adapter.disable();
	}
	
	public boolean isEnabled()
	{
		return m_adapter.isEnabled();
	}

	public String getName() {
		return m_adapter.getName();
	}

    public String setName(String bluetoothName) {
        return Messenger.getInstance().runStringEngineFunction(activity -> {
            setBluetoothName(bluetoothName, activity);
        });
    }

	@SuppressWarnings("unused")
	BluetoothSocket connectToRemoteDevice(String remoteDeviceName, String remoteDeviceAddress, UUID service) {
		Timber.d("Connecting to bluetooth device %s", remoteDeviceName);

		BluetoothSocket socket;
		
		if (m_adapter == null)
		    return null;
	    
        // Get a BluetoothSocket for a connection with the given BluetoothDevice
        try {
        	BluetoothDevice device;
    		if (remoteDeviceAddress.isEmpty()) {
	    		device = scanForDevice(remoteDeviceName);
    		} else {
    			device = m_adapter.getRemoteDevice(remoteDeviceAddress);
    		}
    	    
    	    if (device == null)
    	    	return null;

        	m_adapter.cancelDiscovery();

        	socket = device.createRfcommSocketToServiceRecord(service);

        	connectToSocket(socket);
        	
	    } catch (IOException e) {
			Timber.e(e, "Error connecting to remote device");
	    	return null;
	    }
        return socket;
	}

	/**
	 * Task for scanning/pairing bluetooth device.
	 *
	 * Bluetooth scanning is asynchronous, you register a BroadcastReceiver,
	 * call startDiscovery() and broadcast receiver is notified when the
	 * scan finds a device or completes. In order to do this synchronously
	 * so that it fits into the existing SyncClient framework we use a background
	 * tasks with its own Looper/Handler that runs an event loop in order to
	 * get the BroadcastReceiver notifications and exit when it is finished.
	 * We can then wait on this task in order to synchronously do the scan.
	 */
	private final class ScanTask implements CancelableTaskRunner.CancelableTask<BluetoothDevice> {
		
		private Handler m_handler = null;
		private volatile BluetoothDevice m_device = null;
		private boolean m_isDiscovering = false;
		private int m_numDiscoveryStartAttempts = 0;
		private int m_numDiscoveryFinishes = 0;
		private final String m_deviceName;
		
		ScanTask(String deviceName)
		{
			m_deviceName = deviceName;
		}
		
		@Override
		public BluetoothDevice run() {
			Timber.d("Start bluetooth scan thread...");

			Looper.prepare();
			m_handler = new Handler(Looper.myLooper());
			
			final BroadcastReceiver bluetoothBroadcastReceiver = new BroadcastReceiver() {
				
				public void onReceive(Context c, Intent intent) {
					final String action = intent.getAction();
					if (BluetoothDevice.ACTION_FOUND.equals(action)) {
		                // When discovery finds a device
		                BluetoothDevice device = intent.getParcelableExtra(BluetoothDevice.EXTRA_DEVICE);
		                if (device != null && device.getName() != null) {
							Timber.d("Found device %s", device.getName());
			                if (device.getName().equalsIgnoreCase(m_deviceName)) {
			                	m_device = device;
		            			abort();
			                }
		                }
		            } else if (BluetoothAdapter.ACTION_DISCOVERY_STARTED.equals(action)) {
						Timber.d("Discovery started ");
		            } else if (BluetoothAdapter.ACTION_DISCOVERY_FINISHED.equals(action)) {
		            	if (m_isDiscovering) {
			                ++m_numDiscoveryFinishes;
							Timber.d("Scan complete %s", m_numDiscoveryFinishes);
	            			// Restart scan but wait 2 seconds
			            	// since discovery is resource intensive
							Timber.d("Bluetooth discovery finished, restarting in 2 seconds");
			            	m_handler.postDelayed(new Runnable() {
			                    public void run() {
			            			try {
										startDiscovery();
									} catch (SyncError e) {
										Timber.e(e, "Failed to start bluetooth scan");
										abort();
									}
			                    }
			            	}, 2000);
		            	}
		            }
				}
			};

			IntentFilter filter = new IntentFilter(BluetoothAdapter.ACTION_STATE_CHANGED);
			filter.addAction(BluetoothDevice.ACTION_FOUND);
			filter.addAction(BluetoothAdapter.ACTION_DISCOVERY_STARTED);
			filter.addAction(BluetoothAdapter.ACTION_DISCOVERY_FINISHED);
			
			m_context.registerReceiver(bluetoothBroadcastReceiver, filter,
					null, m_handler);

			try {
				findDevice();
	
				Looper.loop();
			} catch (SyncError ex) {
				Timber.e(ex, "Failed to start bluetooth discovery");
			}
			m_context.unregisterReceiver(bluetoothBroadcastReceiver);
			Timber.d("Bluetooth scan thread exiting.");
			
			return m_device;
		}

		@Override
		public void abort()
		{
			m_handler.getLooper().quit();
		}

		private void findDevice() throws SyncError
		{
			Timber.d("Scanning for bluetooth device %s", m_deviceName);
            // If we're already discovering, stop it
            if (m_adapter.isDiscovering()) {
            	m_adapter.cancelDiscovery();
            }
            
            startDiscovery();
        }
		
		private void startDiscovery() throws SyncError
		{
            m_isDiscovering = true;
            ++m_numDiscoveryStartAttempts;
			Timber.d("Attempt start discovery %d", m_numDiscoveryStartAttempts);
            boolean discoveryStarted = m_adapter.startDiscovery();
            if (!discoveryStarted) {
            	throw new SyncError();
            }
            
            // If discovery doesn't finish after 30 seconds then
            // Bluetooth is hosed and we need to bail
        	m_handler.postDelayed(new Runnable() {
        		private final int startNumber = m_numDiscoveryStartAttempts;
                public void run() {
        			if (m_numDiscoveryFinishes < startNumber) {
						Timber.e("Didn't receive bluetooth scan complete message within 30 seconds of starting scan. Start number %d completions %d", startNumber, m_numDiscoveryFinishes);
        				abort();
        			}
                }
        	}, 30 * 1000);
		}		
	}

	/**
	 * Task for connecting to BluetoothSocket
	 * 
	 * Have to connect to the socket in a background thread so that we can put
	 * a timeout on it. The connect call is blocking and has no timeout.
	 */
	private class ConnectTask implements CancelableTaskRunner.CancelableTask<Void> {

		private final BluetoothSocket socket;

		ConnectTask(BluetoothSocket socket) {
			this.socket = socket;
		}

		@Override
		public Void run() {

			// Connect with retries to give some extra time in case
			// the client starts before server.
			long startTime = SystemClock.elapsedRealtime();
			while (!Thread.currentThread().isInterrupted()) {
				try {
					// This is a blocking call and will only return on a successful connection
					socket.connect();
					break;
				} catch (IOException e) {
					long currentTime = SystemClock.elapsedRealtime();
					long elapsed = currentTime - startTime;
					if (elapsed > 30000)
						break;
					Timber.d(e, "Failed to connect after %d ms, retrying...", elapsed);
					try {
						Thread.sleep(1000);
					} catch (InterruptedException e1) {
						break;
					}
				}
			}
			return null;
		}

		@Override
		public void abort() {
			// Closing socket will force exception in socket.connect
			// in run causing it to abort
	    	try {
				socket.close();
			} catch (IOException ignored) {
			} 
		}
	}
	
	private void connectToSocket(BluetoothSocket socket) throws IOException 
	{
		try {
			cancelableTaskRunner.run(Executors.newSingleThreadExecutor(),
					new ConnectTask(socket), cancelChecker, 
					60, TimeUnit.SECONDS);
		} catch (ExecutionException e) {
			throw new IOException(e);
		}
		
		if (!isCancelled() && !socket.isConnected()) {
			throw new IOException("Timeout connecting to socket");
		}
	}

	private BluetoothDevice scanForDevice(String serverDeviceName) throws IOException
	{
		// Need to device scanning in a separate thread so we can use
		// a broadcast receiver and wait synchronously.
		try {
			return cancelableTaskRunner.run(Executors.newSingleThreadExecutor(),
					new ScanTask(serverDeviceName), cancelChecker, 
					60, TimeUnit.SECONDS);
		} catch (ExecutionException e) {
			throw new IOException(e);
		}		
	}
	
	@SuppressWarnings("unused")
	public BluetoothSocket acceptConnection(UUID serviceUUID) {
				
		if ( makeBluetoothDiscoverable() == 0) {
			Timber.d("Failed to make bluetooth adapter discoverable. Will not be able to connect");
			return null;
		}

	    final BluetoothAdapter adapter = BluetoothAdapter.getDefaultAdapter();

		Timber.d("Waiting for connections");
		BluetoothSocket clientSocket = waitForClientConnection(serviceUUID);
		if (clientSocket == null)
			Timber.d("Connection cancelled");
		else
			Timber.d("Got connection from: %s", clientSocket.getRemoteDevice().getName());
		return clientSocket;
	}

	private static long makeBluetoothDiscoverable()
	{
		return Messenger.getInstance().runLongEngineFunction(activity -> {
			// Need to make bluetooth discoverable to allow clients to connect
			// to server. This will bring up a dialog to ask user to make
			// discoverable, Android doesn't let us do it directly.
			Intent discoverableIntent = new Intent(BluetoothAdapter.ACTION_REQUEST_DISCOVERABLE);
			discoverableIntent.putExtra(BluetoothAdapter.EXTRA_DISCOVERABLE_DURATION, 60);
			activity.startActivityForResult(discoverableIntent, EntryActivity.BluetoothDiscoverableCode);
		});
	}

	private BluetoothSocket waitForClientConnection(UUID serviceUUID)
	{
		// Normally we could just call BluetoothServerSocket.accept() to wait for a client
		// connection but that is a synchronous call which prevents us being
		// able to cancel. Instead we call accept in a secondary thread.
		try {
			return cancelableTaskRunner.run(Executors.newSingleThreadExecutor(),
					new AcceptTask(serviceUUID), cancelChecker, 
					-1, null);
		} catch (ExecutionException e) {
			return null;
		}
	}

	private native boolean isCancelled();
	
	private class AcceptTask implements CancelableTaskRunner.CancelableTask<BluetoothSocket> {

		private BluetoothServerSocket serverSocket = null;
		private BluetoothSocket clientSocket = null;
		private final UUID serviceUUID;
		
		AcceptTask(UUID serviceUUID)
		{
			this.serviceUUID = serviceUUID;
		}
				
		@Override
		public BluetoothSocket run() {
			try {
				serverSocket = BluetoothAdapter.getDefaultAdapter().listenUsingRfcommWithServiceRecord(
						"CSPro Smart Sync", serviceUUID);
				clientSocket = serverSocket.accept();
			} catch (IOException e) {
				Timber.d(e, "Error accepting bluetooth connection");
			} finally {
				closeServerSocket();				
			}
			return clientSocket;
		}

		@Override
		public void abort() {
			closeServerSocket();			
		}
		
		private void closeServerSocket()
		{
			synchronized(this) {
				if (serverSocket != null) {
					try {
						serverSocket.close();
						serverSocket = null;
					} catch (IOException ignored) {
					}
				}
			}			
		}
	}

	private void setBluetoothName(String sNewName, Activity m_context) {
        final BluetoothAdapter myBTAdapter = BluetoothAdapter.getDefaultAdapter();
        final long lTimeToGiveUp_ms = System.currentTimeMillis() + m_bluetooth_name_change_timeout;

        if (myBTAdapter == null) {
            Messenger.getInstance().engineFunctionComplete(m_context.getString(R.string.bluetooth_name_change_device_not_supported));
            return;
        }

        //Checking if the name is not the same as the old one
        String sOldName = myBTAdapter.getName();
        if (sOldName.equalsIgnoreCase(sNewName)) {
            Messenger.getInstance().engineFunctionComplete(null);
            return;
        }

        final Handler myTimerHandler = new Handler(Looper.getMainLooper());
        myBTAdapter.enable();
        FragmentManager fm = m_context.getFragmentManager();
        MySpinnerDialog myInstance = new MySpinnerDialog();
        myInstance.show(fm, "bt_name_change_tag");

        final int delayTick = 500;
        myTimerHandler.postDelayed(
            new Runnable() {
                @Override
                public void run() {
                    if (myBTAdapter.isEnabled()) {
                        myBTAdapter.setName(sNewName);
                        if (sNewName.equalsIgnoreCase(myBTAdapter.getName())) {
                            myInstance.dismiss();
                            Messenger.getInstance().engineFunctionComplete(null);
                        }
                    }
                    if (!sNewName.equalsIgnoreCase(myBTAdapter.getName())) {
                        if (System.currentTimeMillis() < lTimeToGiveUp_ms) {
                            myTimerHandler.postDelayed(this, delayTick);
                            if (!myBTAdapter.isEnabled()) {
                                //Log.i(TAG_MODULE, "Update BT Name: waiting on BT Enable");
                            } else {
                                //Log.i(TAG_MODULE, "Update BT Name: waiting for Name (" + sNewName + ") to set in");
                            }
                        } else { //timeout...
                            Messenger.getInstance().engineFunctionComplete(m_context.getString(R.string.bluetooth_name_change_error) + sNewName);
                        }
                    }
                }
            }, delayTick);
    }

    public static class MySpinnerDialog extends DialogFragment {

        public MySpinnerDialog() {
            // use empty constructors. If something is needed use onCreate's
        }

        @Override
        public Dialog onCreateDialog(final Bundle savedInstanceState) {

            ProgressDialog _dialog = new ProgressDialog(getActivity());
            this.setStyle(STYLE_NO_TITLE, getTheme()); // You can use styles or inflate a view
            _dialog.setMessage(getString(R.string.bluetooth_name_change_wait_text)); // set your messages if not inflated from XML

            _dialog.setCancelable(false);

            return _dialog;
        }
    }

}
