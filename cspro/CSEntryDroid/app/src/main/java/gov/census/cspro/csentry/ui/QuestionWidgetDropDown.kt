package gov.census.cspro.csentry.ui

import android.content.*
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.*
import android.widget.AdapterView.OnItemSelectedListener
import androidx.recyclerview.widget.RecyclerView
import com.bumptech.glide.RequestManager
import gov.census.cspro.csentry.R
import gov.census.cspro.dict.ValuePair
import gov.census.cspro.engine.*
import gov.census.cspro.form.CDEField
import java.util.*

/**
 * UI Widget for drop down list
 */
internal class QuestionWidgetDropDown(field: CDEField, adapter: RecyclerView.Adapter<QuestionWidgetViewHolder>, val imageLoader: RequestManager, setFocus: Boolean, emitNextPage: Boolean, showCodes: Boolean) : QuestionWidget(field, adapter, setFocus, emitNextPage, showCodes) {

    private var selectedResponse: Int = field.selectedIndex

    override val allItemViewTypes: IntArray
        get() = intArrayOf(VIEW_TYPE_QUESTION_COMMON_PARTS, VIEW_TYPE_DROP_DOWN)

    override fun getItemViewType(position: Int): Int {
        return if (position == 0) VIEW_TYPE_QUESTION_COMMON_PARTS else VIEW_TYPE_DROP_DOWN
    }

    override fun copyResponseToField() {
        if (selectedResponse >= 0) field.selectedIndex = selectedResponse
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
            VIEW_TYPE_DROP_DOWN -> {
                createDropDownViewHolder(viewGroup)
            }
            else -> {
                throw RuntimeException("Invalid view type")
            }
        }
    }

    override fun onBindViewHolder(viewHolder: QuestionWidgetViewHolder, position: Int,
                                  nextPageListener: NextPageListener?, editNotes: Boolean) {
        if (position == 0) {
            bindCommonParts(viewHolder, editNotes)
        } else {
            bindDropDown(viewHolder, nextPageListener)
        }
    }

    override val itemCount: Int
        get() = 2

    private fun createDropDownViewHolder(viewGroup: ViewGroup): ViewHolder {
        val inflater = LayoutInflater.from(viewGroup.context)
        val view = inflater.inflate(R.layout.question_widget_dropdown, viewGroup, false)
        return ViewHolder(view)
    }

    private fun bindDropDown(baseViewHolder: QuestionWidgetViewHolder, nextPageListener: NextPageListener?) {
        val viewHolder = baseViewHolder as ViewHolder
        viewHolder.bind(this, field.responses, selectedResponse, field.isReadOnly, nextPageListener)
    }

    private class ViewHolder(itemView: View) : QuestionWidgetViewHolder(itemView) {
        private val spinner: Spinner = itemView.findViewById(R.id.spinnerDropdown)
        private var questionWidget: QuestionWidgetDropDown? = null
        private var ignoreInitialSpinnerSelection = false

        fun bind(questionWidget: QuestionWidgetDropDown, responses: Array<ValuePair>, selectedIndex: Int, isReadOnly: Boolean, nextPageListener: NextPageListener?) {

            // disconnect any existing listener while value is changed
            spinner.onItemSelectedListener = null
            val values = ArrayList<ValuePair>(responses.size + 1)
            // Add an empty entry for no selection, this will not be shown in popup
            values.add(ValuePair("", "", android.graphics.Color.BLACK,  null, false))
            Collections.addAll(values, *responses)
            val adapter = Adapter(spinner.context, values, questionWidget.showCodes, questionWidget.imageLoader)
            spinner.adapter = adapter
            spinner.setSelection(selectedIndex + 1) // add 1 because of hidden element at start

            // Spinner always fires the selected event during first layout or after setting
            // the listener. Ignore the first event so we don't fire next page listener.
            ignoreInitialSpinnerSelection = true
            if (isReadOnly) {
                spinner.isEnabled = false
            } else {
                spinner.isEnabled = true
                spinner.onItemSelectedListener = object : OnItemSelectedListener {
                    override fun onItemSelected(parent: AdapterView<*>?, view: View, position: Int, rowId: Long) {
                        if (ignoreInitialSpinnerSelection) {
                            ignoreInitialSpinnerSelection = false
                        } else {
                            this@ViewHolder.questionWidget?.selectedResponse = position - 1 // subtract 1 because of
                            // hidden element at start
                            if (nextPageListener != null && this@ViewHolder.questionWidget?.emitNextPage == true &&
                                EngineInterface.getInstance().autoAdvanceOnSelectionFlag) nextPageListener.OnNextPage()
                        }
                    }

                    override fun onNothingSelected(adapterView: AdapterView<*>?) {
                        this@ViewHolder.questionWidget?.selectedResponse = -1
                    }
                }
            }
            this.questionWidget = questionWidget
        }

        /**
         * Special adapter that handles custom layout and
         * the hidden empty element in the list of items for no selection
         */
        private inner class Adapter(context: Context, values: List<ValuePair?>, private val m_showCodes: Boolean, private val m_imageLoader: RequestManager) : ArrayAdapter<ValuePair?>(context, R.layout.dropdown_answer_list_item, R.id.valueLabel, values) {
            override fun getView(position: Int, convertView: View?, parent: ViewGroup): View {
                val response = getItem(position)
                val currentView = if (convertView == null) {
                    val inflater = LayoutInflater.from(parent.context)
                    inflater.inflate(R.layout.dropdown_answer_list_item, parent, false)
                } else convertView
                val labelView = currentView.findViewById<TextView>(R.id.valueLabel)
                labelView?.text = response?.label ?: ""
                labelView?.setTextColor(response?.textColor ?: android.graphics.Color.BLACK)
                val codeView = currentView.findViewById<TextView>(R.id.valueCode)
                val code = response?.code
                if (!m_showCodes || Util.stringIsNullOrEmpty(code)) {
                    codeView?.visibility = View.GONE
                } else {
                    codeView?.visibility = View.VISIBLE
                    codeView?.text = code
                }
                return currentView
            }

            override fun getDropDownView(position: Int, convertView: View?, parent: ViewGroup): View {
                val response = getItem(position)
                if (position == 0) {
                    // This is the empty item that we don't want to show in popup
                    // Regular views are tagged so create a new view if attempting
                    // to reuse one of those.
                    return if (convertView == null || convertView.tag != null) {
                        View(context) // empty view will be zero height so it won't show up
                    } else {
                        convertView
                    }
                } else {
                    val view = if (convertView == null ||convertView.tag == null) {
                        val inflater = LayoutInflater.from(parent.context)
                        inflater.inflate(R.layout.dropdown_answer_list_popup_item, parent, false)
                            .also { it.tag = 1 } // Tag to differentiate from the empty view
                    } else {
                        convertView
                    }
                    val labelView = view.findViewById<TextView>(R.id.valueLabel)
                    labelView.text = response?.label ?: ""
					labelView.setTextColor(response?.textColor ?: android.graphics.Color.BLACK)
                    val codeView = view.findViewById<TextView>(R.id.valueCode)
                    val code = response?.code
                    if (!m_showCodes || Util.stringIsNullOrEmpty(code)) {
                        codeView.visibility = View.GONE
                    } else {
                        codeView.visibility = View.VISIBLE
                        codeView.text = code
                    }
                    val imageView = view.findViewById<ImageView>(R.id.valueSetImage)
                    if (response != null && !Util.stringIsNullOrEmpty(response.imagePath)) {
                        imageView.visibility = View.VISIBLE
                        m_imageLoader.load(response.imagePath)
                            .signature(getFileSignature(response.imagePath))
                            .fitCenter()
                            .into(imageView)
                    } else {
                        m_imageLoader.clear(imageView)
                        imageView.visibility = View.GONE
                    }
                    return view
                }
            }
        }

    }

}