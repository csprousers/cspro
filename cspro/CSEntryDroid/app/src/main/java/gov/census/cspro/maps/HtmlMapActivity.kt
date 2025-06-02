package gov.census.cspro.maps

import android.annotation.SuppressLint
import android.graphics.Bitmap
import android.net.Uri
import android.os.Build
import android.os.Bundle
import android.view.View
import android.webkit.JavascriptInterface
import android.webkit.WebMessage
import android.webkit.WebView
import androidx.annotation.RequiresApi
import androidx.appcompat.app.AppCompatActivity
import gov.census.cspro.csentry.R
import gov.census.cspro.engine.EngineInterface
import gov.census.cspro.html.WebViewClientWithVirtualFileSupport


class HtmlMapActivity : AppCompatActivity() {
    private lateinit var webView: WebView
    private var jniObjectPtr: Long = -1
    private var mappingUrl: String? = null

    companion object {
        const val JNI_OBJECT_PTR  = "JNI_OBJECT_PTR"
        const val MAPPING_URL = "MAPPING_URL"
    }

    // these values must match those in AndroidHtmlMapUI.cpp
    private object RequestType {
        const val POST_WEB_MESSAGE = 1
        const val HIDE = 2
        const val SAVE_SNAPSHOT = 3
        const val SET_WINDOW_TITLE = 4
    }

    @SuppressLint("SetJavaScriptEnabled")
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        setContentView(R.layout.activity_generic_webview)

        webView = findViewById<View>(R.id.generic_webview) as WebView
        webView.settings.javaScriptEnabled = true

        // enable zooming but don't show zoom overlay
        webView.settings.builtInZoomControls = true
        webView.settings.displayZoomControls = false

        // start zoomed out
        webView.settings.loadWithOverviewMode = true

        // add the JavaScript interface.
        webView.addJavascriptInterface(this, "AndroidHtmlMap")

        // set up the web client
        webView.webViewClient = WebViewClientWithVirtualFileSupport(this, true)

        // connect AndroidHtmlMapUI with HtmlMapActivity
        jniObjectPtr = intent.getLongExtra(JNI_OBJECT_PTR, jniObjectPtr)
        mappingUrl = intent.getStringExtra(MAPPING_URL)
        EngineInterface.getInstance().htmlMapNotifyLifecycle(jniObjectPtr, this)
    }

    override fun onResume() {
        super.onResume()

        mappingUrl?.let {
            webView.loadUrl(it)
        }
    }

    override fun onDestroy() {
        // disconnect AndroidHtmlMapUI with HtmlMapActivity
        EngineInterface.getInstance().htmlMapNotifyLifecycle(jniObjectPtr, null)
        super.onDestroy()
    }

    @RequiresApi(Build.VERSION_CODES.M)
    fun handleRequest(type: Int, data: String?) {
        when (type) {
            RequestType.POST_WEB_MESSAGE -> {
                postWebMessage(data)
            }
            RequestType.HIDE -> {
                runOnUiThread {
                    finish()
                }
            }
            RequestType.SAVE_SNAPSHOT -> {
                data?.let {
                    saveSnapshot(it)
                }
            }
            RequestType.SET_WINDOW_TITLE -> {
                data?.let {
                    runOnUiThread {
                        setTitle(it)
                    }
                }
            }
        }
    }

    // for sending messages to JavaScript
    @RequiresApi(Build.VERSION_CODES.M)
    fun postWebMessage(message: String?) {
        webView.post {
            webView.postWebMessage(WebMessage(message), Uri.parse(mappingUrl))
        }
    }

    // for receiving messages, called via JavaScript using AndroidHtmlMap.postMessage
    @JavascriptInterface
    @RequiresApi(Build.VERSION_CODES.M)
    fun postMessage(message: String) {
        EngineInterface.getInstance().htmlMapNotifyWebMessageReceived(jniObjectPtr, message)
    }

    private fun saveSnapshot(imageFilePath: String) {
        val bitmap = Bitmap.createBitmap(webView.width, webView.height, Bitmap.Config.ARGB_8888)
        val canvas = android.graphics.Canvas(bitmap)
        webView.draw(canvas)
        MapFragment.saveSnapshotToDisk(bitmap, imageFilePath)
    }
}
