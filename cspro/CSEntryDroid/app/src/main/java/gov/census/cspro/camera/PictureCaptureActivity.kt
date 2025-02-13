package gov.census.cspro.camera

import android.app.Activity
import android.content.Intent
import android.os.Bundle
import android.view.View
import android.widget.ImageView
import android.widget.TextView
import gov.census.cspro.csentry.R
import gov.census.cspro.engine.Util
import gov.census.cspro.util.Constants
import timber.log.Timber
import java.io.File
import java.io.FileOutputStream

class PictureCaptureActivity : BaseCameraActivity() {
    private lateinit var previewText: TextView
    override var cameraSourcePreview: CameraSourcePreview? = null
    override fun onCreate(icicle: Bundle?) {
        super.onCreate(icicle)
        setContentView(R.layout.activity_picture_capture)
        initUI()
        initCamera(null)
    }

    private fun initUI() {
        cameraSourcePreview = findViewById(R.id.preview)
        previewText = findViewById(R.id.preview_text)
        if (intent != null && !Util.stringIsNullOrEmpty(intent.getStringExtra(Constants.EXTRA_USER_MESSAGE_KEY))) {
            previewText.visibility = View.VISIBLE
            previewText.text = intent.getStringExtra(Constants.EXTRA_USER_MESSAGE_KEY)
        } else {
            previewText.visibility = View.GONE
        }
        val autoFocus = true
        val useFlash = false

        //make camera layout visible to capture and rotate camera.
        findViewById<View>(R.id.camera_action_items).visibility = View.VISIBLE
        val ivCaptureImage = findViewById<ImageView>(R.id.iv_capture_image)
        ivCaptureImage.setOnClickListener { takePicture { data, _ -> saveImageToFile(data) } }
        val ivRotateCamera = findViewById<ImageView>(R.id.iv_rotate_camera)
        ivRotateCamera.setOnClickListener {
            mCameraFacing = if (mCameraFacing == CameraSource.CAMERA_FACING_BACK) CameraSource.CAMERA_FACING_FRONT else CameraSource.CAMERA_FACING_BACK
            createCameraSource(mCameraFacing, autoFocus, useFlash, null)
        }
    }

    private fun saveImageToFile(data: ByteArray) {
        val file = intent.getStringExtra(Constants.EXTRA_CAPTURE_IMAGE_FILE_URL_KEY)?.let { File(it) } ?: tempFile()
        try {
            file.parentFile?.mkdirs()
            FileOutputStream(file).use {
                it.write(data)
            }
            setResult(Activity.RESULT_OK, Intent().apply { putExtra(Constants.EXTRA_CAPTURE_IMAGE_FILE_URL_KEY, file.path) })
        } catch (e: Exception) {
            Timber.e(e)
            setResult(0)
        }
        finish()
    }

    private fun tempFile() = File.createTempFile("photo", null, cacheDir)
}