package gov.census.cspro.engine

import android.webkit.JavascriptInterface
import android.webkit.WebView

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
