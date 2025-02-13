package gov.census.cspro.csentry.ui

import android.app.Activity
import android.content.Intent
import android.net.Uri
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import androidx.activity.result.ActivityResultLauncher
import androidx.activity.result.contract.ActivityResultContracts
import androidx.recyclerview.widget.RecyclerView
import com.bumptech.glide.RequestManager
import gov.census.cspro.camera.PictureCaptureActivity
import gov.census.cspro.csentry.CSEntry
import gov.census.cspro.csentry.databinding.QuestionWidgetPhotoBinding
import gov.census.cspro.engine.EngineInterface
import gov.census.cspro.engine.Util
import gov.census.cspro.form.CDEField
import gov.census.cspro.util.Constants
import timber.log.Timber
import java.io.File
import java.io.FileOutputStream
import java.io.IOException
import java.io.InputStream
import java.lang.Exception


class QuestionWidgetPhoto(field: CDEField, adapter: RecyclerView.Adapter<QuestionWidgetViewHolder>, val imageLoader: RequestManager, setFocus: Boolean, emitNextPage: Boolean, showCodes: Boolean)
    : QuestionWidget(field, adapter, setFocus, emitNextPage, showCodes) {

    private var currentFieldValue: File? = null

    override val allItemViewTypes = intArrayOf(VIEW_TYPE_QUESTION_COMMON_PARTS, VIEW_TYPE_PHOTO)

    override val itemCount = 2

    override fun getItemViewType(position: Int) =
        if (position == 0) VIEW_TYPE_QUESTION_COMMON_PARTS else VIEW_TYPE_PHOTO

    override fun copyResponseToField() {
        // TODO
    }

    override fun supportsResponseFilter() = false

    override fun filterResponses(filterPattern: String) {}

    override val initialScrollPosition = 0

    override fun onCreateViewHolder(viewGroup: ViewGroup, viewType: Int) =
        when (viewType) {
            VIEW_TYPE_QUESTION_COMMON_PARTS -> {
                createCommonPartsViewHolder(viewGroup)
            }
            VIEW_TYPE_PHOTO -> {
                createPhotoViewHolder(viewGroup)
            }
            else -> {
                throw AssertionError("Invalid view type")
            }
        }

    override fun onBindViewHolder(viewHolder: QuestionWidgetViewHolder, position: Int, nextPageListener: NextPageListener?, editNotes: Boolean) {
        if (position == 0)
            bindCommonParts(viewHolder, editNotes)
        else
            bindPhoto(viewHolder, nextPageListener)
    }

    private fun createPhotoViewHolder(parent: ViewGroup): QuestionWidgetViewHolder {
        val inflater = LayoutInflater.from(parent.context)
        val binding = QuestionWidgetPhotoBinding.inflate(inflater, parent, false)
        return PhotoViewHolder(binding)
    }

    private fun bindPhoto(baseViewHolder: QuestionWidgetViewHolder, nextPageListener: NextPageListener?) {
        val photoViewHolder = baseViewHolder as PhotoViewHolder
        photoViewHolder.bind(field, this, nextPageListener)
    }

    class PhotoViewHolder(private val binding: QuestionWidgetPhotoBinding) : QuestionWidgetViewHolder(binding.root) {

        fun bind(field: CDEField, questionWidget: QuestionWidgetPhoto, nextPageListener: NextPageListener?) {

            updateUI(field, questionWidget)

            val cameraActivityLauncher = registerForActivityResult(ActivityResultContracts.StartActivityForResult()) {
                onCameraActivityResult(field, questionWidget, nextPageListener, it.resultCode, it.data)
            }

            binding.buttonTakePicture.setOnClickListener {
                takePhoto(cameraActivityLauncher, field.label)
            }

            val chooseImageActivityLauncher = registerForActivityResult(ActivityResultContracts.StartActivityForResult()) {
                onChooseImageResult(field, questionWidget, nextPageListener, it.resultCode, it.data)
            }

            binding.buttonChoosePicture.setOnClickListener {
                choosePhoto(chooseImageActivityLauncher)
            }

            binding.image.setOnClickListener {
                viewPhoto(questionWidget.currentFieldValue!!, cameraActivityLauncher)
            }
            binding.buttonDelete.setOnClickListener {
                questionWidget.currentFieldValue = null
                updateUI(field, questionWidget)
            }
        }

        private fun viewPhoto(file: File, startForResult: ActivityResultLauncher<Intent>) {
            val intent = Intent(Intent.ACTION_VIEW).apply {
                setDataAndType(Util.getShareableUriForFile(file, itemView.context), "image/jpeg")
                addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION)
            }
            startForResult.launch(intent)
        }

        private fun takePhoto(startForResult: ActivityResultLauncher<Intent>, label: String) {
            val intent = Intent(CSEntry.context, PictureCaptureActivity::class.java)
            if (!Util.stringIsNullOrEmpty(label))
                intent.putExtra(Constants.EXTRA_USER_MESSAGE_KEY, label)
            startForResult.launch(intent)
        }

        private fun choosePhoto(startForResult: ActivityResultLauncher<Intent>) {
            val intent = Intent(Intent.ACTION_GET_CONTENT).apply {
                type = "image/*"
            }
            startForResult.launch(intent)
        }

        private fun updateUI(field: CDEField, questionWidget: QuestionWidgetPhoto) {
            if (questionWidget.currentFieldValue != null) {
                binding.image.visibility = View.VISIBLE
                loadImage(questionWidget.imageLoader, questionWidget.currentFieldValue!!.path)
            } else {
                binding.image.visibility = View.GONE
            }

            if (field.isReadOnly) {
                binding.buttonTakePicture.visibility = View.GONE
                binding.buttonChoosePicture.visibility = View.GONE
                binding.buttonDelete.visibility = View.GONE
            } else {
                binding.buttonTakePicture.visibility = View.VISIBLE
                binding.buttonChoosePicture.visibility = View.VISIBLE
                binding.buttonDelete.visibility =
                    if (questionWidget.currentFieldValue != null)
                        View.VISIBLE
                    else
                        View.GONE
            }
        }

        private fun onCameraActivityResult(field: CDEField, questionWidget: QuestionWidgetPhoto, nextPageListener: NextPageListener?, resultCode: Int, data: Intent?) {
            if (data != null && resultCode != Activity.RESULT_CANCELED) {
                val newFieldValue = File(data.getStringExtra(Constants.EXTRA_CAPTURE_IMAGE_FILE_URL_KEY)!!)
                questionWidget.currentFieldValue = newFieldValue
                updateUI(field, questionWidget)
                if (nextPageListener != null && questionWidget.emitNextPage && EngineInterface.getInstance().autoAdvanceOnSelectionFlag)
                    nextPageListener.OnNextPage()
            }
        }

        private fun onChooseImageResult(field: CDEField, questionWidget: QuestionWidgetPhoto, nextPageListener: NextPageListener?, resultCode: Int, data: Intent?) {
            if (data != null && resultCode != Activity.RESULT_CANCELED) {
                val fullPhotoUri = data.data
                if (fullPhotoUri != null) {
                    val tempFile = File.createTempFile("photo", null, itemView.context.cacheDir)
                    try {
                        copyFromContentURI(fullPhotoUri, tempFile)
                        questionWidget.currentFieldValue = tempFile
                        updateUI(field, questionWidget)
                        if (nextPageListener != null && questionWidget.emitNextPage && EngineInterface.getInstance().autoAdvanceOnSelectionFlag)
                            nextPageListener.OnNextPage()

                    } catch (e: Exception) {
                        Timber.e(e, "Error copying picked image $fullPhotoUri")
                        tempFile.delete()
                    }
                }
            }
        }

        private fun copyFromContentURI(uri: Uri, tempFile: File) {
            val inputStream = itemView.context.contentResolver.openInputStream(uri)
                ?: throw IOException("Failed to open content URI $uri")
            inputStream.use {
                val outputStream = FileOutputStream(tempFile)
                outputStream.use {
                    Util.copyStreams(inputStream, outputStream)
                }
            }
        }

        private fun loadImage(imageLoader: RequestManager, imagePath: String) {
            imageLoader.load(imagePath)
                .signature(getFileSignature(imagePath))
                .fitCenter()
                .into(binding.image)
        }
    }
}