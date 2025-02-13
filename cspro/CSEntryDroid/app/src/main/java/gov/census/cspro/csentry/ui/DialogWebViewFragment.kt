package gov.census.cspro.csentry.ui

import android.annotation.SuppressLint
import android.graphics.Rect
import android.os.Build
import android.os.Bundle
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.view.Window
import android.view.inputmethod.InputMethodManager
import android.webkit.WebSettings
import android.webkit.WebView
import androidx.appcompat.app.AppCompatActivity
import androidx.fragment.app.DialogFragment
import gov.census.cspro.csentry.BuildConfig
import gov.census.cspro.csentry.R
import gov.census.cspro.engine.*
import gov.census.cspro.html.WebViewClientWithVirtualFileSupport
import org.json.JSONObject
import timber.log.Timber
import java.util.concurrent.Semaphore
import kotlin.IllegalArgumentException as IllegalArgumentException1


class DialogWebViewFragment: DialogFragment(R.layout.fragment_dialog_webview) {
    private lateinit var webView: WebView
    private var actionInvoker: ActionInvoker? = null
    private var isResizing = false
    private var userWidth: Int = 0
    private var userHeight: Int = 0

    companion object {
        private const val CUSTOM_DIALOG = "CUSTOM_DIALOG"
        private const val URL = "URL"
        private const val WIDTH = "WIDTH"
        private const val HEIGHT = "HEIGHT"
        private const val ACCESS_TOKEN_OVERRIDE = "ACCESS_TOKEN_OVERRIDE"
        private const val THREAD_WAIT_ID = "THREAD_WAIT_ID"
        private const val JSON_ERROR = "JSON_ERROR"

        fun newInstance(isCustomDialog: Boolean, url: String, actionInvokerAccessTokenOverride: String?, threadId: Long?, jsonDisplayOptions: String?): DialogWebViewFragment {
            val dlgWebViewFragment = DialogWebViewFragment()
            val args = Bundle()
            dlgWebViewFragment.arguments = args
            args.putBoolean(CUSTOM_DIALOG, isCustomDialog)
            args.putString(URL, url)
            args.putString(ACCESS_TOKEN_OVERRIDE, actionInvokerAccessTokenOverride)
            if( threadId != null ) {
                args.putLong(THREAD_WAIT_ID, threadId)
            }
            if( jsonDisplayOptions != null ) {
                try {
                    val displayOptions = DisplayOptionsJsonReader(jsonDisplayOptions)
                    if( displayOptions.width != null && displayOptions.height != null ) {
                        args.putInt(WIDTH, displayOptions.width!!)
                        args.putInt(HEIGHT, displayOptions.height!!)
                    }
                }
                catch(Exc:Exception) {
                    args.putString(JSON_ERROR, Exc.message)
                }
            }
            return dlgWebViewFragment
        }
    }

    override fun onCreateView(inflater: LayoutInflater, container: ViewGroup?, savedInstanceState: Bundle?): View? {
        this.dialog?.setCanceledOnTouchOutside(false)
        this.dialog?.requestWindowFeature(Window.FEATURE_NO_TITLE)

        return super.onCreateView(inflater, container, savedInstanceState)
    }

    @SuppressLint("SetJavaScriptEnabled")
    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        super.onViewCreated(view, savedInstanceState)

        webView = view.findViewById<View>(R.id.mainWebView2) as WebView
        webView.settings.javaScriptEnabled = true // So that help web page menu works
        webView.settings.allowFileAccess = true // allow access to local files
        webView.settings.allowContentAccess = false

        // disable zooming
        webView.settings.builtInZoomControls = false
        webView.settings.displayZoomControls = false

        // disable scrolling for non-custom dialogs
        val isCustomDialog = ( arguments?.getBoolean(CUSTOM_DIALOG) == true )
        webView.isVerticalScrollBarEnabled = isCustomDialog
        webView.isHorizontalScrollBarEnabled = isCustomDialog

        // Start zoomed out
        webView.settings.loadWithOverviewMode = true

        // no caching
        webView.settings.cacheMode = WebSettings.LOAD_NO_CACHE

        // Add the JavaScript interface.
        actionInvoker = ActionInvoker(webView, arguments?.getString(ACCESS_TOKEN_OVERRIDE), HtmlDialogActionInvokerListener())
        webView.addJavascriptInterface(actionInvoker!!, "AndroidActionInvoker")
        webView.addJavascriptInterface(CSProJavaScriptInterface(actionInvoker!!), "CSPro")

        // Set up the web client
        webView.webViewClient = WebViewClientWithVirtualFileSupport(activity, arguments?.getString(URL))

        val lp = webView.layoutParams
        lp.width = resources.displayMetrics.widthPixels
        lp.height = resources.displayMetrics.heightPixels
        webView.layoutParams = lp

        val v = getView()
        if (v != null) {
            v.visibility = View.GONE

            v.addOnLayoutChangeListener { v, left, top, right, bottom, leftWas, topWas, rightWas, bottomWas ->
                val widthWas = rightWas - leftWas // Right exclusive, left inclusive
                val heightWas = bottomWas - topWas // Bottom exclusive, top inclusive
                if (v.width != widthWas || v.height != heightWas) {
                    if (!isResizing){
                        this.setSize(v.width, v.height, false)
                    } else
                        isResizing = false
                }
            }
        }

        val rView = requireActivity().findViewById<ViewGroup>(android.R.id.content).rootView

        webView.viewTreeObserver.addOnGlobalLayoutListener {
            if (keyboardStateChanged(rView)) {
                if (!keyboardShown(rView)) {
                    this.setSize(this.userWidth, this.userHeight, false)
                }
            }
        }
    }

    private var prevKeyboardDiff = -1
    private fun keyboardStateChanged(rootView: View): Boolean {
        val r = Rect()
        rootView.getWindowVisibleDisplayFrame(r)
        val heightDiff: Int = rootView.bottom - r.bottom

        val res = (prevKeyboardDiff >= 0 && heightDiff != this.prevKeyboardDiff)
        this.prevKeyboardDiff = heightDiff

        return res
    }

    private fun keyboardShown(rootView: View): Boolean {
        val softKeyboardHeight = 100
        val r = Rect()
        rootView.getWindowVisibleDisplayFrame(r)
        val dm = rootView.resources.displayMetrics
        val heightDiff: Int = rootView.bottom - r.bottom
        return heightDiff > softKeyboardHeight * dm.density
    }

    fun setSize(w: Int, h: Int, updateSize: Boolean = true) {
        if (w==0 || h==0)
            return

        isResizing = true

        var finalWidth = this.userWidth
        var finalHeight = this.userHeight
        var maxWidth: Int? = w
        var maxHeight: Int? = h
        if (updateSize) {
            finalWidth = (w * resources.displayMetrics.density).toInt()
            finalHeight = ((h + 8) * resources.displayMetrics.density).toInt()

            maxWidth =
                activity?.window?.decorView?.width?.minus((24 * resources.displayMetrics.density).toInt())
            maxHeight =
                activity?.window?.decorView?.height?.minus((64 * resources.displayMetrics.density).toInt())
        }

        if (finalWidth > maxWidth!!)
            finalWidth = maxWidth

        if (finalHeight > maxHeight!!)
            finalHeight = maxHeight

        if (updateSize) {
            this.userWidth = finalWidth
            this.userHeight = finalHeight
        }

        val ml = view?.findViewById<WebView>(R.id.mainWebView2)
        val lp = ml?.layoutParams
        if (lp != null) {
            lp.width = finalWidth
            lp.height = finalHeight
        }
        ml?.layoutParams = lp
        ml?.requestLayout()

        if (BuildConfig.VERSION_CODE < Build.VERSION_CODES.M) {
            val sizeAdjust = (32 * resources.displayMetrics.density).toInt()
            if (dialog != null) dialog!!.window!!.setLayout(
                finalWidth + sizeAdjust,
                finalHeight + sizeAdjust
            )
        }

        val v = view
        if (v!=null)
            v.visibility = View.VISIBLE

        ml?.requestFocus()
    }

    private fun loadDialog() {
        try {
            arguments?.getString(JSON_ERROR).let {
                if (it != null) {
                    throw IllegalArgumentException1(it)
                }
            }

            arguments?.getString(URL)?.let {
                webView.loadUrl(it)

                val width = arguments?.getInt(WIDTH)
                val height = arguments?.getInt(HEIGHT)

                if( width != null && height != null ) {
                    setSize(width, height)
                }
            }

            return
        }
        catch (Exc:Exception) {
            onReturn(null)
        }
    }

    override fun onResume() {
        super.onResume()
        loadDialog()
    }

    override fun onDestroy() {
        actionInvoker?.cancelAndWaitOnActionsInProgress()

        super.onDestroy()

        if (!alreadyDismissed) {
            onReturn(null)
        }
    }

    override fun onSaveInstanceState(outState: Bundle) {
        //super.onSaveInstanceState(outState)
    }

    private var alreadyDismissed = false
    private fun onReturn(jsonText: String?) {
        alreadyDismissed = true

        this.dismiss()
        val threadWaitId = arguments?.getLong(THREAD_WAIT_ID, -1)
        if (threadWaitId != null && threadWaitId >= 0) {
            EngineInterface.getInstance().setThreadWaitComplete(threadWaitId, jsonText)
        } else {
            Messenger.getInstance().engineFunctionComplete(jsonText)
        }
    }

    private fun showKeyboard() {
        val ml = view?.findViewById<WebView>(R.id.mainWebView2)
        val inputMethodManager: InputMethodManager =
            activity?.getSystemService(AppCompatActivity.INPUT_METHOD_SERVICE) as InputMethodManager
        if (ml != null) {
            inputMethodManager.toggleSoftInputFromWindow(
                ml.applicationWindowToken,
                InputMethodManager.SHOW_FORCED, 0
            )
        }
    }

    private inner class HtmlDialogActionInvokerListener: ActionInvokerListener(webView) {
        private fun close(resultsText: String?) {
            this@DialogWebViewFragment.activity?.runOnUiThread {
                this@DialogWebViewFragment.onReturn(resultsText)
            }
        }

        override fun onGetDisplayOptions(webControllerKey: Int): String? {
            if( webControllerKey != actionInvoker?.getWebControllerKey() ) {
                return null
            }

            return try {
                val displayOptionsJsonObject = JSONObject()
                if( userWidth != 0 ) {
                    displayOptionsJsonObject.put(DisplayOptionsJsonReader.WIDTH, userWidth)
                    displayOptionsJsonObject.put(DisplayOptionsJsonReader.HEIGHT, userHeight)
                }
                // CS_TODO add keyboard if desired displayOptionsJsonObject.put(DisplayOptionsJsonReader.KEYBOARD, ....)
                displayOptionsJsonObject.toString()
            } catch( e: Exception ) {
                null
            }
        }

        override fun onSetDisplayOptions(displayOptionsJson: String, webControllerKey: Int): Boolean? {
            if( webControllerKey != actionInvoker?.getWebControllerKey() ) {
                return null
            }

            val displayOptions = DisplayOptionsJsonReader(displayOptionsJson)

            val mutex = Semaphore(0)
            var success = false

            this@DialogWebViewFragment.activity?.runOnUiThread {
                try {
                    if( displayOptions.keyboard == true ) {
                        showKeyboard()
                    }
                    if( displayOptions.width != null && displayOptions.height != null ) {
                        setSize(displayOptions.width!!, displayOptions.height!!)
                    }
                    success = true
                }
                catch(e:Exception) {
                    Timber.e(e)
                }
                mutex.release()
            }

            try {
                mutex.acquire()
            } catch (e: InterruptedException) {
                e.printStackTrace()
            }

            return success
        }

        override fun onCloseDialog(resultsText: String?, webControllerKey: Int): Boolean? {
            return if( webControllerKey == actionInvoker?.getWebControllerKey() ) {
                close(resultsText)
                true
            }
            else {
                false
            }
        }

        override fun onEngineProgramControlExecuted(): Boolean {
            close(null)
            return true
        }
    }

    private class DisplayOptionsJsonReader(displayOptionsJson: String) {
        var width: Int? = null
        var height: Int? = null
        var keyboard: Boolean? = null

        companion object {
            const val WIDTH = "width"
            const val HEIGHT = "height"
            const val KEYBOARD = "keyboard"
        }

        init {
            val displayOptions = JSONObject(displayOptionsJson)
            if( displayOptions.has(WIDTH) ) {
                width = EngineInterface.getInstance().parseDimensionText(displayOptions.getString(WIDTH), true)
            }
            if( displayOptions.has(HEIGHT) ) {
                height = EngineInterface.getInstance().parseDimensionText(displayOptions.getString(HEIGHT), false)
            }
            if( displayOptions.has(KEYBOARD) ) {
                keyboard = try {
                    displayOptions.getBoolean(KEYBOARD)
                } catch( e: Exception ) {
                    // to support the way 7.7 dialogs were coded
                    ( displayOptions.getInt(KEYBOARD) != 0 )
                }
            }
        }
    }
}