package gov.census.cspro.csentry.ui

import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import androidx.recyclerview.widget.RecyclerView
import com.google.android.material.slider.Slider
import gov.census.cspro.commonui.NumericFieldTextView
import gov.census.cspro.commonui.NumericSliderField
import gov.census.cspro.csentry.R
import gov.census.cspro.engine.EngineInterface
import gov.census.cspro.form.CDEField
import timber.log.Timber

/**
 * UI for numeric Slider entry
 */
class QuestionWidgetSliderNumeric internal constructor(private var sliderField: CDEField, adapter: RecyclerView.Adapter<QuestionWidgetViewHolder>, setFocus: Boolean, emitNextPage: Boolean, showCodes: Boolean) : QuestionWidget(sliderField, adapter, setFocus, emitNextPage, showCodes) {

    private var currentFieldValue: Double? = sliderField.numericValue // null indicates blank/notappl

    override val allItemViewTypes: IntArray
        get() = intArrayOf(VIEW_TYPE_QUESTION_COMMON_PARTS,
            VIEW_TYPE_SLIDER_NUMERIC)

    override fun getItemViewType(position: Int): Int {
        return if (position == 0) VIEW_TYPE_QUESTION_COMMON_PARTS else VIEW_TYPE_SLIDER_NUMERIC
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
            VIEW_TYPE_SLIDER_NUMERIC -> {
                createSliderViewHolder(viewGroup)
            }
            else -> {
                throw AssertionError("Invalid view type")
            }
        }
    }

    override fun onBindViewHolder(viewHolder: QuestionWidgetViewHolder, position: Int,
                                  nextPageListener: NextPageListener?, editNotes: Boolean) {
        if (position == 0) bindCommonParts(viewHolder, editNotes) else bindSlider(viewHolder, nextPageListener)
    }

    override val itemCount: Int
        get() = 2

    private fun createSliderViewHolder(viewGroup: ViewGroup): SliderViewHolder {
        val inflater = LayoutInflater.from(viewGroup.context)
        val view = inflater.inflate(R.layout.question_widget_slider_numeric, viewGroup, false)
        return SliderViewHolder(view)
    }

    private fun bindSlider(baseViewHolder: QuestionWidgetViewHolder,
                           nextPageListener: NextPageListener?) {
        val sliderViewHolder = baseViewHolder as SliderViewHolder
        sliderViewHolder.bind(sliderField, this, nextPageListener)
        this.setFocus = false // Only set focus on first bind, first time page is shown
    }

    internal class SliderViewHolder(itemView: View) : QuestionWidgetViewHolder(itemView) {

        private val sliderField: NumericSliderField = itemView.findViewById(R.id.sliderField)
        private val sliderReadingValue: NumericFieldTextView = itemView.findViewById(R.id.sliderReadingValue)
        private val minRange: NumericFieldTextView = itemView.findViewById(R.id.minRange)
        private val maxRange: NumericFieldTextView = itemView.findViewById(R.id.maxRange)

        fun bind(field: CDEField, questionWidget: QuestionWidgetSliderNumeric?,
                 nextPageListener: NextPageListener?) {

            sliderField.clearOnChangeListeners()
            try {
                sliderReadingValue.setFormat(field.integerPartLength, field.fractionalPartLength)
                minRange.setFormat(field.integerPartLength, field.fractionalPartLength)
                maxRange.setFormat(field.integerPartLength, field.fractionalPartLength)
                minRange.setNumericValue(field.sliderMinValue)
                maxRange.setNumericValue(field.sliderMaxValue)

                sliderField.valueFrom = field.sliderMinValue.toFloat()
                sliderField.valueTo = field.sliderMaxValue.toFloat()
                val value = field.numericValue
                if (value != null) {
                    sliderField.value = value.toFloat()
                    sliderReadingValue.setNumericValue(sliderField.value.toDouble())
                } else {
                    sliderField.value = field.sliderMinValue.toFloat()
                    //if fresh start the numeric value is null, then
                    sliderReadingValue.text = ""
                }
                sliderField.stepSize = field.sliderStep.toFloat()
            } catch (e: NullPointerException) {
                Timber.e(e)
            }
            if (field.isReadOnly) {
                sliderField.setReadOnly(true)
            } else {
                sliderField.setReadOnly(false)
                sliderField.addOnChangeListener(Slider.OnChangeListener { _, value, _ ->
                    if (questionWidget != null) {
                        questionWidget.currentFieldValue = value.toDouble()
                        sliderReadingValue.setNumericValue(value.toDouble())
                    }
                })
            }
            itemView.setOnClickListener {
                if (questionWidget?.emitNextPage == true && nextPageListener != null &&
                    EngineInterface.getInstance().autoAdvanceOnSelectionFlag) nextPageListener.OnNextPage()
            }
        }

    }

}