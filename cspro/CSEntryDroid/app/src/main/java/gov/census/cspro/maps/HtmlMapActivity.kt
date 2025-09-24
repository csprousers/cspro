package gov.census.cspro.maps

import android.annotation.SuppressLint
import android.content.pm.PackageManager
import android.graphics.Bitmap
import android.Manifest
import android.net.Uri
import android.os.Build
import android.os.Bundle
import android.os.Looper
import android.view.View
import android.webkit.JavascriptInterface
import android.webkit.WebMessage
import android.webkit.WebView
import androidx.annotation.RequiresApi
import androidx.appcompat.app.AppCompatActivity
import androidx.core.app.ActivityCompat
import com.google.android.gms.location.FusedLocationProviderClient
import com.google.android.gms.location.LocationCallback
import com.google.android.gms.location.LocationRequest
import com.google.android.gms.location.LocationResult
import com.google.android.gms.location.LocationServices
import com.google.android.gms.location.Priority
import gov.census.cspro.csentry.R
import gov.census.cspro.engine.EngineInterface
import gov.census.cspro.html.WebViewClientWithVirtualFileSupport
import gov.census.cspro.location.GpsReader
import gov.census.cspro.util.EdgeToEdgeUtils


class HtmlMapActivity : AppCompatActivity() {
    private lateinit var webView: WebView
    private var jniObjectPtr: Long = -1
    private var mappingUrl: String? = null
    private var locationProvider: FusedLocationProviderClient? = null
    private var locationCallback: LocationCallback? = null

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
        const val SHOW_CURRENT_LOCATION = 5
        const val HIDE_CURRENT_LOCATION = 6
    }

    @SuppressLint("SetJavaScriptEnabled")
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        EdgeToEdgeUtils.setupEdgeToEdge(this, R.layout.activity_generic_webview)

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

        if (locationProvider != null) {
            enableCurrentLocationUpdates()
        }
    }

    override fun onPause() {
        super.onPause()

        disableCurrentLocationUpdates()
    }

    override fun onDestroy() {
        // disconnect AndroidHtmlMapUI with HtmlMapActivity
        EngineInterface.getInstance().htmlMapNotifyLifecycle(jniObjectPtr, null)
        super.onDestroy()
    }

    @RequiresApi(Build.VERSION_CODES.M)
    fun handleRequest(type: Int, data: String?): Boolean {
        when (type) {
            RequestType.POST_WEB_MESSAGE -> {
                postWebMessage(data)
                return true
            }
            RequestType.HIDE -> {
                runOnUiThread {
                    finish()
                }
                return true
            }
            RequestType.SAVE_SNAPSHOT -> {
                data?.let {
                    saveSnapshot(it)
                }
                return true
            }
            RequestType.SET_WINDOW_TITLE -> {
                data?.let {
                    runOnUiThread {
                        title = it
                    }
                }
                return true
            }
            RequestType.SHOW_CURRENT_LOCATION -> {
                return try {
                    enableCurrentLocationUpdates()
                } catch (e: Exception) {
                    false
                }
            }
            RequestType.HIDE_CURRENT_LOCATION -> {
                disableCurrentLocationUpdates()
                return true
            }
        }

        return false
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

    private fun enableCurrentLocationUpdates(): Boolean {
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.M) {
            // postWebMessage requires API level 23, so there is no need to create a location
            // provider if we cannot update the location
            return false
        }
        else if (locationProvider == null) {
            // determine if we have access to location services
            if (ActivityCompat.checkSelfPermission(this, Manifest.permission.ACCESS_FINE_LOCATION) != PackageManager.PERMISSION_GRANTED &&
                ActivityCompat.checkSelfPermission(this, Manifest.permission.ACCESS_COARSE_LOCATION) != PackageManager.PERMISSION_GRANTED) {
                return false
            }
            locationProvider = LocationServices.getFusedLocationProviderClient(this)
        }
        else if (locationCallback != null) {
            // we may already be receiving location updates
            return true
        }

        // set up the request and callback
        val locationRequest = LocationRequest.Builder(
            Priority.PRIORITY_HIGH_ACCURACY,
            GpsReader.UPDATE_INTERVAL_IN_MILLISECONDS)
            .setMinUpdateIntervalMillis(GpsReader.FASTEST_UPDATE_INTERVAL_IN_MILLISECONDS)
            .build()

        locationCallback = object : LocationCallback() {
            override fun onLocationResult(result: LocationResult) {
                val location = result.lastLocation
                location?.let {
                    postWebMessage("{ \"action\": \"updateCurrentLocation\", \"latitude\": ${it.latitude}, \"longitude\": ${it.longitude} }")
                }
            }
        }

        locationCallback?.let {
            locationProvider!!.requestLocationUpdates(
                locationRequest,
                it,
                Looper.getMainLooper()
            )
        }

        return true
    }

    private fun disableCurrentLocationUpdates() {
        locationCallback?.let {
            locationProvider?.removeLocationUpdates(it)
            locationCallback = null
        }
    }
}
