package gov.census.cspro.csentry.ui

import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import androidx.recyclerview.widget.RecyclerView
import gov.census.cspro.commonui.AlphaFieldEditText
import gov.census.cspro.commonui.FieldEditText.ImeNextListener
import gov.census.cspro.csentry.R
import gov.census.cspro.form.CDEField

/**
 * UI Widget for alpha text box capture type question
 */
class QuestionWidgetTextBoxAlpha internal constructor(field: CDEField, adapter: RecyclerView.Adapter<QuestionWidgetViewHolder>, setFocus: Boolean, emitNextPage: Boolean, showCodes: Boolean) : QuestionWidget(field, adapter, setFocus, emitNextPage, showCodes) {

    private var currentFieldValue: String? = this.field.alphaValue?.trim()

    override val allItemViewTypes: IntArray
        get() = intArrayOf(VIEW_TYPE_QUESTION_COMMON_PARTS, VIEW_TYPE_ALPHA_TEXT_BOX)

    override fun getItemViewType(position: Int): Int {
        return if (position == 0) VIEW_TYPE_QUESTION_COMMON_PARTS else VIEW_TYPE_ALPHA_TEXT_BOX
    }

    public override fun copyResponseToField() {
        field.alphaValue = currentFieldValue
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
            VIEW_TYPE_ALPHA_TEXT_BOX -> {
                createTextBoxViewHolder(viewGroup)
            }
            else -> {
                throw AssertionError("Invalid view type")
            }
        }
    }

    override fun onBindViewHolder(viewHolder: QuestionWidgetViewHolder, position: Int,
                                  nextPageListener: NextPageListener?, editNotes: Boolean) {
        if (position == 0) bindCommonParts(viewHolder, editNotes) else bindTextBox(viewHolder, nextPageListener)
    }

    override val itemCount: Int
        get() = 2

    private fun createTextBoxViewHolder(viewGroup: ViewGroup): QuestionWidgetViewHolder {
        val inflater = LayoutInflater.from(viewGroup.context)
        val view = inflater.inflate(R.layout.question_widget_text_box_alpha, viewGroup, false)
        return ViewHolder(view)
    }

    private fun bindTextBox(baseViewHolder: QuestionWidgetViewHolder,
                            nextPageListener: NextPageListener?) {
        val viewHolder = baseViewHolder as ViewHolder
        viewHolder.bind(field, this, setFocus, nextPageListener)
        setFocus = false // Only set focus on first bind, first time page is shown
    }

    private class ViewHolder(itemView: View) : QuestionWidgetViewHolder(itemView) {
        private val editText: AlphaFieldEditText = itemView.findViewById(R.id.editText)
        private var questionWidget: QuestionWidgetTextBoxAlpha? = null
        private var nextPageListener: NextPageListener? = null


        init {
            editText.setTextChangedListener { newText -> questionWidget?.currentFieldValue = newText }
            editText.setImeNextListener(ImeNextListener {
                if (questionWidget != null && questionWidget?.emitNextPage == true && nextPageListener != null) {
                    nextPageListener?.OnNextPage()
                    return@ImeNextListener true
                }
                false
            })
        }

        fun bind(field: CDEField, questionWidget: QuestionWidgetTextBoxAlpha, setFocus: Boolean, nextPageListener: NextPageListener?) {

            // Set the field to null to temporarily disable the listener
            // while we set the initial field value
            this.questionWidget = null
            if (field.isReadOnly) {
                editText.setReadOnly(true)
            } else {
                editText.setReadOnly(false)
                editText.setUppercase(field.isUpperCase)
                editText.setMaxLength(field.alphaLength)
                editText.setMultiline(field.isMultiline)
            }

            // Set initial value
            editText.setText(questionWidget.currentFieldValue)

            // Move cursor to end
            if (!field.isReadOnly) editText.setSelection(editText.text.toString().length)
            this.questionWidget = questionWidget
            this.nextPageListener = nextPageListener
            if (setFocus) editText.focus()
        }

    }

}