package gov.census.cspro.csentry.ui

import android.app.Activity
import android.content.Intent
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import androidx.activity.result.ActivityResultLauncher
import androidx.activity.result.contract.ActivityResultContracts
import androidx.recyclerview.widget.RecyclerView
import com.bumptech.glide.RequestManager
import gov.census.cspro.csentry.CSEntry
import gov.census.cspro.csentry.databinding.QuestionWidgetSignatureBinding
import gov.census.cspro.engine.EngineInterface
import gov.census.cspro.engine.Util
import gov.census.cspro.form.CDEField
import gov.census.cspro.signature.SignatureActivity
import gov.census.cspro.util.Constants
import java.io.File

class QuestionWidgetSignature(field: CDEField, adapter: RecyclerView.Adapter<QuestionWidgetViewHolder>, val imageLoader: RequestManager, setFocus: Boolean, emitNextPage: Boolean, showCodes: Boolean):
    QuestionWidget(field, adapter, setFocus, emitNextPage, showCodes) {

    private var currentFieldValue: File? = null // TODO: initialize to value in field i.e. field.binaryValue

    override val itemCount = 2

    override val allItemViewTypes = intArrayOf(VIEW_TYPE_QUESTION_COMMON_PARTS, VIEW_TYPE_SIGNATURE)

    override fun getItemViewType(position: Int) =
        if (position == 0) VIEW_TYPE_QUESTION_COMMON_PARTS else VIEW_TYPE_SIGNATURE

    override fun copyResponseToField() {
        // TODO: save the signature image to the CDEField
        //field.binaryValue = readFileContents(currentFieldValue)
    }

    override fun supportsResponseFilter() = false

    override fun filterResponses(filterPattern: String) {
    }

    override val initialScrollPosition = 0

    override fun onCreateViewHolder(viewGroup: ViewGroup, viewType: Int) =
        when (viewType) {
            VIEW_TYPE_QUESTION_COMMON_PARTS -> {
                createCommonPartsViewHolder(viewGroup)
            }
            VIEW_TYPE_SIGNATURE -> {
                createSignatureViewHolder(viewGroup)
            }
            else -> {
                throw AssertionError("Invalid view type")
            }
        }

    override fun onBindViewHolder(viewHolder: QuestionWidgetViewHolder, position: Int, nextPageListener: NextPageListener?, editNotes: Boolean) {
        if (position == 0)
            bindCommonParts(viewHolder, editNotes)
        else
            bindSignatureViewHolder(viewHolder, nextPageListener)
    }

    private fun createSignatureViewHolder(parent: ViewGroup): QuestionWidgetViewHolder {
        val inflater = LayoutInflater.from(parent.context)
        val binding = QuestionWidgetSignatureBinding.inflate(inflater, parent, false)
        return SignatureViewHolder(binding)
    }

    private fun bindSignatureViewHolder(baseViewHolder: QuestionWidgetViewHolder, nextPageListener: NextPageListener?) {
        val signatureViewHolder = baseViewHolder as SignatureViewHolder
        signatureViewHolder.bind(field, this, nextPageListener)
    }

    class SignatureViewHolder(private val binding: QuestionWidgetSignatureBinding) : QuestionWidgetViewHolder(binding.root) {

        fun bind(field: CDEField, questionWidget: QuestionWidgetSignature, nextPageListener: NextPageListener?) {

            updateUI(field, questionWidget)

            val signatureActivityLauncher = registerForActivityResult(ActivityResultContracts.StartActivityForResult()) {
                onSignatureActivityResult(field, questionWidget, nextPageListener, it.resultCode, it.data)
            }

            binding.buttonGetSignature.setOnClickListener {
                captureSignature(signatureActivityLauncher, field.label)
            }

            binding.buttonDelete.setOnClickListener {
                questionWidget.currentFieldValue = null
                updateUI(field, questionWidget)
            }

            binding.image.setOnClickListener {
                viewSignature(questionWidget.currentFieldValue!!, signatureActivityLauncher)
            }
        }

        private fun updateUI(field: CDEField, questionWidget: QuestionWidgetSignature) {
            if (questionWidget.currentFieldValue != null) {
                binding.image.visibility = View.VISIBLE
                loadImage(questionWidget.imageLoader, questionWidget.currentFieldValue!!.path)
            } else {
                binding.image.visibility = View.GONE
            }

            if (field.isReadOnly) {
                binding.buttonGetSignature.visibility = View.GONE
                binding.buttonDelete.visibility = View.GONE
            } else {
                binding.buttonGetSignature.visibility = View.VISIBLE
                binding.buttonDelete.visibility =
                    if (questionWidget.currentFieldValue != null)
                        View.VISIBLE
                    else
                        View.GONE
            }
        }

        private fun loadImage(imageLoader: RequestManager, imagePath: String) {
            imageLoader.load(imagePath)
                .signature(getFileSignature(imagePath))
                .fitCenter()
                .into(binding.image)
        }

        private fun captureSignature(startForResult: ActivityResultLauncher<Intent>, label: String) {
            val intent = Intent(CSEntry.context, SignatureActivity::class.java)
            if (!Util.stringIsNullOrEmpty(label))
                intent.putExtra(Constants.EXTRA_SIGNATURE_MESSAGE_KEY, label)
            startForResult.launch(intent)
        }

        private fun onSignatureActivityResult(field: CDEField, questionWidget: QuestionWidgetSignature, nextPageListener: NextPageListener?, resultCode: Int, data: Intent?) {
            if (data != null && resultCode != Activity.RESULT_CANCELED) {
                val newFieldValue = File(data.getStringExtra(Constants.EXTRA_SIGNATURE_FILE_URL_KEY)!!)
                questionWidget.currentFieldValue = newFieldValue
                updateUI(field, questionWidget)
                if (nextPageListener != null && questionWidget.emitNextPage && EngineInterface.getInstance().autoAdvanceOnSelectionFlag)
                    nextPageListener.OnNextPage()
            }
        }

        private fun viewSignature(file: File, startForResult: ActivityResultLauncher<Intent>) {
            val intent = Intent(Intent.ACTION_VIEW).apply {
                setDataAndType(Util.getShareableUriForFile(file, itemView.context), "image/png")
                addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION)
            }
            startForResult.launch(intent)
        }

    }
}