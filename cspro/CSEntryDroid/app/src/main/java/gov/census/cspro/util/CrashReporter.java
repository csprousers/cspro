package gov.census.cspro.util;

import gov.census.cspro.engine.EngineInterface;
import gov.census.cspro.engine.Util;
import timber.log.Timber;

import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.FileWriter;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.PrintWriter;
import java.io.StringWriter;
import java.lang.Thread.UncaughtExceptionHandler;
import java.util.Map;
import android.content.Context;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager.NameNotFoundException;
import android.os.Build;

/**
 * Simple crash logging class
 * 
 * Replaces the default uncaught exception handler with one
 * that writes a crash report to the file crashreport.txt in
 * the csentry directory. The report contains basic info about
 * the device, the exception message, stack trace and the logcat.
 * 
 * To use it call CrashReporter.install(this) in application 
 * or main activity onCreate. For Android versions before
 * Jelly Bean you will also need to add the following permission
 * to the manifest:
 * 
 * <uses-permission android:name="android.permission.READ_LOGS"/>
 * 
 * In Jelly Bean and above the permission is not required however
 * you only get the log entries for your own app.
 */
public class CrashReporter {

	private static Context m_context;
	private static volatile boolean  m_crashing;
	private static Thread.UncaughtExceptionHandler m_defaultHandler;
	
	public static void install(Context context)
	{
		m_context = context;
		
		m_defaultHandler = Thread.getDefaultUncaughtExceptionHandler();
		Thread.setDefaultUncaughtExceptionHandler(new UncaughtHandler());
	}
	
	private static class UncaughtHandler implements UncaughtExceptionHandler {

		@Override
		public void uncaughtException(Thread thread, Throwable exception) 
		{
			try {
				
				// Avoid infinite loop if already in crash handler
				if (m_crashing)
					return;
				
				m_crashing = true;

				Timber.e(exception, "Starting crash dump...");
				
				StringWriter report = new StringWriter();
				PrintWriter writer = new PrintWriter(report);
				
				writer.print("CSEntry Crash Report\r\n");
				writer.print("--------------------\r\n");
				
				writer.print("\r\n-- Device --\r\n");
				dumpDeviceInfo(writer);
				
				writer.print("\r\n-- Exception --\r\n");
				dumpException(exception, writer);

				writer.print("\r\n-- Threads --\r\n");
				dumpThreads(writer);
				
				writer.print("\r\n-- Log --\r\n");
				dumpLog(writer);
					
				String reportFilename = Util.combinePath(EngineInterface.getInstance().csEntryDirectory.getPath(), "crashreport.txt");
				writeToFile(report.toString(), reportFilename);

				Timber.d("Wrote crash report to %s", reportFilename);
				
			} catch (Exception e) {
				try {
					Timber.e(e, "Error generating crash report");
				} catch (Exception ignored)	{}
			} finally {
				if (m_defaultHandler != null) {
					m_defaultHandler.uncaughtException(thread, exception);
				} else {
					// If there is no default then just kill the process.
					// Want to make sure it doesn't stick around in crashed state.
					android.os.Process.killProcess(android.os.Process.myPid());
				}
			}
			
		}

		private void dumpThreads(PrintWriter writer) 
		{
			Map<Thread, StackTraceElement[]> stackTraces = Thread.getAllStackTraces();
			for (Map.Entry<Thread, StackTraceElement[]> entry : stackTraces.entrySet()) {
				Thread thread = entry.getKey();
				StackTraceElement[] trace = entry.getValue();
				writer.print(thread.getId() == Thread.currentThread().getId() ? "***" : "   ");
				writer.printf("%d\t%s\t(%s)\t", thread.getId(), thread.getName(), thread.getState().toString());
				String codeLocation = trace != null && trace.length > 0 ? trace[0].toString() : "";
				writer.print(codeLocation);
				writer.print("\r\n");
			}
		}

		private void dumpDeviceInfo(PrintWriter writer) 
		{
			try{
				PackageInfo info = m_context.getPackageManager().getPackageInfo(m_context.getPackageName(), 0);
				writer.printf("CSEntry Version: %s\r\nVersion Code: %d\r\n", info.versionName, info.versionCode);
			}
			catch (NameNotFoundException ignore) {}
			
			writer.printf("Android Version: %s\r\n", Build.VERSION.RELEASE);
			writer.printf("API Level: %d\r\n", android.os.Build.VERSION.SDK_INT);
			writer.printf("Device: %s %s\r\n", android.os.Build.BRAND, android.os.Build.DEVICE);
			writer.printf("Model: %s (%s)\r\n", android.os.Build.MODEL, android.os.Build.PRODUCT);
		}

		private void dumpLog(PrintWriter writer) 
		{
			try {
				String logcat = readLogcat();
				writer.print(logcat);
			} catch (IOException e) {
				Timber.e(e, "Error reading logcat");
				writer.println("Unable to read log: " + e.getMessage());
			}
		}

		private void dumpException(Throwable exception, PrintWriter writer)
		{
			writer.print(exception.toString());
			writer.print("\r\n");
			
			for (StackTraceElement el : exception.getStackTrace()) {
				writer.print("\tat ");
				writer.print(el.toString());
				writer.print("\r\n");
			}
			
			if (exception.getCause() != null) {
				writer.print("\tCaused by:\r\n");
				dumpException(exception.getCause(), writer);
			}
		}
		
		private static void writeToFile(String report, String filename) throws IOException
		{
            BufferedWriter writer = new BufferedWriter(new FileWriter(filename));
            writer.write(report);
            writer.flush();
            writer.close();
	    }
		
		private static String readLogcat() throws IOException
		{
			Process process = Runtime.getRuntime().exec("logcat -d -v time");
			BufferedReader bufferedReader = new BufferedReader(
					new InputStreamReader(process.getInputStream()));

			StringBuilder log = new StringBuilder();
			String line;
			while ((line = bufferedReader.readLine()) != null) {
				log.append(line);
				log.append("\r\n");
			}

			return log.toString();
		}
	}
}
