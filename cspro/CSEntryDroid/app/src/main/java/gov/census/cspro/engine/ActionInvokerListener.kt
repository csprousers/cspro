package gov.census.cspro.engine

import android.net.Uri
import android.os.Build
import android.webkit.WebMessage
import android.webkit.WebView
import androidx.annotation.RequiresApi

open class ActionInvokerListener(private val webView: WebView) {

    init {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            assert(webView.webChromeClient == null)
        }
    }

    // default implementations match those in zAction/Listener.h
    open fun onGetDisplayOptions(webControllerKey: Int): String? {
        return null
    }

    open fun onSetDisplayOptions(displayOptionsJson: String, webControllerKey: Int): Boolean? {
        return null
    }

    @RequiresApi(Build.VERSION_CODES.O)
    fun onSetWebViewOptions(option: String) {
        webView.post {
            if (webView.webChromeClient == null) {
                webView.webChromeClient = ActionInvokerWebChromeClient()
            }

            assert(webView.webChromeClient is ActionInvokerWebChromeClient)

            (webView.webChromeClient as ActionInvokerWebChromeClient).addOption(option)
        }
    }

    open fun onClose(resultsText: String?, webControllerKey: Int): Boolean? {
        return null
    }

    open fun onEngineProgramControlExecuted(): Boolean {
        return false
    }

    @RequiresApi(Build.VERSION_CODES.M)
    fun onPostWebMessage(message: String, targetOrigin: String?) {
        val webMessage = WebMessage(message)
        val targetOriginUri = Uri.parse(targetOrigin ?: "*")
        webView.post {
            webView.postWebMessage(webMessage, targetOriginUri)
        }
    }
}
