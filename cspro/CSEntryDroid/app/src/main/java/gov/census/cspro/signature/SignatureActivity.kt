package gov.census.cspro.signature

import android.app.Activity
import android.content.Intent
import android.graphics.Bitmap
import android.os.Bundle
import android.text.TextUtils
import androidx.appcompat.app.AppCompatActivity
import gov.census.cspro.csentry.databinding.ActivitySignatureBinding
import gov.census.cspro.engine.EngineInterface
import gov.census.cspro.util.Constants
import timber.log.Timber
import java.io.File
import java.io.FileOutputStream


class SignatureActivity :
    AppCompatActivity(),
    SignatureView.OnSignedListener {

    private lateinit var binding: ActivitySignatureBinding

    override fun onCreate(savedInstanceState: Bundle?)
    {
        super.onCreate(savedInstanceState)

        // If is killed in background while this activity is shown, Android will try to restore
        // directly to this activity but that causes issues since the EntryActivity is not restored
        // to the point where it called to display the signature. So just close the Signature activity so
        // that the EntryActivity is restored at the start of the questionnaire.
        if (EngineInterface.getInstance() == null || !EngineInterface.getInstance().isApplicationOpen) {
            finish()
        }

        binding = ActivitySignatureBinding.inflate(layoutInflater)
        setContentView(binding.root)

        initUI()
    }

    private fun initUI() {

        val optionalOverlayText = intent.getStringExtra(Constants.EXTRA_SIGNATURE_MESSAGE_KEY)
        if (optionalOverlayText != null && !TextUtils.isEmpty(optionalOverlayText))
            binding.previewText.text = optionalOverlayText

        binding.buttonClear.setOnClickListener { binding.signatureView.clear() }
        binding.buttonSave.setOnClickListener {
            onSignatureCaptured(binding.signatureView.getSignatureBitmap())
        }

        binding.signatureView.setOnSignedListener(this)

    }

    override fun onSigned()
    {
        binding.buttonSave.isEnabled = true
        binding.buttonClear.isEnabled = true
    }

    override fun onClear()
    {
        binding.buttonSave.isEnabled = false
        binding.buttonClear.isEnabled = false
    }

    private fun onSignatureCaptured(bitmap: Bitmap)
    {
        val file = intent.getStringExtra(Constants.EXTRA_SIGNATURE_FILE_URL_KEY)?.let { File(it) } ?: tempFile()
        val extension = file.extension
        try {
            file.parentFile?.mkdirs()
            FileOutputStream(file).use {
                // Convert the output file to Image such as .png/.jpg
                if (extension.contains("png"))
                {
                    bitmap.compress(Bitmap.CompressFormat.PNG, 100, it)
                } else if (extension.contains("jpg") || extension.contains("jpeg"))
                {
                    bitmap.compress(Bitmap.CompressFormat.JPEG, 100, it)
                } else
                    error("Invalid extension")
            }

            setResult(Activity.RESULT_OK, Intent().apply { putExtra(Constants.EXTRA_SIGNATURE_FILE_URL_KEY, file.path) })
        } catch (e: Exception)
        {
            Timber.e(e, "Error saving signature to file ${file.path}")
            setResult(RESULT_CANCELED)
        }
        finish()
    }

    private fun tempFile() = File.createTempFile("signature", ".png", cacheDir)
}