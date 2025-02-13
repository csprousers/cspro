package gov.census.cspro.csentry.ui

import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.*
import androidx.recyclerview.widget.RecyclerView
import com.bumptech.glide.RequestManager
import gov.census.cspro.csentry.R
import gov.census.cspro.dict.ValuePair
import gov.census.cspro.engine.Util
import gov.census.cspro.form.CDEField

/**
 * UI widget for toggle button capture type
 */
internal class QuestionWidgetToggleButton(field: CDEField, adapter: RecyclerView.Adapter<QuestionWidgetViewHolder>, val imageLoader: RequestManager, setFocus: Boolean, emitNextPage: Boolean, showCodes: Boolean) : QuestionWidget(field, adapter, setFocus, emitNextPage, showCodes) {

    private var selectedResponse: Int = field.selectedIndex
	private val responses: Array<ValuePair> = field.responses

    override val allItemViewTypes: IntArray
        get() = intArrayOf(VIEW_TYPE_QUESTION_COMMON_PARTS, VIEW_TYPE_TOGGLE_BUTTON_SLIDER)

    override fun getItemViewType(position: Int): Int {
        return if (position == 0) VIEW_TYPE_QUESTION_COMMON_PARTS else VIEW_TYPE_TOGGLE_BUTTON_SLIDER
    }

    public override fun copyResponseToField() {
		field.selectedIndex = selectedResponse
    }

    override fun supportsResponseFilter() = false

    override fun filterResponses(filterPattern: String) {
    }

    override val initialScrollPosition = 0

    override fun onCreateViewHolder(viewGroup: ViewGroup, viewType: Int): QuestionWidgetViewHolder {
        return when (viewType) {
            VIEW_TYPE_QUESTION_COMMON_PARTS -> {
                createCommonPartsViewHolder(viewGroup)
            }
            VIEW_TYPE_TOGGLE_BUTTON_SLIDER -> {
                createToggleButtonViewHolder(viewGroup)
            }
            else -> {
                throw AssertionError("Invalid view type")
            }
        }
    }

    override fun onBindViewHolder(viewHolder: QuestionWidgetViewHolder, position: Int,
                                  nextPageListener: NextPageListener?, editNotes: Boolean) {
        if (position == 0) {
            bindCommonParts(viewHolder, editNotes)
        } else {
            bindToggleButton(viewHolder)
        }
    }

    override val itemCount = 2

    private fun createToggleButtonViewHolder(viewGroup: ViewGroup): ViewHolder {
        val inflater = LayoutInflater.from(viewGroup.context)
        val view = inflater.inflate(R.layout.toggle_button_answer_list_item, viewGroup, false)
        return ViewHolder(view)
    }

    private fun bindToggleButton(baseViewHolder: QuestionWidgetViewHolder) {
        val viewHolder = baseViewHolder as ViewHolder
        assert(responses.size == 1)
        viewHolder.bind(this, responses[0], selectedResponse == 0)
    }

    private class ViewHolder(itemView: View) : QuestionWidgetViewHolder(itemView) {
        private val labelTextView: TextView = itemView.findViewById(R.id.valueLabel)
        private val codeTextView: TextView = itemView.findViewById(R.id.valueCode)
        private val toggleButtonSwitch: Switch = itemView.findViewById(R.id.toggleButtonSwitch)
        private val imageView: ImageView = itemView.findViewById(R.id.valueSetImage)
        private var questionWidget: QuestionWidgetToggleButton? = null

        fun bind(questionWidget: QuestionWidgetToggleButton, response: ValuePair, isSelected: Boolean) {
            labelTextView.text = response.label
			labelTextView.setTextColor(response.textColor)
            val code = response.code
            if (!questionWidget.showCodes || Util.stringIsNullOrEmpty(code)) {
                codeTextView.visibility = View.GONE
            } else {
                codeTextView.visibility = View.VISIBLE
                codeTextView.text = code
            }

            // Set the image if there is one.
            val imagePath = response.imagePath
            if (!Util.stringIsNullOrEmpty(imagePath)) {
                loadImage(questionWidget.imageLoader, imagePath)
            } else {
                clearImage(questionWidget.imageLoader)
            }
            toggleButtonSwitch.isChecked = isSelected;
            if (!questionWidget.field.isReadOnly) {
                toggleButtonSwitch.isEnabled = true
                itemView.setOnClickListener {
                    toggleButtonSwitch.isChecked = !toggleButtonSwitch.isChecked
                    this.questionWidget?.selectedResponse = if (toggleButtonSwitch.isChecked) 0 else -1
                }
            } else {
                toggleButtonSwitch.isEnabled = false
                itemView.setOnClickListener(null)
            }
            this.questionWidget = questionWidget
        }

        private fun clearImage(imageLoader: RequestManager) {
            imageLoader.clear(imageView)
            imageView.visibility = View.GONE
        }

        private fun loadImage(imageLoader: RequestManager, imagePath: String?) {
            imageView.visibility = View.VISIBLE
            imageLoader.load(imagePath)
                .signature(getFileSignature(imagePath))
                .fitCenter()
                .into(imageView)
        }
    }
}
