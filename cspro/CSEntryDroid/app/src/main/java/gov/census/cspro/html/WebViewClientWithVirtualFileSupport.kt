package gov.census.cspro.html

import android.app.Activity
import android.webkit.WebResourceRequest
import android.webkit.WebResourceResponse
import android.webkit.WebView
import android.webkit.WebViewClient
import androidx.webkit.WebViewAssetLoader

open class WebViewClientWithVirtualFileSupport(activity: Activity?, mapHtmlDirectory: Boolean): WebViewClient() {
    private var virtualFileAssetLoader: WebViewAssetLoader
    private var htmlDirectoryAssetLoader: WebViewAssetLoader? = null

    companion object {
        const val ASSET_LOADER_ROOT_URL = "https://appassets.androidplatform.net/"
    }

    init {
        // create an asset loader to serve virtual file mappings
        virtualFileAssetLoader = WebViewAssetLoader.Builder()
            .addPathHandler(
                VirtualFileMappingPathHandler.PATH_PREFIX,
                VirtualFileMappingPathHandler()
            )
            .build()

        if( mapHtmlDirectory && activity != null ) {
            htmlDirectoryAssetLoader = WebViewAssetLoader.Builder()
                .addPathHandler(
                    HtmlDirectoryPathHandler.PATH_PREFIX,
                    HtmlDirectoryPathHandler(activity)
                )
                .build()
        }
    }

    // when receiving a URL created by a virtual file mapping, map the /html directory
    constructor(activity: Activity?, url: String?) : this(activity, VirtualFileMappingPathHandler.isVirtualFileMappingUrl(url))

    override fun shouldInterceptRequest(view: WebView?, request: WebResourceRequest): WebResourceResponse? {
        // first try resolving as a virtual file (so that URLs like /lfs/... are not mapped as /html/lfs/...);
        // if not, try to resolve in the /html directory
        return virtualFileAssetLoader.shouldInterceptRequest(request.url)
            ?: htmlDirectoryAssetLoader?.shouldInterceptRequest(request.url)
    }
}
