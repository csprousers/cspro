package gov.census.cspro.html

import android.webkit.WebResourceResponse
import androidx.webkit.WebViewAssetLoader
import gov.census.cspro.engine.EngineInterface
import timber.log.Timber
import java.io.ByteArrayInputStream

class VirtualFileMappingPathHandler : WebViewAssetLoader.PathHandler {

    companion object {
        const val PATH_PREFIX = "/lfs/"
        private const val URL_PREFIX = WebViewClientWithVirtualFileSupport.ASSET_LOADER_ROOT_URL + "lfs/"

        fun isVirtualFileMappingUrl(url: String?): Boolean {
            return ( url?.startsWith(URL_PREFIX) == true )
        }
    }

    override fun handle(path: String): WebResourceResponse? {
        try {
            val virtualFile = EngineInterface.getInstance().getVirtualFile(path)

            if( virtualFile != null ) {
                return WebResourceResponse(virtualFile.contentType, null, ByteArrayInputStream(virtualFile.content))
            }
        } catch( e: Exception ) {
            Timber.e(e)
        }

        return null
    }

}
