package gov.census.cspro.engine

import android.net.Uri
import android.os.Build
import android.webkit.JavascriptInterface
import android.webkit.PermissionRequest
import android.webkit.WebChromeClient
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


class ActionInvokerWebChromeClient : WebChromeClient() {
    private val options: MutableSet<String> = mutableSetOf()

    fun addOption(option: String) {
        options.add(option)
    }

    override fun onPermissionRequest(request: PermissionRequest) {
        val allowedResources = request.resources.filter { resource ->
            options.contains(resource)
        }.toTypedArray()

        if (allowedResources.isNotEmpty()) {
            request.grant(allowedResources)
        } else {
            request.deny()
        }
    }
}


open class ActionInvoker(private val webView: WebView, private val actionInvokerAccessTokenOverride: String?, protected val listener: ActionInvokerListener) {
    private var webControllerKey: Int? = null

    fun getWebControllerKey(): Int {
        if( webControllerKey == null ) {
            webControllerKey = EngineInterface.getInstance().actionInvokerCreateWebController(actionInvokerAccessTokenOverride)
        }
        return webControllerKey!!
    }

    fun cancelAndWaitOnActionsInProgress() {
        if( webControllerKey != null ) {
            EngineInterface.getInstance().actionInvokerCancelAndWaitOnActionsInProgress(webControllerKey!!)
        }
    }

    @JavascriptInterface
    fun run(message: String): String {
        return runSync(message)
    }

    protected open fun runSync(message: String): String {
        return EngineInterface.getInstance().actionInvokerProcessMessage(getWebControllerKey(), listener, message, false, false)
            ?: ""
    }

    @JavascriptInterface
    fun runAsync(message: String) {
        runAsync(message, null)
    }

    abstract class OldCSProObjectRunAsyncHandler {
         abstract fun process(webView: WebView, response: String)
    }

    open fun runAsync(message: String, oldCSProObjectRunAsyncHandler: OldCSProObjectRunAsyncHandler?) {
        // call asynchronously
        Thread {
            runAsyncWorker(message, oldCSProObjectRunAsyncHandler)
        }.start()
    }

    fun runAsyncWorker(message: String, oldCSProObjectRunAsyncHandler: OldCSProObjectRunAsyncHandler?) {
        val calledByOldCSProObject = ( oldCSProObjectRunAsyncHandler != null )
        val javaScriptResponse = EngineInterface.getInstance().actionInvokerProcessMessage(getWebControllerKey(), listener, message, true, calledByOldCSProObject)

        if( javaScriptResponse != null ) {
            if( calledByOldCSProObject ) {
                oldCSProObjectRunAsyncHandler!!.process(webView, javaScriptResponse)
            }
            else {
                webView.post {
                    webView.evaluateJavascript(javaScriptResponse, null)
                }
            }
        }
    }
}
