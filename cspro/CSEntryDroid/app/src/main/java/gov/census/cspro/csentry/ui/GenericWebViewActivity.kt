/***************************************************************************************
 *
 * CSEntry for Android
 *
 * Module:		GenericWebViewActivity.kt
 *
 * Description: UI class for displaying any old webpage.
 *
 */
package gov.census.cspro.csentry.ui

import android.annotation.SuppressLint
import android.graphics.Bitmap
import android.os.Bundle
import android.view.KeyEvent
import android.view.View
import android.webkit.*
import android.widget.ProgressBar
import androidx.appcompat.app.AppCompatActivity
import gov.census.cspro.csentry.R
import gov.census.cspro.engine.*


class GenericWebViewActivity : AppCompatActivity() {

    private lateinit var progressBar: ProgressBar

    @SuppressLint("SetJavaScriptEnabled")
    override fun onCreate(savedInstanceState: Bundle?) {

        super.onCreate(savedInstanceState)

        setContentView(R.layout.activity_generic_webview)

        // keep weblinks inside the webview
        progressBar = findViewById<View>(R.id.progressbar_webview_loading) as ProgressBar
        val webView = findViewById<View>(R.id.generic_webview) as WebView
        webView.settings.javaScriptEnabled = true // So that help web page menu works
        webView.settings.allowFileAccess = true // allow access to local files

        // enable zooming but don't show zoom overlay
        webView.settings.builtInZoomControls = true
        webView.settings.displayZoomControls = false

        // Start zoomed out
        webView.settings.loadWithOverviewMode = true

        webView.webViewClient = object : WebViewClient() {
            override fun onPageFinished(view: WebView, url: String) {
                super.onPageFinished(view, url)
                // turn off the spinner
                progressBar.visibility = View.GONE

                // Set title of activity to title from HTML document
                if (!Util.stringIsNullOrEmpty(view.title)) this@GenericWebViewActivity.title = view.title
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
            val webView = findViewById<View>(R.id.generic_webview) as WebView
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
        val url = intent.getStringExtra(URL_PARAM)

        // load the url
        if (!Util.stringIsNullOrEmpty(url)) {
            val webView = findViewById<View>(R.id.generic_webview) as WebView
            webView.loadUrl(url!!)
        }
    }

    companion object {
        const val URL_PARAM = "URL.PARAM"
    }
}
