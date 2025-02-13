package gov.census.cspro.csentry.ui

import android.graphics.drawable.Drawable
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.ImageView
import android.widget.RadioButton
import android.widget.TextView
import androidx.recyclerview.widget.RecyclerView
import com.bumptech.glide.RequestManager
import com.bumptech.glide.load.DataSource
import com.bumptech.glide.load.engine.GlideException
import com.bumptech.glide.request.RequestListener
import com.bumptech.glide.request.RequestOptions
import com.bumptech.glide.request.target.Target
import gov.census.cspro.csentry.R
import gov.census.cspro.dict.ValuePair
import gov.census.cspro.engine.EngineInterface
import gov.census.cspro.engine.Util
import gov.census.cspro.form.CDEField
import timber.log.Timber
import java.util.regex.Pattern

/**
 * UI Widget for radio button capture type questions
 */
internal class QuestionWidgetRadioButtons(field: CDEField, adapter: RecyclerView.Adapter<QuestionWidgetViewHolder>, val imageLoader: RequestManager, setFocus: Boolean, emitNextPage: Boolean, showCodes: Boolean) : QuestionWidget(field, adapter, setFocus, emitNextPage, showCodes) {

    private var selectedResponse: Int = field.selectedIndex
    private val responses: Array<ValuePair> = field.responses
    private var filteredResponses: Array<ValuePair> = responses

    override val allItemViewTypes: IntArray
        get() = intArrayOf(VIEW_TYPE_QUESTION_COMMON_PARTS, VIEW_TYPE_RADIO_BUTTON_LIST_ITEM)

    override fun getItemViewType(position: Int): Int {
        return if (position == 0) VIEW_TYPE_QUESTION_COMMON_PARTS else VIEW_TYPE_RADIO_BUTTON_LIST_ITEM
    }

    public override fun copyResponseToField() {
        if (selectedResponse >= 0) field.selectedIndex = selectedResponse
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
        get() = selectedResponse + 1

    override fun onCreateViewHolder(viewGroup: ViewGroup, viewType: Int): QuestionWidgetViewHolder {
        return when (viewType) {
            VIEW_TYPE_QUESTION_COMMON_PARTS -> {
                createCommonPartsViewHolder(viewGroup)
            }
            VIEW_TYPE_RADIO_BUTTON_LIST_ITEM -> {
                createRadioButtonViewHolder(viewGroup)
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
            bindRadioButton(viewHolder, position - 1, nextPageListener)
        }
    }

    override val itemCount: Int
        get() = 1 + filteredResponses.size

    private fun createRadioButtonViewHolder(viewGroup: ViewGroup): ViewHolder {
        val inflater = LayoutInflater.from(viewGroup.context)
        val view = inflater.inflate(R.layout.radio_button_answer_list_item, viewGroup, false)
        return ViewHolder(view)
    }

    private fun bindRadioButton(baseViewHolder: QuestionWidgetViewHolder, responseIndex: Int,
                                nextPageListener: NextPageListener?) {
        val viewHolder = baseViewHolder as ViewHolder
        viewHolder.bind(this, filteredResponses[responseIndex], responseIndex,
            getUnfilteredPosition(responseIndex) == selectedResponse, nextPageListener)
    }

    private class ViewHolder(itemView: View) : QuestionWidgetViewHolder(itemView) {
        private val labelTextView: TextView = itemView.findViewById(R.id.valueLabel)
        private val codeTextView: TextView = itemView.findViewById(R.id.valueCode)
        private val imageView: ImageView = itemView.findViewById(R.id.valueSetImage)
        private val radioButton: RadioButton = itemView.findViewById(R.id.radioButton)
        private var index = 0
        private var questionWidget: QuestionWidgetRadioButtons? = null
        private var nextPageListener: NextPageListener? = null

        fun bind(questionWidget: QuestionWidgetRadioButtons, response: ValuePair,
                 index: Int, isSelected: Boolean, nextPageListener: NextPageListener?) {
            this.index = index
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
                radioButton.visibility = View.VISIBLE
                itemView.isEnabled = true
            } else {
                radioButton.visibility = View.GONE
                itemView.isEnabled = false
            }
            radioButton.isChecked = isSelected

            // Set the image if there is one.
            val imagePath = response.imagePath
            if (!Util.stringIsNullOrEmpty(imagePath)) {
                loadImage(questionWidget.imageLoader, imagePath)
            } else {
                clearImage(questionWidget.imageLoader)
            }
            if (!questionWidget.field.isReadOnly) {
                itemView.setOnClickListener {
                    if (!radioButton.isChecked) {
                        radioButton.isChecked = true
                        this.questionWidget?.onSelectItem(this.index)
                        if (this.questionWidget?.emitNextPage == true && this.nextPageListener != null &&
                            EngineInterface.getInstance().autoAdvanceOnSelectionFlag) this.nextPageListener?.OnNextPage()
                    }
                }
                radioButton.isEnabled = true
            } else {
                itemView.setOnClickListener(null)
                radioButton.isEnabled = false
            }
            this.questionWidget = questionWidget
            this.nextPageListener = nextPageListener
        }

        private fun clearImage(imageLoader: RequestManager) {
            imageLoader.clear(imageView)
            imageView.visibility = View.GONE
        }

        private fun loadImage(imageLoader: RequestManager, imagePath: String?) {
            imageView.visibility = View.VISIBLE
            val opts = RequestOptions().fitCenter()

            imageLoader.load(imagePath)
                .signature(getFileSignature(imagePath))
                .listener(object : RequestListener<Drawable> {
                    override fun onLoadFailed(e: GlideException?, model: Any?, target: Target<Drawable>?, isFirstResource: Boolean): Boolean {
                        Timber.e(e, "Error loading image for radio button ")
                        e?.rootCauses?.forEach {
                            Timber.e(it, "Caused by")
                        }
                        return false
                    }

                    override fun onResourceReady(resource: Drawable?, model: Any?, target: Target<Drawable>?, dataSource: DataSource?, isFirstResource: Boolean): Boolean {
                        return false
                    }

            }).apply(opts).into(imageView)
        }
    }

    private fun onSelectItem(index: Int) {
        val oldSelection = selectedResponse
        selectedResponse = getUnfilteredPosition(index)
        if (oldSelection != -1) {
            val oldSelectionFiltered = getFilteredPosition(oldSelection)
            if (oldSelectionFiltered != -1) notifyItemChanged(oldSelectionFiltered + 1)
        }
    }

    /**
     * Convert a position of an item in original unfiltered response array to position
     * in filtered response array
     * @param unfilteredPosition Position in m_responses
     * @return Position in m_filteredResponses or -1 if not in filtered array
     */
    private fun getFilteredPosition(unfilteredPosition: Int): Int {
//        if (m_filteredResponses == m_responses) return unfilteredPosition
//        for (i in m_filteredResponses.indices) if (m_filteredResponses[i] === m_responses[unfilteredPosition]) return i
//        return -1

        return filteredResponses.indexOfFirst { it === responses[unfilteredPosition] }
    }

    /**
     * Convert a position of an item in filtered response array to position
     * in original unfiltered response array
     * @param filteredPosition Position in m_filteredResponses
     * @return Position in m_responses
     */
    @Suppress("ReplaceArrayEqualityOpWithArraysEquals")
    private fun getUnfilteredPosition(filteredPosition: Int): Int {
        if (filteredResponses == responses) return filteredPosition
        for (i in responses.indices) if (responses[i] === filteredResponses[filteredPosition]) return i
        return -1
    }

}