package gov.census.cspro.csentry.ui

import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.CheckBox
import android.widget.ImageView
import android.widget.TextView
import android.widget.Toast
import androidx.recyclerview.widget.RecyclerView
import com.bumptech.glide.RequestManager
import gov.census.cspro.csentry.R
import gov.census.cspro.dict.ValuePair
import gov.census.cspro.engine.Util
import gov.census.cspro.form.CDEField
import java.util.regex.Pattern

/**
 * UI widget for check box capture type
 */
internal class QuestionWidgetCheckBoxes(field: CDEField, adapter: RecyclerView.Adapter<QuestionWidgetViewHolder>, val imageLoader: RequestManager, setFocus: Boolean, emitNextPage: Boolean, showCodes: Boolean) : QuestionWidget(field, adapter, setFocus, emitNextPage, showCodes) {

    private val responses: Array<ValuePair> = field.responses
    private var filteredResponses: Array<ValuePair> = responses
    private val checkedResponses: MutableSet<ValuePair> = field.checkedIndices.map { i -> responses[i] }.toMutableSet()
    private val maxSelections: Int = field.maxCheckboxSelections

    override val allItemViewTypes: IntArray
        get() = intArrayOf(VIEW_TYPE_QUESTION_COMMON_PARTS, VIEW_TYPE_CHECK_BOX_LIST_ITEM)

    override fun getItemViewType(position: Int): Int {
        return if (position == 0) VIEW_TYPE_QUESTION_COMMON_PARTS else VIEW_TYPE_CHECK_BOX_LIST_ITEM
    }

    public override fun copyResponseToField() {
        field.checkedIndices = responses.indices.filter { checkedResponses.contains(responses[it]) }.toIntArray()
    }

    override fun supportsResponseFilter(): Boolean {
        return true
    }

    override fun filterResponses(filterPattern: String) {
        val numOldItems = filteredResponses.size
        filteredResponses = if (Util.stringIsNullOrEmptyTrim(filterPattern)) {
            responses
        } else {
            val searchPattern = Pattern.compile(filterPattern, Pattern.CASE_INSENSITIVE or Pattern.LITERAL)
            responses.filter { searchPattern.matcher(it.label).find() }.toTypedArray()

        }
        if (numOldItems != filteredResponses.size) {
            notifyItemRangeRemoved(1, numOldItems)
            notifyItemRangeInserted(1, filteredResponses.size)
        }
    }

    override val initialScrollPosition: Int
        get() {
            for (i in responses.indices) {
                if (checkedResponses.contains(responses[i])) return i + 1
            }
            return 0
        }

    override fun onCreateViewHolder(viewGroup: ViewGroup, viewType: Int): QuestionWidgetViewHolder {
        return when (viewType) {
            VIEW_TYPE_QUESTION_COMMON_PARTS -> {
                createCommonPartsViewHolder(viewGroup)
            }
            VIEW_TYPE_CHECK_BOX_LIST_ITEM -> {
                createCheckBoxViewHolder(viewGroup)
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
            bindCheckBox(viewHolder, position - 1)
        }
    }

    override val itemCount: Int
        get() = 1 + filteredResponses.size

    private fun createCheckBoxViewHolder(parent: ViewGroup): QuestionWidgetViewHolder {
        val inflater = LayoutInflater.from(parent.context)
        val view = inflater.inflate(R.layout.check_box_answer_list_item, parent, false)
        return ViewHolder(view)
    }

    private fun bindCheckBox(baseViewHolder: QuestionWidgetViewHolder, responseIndex: Int) {
        val viewHolder = baseViewHolder as ViewHolder
        viewHolder.bind(this, filteredResponses[responseIndex],
            checkedResponses.contains(filteredResponses[responseIndex]))
    }

    private class ViewHolder(itemView: View) : QuestionWidgetViewHolder(itemView) {
        private val labelTextView: TextView = itemView.findViewById(R.id.valueLabel)
        private val codeTextView: TextView = itemView.findViewById(R.id.valueCode)
        private val checkBox: CheckBox = itemView.findViewById(R.id.checkbox)
        private val imageView: ImageView = itemView.findViewById(R.id.valueSetImage)
        private var response: ValuePair? = null
        private var questionWidget: QuestionWidgetCheckBoxes? = null

        fun bind(questionWidget: QuestionWidgetCheckBoxes, response: ValuePair, isSelected: Boolean) {
            this.response = response
            labelTextView.text = response.label
			labelTextView.setTextColor(response.textColor)
            val code = response.code
            if (!questionWidget.showCodes || Util.stringIsNullOrEmpty(code)) {
                codeTextView.visibility = View.GONE
            } else {
                codeTextView.visibility = View.VISIBLE
                codeTextView.text = code
            }
            if (response.isSelectable) {
                checkBox.visibility = View.VISIBLE
                itemView.isEnabled = true
            } else {
                checkBox.visibility = View.GONE
                itemView.isEnabled = false
            }

            // Set the image if there is one.
            val imagePath = response.imagePath
            if (!Util.stringIsNullOrEmpty(imagePath)) {
                loadImage(questionWidget.imageLoader, imagePath)
            } else {
                clearImage(questionWidget.imageLoader)
            }
            checkBox.isChecked = isSelected
            if (!questionWidget.field.isReadOnly) {
                checkBox.isEnabled = true
                itemView.setOnClickListener {
                    if (checkBox.isChecked) {
                        checkBox.isChecked = false
                        this.response?.let { this.questionWidget?.checkedResponses?.remove(it) }
                    } else {
                        val widget = this.questionWidget
                        if (widget != null && widget.checkedResponses.size >= widget.maxSelections) {
                            val message = String.format(checkBox.context.getString(R.string.validation_too_many_checks),
                                widget.maxSelections)
                            Toast.makeText(checkBox.context, message, Toast.LENGTH_SHORT).show()
                            checkBox.isChecked = false
                        } else {
                            checkBox.isChecked = true
                            this.response?.let { widget?.checkedResponses?.add(it) }
                        }
                    }
                }
            } else {
                checkBox.isEnabled = false
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