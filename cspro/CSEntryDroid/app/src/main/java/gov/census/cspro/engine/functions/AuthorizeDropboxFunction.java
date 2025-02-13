package gov.census.cspro.engine.functions;

import android.app.Activity;
import gov.census.cspro.csentry.R;
import gov.census.cspro.csentry.ui.EntryActivity;
import com.dropbox.core.android.Auth;
import gov.census.cspro.util.DbxRequestConfigFactory;

public class AuthorizeDropboxFunction implements EngineFunction {
    private static boolean authenticationStarted  = false;
	@Override
	public void runEngineFunction(Activity activity) {
        authenticationStarted = true;
        Auth.startOAuth2PKCE(activity, activity.getString(R.string.dropbox_key), DbxRequestConfigFactory.getRequestConfig());
	}
    public static boolean isAuthenticating()
    {
        return authenticationStarted;
    }

    public static void setAuthenticationComplete()
    {
        authenticationStarted  = false;
    }

}
