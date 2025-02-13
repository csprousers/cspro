package gov.census.cspro.csentry.ui

import android.annotation.SuppressLint
import android.app.Activity
import android.content.Intent
import android.text.InputType
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.ImageButton
import android.widget.Toast
import androidx.activity.result.contract.ActivityResultContracts
import androidx.appcompat.app.AppCompatActivity
import androidx.recyclerview.widget.RecyclerView
import gov.census.cspro.camera.BarcodeCaptureActivity
import gov.census.cspro.commonui.AlphaFieldEditText
import gov.census.cspro.commonui.ErrorDialogFragment
import gov.census.cspro.csentry.CSEntry
import gov.census.cspro.csentry.R
import gov.census.cspro.engine.EngineInterface
import gov.census.cspro.form.CDEField
import gov.census.cspro.util.Constants

class QuestionWidgetBarcodeAlpha internal constructor(field: CDEField, adapter: RecyclerView.Adapter<QuestionWidgetViewHolder>, setFocus: Boolean, emitNextPage: Boolean, showCodes: Boolean) : QuestionWidget(field, adapter, setFocus, emitNextPage, showCodes) {

    private var currentFieldValue: String? = this.field.alphaValue?.trim() // null indicates blank/notappl

    override val allItemViewTypes: IntArray
        get() = intArrayOf(VIEW_TYPE_QUESTION_COMMON_PARTS,
            VIEW_TYPE_BARCODE_BUTTON_ALPHA)

    override fun getItemViewType(position: Int): Int {
        return if (position == 0) VIEW_TYPE_QUESTION_COMMON_PARTS else VIEW_TYPE_BARCODE_BUTTON_ALPHA
    }

    override fun copyResponseToField() {
        if (currentFieldValue == null) field.numericValue = null else field.alphaValue = currentFieldValue
    }

    override fun supportsResponseFilter(): Boolean {
        return false
    }

    override fun filterResponses(filterPattern: String) {}
    override val initialScrollPosition: Int
        get() = 0

    override fun onCreateViewHolder(viewGroup: ViewGroup, viewType: Int): QuestionWidgetViewHolder {
        return when (viewType) {
            VIEW_TYPE_QUESTION_COMMON_PARTS -> {
                createCommonPartsViewHolder(viewGroup)
            }
            VIEW_TYPE_BARCODE_BUTTON_ALPHA -> {
                createBarcodeViewHolder(viewGroup)
            }
            else -> {
                throw AssertionError("Invalid view type")
            }
        }
    }

    override fun onBindViewHolder(viewHolder: QuestionWidgetViewHolder, position: Int,
                                  nextPageListener: NextPageListener?, editNotes: Boolean) {
        if (position == 0) bindCommonParts(viewHolder, editNotes) else bindBarcode(viewHolder, nextPageListener)
    }

    override val itemCount: Int
        get() = 2

    private fun createBarcodeViewHolder(viewGroup: ViewGroup): BarcodeViewHolder {
        val inflater = LayoutInflater.from(viewGroup.context)
        val view = inflater.inflate(R.layout.question_widget_barcode_button_alpha, viewGroup, false)
        return BarcodeViewHolder(view)
    }

    private fun bindBarcode(baseViewHolder: QuestionWidgetViewHolder, nextPageListener: NextPageListener?) {
        val barcodeViewHolder = baseViewHolder as BarcodeViewHolder
        barcodeViewHolder.bind(field, currentFieldValue, this, nextPageListener)
        setFocus = false // Only set focus on first bind, first time page is shown
    }

    @SuppressLint("ClickableViewAccessibility")
    internal class BarcodeViewHolder(itemView: View) : QuestionWidgetViewHolder(itemView) {
        private val barcodeReader: ImageButton = itemView.findViewById(R.id.barcodeRead)
        private val barcodeValue: AlphaFieldEditText = itemView.findViewById(R.id.barcodeValue)

        init {
            barcodeValue.setOnClickListener(null)
            barcodeValue.clearFocus()
            barcodeValue.setOnTouchListener(null)
            barcodeValue.inputType = InputType.TYPE_NULL
            barcodeValue.isEnabled = false
            barcodeValue.isFocusable = false
            barcodeValue.isFocusableInTouchMode = false
            barcodeValue.isLongClickable = false
            barcodeValue.isClickable = false
        }

        private fun onBarcodeScanResult(field: CDEField, resultCode: Int, intent: Intent?) {
            if (intent != null && resultCode != Activity.RESULT_CANCELED) {
                val barcodeWidgetReadValue = intent.getStringExtra(Constants.EXTRA_BARCODE_DISPLAYVALUE_KEY)!!
                if (field.alphaLength >= barcodeWidgetReadValue.length) {
                    barcodeValue.setText(barcodeWidgetReadValue)
                 } else {
                    val message = String.format(CSEntry.context.getString(R.string.error_barcode_length_small), barcodeWidgetReadValue, field.alphaLength.toString() + "")

                    // If length of barcode is smaller than the expected length
                    // then show an error since you have to specify the length
                    val errorDialog = ErrorDialogFragment.newInstance(
                        CSEntry.context.getString(R.string.error_case_title),
                        message, resultCode)
                    try {
                        errorDialog.show( (itemView.context as AppCompatActivity).supportFragmentManager, "errorDialog")
                    } catch (e: IllegalStateException) {
                        Toast.makeText(CSEntry.context, message, Toast.LENGTH_LONG).show()
                        //do nothing as this is trying to show dialog when state has changed.
                    }
                }
            }
        }

        fun bind(field: CDEField, barcodeReadValue: String?, questionWidget: QuestionWidgetBarcodeAlpha, nextPageListener: NextPageListener?) {

            if (barcodeReadValue != null) {
                barcodeValue.setText(barcodeReadValue)
            }

            val startForResult = registerForActivityResult(ActivityResultContracts.StartActivityForResult()) {
                onBarcodeScanResult(field, it.resultCode, it.data)
            }

            barcodeReader.setOnClickListener {
                val intent = Intent(CSEntry.context, BarcodeCaptureActivity::class.java)
                intent.putExtra(Constants.EXTRA_BARCODE_KEY, Constants.EXTRA_BARCODE_VALUE)
                startForResult.launch(intent)
            }

            barcodeValue.setTextChangedListener { newText -> questionWidget.currentFieldValue = newText }
            itemView.setOnClickListener {
                if (questionWidget.emitNextPage && nextPageListener != null &&
                    EngineInterface.getInstance().autoAdvanceOnSelectionFlag) nextPageListener.OnNextPage()
            }
        }

    }

}