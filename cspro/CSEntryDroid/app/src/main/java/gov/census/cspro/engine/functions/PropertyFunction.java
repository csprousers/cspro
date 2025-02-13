package gov.census.cspro.engine.functions;

import android.app.Activity;
import android.content.Context;
import android.content.SharedPreferences;

import gov.census.cspro.csentry.R;
import gov.census.cspro.csentry.ui.EntryActivity;
import gov.census.cspro.engine.EngineInterface;
import gov.census.cspro.engine.Messenger;


public class PropertyFunction implements EngineFunction
{
    private enum Parameter
    {
        ShowLabelsInCaseTree,
        ShowNavigationControls,
        ShowSkippedFields,
        WindowTitle
    }

	private boolean m_get;
	private Parameter m_parameter;
	private String m_value;

    public PropertyFunction(boolean get,String parameter,String value)
    {
		m_get = get;
		m_value = value;

		// set a default value, though this will never get used because invalid
		// parameters are stopped in the C++ code
        m_parameter = Parameter.WindowTitle;

        for( Parameter param : Parameter.values() )
        {
            if( param.name().equals(parameter) )
            {
                m_parameter = param;
                break;
            }
        }
    }

    private static final String YesValue = "Yes";
    private static final String NoValue = "No";

    private String booleanToString(boolean value)
    {
        return value ? YesValue : NoValue;
    }

    private boolean stringToBoolean(String value) throws Exception
    {
        if( value.equalsIgnoreCase(YesValue) )
            return true;

        else if( value.equalsIgnoreCase(NoValue) )
            return false;

        else
            // this shouldn't happen because invalid values are stopped in the C++ code
            throw new Exception();
    }

    private int parameterToResourceId()
    {
        switch( m_parameter )
        {
            case ShowLabelsInCaseTree:
                return R.string.menu_casetree_save_show_labels_state;

            case ShowNavigationControls:
                return R.string.menu_questionnaire_save_nav_control_state;

            case ShowSkippedFields:
            default:
                return R.string.menu_casetree_save_show_skipped_state;
        }
    }

    private boolean getSharedPreference(Activity activity)
    {
        int resId = parameterToResourceId();
        boolean defaultValue = ( resId == R.string.menu_casetree_save_show_skipped_state ) ? false : true;
        SharedPreferences sharedPreferences = activity.getPreferences(Context.MODE_PRIVATE);
        return sharedPreferences.getBoolean(activity.getString(resId),defaultValue);
    }

    public void runEngineFunction(Activity activity)
    {
		String value = null;

		try
		{
            if( m_get )
			{
                switch( m_parameter )
                {
                    case ShowLabelsInCaseTree:
                    case ShowNavigationControls:
                    case ShowSkippedFields:
                        value = booleanToString(getSharedPreference(activity));
                        break;

                    case WindowTitle:
                    {
                        EngineInterface appiface = EngineInterface.getInstance();
                        value = appiface.getWindowTitle();
                        break;
                    }
                }
			}

			else // put
			{
                EntryActivity entryActivity = (EntryActivity)activity;

                switch( m_parameter )
                {
                    case ShowLabelsInCaseTree:
                    case ShowNavigationControls:
                    case ShowSkippedFields:
                    {
                        boolean currentValue = getSharedPreference(activity);
                        boolean newValue = stringToBoolean(m_value);

                        if( currentValue != newValue )
                        {
                            if( m_parameter == Parameter.ShowLabelsInCaseTree )
                                entryActivity.ShowLabels();

                            else if( m_parameter == Parameter.ShowNavigationControls )
                                entryActivity.toggleNavigationControls();

                            else if( m_parameter == Parameter.ShowSkippedFields )
                                entryActivity.ShowOrHideSkippedFields();
                        }

                        break;
					}                        

                    case WindowTitle:
                    {
                        EngineInterface appiface = EngineInterface.getInstance();
                        appiface.setWindowTitle(m_value);
                        entryActivity.updateWindowTitle();
                        break;
                    }
                }
			}            
        }
		
		catch( Exception exception )
		{
		}
		
		Messenger.getInstance().engineFunctionComplete(value);
    }
}
