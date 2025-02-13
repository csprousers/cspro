package gov.census.cspro.csentry

import android.app.AlertDialog
import android.os.Bundle
import android.preference.Preference
import android.preference.PreferenceActivity
import android.preference.PreferenceFragment
import android.preference.PreferenceScreen
import android.widget.Toast
import gov.census.cspro.engine.EngineInterface
import gov.census.cspro.util.CredentialStore

class SettingsActivity : PreferenceActivity() {

    @Deprecated("Deprecated in Java")
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        fragmentManager
            .beginTransaction()
            .replace(android.R.id.content, SettingsFragment())
            .commit()
    }

    class SettingsFragment : PreferenceFragment() {
        @Deprecated("Deprecated in Java")
        public override fun onCreate(savedInstanceState: Bundle?) {
            super.onCreate(savedInstanceState)
            preferenceManager.sharedPreferencesName = getString(R.string.preferences_file_global)
            addPreferencesFromResource(R.xml.settings)
            if (!EngineInterface.GetSystemSettingBoolean(SystemSettings.MenuShowHiddenApplications, true)) getPreferenceScreen().removePreference(findPreference(getString(R.string.preferences_show_hidden_applications)))
        }

        @Deprecated("Deprecated in Java")
        public override fun onPreferenceTreeClick(preferenceScreen: PreferenceScreen, preference: Preference): Boolean {
            val key: String = preference.getKey()
            if ((key == getString(R.string.preferences_clear_credentials))) {
                clearCredentials()
                return true
            }
            return false
        }

        private fun clearCredentials() {
            val credentialStore = CredentialStore(activity)
            val numberCredentials: Int = credentialStore.GetNumberCredentials()
            if (numberCredentials == 0) {
                val message: String = EngineInterface.GetRuntimeString(94331,
                    "There are no saved credentials")
                Toast.makeText(activity, message, Toast.LENGTH_LONG).show()
            } else {
                val formatter: String = EngineInterface.GetRuntimeString(94332,
                    "Are you sure that you want to delete %d credential(s)?")
                val message: String = String.format(formatter, numberCredentials)
                AlertDialog.Builder(activity)
                    .setMessage(message)
                    .setIcon(android.R.drawable.ic_dialog_alert)
                    .setPositiveButton(android.R.string.yes) { dialog, whichButton -> credentialStore.Clear() }
                    .setNegativeButton(android.R.string.no, null)
                    .show()
            }
        }
    }
}