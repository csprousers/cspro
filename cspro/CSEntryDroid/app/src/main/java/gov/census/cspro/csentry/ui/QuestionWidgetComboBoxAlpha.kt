package gov.census.cspro.csentry.ui

import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import androidx.recyclerview.widget.RecyclerView
import com.bumptech.glide.RequestManager
import gov.census.cspro.commonui.FieldEditText.ImeNextListener
import gov.census.cspro.commonui.MultiBoxAlpha
import gov.census.cspro.csentry.R
import gov.census.cspro.dict.ValuePair
import gov.census.cspro.form.CDEField

class QuestionWidgetComboBoxAlpha internal constructor(field: CDEField, adapter: RecyclerView.Adapter<QuestionWidgetViewHolder>, val imageLoader: RequestManager, setFocus: Boolean, emitNextPage: Boolean, showCodes: Boolean) : QuestionWidget(field, adapter, setFocus, emitNextPage, showCodes) {

    private var currentFieldValue: String? = this.field.alphaValue?.trim()
    private val responses: Array<ValuePair> = field.responses

    override val allItemViewTypes: IntArray
        get() = intArrayOf(VIEW_TYPE_QUESTION_COMMON_PARTS, VIEW_TYPE_COMBO_BOX_ALPHA)

    override fun getItemViewType(position: Int): Int {
        return if (position == 0) VIEW_TYPE_QUESTION_COMMON_PARTS else VIEW_TYPE_COMBO_BOX_ALPHA
    }

    override fun copyResponseToField() {
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
            VIEW_TYPE_COMBO_BOX_ALPHA -> {
                createComboBoxViewHolder(viewGroup)
            }
            else -> {
                throw AssertionError("Invalid view type")
            }
        }
    }

    override fun onBindViewHolder(viewHolder: QuestionWidgetViewHolder, position: Int, nextPageListener: NextPageListener?, editNotes: Boolean) {
        if (position == 0) {
            bindCommonParts(viewHolder, editNotes)
        } else {
            bindComboBox(viewHolder, nextPageListener)
        }
    }

    override val itemCount: Int
        get() = 2

    private fun createComboBoxViewHolder(viewGroup: ViewGroup): QuestionWidgetViewHolder {
        val inflater = LayoutInflater.from(viewGroup.context)
        val view = inflater.inflate(R.layout.question_widget_combo_box_alpha, viewGroup, false)
        return ViewHolder(view)
    }

    private fun bindComboBox(baseViewHolder: QuestionWidgetViewHolder, nextPageListener: NextPageListener?) {
        val viewHolder = baseViewHolder as ViewHolder
        viewHolder.bind(field, this, setFocus, nextPageListener)
        setFocus = false // Only set focus on first bind, first time page is shown
    }

    private class ViewHolder(itemView: View) : QuestionWidgetViewHolder(itemView) {

        private val comboBox: MultiBoxAlpha = itemView.findViewById(R.id.multiBox)
        private var questionWidget: QuestionWidgetComboBoxAlpha? = null
        private var nextPageListener: NextPageListener? = null

        init {

            comboBox.setValueChangedListener(object : MultiBoxAlpha.ValueChangedListener {
                override fun onEditTextChanged(value: String) {
                    questionWidget?.currentFieldValue = value

                }

                override fun onListItemSelected(itemIndex: Int) {}
            })

            comboBox.editText.setImeNextListener(ImeNextListener {
                if (questionWidget != null && questionWidget?.emitNextPage == true && nextPageListener != null) {
                    nextPageListener?.OnNextPage()
                    return@ImeNextListener true
                }
                false
            })

        }

        fun bind(field: CDEField, questionWidget: QuestionWidgetComboBoxAlpha, setFocus: Boolean, nextPageListener: NextPageListener?) {

            // Set the field to null to temporarily disable the listener
            // while we set the initial field value
            this.questionWidget = null
            comboBox.editText.setUppercase(field.isUpperCase)
            comboBox.editText.setMaxLength(field.alphaLength)
            comboBox.editText.setMultiline(false)
            comboBox.setResponses(questionWidget.responses, questionWidget.showCodes)

            // Set initial value
            comboBox.setAlphaValue(questionWidget.currentFieldValue)
            if (field.isReadOnly) {
                comboBox.setReadOnly(true)
            } else {
                comboBox.setReadOnly(false)

                // Move cursor to end
                comboBox.editText.setSelection(comboBox.editText.text.toString().length)
            }
            this.questionWidget = questionWidget
            comboBox.setImageLoader(questionWidget.imageLoader)
            this.nextPageListener = nextPageListener
            if (setFocus) comboBox.editText.focus()
        }

    }

}