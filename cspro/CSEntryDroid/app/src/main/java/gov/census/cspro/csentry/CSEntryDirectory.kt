package gov.census.cspro.csentry

import android.content.Context
import android.content.SharedPreferences
import android.media.MediaScannerConnection
import android.os.Environment
import timber.log.Timber
import java.io.File
import java.io.IOException

class CSEntryDirectory(context: Context) {

    val path: String

    private val csEntrySubdir = "csentry"
    private var firstTimeWithNewDirectory = false

    init {
        // Starting with CSEntry 7.5 the csentry directory was moved from the global external storage dir (/mnt/sdcard)
        // to app specific external files dir (/mnt/sdcard/Android/data/gov.census.cspro.csentry/files).
        // This is necessary due to external storage restrictions in Android 10/11.
        // To support legacy apps we check if the old csentry exists and if it does we use that, otherwise
        // we use the new location.
        // Result is stored in preferences so we don't have to do this every time - also so that adding the files have moved message
        // in legacy directory doesn't mess up this check the second time.
        val sharedPref: SharedPreferences = context.getSharedPreferences(context.getString(R.string.preferences_file_global), Context.MODE_PRIVATE)
        val csentryPath = sharedPref.getString("csentrydirectory", null)
        if (csentryPath == null || !File(csentryPath).canWrite()) {
            // First time running app (or at least version of app that stores csentry dir in settings)
            val legacyCSEntryDirectory = getLegacyDirectory()
            if (legacyCSEntryDirectory.exists() && legacyCSEntryDirectory.isDirectory && legacyCSEntryDirectory.canWrite()) {
                // Must be an existing install, use the legacy directory
                path = legacyCSEntryDirectory.absolutePath
            } else {
                // Either the directory doesn't exist or we don't have permissions so it is a new install
                path = getDirectory(context).absolutePath
                firstTimeWithNewDirectory = true
            }
            sharedPref.edit().putString("csentrydirectory", path).apply()
        } else {
            path = csentryPath
        }

        createDirectory(context)

    }

    private fun createDirectory(context: Context) {

        val dir = File(path)

        if (!dir.exists()) {

            // create the directory
            if (!dir.mkdir()) Timber.e("Failed to create directory %s", path)

            // Force media rescan so it shows up when connected to PC
            mediaScanDirectory(dir, context)
        }

        if (firstTimeWithNewDirectory)
            createCSEntryHasMovedFile(context)
    }

    private fun createCSEntryHasMovedFile(context: Context) {

        try {
            @Suppress("DEPRECATION") val csEntryHasMovedFile = File(Environment.getExternalStorageDirectory(), "CSEntry Has Moved.html")
            if (!csEntryHasMovedFile.exists()) {
                csEntryHasMovedFile.writeText("""<!doctype html>
                                            <html>
                                            <head>
                                            <meta charset="utf-8">
                                            <title>CSEntry Has Moved</title>
                                            </head>
                                            <body>
                                            <h1>The CSEntry Folder Has Moved</h1>
                                            <p>Starting with CSPro version 7.5, CSEntry applications and data are no longer stored in this folder. These files are now stored in the directory <b>Android/data/gov.census.cspro.csentry/files/csentry</b></p>
                                            <p>This change is due to restrictions in newer versions of Android that restrict access by apps to files on the device storage.</p>
                                            <p>Please see the CSPro help for more information.</p>
                                            </body>
                                            </html>""".trimMargin())
                mediaScanFile(context, csEntryHasMovedFile)
            }
        } catch (e: IOException) {
            Timber.w(e, "Error creating 'CSEntry Has Moved.html'")
        }
    }

    private fun mediaScanDirectory(dir: File, context: Context) {
        // If you ask media scanner to scan a folder then it shows up as a zero
        // byte file so the workaround is to create a zero byte file in the folder,
        // scan that file, and then delete the file.
        try {
            val temp = File(dir, "_temp.txt")
            temp.createNewFile()
            mediaScanFile(context, temp)
        } catch (e: IOException) {
            Timber.e(e, "Error adding CSEntry directory to media scanner")
        }
    }

    private fun mediaScanFile(context: Context, file: File) {
        MediaScannerConnection.scanFile(context, arrayOf(file.absolutePath),
            null
        ) { path, _ ->
            File(path).delete()
            MediaScannerConnection.scanFile(context, arrayOf(path), null, null)
        }
    }

    @Suppress("DEPRECATION")
    private fun getLegacyDirectory() = File(Environment.getExternalStorageDirectory(), csEntrySubdir)

    private fun getDirectory(context: Context) = File(context.getExternalFilesDir(null), csEntrySubdir)
}