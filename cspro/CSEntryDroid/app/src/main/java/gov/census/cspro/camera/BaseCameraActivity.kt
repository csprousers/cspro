package gov.census.cspro.camera

import android.Manifest
import android.app.AlertDialog
import android.content.Intent
import android.content.pm.PackageManager
import android.hardware.Camera
import android.os.Bundle
import android.util.DisplayMetrics
import androidx.appcompat.app.AppCompatActivity
import androidx.core.app.ActivityCompat
import com.google.android.gms.common.ConnectionResult
import com.google.android.gms.common.GoogleApiAvailability
import com.google.android.gms.vision.Detector
import com.karumi.dexter.Dexter
import com.karumi.dexter.PermissionToken
import com.karumi.dexter.listener.PermissionDeniedResponse
import com.karumi.dexter.listener.PermissionGrantedResponse
import com.karumi.dexter.listener.PermissionRequest
import com.karumi.dexter.listener.single.PermissionListener
import gov.census.cspro.csentry.R
import gov.census.cspro.csentry.SystemSettings
import gov.census.cspro.engine.EngineInterface
import gov.census.cspro.util.Constants
import timber.log.Timber
import java.io.IOException

abstract class BaseCameraActivity : AppCompatActivity() {
    private var mCameraSource: CameraSource? = null
    @JvmField
    protected var mCameraFacing = CameraSource.CAMERA_FACING_BACK
    protected abstract val cameraSourcePreview: CameraSourcePreview?

    /**
     * Initializes the UI and creates the detector pipeline.
     */
    public override fun onCreate(icicle: Bundle?) {
        super.onCreate(icicle)
    }

    fun initCamera(detector: Detector<*>?) {
        Dexter
            .withContext(this)
            .withPermission(Manifest.permission.CAMERA)
            .withListener( object : PermissionListener {

                override fun onPermissionGranted(response: PermissionGrantedResponse) {
                    launchCamera(detector)
                }

                override fun onPermissionDenied(response: PermissionDeniedResponse) {
                    showPermissionDeniedError(R.string.permission_camera_rationale) { finish() }
                }

                override fun onPermissionRationaleShouldBeShown(permission: PermissionRequest, token: PermissionToken) {
                    showPermissionDeniedError(R.string.dialog_no_camera_permission) { token.continuePermissionRequest() }
                }
            } )
            .check()
    }

    private fun launchCamera(detector: Detector<*>?) {
        val autoFocus = true
        val useFlash = false
        createCameraSource(mCameraFacing, autoFocus, useFlash, detector)
    }

    private fun showPermissionDeniedError(messageId: Int, onDismiss: () -> Unit) {
        AlertDialog.Builder(this)
            .setTitle(R.string.dialog_no_camera_permission_title)
            .setMessage(messageId)
            .setPositiveButton(R.string.modal_dialog_helper_ok_text) { _, _ -> onDismiss() }
            .show()
    }

    private fun getDesiredPictureSize() : Pair<Double, Double> {
        val metrics = DisplayMetrics()
        windowManager.defaultDisplay.getMetrics(metrics)
        var width: Double = metrics.widthPixels.toDouble()
        var height: Double = metrics.heightPixels.toDouble()

        //getting aspect ratio from settings
        try {
            val ar = EngineInterface.GetSystemSettingString(SystemSettings.CameraAspectRatio, "").toDouble()
            if (ar > 0) {
                width = ar
                height = 1.0
            }
        } catch (e: Exception) {
            //nothing to do here
        }

        return width to height
    }

    /**
     * Creates and starts the camera.
     */
    fun createCameraSource(cameraFacing: Int, autoFocus: Boolean, useFlash: Boolean, detector: Detector<*>?) {

         // Creates and starts the camera.  Note that this uses a higher resolution in comparison
        // to other detection examples to enable the barcode detector to detect small barcodes
        // at long distances.
        //val metrics = DisplayMetrics()
        //windowManager.defaultDisplay.getMetrics(metrics)
        val (w, h) = getDesiredPictureSize()
        var builder = CameraSource.Builder(applicationContext, detector)
            .setFacing(cameraFacing)
            .setRequestedPreviewSize(w, h)
            .setRequestedFps(24.0f)

        // make sure that auto focus is an available option
		builder = builder.setFocusMode(if (autoFocus) Camera.Parameters.FOCUS_MODE_CONTINUOUS_PICTURE else null)
		
        if (mCameraSource != null) {
            cameraSourcePreview!!.stop()
            mCameraSource!!.stop()
            mCameraSource!!.release()
        }
        mCameraSource = builder
            .setFlashMode(if (useFlash) Camera.Parameters.FLASH_MODE_TORCH else null)
            .build()
        if (cameraSourcePreview != null && ActivityCompat.checkSelfPermission(this, Manifest.permission.CAMERA) == PackageManager.PERMISSION_GRANTED) {
            try {
                cameraSourcePreview!!.start(mCameraSource)
            } catch (e: IOException) {
                e.printStackTrace()
            }
        }
    }

    // Restarts the camera
    override fun onResume() {
        super.onResume()
        startCameraSource()
    }

    // Stops the camera
    override fun onPause() {
        super.onPause()
        if (cameraSourcePreview != null) {
            cameraSourcePreview!!.stop()
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

    /**
     * Releases the resources associated with the camera source, the associated detectors, and the
     * rest of the processing pipeline.
     */
    override fun onDestroy() {
        super.onDestroy()
        if (cameraSourcePreview != null) {
            cameraSourcePreview!!.release()
        }
    }

    /**
     * Starts or restarts the camera source, if it exists.  If the camera source doesn't exist yet
     * (e.g., because onResume was called before the camera source was created), this will be called
     * again when the camera source is created.
     */
    @Throws(SecurityException::class)
    private fun startCameraSource() {
        // check that the device has play services available.
        val code = GoogleApiAvailability.getInstance().isGooglePlayServicesAvailable(
            applicationContext)
        if (code != ConnectionResult.SUCCESS) {
            val dlg = GoogleApiAvailability.getInstance().getErrorDialog(this, code, RC_HANDLE_GMS)
            dlg?.show()
        }
        if (mCameraSource != null) {
            try {
                cameraSourcePreview!!.start(mCameraSource)
            } catch (e: IOException) {
                Timber.e(e, "Unable to start camera source.")
                mCameraSource!!.release()
                mCameraSource = null
            }
        }
    }

    fun takePicture(pictureCallback: Camera.PictureCallback?) {
        mCameraSource!!.takePicture(pictureCallback)
    }

    companion object {
        // Intent request code to handle updating play services if needed.
        private const val RC_HANDLE_GMS = 9001
    }
}