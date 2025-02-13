package gov.census.cspro.camera

import android.content.Intent
import android.net.Uri
import android.os.Bundle
import android.os.Handler
import android.os.PersistableBundle
import android.view.View
import android.webkit.URLUtil
import android.widget.TextView
import com.google.android.gms.vision.MultiProcessor
import com.google.android.gms.vision.barcode.Barcode
import com.google.android.gms.vision.barcode.BarcodeDetector
import gov.census.cspro.csentry.R
import gov.census.cspro.engine.Util
import gov.census.cspro.util.Constants
import java.util.*

class BarcodeCaptureActivity : BaseCameraActivity(), BarcodeTracker.BarcodeGraphicTrackerCallback {
    override var cameraSourcePreview: CameraSourcePreview? = null
        private set

    /**
     * Initializes the UI and creates the detector pipeline.
     */
    override fun onCreate(icicle: Bundle?) {
        super.onCreate(icicle)
        setContentView(R.layout.activity_barcode_capture)
        initUI()
        initCamera(barcodeDetector)
    }

    private fun initUI() {
        cameraSourcePreview = findViewById(R.id.preview)
        val mPreviewText = findViewById<TextView>(R.id.preview_text)
        if (mPreviewText != null && intent != null && !Util.stringIsNullOrEmpty(intent.getStringExtra(Constants.EXTRA_USER_MESSAGE_KEY))) {
            mPreviewText.visibility = View.VISIBLE
            mPreviewText.text = intent.getStringExtra(Constants.EXTRA_USER_MESSAGE_KEY)
        } else {
            mPreviewText!!.visibility = View.GONE
        }
    }// We Download application using QR code only

    // A barcode detector is created to track barcodes.  An associated multi-processor instance
    // is set to receive the barcode detection results, track the barcodes, and maintain
    // graphics for each barcode on screen.  The factory is used by the multi-processor to
    // create a separate tracker instance for each barcode.
    private val barcodeDetector: BarcodeDetector
        get() {
            val context = applicationContext

            // A barcode detector is created to track barcodes.  An associated multi-processor instance
            // is set to receive the barcode detection results, track the barcodes, and maintain
            // graphics for each barcode on screen.  The factory is used by the multi-processor to
            // create a separate tracker instance for each barcode.
            val barcodeBuilder = BarcodeDetector.Builder(context)
            if (intent != null && intent.getStringExtra(Constants.EXTRA_BARCODE_KEY) != null) {
                barcodeBuilder.setBarcodeFormats(Barcode.ALL_FORMATS)
            } else { // We Download application using QR code only
                barcodeBuilder.setBarcodeFormats(Barcode.QR_CODE)
            }
            val barcodeDetector = barcodeBuilder.build()
            val barcodeFactory = BarcodeTrackerFactory(this)
            barcodeDetector!!.setProcessor(MultiProcessor.Builder(barcodeFactory).build())
            return barcodeDetector
        }

    override fun onDetectedQrCode(barcode: Barcode) {

        //To Download application
        if ((barcode.displayValue.contains("http://www.csprousers.org")
                || barcode.displayValue.contains("https://www.csprousers.org")) && intent.getStringExtra(Constants.EXTRA_BARCODE_KEY) == null) {

            val uriValue = Uri.parse(barcode.displayValue)

            val server = uriValue.getQueryParameter("server")
            val app = uriValue.getQueryParameter("app")
            val cred = uriValue.getQueryParameter("cred")

            if (uriValue != null
                && server != null
                && (URLUtil.isValidUrl(server)
                    || server.toLowerCase(Locale.ROOT).contains("ftp")
                    || server.toLowerCase(Locale.ROOT).contains("dropbox"))
                && app != null
            ) {
                val intent = Intent()
                intent.putExtra("xserver", server);
                intent.putExtra("xapp", app);
                intent.putExtra("xcred", cred);
                setResult(Constants.INTENT_STARTACTIVITY_DEEPLINK, intent)
            }

            finish()

            // To capture message from Survey
            //It allows you to take a barcode with/without a message, and then will display the return value of the barcode.read() operation.
        } else if (!Util.stringIsNullOrEmptyTrim(barcode.displayValue)) {
            if (intent.getStringExtra(Constants.EXTRA_BARCODE_KEY) != null) {
                val resultIntent = Intent()
                resultIntent.putExtra(Constants.EXTRA_BARCODE_DISPLAYVALUE_KEY, barcode.displayValue)
                setResult(Constants.INTENT_FINISHACTIVITY_BARCODE, resultIntent)
                Timer().schedule(object : TimerTask() {
                    override fun run() {
                        finish()
                    }
                }, 500)
            }
        }
    }

    override fun onBackPressed() {
        if (intent.getStringExtra(Constants.EXTRA_BARCODE_KEY) != null) {

            //If the user cancels the barcode scanning, the function should return a blank string.
            val resultIntent = Intent()
            resultIntent.putExtra(Constants.EXTRA_BARCODE_DISPLAYVALUE_KEY, "")
            setResult(RESULT_CANCELED, resultIntent)
        }
        super.onBackPressed()
    }
}