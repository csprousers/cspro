package gov.census.cspro.csentry

import android.app.AlertDialog
import android.os.Bundle
import android.widget.Toast
import androidx.activity.enableEdgeToEdge
import androidx.appcompat.app.AppCompatActivity
import androidx.preference.Preference
import androidx.preference.PreferenceFragmentCompat
import gov.census.cspro.engine.EngineInterface
import gov.census.cspro.util.CredentialStore
import gov.census.cspro.util.EdgeToEdgeUtils

class SettingsActivity : AppCompatActivity() {

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        // Enable edge-to-edge
        EdgeToEdgeUtils.setupEdgeToEdge(this, R.layout.activity_settings)

        // Replace with modern fragment transaction
        if (savedInstanceState == null) {
            supportFragmentManager
                .beginTransaction()
                .replace(R.id.settings_container, SettingsFragment())
                .commit()
        }
    }

    class SettingsFragment : PreferenceFragmentCompat() {

        override fun onCreatePreferences(savedInstanceState: Bundle?, rootKey: String?) {
            // Set the shared preferences file name
            preferenceManager.sharedPreferencesName = getString(R.string.preferences_file_global)

            // Load preferences from XML
            setPreferencesFromResource(R.xml.settings, rootKey)

            // Remove hidden applications preference if not enabled in system settings
            if (!EngineInterface.GetSystemSettingBoolean(SystemSettings.MenuShowHiddenApplications, true)) {
                val hiddenAppsPreference = findPreference<Preference>(getString(R.string.preferences_show_hidden_applications))
                hiddenAppsPreference?.let {
                    preferenceScreen.removePreference(it)
                }
            }
        }

        override fun onPreferenceTreeClick(preference: Preference): Boolean {
            when (preference.key) {
                getString(R.string.preferences_clear_credentials) -> {
                    clearCredentials()
                    return true
                }
            }
            return super.onPreferenceTreeClick(preference)
        }

        private fun clearCredentials() {
            val credentialStore = CredentialStore(requireActivity())
            val numberCredentials: Int = credentialStore.GetNumberCredentials()

            if (numberCredentials == 0) {
                val message: String = EngineInterface.GetRuntimeString(94331,
                    "There are no saved credentials")
                Toast.makeText(requireContext(), message, Toast.LENGTH_LONG).show()
            } else {
                val formatter: String = EngineInterface.GetRuntimeString(94332,
                    "Are you sure that you want to delete %d credential(s)?")
                val message: String = String.format(formatter, numberCredentials)

                AlertDialog.Builder(requireContext())
                    .setMessage(message)
                    .setIcon(android.R.drawable.ic_dialog_alert)
                    .setPositiveButton(android.R.string.yes) { _, _ ->
                        credentialStore.Clear()
                    }
                    .setNegativeButton(android.R.string.no, null)
                    .show()
            }
        }
    }
}