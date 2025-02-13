package gov.census.cspro.csentry.ui

import android.annotation.SuppressLint
import android.content.Context
import android.content.Intent
import android.util.AttributeSet
import android.webkit.WebResourceRequest
import android.webkit.WebView
import gov.census.cspro.engine.ActionInvoker
import gov.census.cspro.engine.CSProJavaScriptInterface
import gov.census.cspro.html.WebViewClientWithVirtualFileSupport


class QuestionTextView @JvmOverloads constructor(context: Context, attrs: AttributeSet? = null, defStyleAttr: Int = 0): WebView(context, attrs, defStyleAttr) {
    private var actionInvoker: ActionInvoker? = null

    init {
        @SuppressLint("SetJavaScriptEnabled")
        settings.javaScriptEnabled = true
        settings.allowFileAccess = false
        settings.allowContentAccess = false

        if( context is EntryActivity ) {
            setupWebViewClient(context)
        }
    }

    protected fun finalize() {
        actionInvoker?.cancelAndWaitOnActionsInProgress()
    }

    fun clear() {
        loadUrl("about:blank")
    }

    private fun setupWebViewClient(entryActivity: EntryActivity) {
        // Add the JavaScript interface.
        actionInvoker = entryActivity.createQuestionTextActionInvoker(this)
        addJavascriptInterface(actionInvoker!!, "AndroidActionInvoker")
        addJavascriptInterface(CSProJavaScriptInterface(actionInvoker!!), "CSPro")

        // WebViewClient accomplishes two things:
        //  1) Allows loading resources (images, css...) that are in app directory using relative paths (via WebViewClientWithVirtualFileSupport)
        //  2) Any links in the html are opened in a separate browser window
        webViewClient = object : WebViewClientWithVirtualFileSupport(entryActivity, true) {
            override fun shouldOverrideUrlLoading(view: WebView?, request: WebResourceRequest?): Boolean {
                if (!request!!.isForMainFrame) // Allow iframe src to open in webview
                    return false

                // All other links are links that are clicked on by user and should
                // be open in the browser rather than in the question text view
                val intent = Intent(Intent.ACTION_VIEW, request.url)
                view?.context?.startActivity(intent)
                return true
            }
        }
    }
}
