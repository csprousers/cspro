package gov.census.cspro.csentry.ui

import android.app.AlertDialog
import android.bluetooth.BluetoothAdapter
import android.bluetooth.BluetoothClass
import android.bluetooth.BluetoothDevice
import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent
import android.content.IntentFilter
import android.os.Bundle
import android.os.Handler
import android.widget.ArrayAdapter
import androidx.appcompat.app.AppCompatActivity
import gov.census.cspro.csentry.R
import timber.log.Timber

/**
 * Shows a dialog listing detected bluetooth devices and
 * when user chooses one sends the address of the device back
 * in the intent.
 */
class ChooseBluetoothDeviceActivity : AppCompatActivity() {

    // Wrapper for BluetoothDevice to use in array adapter
    // that shows device name in list.
    private inner class DeviceListItem(val m_device: BluetoothDevice) {
        override fun toString(): String {
            return m_device.name
        }
    }

    private lateinit var m_adapter: BluetoothAdapter
    private var m_deviceListDialog: AlertDialog? = null
    private lateinit var m_devicesArrayAdapter: ArrayAdapter<DeviceListItem>
    private val m_handler = Handler()
    private var m_scanning = false

    override fun onCreate(savedInstanceState: Bundle?) {

        super.onCreate(savedInstanceState)
        m_adapter = BluetoothAdapter.getDefaultAdapter()
        m_devicesArrayAdapter = ArrayAdapter(this, android.R.layout.select_dialog_item)
        val builder = AlertDialog.Builder(this)
        val customTitleView = layoutInflater.inflate(R.layout.choose_bluetooth_title, null)
        builder.setCustomTitle(customTitleView)
        builder.setTitle(R.string.sync_choose_bluetooth_title)
        builder.setAdapter(m_devicesArrayAdapter) { _, which ->
            val selectedItem = m_devicesArrayAdapter.getItem(which)
            unregisterReceiver(m_receiver)
            m_receiver = null

            // Cancel discovery because it's costly and we're about to connect
            stopDiscovery()
            val name = selectedItem?.m_device
            val address = selectedItem?.m_device?.address

            // Create the result Intent and include the MAC address
            val intent = Intent()
            intent.putExtra(EXTRA_DEVICE_NAME, name)
            intent.putExtra(EXTRA_DEVICE_ADDRESS, address)

            // Set result and finish this Activity
            setResult(RESULT_OK, intent)
            finish()
        }
        builder.setNegativeButton(getString(R.string.modal_dialog_helper_cancel_text)
        ) { _, _ ->
            unregisterReceiver(m_receiver)
            m_receiver = null
            setResult(RESULT_CANCELED)
            finish()
        }
        builder.setOnCancelListener {
            unregisterReceiver(m_receiver)
            m_receiver = null
            setResult(RESULT_CANCELED)
            finish()
        }
        m_deviceListDialog = builder.create()

        // Register for broadcasts when a device is discovered
        var filter = IntentFilter(BluetoothDevice.ACTION_FOUND)
        registerReceiver(m_receiver, filter)

        // Register for broadcasts when discovery has started/finished
        filter = IntentFilter(BluetoothAdapter.ACTION_DISCOVERY_FINISHED)
        registerReceiver(m_receiver, filter)
        filter = IntentFilter(BluetoothAdapter.ACTION_DISCOVERY_STARTED)
        registerReceiver(m_receiver, filter)

        // Register for broadcasts when bluetooth enable/disable state changes
        filter = IntentFilter(BluetoothAdapter.ACTION_STATE_CHANGED)
        registerReceiver(m_receiver, filter)
        m_deviceListDialog?.show()
    }

    override fun onStart() {
        super.onStart()

        // If bluetooth is already on then start discovery right away,
        // otherwise enable bluetooth and wait for notification that it is on
        // to start discovery
        if (m_adapter.isEnabled) doDiscovery() else {
            m_adapter.enable()
        }
    }

    override fun onStop() {
        stopDiscovery()
        super.onStop()
    }

    override fun onDestroy() {
        super.onDestroy()

        // Make sure we're not doing discovery anymore
        stopDiscovery()

        // Unregister broadcast listeners
        if (m_receiver != null) {
            unregisterReceiver(m_receiver)
            m_receiver = null
        }
    }

    private fun doDiscovery() {
        Timber.d("Scanning for bluetooth devices...")

        // If we're already discovering, stop it
        stopDiscovery()

        // Request discover from BluetoothAdapter
        m_adapter.startDiscovery()
    }

    private fun stopDiscovery() {
        m_scanning = false
        if (m_adapter != null && m_adapter.isDiscovering) {
            Timber.d("Canceling bluetooth discovery")
            m_adapter.cancelDiscovery()
        }
    }

    // The BroadcastReceiver that listens for discovered devices and
    // restarts discovery if nothing is picked
    private var m_receiver: BroadcastReceiver? = object : BroadcastReceiver() {
        override fun onReceive(context: Context, intent: Intent) {
            val action = intent.action

            // When bluetooth is turned on
            if (BluetoothAdapter.ACTION_STATE_CHANGED == action) {
                if (intent.extras?.getInt(BluetoothAdapter.EXTRA_STATE) == BluetoothAdapter.STATE_ON) {
                    Timber.d("Bluetooth is now enabled")
                    // Bluetooth is enabled now, can start scan
                    doDiscovery()
                }
                // When discovery finds a device
            } else if (BluetoothDevice.ACTION_FOUND == action) {

                // Get the BluetoothDevice object from the Intent
                val device = intent.getParcelableExtra<BluetoothDevice>(BluetoothDevice.EXTRA_DEVICE)
                Timber.d("Found device: %s", if (device == null || device.name == null) "null" else device.name)
                if (device != null && device.name != null && device.address != null && device.bluetoothClass != null) {
                    val bluetoothMajorClass = device.bluetoothClass.majorDeviceClass
                    Timber.d("Found bluetooth device name=" + device.name +
                        " class major=" + bluetoothMajorClass +
                        " class minor=" + device.bluetoothClass.deviceClass)
                    // Other android devices seem to come across as either computer or phone so we
                    // can filter out other stuff like headsets and health devices.
                    if (bluetoothMajorClass == BluetoothClass.Device.Major.COMPUTER ||
                        bluetoothMajorClass == BluetoothClass.Device.Major.PHONE) {
                        // If it's already in the list then ignore it
                        var found = false
                        for (i in 0 until m_devicesArrayAdapter.count) {
                            if (m_devicesArrayAdapter.getItem(i)?.m_device?.address == device.address) {
                                found = true
                                break
                            }
                        }
                        if (!found) {
                            m_devicesArrayAdapter.add(DeviceListItem(device))
                        }
                    }
                }
            } else if (BluetoothAdapter.ACTION_DISCOVERY_STARTED == action) {
                m_scanning = true
                Timber.d("Bluetooth discovery started")
            } else if (BluetoothAdapter.ACTION_DISCOVERY_FINISHED == action) {
                // When discovery is finished, start again in case new ones are added but wait 2 seconds
                // since discovery is resource intensive
                if (m_scanning) {
                    Timber.d("Bluetooth discovery finished, restarting in 2 seconds.")
                    m_handler.postDelayed({ doDiscovery() }, 2000)
                }
            }
        }
    }

    companion object {
        // Return Intent extra
        var EXTRA_DEVICE_NAME = "device_name"
        var EXTRA_DEVICE_ADDRESS = "device_address"
    }
}