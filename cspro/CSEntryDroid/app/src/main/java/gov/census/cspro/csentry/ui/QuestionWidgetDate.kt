package gov.census.cspro.csentry.ui

import android.annotation.SuppressLint
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.DatePicker
import androidx.recyclerview.widget.RecyclerView
import gov.census.cspro.csentry.R
import gov.census.cspro.engine.Util
import gov.census.cspro.form.CDEField
import java.text.ParseException
import java.text.SimpleDateFormat
import java.util.*

/**
 * UI widget for date capture type
 */
internal class QuestionWidgetDate(field: CDEField, adapter: RecyclerView.Adapter<QuestionWidgetViewHolder>, setFocus: Boolean, emitNextPage: Boolean, showCodes: Boolean) : QuestionWidget(field, adapter, setFocus, emitNextPage, showCodes) {

    private var year: Int
    private var month: Int
    private var day: Int

    init {
        val calendar = if (field.isAlpha) {
            val dateString = field.alphaValue
            if (Util.stringIsNullOrEmptyTrim(dateString)) Calendar.getInstance() // default to current date
            else toDate(field.alphaValue, field.captureDateFormat)
        } else {
            if (field.numericValue == null) {
                Calendar.getInstance() // default to current date
            } else {
                toDate(field.numericValue?.toInt().toString(), field.captureDateFormat)
            }
        }

        // If date could not be parsed default to current date
        val c = calendar ?: Calendar.getInstance()
        year = c[Calendar.YEAR]
        month = c[Calendar.MONTH]
        day = c[Calendar.DAY_OF_MONTH]
    }

    override val allItemViewTypes: IntArray
        get() = intArrayOf(VIEW_TYPE_QUESTION_COMMON_PARTS, VIEW_TYPE_DATE_PICKER)

    override fun getItemViewType(position: Int): Int {
        return if (position == 0) VIEW_TYPE_QUESTION_COMMON_PARTS else VIEW_TYPE_DATE_PICKER
    }

    override fun copyResponseToField() {
        val dateString = toDateString(year, month, day, field.captureDateFormat)
        if (field.isAlpha) {
            field.alphaValue = dateString
        } else {
            field.numericValue = dateString.toDouble()
        }
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
            VIEW_TYPE_DATE_PICKER -> {
                createDateViewHolder(viewGroup)
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
            bindDate(viewHolder)
        }
    }

    override val itemCount: Int
        get() = 2

    private fun createDateViewHolder(viewGroup: ViewGroup): ViewHolder {
        val inflater = LayoutInflater.from(viewGroup.context)
        val view = inflater.inflate(R.layout.question_widget_date_picker, viewGroup, false)
        return ViewHolder(view)
    }

    private fun bindDate(baseViewHolder: QuestionWidgetViewHolder) {
        val viewHolder = baseViewHolder as ViewHolder
        viewHolder.bind(this)
    }

    private class ViewHolder(itemView: View) : QuestionWidgetViewHolder(itemView) {
        private val datePicker: DatePicker = itemView.findViewById(R.id.datePicker)
        private var questionWidget: QuestionWidgetDate? = null

        fun bind(questionWidget: QuestionWidgetDate) {
            updateSpinners(questionWidget.field.captureDateFormat)

            // disconnect any existing listener while value is changed
            this.questionWidget = null
            datePicker.init(questionWidget.year, questionWidget.month, questionWidget.day
            ) { _, year, monthOfYear, dayOfMonth ->

                this.questionWidget?.year = year
                this.questionWidget?.month = monthOfYear
                this.questionWidget?.day = dayOfMonth

            }
            datePicker.isEnabled = !questionWidget.field.isReadOnly
            this.questionWidget = questionWidget
        }

        private fun updateSpinners(format: String) {
            setSpinnerVisibility("android:id/year", format.contains("y"))
            setSpinnerVisibility("android:id/month", format.contains("M"))
            setSpinnerVisibility("android:id/day", format.contains("d"))
        }

        private fun setSpinnerVisibility(spinnerName: String, show: Boolean) {
            // get the resource id for the spinner
            val spinnerId = datePicker.context.resources.getIdentifier(spinnerName, null, null)

            // if we get the right value continue
            if (spinnerId != 0) {
                // get the component from the datepicker
                val spinner = datePicker.findViewById<View>(spinnerId)
                if (spinner != null) {
                    // show/hide the control from the composite
                    spinner.visibility = if (show) View.VISIBLE else View.GONE
                }
            }
        }

    }

    // date formatting
    @SuppressLint("SimpleDateFormat")
    private fun toDateString(year: Int, monthOfYear: Int, dayOfMonth: Int, javaFormat: String): String {
        val cal = Calendar.getInstance()
        val formattedDate: String
        val dateFormatter = SimpleDateFormat(javaFormat)
        // set the calendar fields
        cal[Calendar.MONTH] = monthOfYear
        cal[Calendar.DAY_OF_MONTH] = dayOfMonth
        cal[Calendar.YEAR] = year
        // convert to a string
        formattedDate = dateFormatter.format(cal.time)
        return formattedDate
    }

    private fun padDateFormat(dateValue: String, dateFormat: String): String {
        return if (dateValue.length < dateFormat.length)
            Util.padLeft(dateFormat.length, '0', dateValue)
        else
            dateValue
    }

    @SuppressLint("SimpleDateFormat")
    private fun toDate(dateValue: String?, dateFormat: String): Calendar? {
        return dateValue?.let { dateString ->

            try {

                // convert the string to a calendar object
                val date = SimpleDateFormat(dateFormat).parse(padDateFormat(dateString, dateFormat))!!
                Calendar.getInstance().also { it.time = date }

            } catch (ignored: ParseException) {
                null
            }
        }
    }
}