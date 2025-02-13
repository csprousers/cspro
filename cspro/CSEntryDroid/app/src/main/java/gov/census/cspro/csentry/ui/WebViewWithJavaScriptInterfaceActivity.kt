package gov.census.cspro.csentry.ui

import android.annotation.SuppressLint
import android.graphics.Bitmap
import android.os.Bundle
import android.text.Html
import android.view.KeyEvent
import android.view.View
import android.webkit.URLUtil
import android.webkit.WebView
import android.widget.ProgressBar
import androidx.appcompat.app.AppCompatActivity
import gov.census.cspro.csentry.R
import gov.census.cspro.engine.*
import gov.census.cspro.html.WebViewClientWithVirtualFileSupport
import java.io.File
import java.net.URI
import java.net.URISyntaxException


@SuppressLint("SetJavaScriptEnabled")
class WebViewWithJavaScriptInterfaceActivity : AppCompatActivity() {
    private lateinit var progressBar: ProgressBar
    private lateinit var webView: WebView
    private var actionInvoker: ActionInvoker? = null

    companion object {
        const val TITLE = "TITLE"
        const val URL = "URL"
        const val ACCESS_TOKEN_OVERRIDE = "ACCESS_TOKEN_OVERRIDE"
        const val THREAD_WAIT_ID = "THREAD_WAIT_ID"
    }

    @SuppressLint("AddJavascriptInterface")
    override fun onCreate(savedInstanceState: Bundle?) {

        super.onCreate(savedInstanceState)

        setContentView(R.layout.activity_generic_webview)

        // keep weblinks inside the webview
        progressBar = findViewById<View>(R.id.progressbar_webview_loading) as ProgressBar
        webView = findViewById<View>(R.id.generic_webview) as WebView
        webView.settings.javaScriptEnabled = true // So that help web page menu works
        webView.settings.allowFileAccess = true // allow access to local files
        webView.settings.allowContentAccess = false

        // enable zooming but don't show zoom overlay
        webView.settings.builtInZoomControls = true
        webView.settings.displayZoomControls = false

        // Start zoomed out
        webView.settings.loadWithOverviewMode = true

        // Add the JavaScript interface.
        actionInvoker = ActionInvoker(webView, intent.getStringExtra(ACCESS_TOKEN_OVERRIDE), WebViewActionInvokerListener())
        webView.addJavascriptInterface(actionInvoker!!, "AndroidActionInvoker")
        webView.addJavascriptInterface(CSProJavaScriptInterface(actionInvoker!!), "CSPro")

        val title = intent.getStringExtra(TITLE)
        if (title == null) {
            this.supportActionBar?.hide()
        }

        // Set up the web client
        webView.webViewClient = object : WebViewClientWithVirtualFileSupport(this@WebViewWithJavaScriptInterfaceActivity, intent.getStringExtra(URL)) {

            override fun onPageFinished(view: WebView, url: String) {
                super.onPageFinished(view, url)
                // turn off the spinner
                progressBar.visibility = View.GONE

                // Set the activity title to the overridden title specified by the user
                // or the HTML document's title
                val overridenTitle = intent.getStringExtra(TITLE)
                if (overridenTitle == null) {
                    this@WebViewWithJavaScriptInterfaceActivity.supportActionBar?.hide()
                } else if (!Util.stringIsNullOrEmpty(title)) {
                    this@WebViewWithJavaScriptInterfaceActivity.title = overridenTitle
                }
            }

            override fun onPageStarted(view: WebView, url: String, favicon: Bitmap?) {
                // turn on the spinner
                progressBar.visibility = View.VISIBLE
                super.onPageStarted(view, url, favicon)
            }
        }
    }

    override fun onKeyDown(keyCode: Int, event: KeyEvent): Boolean {
        var result = false
        if (keyCode == KeyEvent.KEYCODE_BACK) {
            // test to see if there are anymore web pages this webview can navigate
            // backwards to, if there aren't bailout
            if (webView.canGoBack()) {
                webView.goBack()
                result = true
            } else {
                finish()
            }
        }
        return result
    }

    override fun onResume() {
        super.onResume()
        val url = intent.getStringExtra(URL)

        // load the url
        if (!Util.stringIsNullOrEmpty(url)) {
            // Reading local file
            if (URLUtil.isFileUrl(url)) {
				if (!fileUrlExists(url!!)) {
                    // WebView doesn't always show an error if file url can't be loaded so
                    // handle that case here.
                    val message = String.format(getString(R.string.error_file_not_found), Html.fromHtml(url))
                    webView.loadData(message, "text/plain", "utf-8")
                } else {
                    webView.loadUrl(url)
                }
            } else {
                webView.loadUrl(url!!)
            }
        }
    }

    override fun onDestroy() {
        actionInvoker?.cancelAndWaitOnActionsInProgress()

        // end any thread waiting on this activity to end
        val threadWaitId = intent.getLongExtra(THREAD_WAIT_ID, -1)
        EngineInterface.getInstance().setThreadWaitComplete(threadWaitId, null)

        super.onDestroy()
    }

    private fun fileUrlExists(url: String): Boolean {
        val uri: URI
        return try {
            uri = URI(url)
            val f = File(uri)
            f.exists()
        } catch (e: URISyntaxException) {
            false
        }
    }

    private inner class WebViewActionInvokerListener: ActionInvokerListener(webView) {
        private fun close() {
            webView.post {
                finish()
            }
        }

        override fun onCloseDialog(resultsText: String?, webControllerKey: Int): Boolean? {
            return if( webControllerKey == actionInvoker?.getWebControllerKey() ) {
                close()
                true
            }
            else {
                false
            }
        }

        override fun onEngineProgramControlExecuted(): Boolean {
            close()
            return true
        }
    }
}
