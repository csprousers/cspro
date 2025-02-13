package gov.census.cspro.util

import android.webkit.WebResourceResponse
import androidx.webkit.WebViewAssetLoader
import gov.census.cspro.engine.EngineInterface
import timber.log.Timber
import java.io.File
import java.io.FileInputStream
import java.io.IOException
import java.net.URLConnection

/**
 * Loads assets relative to a specified directory
 * Does not allow anything outside the CSEntry directory for security reasons.
 * We can't use WebViewAssetLoader.InternalStoragePathHandler for this because it explicitly
 * does not allow anything on external storage.
 */
class AppDirectoryResourceHandler(directory: String): WebViewAssetLoader.PathHandler {
    private val baseDirectory = File(directory).canonicalFile

    override fun handle(path: String): WebResourceResponse {
        try {
            val file = File(baseDirectory, path)
            if (inCSEntryDirectory(file)) {
                val mimeType = guessMimeType(path)
                return WebResourceResponse(mimeType, null, FileInputStream(file))
            } else {
                Timber.e("The requested file: $path is outside the CSEntry directory: ${EngineInterface.getInstance().csEntryDirectory.path}")
            }
        } catch (e: IOException) {
            Timber.e(e,"Error opening the requested path: $path")
        }
        return WebResourceResponse(null, null, null)
    }

    private fun guessMimeType(filePath: String) = URLConnection.guessContentTypeFromName(filePath) ?: "text/plain"

    private fun inCSEntryDirectory(file: File) = file.canonicalPath.startsWith(EngineInterface.getInstance().csEntryDirectory.path)
}
