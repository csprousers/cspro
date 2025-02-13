package gov.census.cspro.util;

import android.content.Context;
import android.content.SharedPreferences;
import android.os.Build;

import androidx.security.crypto.EncryptedSharedPreferences;
import androidx.security.crypto.MasterKeys;

import java.io.IOException;
import java.security.GeneralSecurityException;

import gov.census.cspro.csentry.CSEntry;
import gov.census.cspro.csentry.R;
import timber.log.Timber;

/**
 * Credential storage using shared preferences
*/
public class CredentialStore {

	private SharedPreferences m_preferences;
	
	public CredentialStore(Context context)
    {
        String masterKeyAlias = null;
        if (Build.VERSION.SDK_INT < 23) {
            createSharedPrefsForOlderDevice(context);
        } else {
            try {
                masterKeyAlias = MasterKeys.getOrCreate(MasterKeys.AES256_GCM_SPEC);

                m_preferences = EncryptedSharedPreferences.create(
                    context.getString(R.string.preferences_file_credentials),
                    masterKeyAlias,
                    context,
                    EncryptedSharedPreferences.PrefKeyEncryptionScheme.AES256_SIV,
                    EncryptedSharedPreferences.PrefValueEncryptionScheme.AES256_GCM
                );
            } catch (GeneralSecurityException e)
            {
                Timber.e(e, "Error reading Credential");
            } catch (IOException e)
            {
                Timber.e(e, "Error reading Credential");
            } catch (NullPointerException e)
            {
                if(masterKeyAlias == null) {
                    Timber.e(e, "masterKeyAlias null");
                    createSharedPrefsForOlderDevice(context);
                }
            }
        }
    }

    private void createSharedPrefsForOlderDevice(Context context) {
        m_preferences = context.getSharedPreferences(context.getString(R.string.preferences_file_credentials), Context.MODE_PRIVATE);
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
		return m_preferences.getString(attribute, null);// needed?
        } catch (SecurityException ex) {
            if (Build.VERSION.SDK_INT > 22) {
                m_preferences = CSEntry.Companion.getContext().getSharedPreferences(CSEntry.Companion.getContext().getString(R.string.preferences_file_credentials), Context.MODE_PRIVATE);
                return m_preferences.getString(attribute, null);
            }
        }
        return m_preferences.getString(attribute, null);
	}

	public int GetNumberCredentials()
	{
	    try {
            return m_preferences.getAll().size();
        } catch (SecurityException ex) {
            if (Build.VERSION.SDK_INT > 22) {
                m_preferences = CSEntry.Companion.getContext().getSharedPreferences(CSEntry.Companion.getContext().getString(R.string.preferences_file_credentials), Context.MODE_PRIVATE);
                return m_preferences.getAll().size();
            }
        }
        return -1;
	}

	public void Clear()
    {
        SharedPreferences.Editor editor = m_preferences.edit();
        editor.clear();
        editor.commit();
    }
}
