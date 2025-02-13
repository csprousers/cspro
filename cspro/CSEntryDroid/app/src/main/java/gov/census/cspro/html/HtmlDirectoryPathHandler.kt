package gov.census.cspro.html

import android.app.Activity
import android.webkit.WebResourceResponse
import androidx.webkit.WebViewAssetLoader
import gov.census.cspro.engine.Util
import timber.log.Timber
import java.io.File
import java.io.FileInputStream
import java.net.URLConnection

class HtmlDirectoryPathHandler(activity: Activity): WebViewAssetLoader.PathHandler {
    private val directory: File

    companion object {
        const val PATH_PREFIX = "/"
        var assetsDirectory: String? = null
    }

    init {
        assert(assetsDirectory != null)
        directory = File(Util.combinePath(assetsDirectory, "html")).canonicalFile
    }

    override fun handle(path: String): WebResourceResponse? {
        return try {
            val file = File(directory, path)
            WebResourceResponse(URLConnection.guessContentTypeFromName(path), null, FileInputStream(file))
        } catch( e: Exception ) {
            Timber.e(e, "Error opening the requested path: $path")
            null
        }
    }
}
