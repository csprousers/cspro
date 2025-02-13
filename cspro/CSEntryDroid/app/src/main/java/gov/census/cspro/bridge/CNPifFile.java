package gov.census.cspro.bridge;

import android.os.Bundle;

import java.util.ArrayList;

import gov.census.cspro.engine.Util;

public class CNPifFile
{
	public enum ShowInApplicationListing { Always, Hidden, Never };
	
	private boolean valid;
	private String description;
	private boolean entryAppType;
	private ShowInApplicationListing showInApplicationListing;
	private String inputFilename;
	private String appFilename;
	private String[] externalFilenames;
	private String[] userFilenames;
	private String writeFilename;
	private String onExitFilename;
	private String pffDirectory;
	
	protected native long LoadPif(String filename);
	protected native String GetDescription(long reference);
	protected native boolean IsAppTypeEntry(long reference);
	protected native int GetShowInApplicationListing(long reference);
	protected native String GetInputFilename(long reference);
	protected native String GetAppFilename(long reference);
	protected native void ClosePif(long reference);
	protected native String[] GetExternalFilenames(long reference);
	protected native String[] GetUserFilenames(long reference);
	protected native String GetWriteFilename(long reference);
	protected native String GetOnExitFilename(long reference);
	
	public CNPifFile(String filename)
	{
		long reference = LoadPif(filename);
		
		valid = ( reference != 0 );
		
		if( valid )
		{
			pffDirectory = Util.removeFilename(filename);
			description = GetDescription(reference);
            entryAppType = IsAppTypeEntry(reference);
			showInApplicationListing = ShowInApplicationListing.values()[GetShowInApplicationListing(reference)];
			inputFilename = GetInputFilename(reference);
			appFilename = GetAppFilename(reference);
			externalFilenames = GetExternalFilenames(reference);
			userFilenames = GetUserFilenames(reference);
			writeFilename = GetWriteFilename(reference);
			onExitFilename = GetOnExitFilename(reference);
			ClosePif(reference);
		}
	}
	
	public boolean IsValid()
	{
		return valid;
	}
	
	public String GetDescription()
	{
		return description;		
	}

	public Boolean IsAppTypeEntry()
	{
		return entryAppType;
	}

	public boolean ShouldShowInApplicationListing(boolean showHiddenApplications)
	{
		// if the description is empty then we won't display it in the Applications Listing screen
		return ( description.trim().length() > 0 ) && ( ( showInApplicationListing == ShowInApplicationListing.Always ) ||
				( showHiddenApplications && ( showInApplicationListing == ShowInApplicationListing.Hidden ) ) );		
	}
	
	public String GetInputFilename()
	{
		return inputFilename;		
	}
	
	public String GetAppFilename()
	{
		return appFilename;
	}
	
	public String[] GetExternalFilenames()
	{
		return externalFilenames;
	}

	public String[] GetUserFilenames()
	{
		return userFilenames;
	}

	public String GetWriteFilename()
	{
		return writeFilename;
	}
	
	public String GetOnExitFilename()
	{
		return onExitFilename;
	}

    public native static String CreatePffFromIntentExtras(String pff_file, Bundle bundle_extras);
}
