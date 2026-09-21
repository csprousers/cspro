package gov.census.cspro.util;

import android.content.Context;
import android.content.SharedPreferences;

import androidx.security.crypto.EncryptedSharedPreferences;
import androidx.security.crypto.MasterKeys;

import java.io.IOException;
import java.security.GeneralSecurityException;

import gov.census.cspro.csentry.R;
import timber.log.Timber;

/**
 * Credential storage using shared preferences
*/
public class CredentialStore {

    private final Context m_context;
    private final String m_fileName;
    private SharedPreferences m_preferences;

    public CredentialStore(Context context)
    {
        m_context = context.getApplicationContext();
        m_fileName = m_context.getString(R.string.preferences_file_credentials);

        m_preferences = createEncryptedPrefs();

        if (m_preferences == null) {
            // Keystore/prefs file may be corrupted (backup restore, OS upgrade): reset once and retry
            m_context.deleteSharedPreferences(m_fileName);
            m_preferences = createEncryptedPrefs();
        }

        if (m_preferences == null) {
            createPlainSharedPrefs();
        }
    }

    private SharedPreferences createEncryptedPrefs() {
        try {
            String masterKeyAlias = MasterKeys.getOrCreate(MasterKeys.AES256_GCM_SPEC);
            return EncryptedSharedPreferences.create(
                m_fileName,
                masterKeyAlias,
                m_context,
                EncryptedSharedPreferences.PrefKeyEncryptionScheme.AES256_SIV,
                EncryptedSharedPreferences.PrefValueEncryptionScheme.AES256_GCM
            );
        } catch (GeneralSecurityException | IOException | RuntimeException e) {
            Timber.e(e, "Error creating encrypted credential store");
            return null;
        }
    }

    // Separate file so plain entries never mix with the encrypted file
    private void createPlainSharedPrefs() {
        try {
            m_preferences = m_context.getSharedPreferences(m_fileName + "_fallback", Context.MODE_PRIVATE);
        } catch (RuntimeException e) {
            Timber.e(e, "Error creating fallback shared prefs");
        }
    }

    public void Store(String attribute, String secret_value)
    {
        SharedPreferences.Editor editor = m_preferences.edit();
        editor.putString(attribute, secret_value);
        editor.commit();
    }

    public String Retrieve(String attribute)
    {
        try {
            return m_preferences.getString(attribute, null);
        } catch (SecurityException ex) {
            createPlainSharedPrefs();
            return m_preferences.getString(attribute, null);
        }
    }

    public int GetNumberCredentials()
    {
        try {
            return m_preferences.getAll().size();
        } catch (SecurityException ex) {
            createPlainSharedPrefs();
            return m_preferences.getAll().size();
        }
    }

    public void Clear()
    {
        SharedPreferences.Editor editor = m_preferences.edit();
        editor.clear();
        editor.commit();
    }
}
