package gov.census.cspro.engine;

// Java Imports

import android.app.Activity;
import android.content.Context;
import android.content.res.Resources;
import android.net.Uri;
import android.os.Build;
import android.util.SparseArray;
import android.view.View;
import android.view.ViewGroup;
import android.view.inputmethod.InputMethodManager;
import android.widget.TextView;

import androidx.core.content.FileProvider;

import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.ArrayList;
import java.util.Collection;
import java.util.Collections;
import java.util.List;
import java.util.Locale;

// Android Imports

public class Util
{

	public static String combinePath(String p1, String p2)
	{
		File f = new File(p1,p2);
		return f.toString();
	}

	public static int dpToPx(int dp, Resources resources)
	{
		float 	density = resources.getDisplayMetrics().density;
		return (int) (dp * density);
	}

	public static String padLeft(int stringWidth, char padChar, String inputString)
	{
		StringBuilder builder = new StringBuilder(inputString);

		if(inputString.length() < stringWidth)
		{
			int padwidth = stringWidth - inputString.length();

			for(int i = 0; i < padwidth; i++)
			{
				builder.insert(0, padChar);
			}
		}

		return builder.toString();
	}

	/**
	 * Strip filename from path leaving only directory.
	 *
	 * @param file Path from which to remove filename
	 * @return Path with filename removed
	 */
	public static String removeFilename(String file)
	{
		int slashPos = file.lastIndexOf(File.separator);

		// Handle path with trailing /
		if (slashPos == file.length() - 1) {
			if (slashPos != 0)
				slashPos = file.substring(0, file.length() - 1).lastIndexOf(File.separator);
		}

		if( slashPos != -1 )
			return file.substring(0,slashPos + 1);

		return file;
	}

	public static String removeDirectory(String path)
	{
		int slashPos = path.lastIndexOf(File.separator);

		if( slashPos != -1 )
			return path.substring(slashPos + 1);

		return path;
	}

	public static void hideInputMethod(Activity activity)
	{
		 InputMethodManager inputManager = (InputMethodManager) activity
		            .getSystemService(Context.INPUT_METHOD_SERVICE);

		    //check if no view has focus:
		    View v=activity.getCurrentFocus();
		    if(v==null)
		        return;

		    v.clearFocus();

		    // activity.getWindow().setSoftInputMode(WindowManager.LayoutParams.SOFT_INPUT_STATE_HIDDEN);
		    inputManager.hideSoftInputFromWindow(v.getWindowToken(), InputMethodManager.HIDE_NOT_ALWAYS);
	}

	public static void setFocus(final View v)
	{
		v.post(new Runnable() {
			public void run() {
				v.requestFocus();
				InputMethodManager imm = (InputMethodManager) v.getContext().getSystemService(Context.INPUT_METHOD_SERVICE);
				if (imm != null) {
					imm.showSoftInput(v, InputMethodManager.SHOW_IMPLICIT);
				}
			}
		});
	}

	public static boolean stringIsNullOrEmpty(String t)
	{
		return t == null || t.length() == 0;
	}

	public static boolean stringIsNullOrEmptyTrim(String t)
	{
		return t == null || t.trim().length() == 0;
	}

	public static String[] getApplicationsInDirectory(String searchDir)
	{
		String[] apps = null;

		File dir = new File(searchDir);

	    if(dir.isDirectory())
	    {
			String[] files = dir.list();

	    	if(files != null && files.length > 0)
	    	{
	    		ArrayList<String> list = new ArrayList<>();

				for (String file : files)
				{
					// the list command doesn't return a list of full paths
					// the list is only the name itself
					// combine the path with the directory we're looking in
					String filepath = Util.combinePath(searchDir, file);

					File f = new File(filepath);

					if (f.isDirectory())
					{
						// recursively search the directory
						String[] subdirfiles = getApplicationsInDirectory(filepath);

						if (subdirfiles != null && subdirfiles.length > 0)
						{
							Collections.addAll(list, subdirfiles);
						}
					} else
					{
						int extIndex = filepath.lastIndexOf(".pff");
						if (extIndex >= 0 && extIndex == filepath.length() - 4)
							list.add(filepath);
					}
				}

	    		apps = new String[list.size()];
	    		list.toArray(apps);
	    	}
        }

		return apps;
	}

	public static void copyStreams(InputStream in, OutputStream out) throws IOException
	{
	    byte[] buffer = new byte[16 * 1024];
	    int read;

	    while( ( read = in.read(buffer) ) != -1 )
	    	out.write(buffer,0,read);
	}

	public static void copyAndCloseStreams(InputStream in,OutputStream out) throws IOException
	{
		try {
			copyStreams(in, out);
		} finally {
	        in.close();
	        out.close();
		}
	}

	// Enable/disable all the controls in a ViewGroup.
	// Useful when you want to disable all controls when doing
	// an operation.
	private static void enableDisableViewAndChildren(View view, boolean enabled)
	{
		if (view == null)
			return;

		view.setEnabled(enabled);
		if (view instanceof ViewGroup) {
			ViewGroup viewGroup = (ViewGroup) view;
			int childCount = viewGroup.getChildCount();
			for (int i = 0; i < childCount; i++) {
				enableDisableViewAndChildren(viewGroup.getChildAt(i), enabled);
			}
		}
	}

	public static void enableViewAndChildren(View view)
	{
		enableDisableViewAndChildren(view, true);
	}

	public static void disableViewAndChildren(View view)
	{
		enableDisableViewAndChildren(view, false);
	}

	static String getLocaleLanguage(Resources resources)
	{
		String localeLanguage;

		try
		{
			Locale locale;

			if( Build.VERSION.SDK_INT >= Build.VERSION_CODES.N )
				locale = resources.getConfiguration().getLocales().get(0);

			else
				locale = resources.getConfiguration().locale;

			localeLanguage = locale.toString();
		}

		catch( Exception exception )
		{
			localeLanguage = "";
		}

		return localeLanguage;
	}

	/**
	 * Converts a collection to an array of primitive types
	 *
	 * @param collection Collection of Integers
	 * @return Array of integers
	 */
	static public int[] collectionToPrimitiveArray(Collection<Integer> collection) {

		int[] array = new int[collection.size()];
		int i = 0;
		for (Integer e : collection) {
			array[i++] = e;
		}
		return array;
	}

	static public Uri getShareableUriForFile(File f, Context context)
	{
		Uri uri;
		if( Build.VERSION.SDK_INT >= Build.VERSION_CODES.N ){
			// For Android N and above use content provider instead
			// of bare file URI. This avoids FileUriExposedException due to security
			// restrictions introduced in Android N.
			uri = FileProvider.getUriForFile(context, "gov.census.cspro.csentry.fileprovider", f);
		} else {
			// For older versions use raw uri
			uri = Uri.fromFile(f);
		}
		return uri;
	}

	public static <C> List<C> sparseArrayValues(SparseArray<C> sparseArray) {
		if (sparseArray == null) return null;
		List<C> arrayList = new ArrayList<>(sparseArray.size());

		for (int i = 0; i < sparseArray.size(); i++)
			arrayList.add(sparseArray.valueAt(i));
		return arrayList;
	}
}
