package gov.census.cspro.csentry

import android.Manifest
import android.app.AlertDialog
import android.content.Intent
import android.content.SharedPreferences
import android.content.pm.PackageManager
import android.net.Uri
import android.os.Build
import android.os.Bundle
import android.view.Menu
import android.view.MenuItem
import androidx.appcompat.app.AppCompatActivity
import androidx.core.app.ActivityCompat
import gov.census.cspro.csentry.ui.ServiceTermsActivity
import gov.census.cspro.engine.EngineInterface
import gov.census.cspro.engine.EngineMessage
import gov.census.cspro.engine.IEngineMessageCompletedListener
import gov.census.cspro.smartsync.addapp.AddApplicationActivity
import gov.census.cspro.smartsync.addapp.UpdateApplicationsActivity
import java.io.File

class ApplicationsListActivity constructor() : AppCompatActivity(), IEngineMessageCompletedListener {
    private var m_appsFragment: ApplicationsFragment? = null

    // Whether or not to automatically launch the application if there is
    // only one installed application.
    private var m_launchSingleEntryApp: Boolean = true

    // If already asked user to grant system permissions
    private var m_alreadyAskedPermissions: Boolean = false

    //Update this list for Android 6.0 and above if new permissions are added. 
    //These permissions are checked only if the API Level of the device > 23 (Marshmallow and above)
    //Any device < Android 6.0 will have permissions available on install
    private val PERMISSIONS: Array<String> = arrayOf(Manifest.permission.READ_EXTERNAL_STORAGE,
        Manifest.permission.WRITE_EXTERNAL_STORAGE,
        Manifest.permission.ACCESS_COARSE_LOCATION,
        Manifest.permission.ACCESS_FINE_LOCATION,
        Manifest.permission.GET_ACCOUNTS,
        Manifest.permission.CAMERA,
        Manifest.permission.RECORD_AUDIO,
        Manifest.permission.INTERNET,
        Manifest.permission.ACCESS_NETWORK_STATE,
        Manifest.permission.BLUETOOTH,
        Manifest.permission.BLUETOOTH_ADMIN,
        Manifest.permission.BLUETOOTH_SCAN,
        Manifest.permission.BLUETOOTH_CONNECT,
        Manifest.permission.BLUETOOTH_ADVERTISE)

    // Permissions without which app can't function at all.
    // With these at least we can do basic data entry even
    // if some functions won't work correctly.
    private val REQUIRED_PERMISSIONS: Array<String> = arrayOf(
        Manifest.permission.READ_EXTERNAL_STORAGE,
        Manifest.permission.WRITE_EXTERNAL_STORAGE)

    @Deprecated("Deprecated in Java")
    override fun onActivityResult(requestCode: Int, resultCode: Int, data: Intent?) {
        super.onActivityResult(requestCode, resultCode, data)
        if (requestCode == TERMS_ACTIVITY) {
            if (resultCode == RESULT_OK) {    // store the preferences to disk
                val termsDisplayedProperty: String = getString(R.string.terms_displayed_state)
                val sharedPref: SharedPreferences = getSharedPreferences(getString(R.string.preferences_file_global), MODE_PRIVATE)
                val editor: SharedPreferences.Editor = sharedPref.edit()
                editor.putBoolean(termsDisplayedProperty, true)
                editor.apply()

                // Don't auto launch the app on first install after terms of service
                m_launchSingleEntryApp = false
            } else {
                setResult(RESULT_CANCELED)
                finish()
            }
        }
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.applications_layout)
    }

    override fun onCreateOptionsMenu(menu: Menu): Boolean {
        menuInflater.inflate(R.menu.menu_applications_list, menu)
        return true
    }

    override fun onPrepareOptionsMenu(menu: Menu): Boolean {
        menu.findItem(R.id.menu_applications_add_application).isVisible = EngineInterface.GetSystemSettingBoolean(SystemSettings.MenuAddApplication, true)
        menu.findItem(R.id.menu_applications_update_applications).isVisible = EngineInterface.GetSystemSettingBoolean(SystemSettings.MenuAddApplication, true)
        menu.findItem(R.id.menu_applications_settings).isVisible = EngineInterface.GetSystemSettingBoolean(SystemSettings.MenuSettings, true)
        menu.findItem(R.id.menu_applications_list_help).isVisible = EngineInterface.GetSystemSettingBoolean(SystemSettings.MenuHelp, true)
        return true
    }

    public override fun onOptionsItemSelected(item: MenuItem): Boolean {
        when (item.getItemId()) {
            R.id.menu_applications_list_about -> {
                startActivity(Intent(this, AboutActivity::class.java))
                return true
            }
            R.id.menu_applications_list_help -> {
                SystemSettings.LaunchHelp(this)
                return true
            }
            R.id.menu_applications_add_application -> {
                startActivity(Intent(this, AddApplicationActivity::class.java))
                return true
            }
            R.id.menu_applications_update_applications -> {
                startActivity(Intent(this, UpdateApplicationsActivity::class.java))
                return true
            }
            R.id.menu_applications_settings -> {
                startActivity(Intent(this, SettingsActivity::class.java))
                return true
            }
            else -> return super.onOptionsItemSelected(item)
        }
    }

    override fun onResume() {
        super.onResume()

        // check to see if the terms of service were accepted before moving on
        val sharedPref: SharedPreferences = getSharedPreferences(getString(R.string.preferences_file_global), MODE_PRIVATE)
        val termsDisplayedProperty: String = getString(R.string.terms_displayed_state)
        val termsDisplayed: Boolean = sharedPref.getBoolean(termsDisplayedProperty, false)
        if (!termsDisplayed) {
            // start the terms of service GUI, we will come back to onResume after they have
            // been accepted.
            startActivityForResult(Intent(this, ServiceTermsActivity::class.java), TERMS_ACTIVITY)
        } else if (!checkPermissions()) {
            requestPermissions()
        } else {
            // If we got here then terms of service are accepted and permissions
            // are granted.

            // instantiate the application interface
            EngineInterface.CreateEngineInterfaceInstance(application)

            m_appsFragment?.displayApplications(sharedPref.getBoolean(getString(R.string.preferences_show_hidden_applications), false))

            // Only launch the single entry the first time
            // we startup, not when we come back to this activity from entry
            if (m_launchSingleEntryApp) {
                m_launchSingleEntryApp = false
                m_appsFragment?.launchSingleEntryApp()
            }
        }
    }

    fun openApplication(applicationFilename: String, appdesc: String, entryApp: Boolean) {
        if (entryApp) {
            val intent = Intent(this, CaseListActivity::class.java)
            intent.data = Uri.fromFile(File(applicationFilename))
            intent.putExtra(CaseListActivity.APP_DESCRIPTION, appdesc)
            startActivity(intent)
        } else {
            runNonEntryApplication(applicationFilename)
        }
    }

    fun setApplicationsFragment(frag: ApplicationsFragment?) {
        m_appsFragment = frag
    }

    public override fun onMessageCompleted(msg: EngineMessage) {}
    private fun checkPermissions(): Boolean {
        // If we have not already asked then we need to ask
        // if we don't already have all the permissions.
        // After the first time, we only ask if we don't have the
        // required ones.
        if (!m_alreadyAskedPermissions) return hasPermissions(*PERMISSIONS) else return hasPermissionsApi33(*REQUIRED_PERMISSIONS)
    }

    private fun hasPermissions(vararg permissions: String): Boolean {
        // Prior to Android 6.0 (Marshmallow), all permissions are granted on
        // app installation. On Marshmallow and above need to check the runtime permissions
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            for (permission: String in permissions) {
                if (ActivityCompat.checkSelfPermission(this, permission) != PackageManager.PERMISSION_GRANTED) {
                    return false
                }
            }
            return true
        } else {
            return true
        }
    }

    private fun hasPermissionsApi33(vararg permissions: String): Boolean {
        // Prior to Android 6.0 (Marshmallow), all permissions are granted on
        // app installation. On Marshmallow and above need to check the runtime permissions
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M &&  Build.VERSION.SDK_INT < Build.VERSION_CODES.TIRAMISU) {
            for (permission: String in permissions) {
                if (ActivityCompat.checkSelfPermission(this, permission) != PackageManager.PERMISSION_GRANTED) {
                    return false
                }
            }
            return true
        } else {
            return true
        }
    }

    private fun requestPermissions() {

        // First time we just show system permissions dialog
        if (!m_alreadyAskedPermissions) {
            m_alreadyAskedPermissions = true
            launchRequestPermissionsDialogs()
            return
        }

        // Second or later attempt: check the system shouldShowRationale settings for the permissions
        // to determine if we should explain why we need permissions
        var shouldShowRationale: Boolean = false
        for (permission: String in PERMISSIONS) {
            shouldShowRationale = shouldShowRationale or ActivityCompat.shouldShowRequestPermissionRationale(this, permission)
        }
        if (!shouldShowRationale) {
            // If we already asked and shouldShowRequestPermissionRationale returns false
            // that means they checked the "never show again" box on all permissions so we should abort.
            val builder: AlertDialog.Builder = AlertDialog.Builder(this)
            builder.setMessage(getString(R.string.abort_due_to_missing_permissions))
                .setTitle(getString(R.string.app_name))
                .setPositiveButton(getString(R.string.exit_application)) { _, _ -> finish() }
                .show()
            return
        }

        // Show the rationale and then let them try again or abort.
        val builder: AlertDialog.Builder = AlertDialog.Builder(this)
        builder.setMessage(getString(R.string.need_permissions_to_function))
            .setTitle(getString(R.string.app_name))
            .setPositiveButton(getString(R.string.try_again)) { _, _ -> launchRequestPermissionsDialogs() }
            .setNegativeButton(getString(R.string.exit_application)) { _, _ ->
                finish() // abort
            }
            .show()
    }

    private fun launchRequestPermissionsDialogs() {
        m_alreadyAskedPermissions = true

        //We are not using requestCode for the callback as we are requesting all the permissions 
        //in this single activity
        val requestCode: Int = 0
        ActivityCompat.requestPermissions(this, PERMISSIONS, requestCode)
    }

    private fun runNonEntryApplication(applicationFilename: String?) {
        val intent = Intent(this, NonEntryApplicationActivity::class.java)
        intent.data = Uri.fromFile(File(applicationFilename))
        startActivity(intent)
    }

    companion object {
        private val TERMS_ACTIVITY: Int = 10001
    }
}