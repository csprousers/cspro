package gov.census.cspro.csentry.ui

import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import androidx.recyclerview.widget.RecyclerView
import gov.census.cspro.commonui.FieldEditText.ImeNextListener
import gov.census.cspro.commonui.NumericFieldEditText
import gov.census.cspro.csentry.R
import gov.census.cspro.form.CDEField

/**
 * UI for numeric text entry boxes
 */
class QuestionWidgetTextBoxNumeric internal constructor(field: CDEField, adapter: RecyclerView.Adapter<QuestionWidgetViewHolder>, setFocus: Boolean, emitNextPage: Boolean, showCodes: Boolean) : QuestionWidget(field, adapter, setFocus, emitNextPage, showCodes) {

    private var currentFieldValue: Double? = field.numericValue // null indicates blank/notappl

    override val allItemViewTypes: IntArray
        get() = intArrayOf(VIEW_TYPE_QUESTION_COMMON_PARTS,
            VIEW_TYPE_NUMERIC_TEXT_BOX)

    override fun getItemViewType(position: Int): Int {
        return if (position == 0) VIEW_TYPE_QUESTION_COMMON_PARTS else VIEW_TYPE_NUMERIC_TEXT_BOX
    }

    override fun copyResponseToField() {
        field.numericValue = currentFieldValue
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
            VIEW_TYPE_NUMERIC_TEXT_BOX -> {
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

    private fun createTextBoxViewHolder(viewGroup: ViewGroup): ViewHolder {
        val inflater = LayoutInflater.from(viewGroup.context)
        val view = inflater.inflate(R.layout.question_widget_text_box_numeric, viewGroup, false)
        return ViewHolder(view)
    }

    private fun bindTextBox(baseViewHolder: QuestionWidgetViewHolder, nextPageListener: NextPageListener?) {
        val viewHolder = baseViewHolder as ViewHolder
        viewHolder.bind(field, this, setFocus, nextPageListener)
        setFocus = false // Only set focus on first bind, first time page is shown
    }

    private class ViewHolder(itemView: View) : QuestionWidgetViewHolder(itemView) {
        private val editText: NumericFieldEditText = itemView.findViewById(R.id.editText)
        private var questionWidget: QuestionWidgetTextBoxNumeric? = null
        private var nextPageListener: NextPageListener? = null

        init {
            editText.setValueChangedListener(object : NumericFieldEditText.ValueChangedListener {
                override fun onValueChanged(value: Double) {

                    questionWidget?.currentFieldValue = value

                }

                override fun onValueBlank() {
                    questionWidget?.currentFieldValue = null
                }
            })
            editText.setImeNextListener(ImeNextListener {
                if (questionWidget != null && questionWidget?.emitNextPage == true && nextPageListener != null) {
                    nextPageListener?.OnNextPage()
                    return@ImeNextListener true
                }
                false
            })
        }

        fun bind(field: CDEField, questionWidget: QuestionWidgetTextBoxNumeric,
                 setFocus: Boolean, nextPageListener: NextPageListener?) {

            // Set the field to null to temporarily disable the listener
            // while we set the initial field value
            this.questionWidget = null
            editText.setFormat(field.integerPartLength, field.fractionalPartLength)

            // Set initial value
            questionWidget.currentFieldValue?.let {
                editText.setNumericValue(it)
            } ?: editText.setText("")

            if (field.isReadOnly) {
                editText.setReadOnly(true)
            } else {
                editText.setReadOnly(false)

                // Move cursor to end
                editText.setSelection(editText.text.toString().length)
            }
            this.questionWidget = questionWidget
            this.nextPageListener = nextPageListener
            if (setFocus) editText.focus()
        }

    }

}