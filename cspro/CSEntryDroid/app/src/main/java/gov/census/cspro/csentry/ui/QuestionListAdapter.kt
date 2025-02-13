package gov.census.cspro.csentry.ui

import android.util.Pair
import android.view.ViewGroup
import androidx.collection.ArrayMap
import androidx.core.util.component1
import androidx.core.util.component2
import androidx.recyclerview.widget.RecyclerView
import gov.census.cspro.engine.*

/**
 * RecyclerView adapter for questions displayed together on same screen
 *
 * Each field on the screen is represented by multiple items in the RecyclerView. Each field
 * has one entry for the common parts of a question (label, CAPI text...) and one or more others
 * for the UI for entering the value. For a text field this will be just a text box but for radio
 * buttons or check boxes there will be an item for each response.
 *
 * This adapter takes a list of QuestionWidgets each of which represents one CDEField
 * to be displayed on the screen. The QuestionWidget will add the appropriate number
 * of views into the adapter for the question type.
 *
 * The adapter manages the mapping from positions in the RecyclerView to the QuestionWidget
 * that "owns" the view at that positions (see getQuestionAtPosition). A text box QuestionWidget
 * would manager two views: one for the common parts and one for the text box. A radio button
 * QuestionWidget might manage a dozen views since there will be a separate view for each
 * possible response. When the adapter is asked to get the view type for or to bind the view at a
 * particular position it uses this mapping to obtain the QuestionWidget that owns the view at that
 * position and forwards the request to the QuestionWidget. Each QuestionWidget acts like a mini
 * adapter that knows the view types, view holders and binding of the the positions that it owns.
 *
 */
internal class QuestionListAdapter
/**
 * Constructor
 *
 * @param nextPageListener Listener that is called when user finishes entering data in
 * last field on page.
 */(private val nextPageListener: NextPageListener) : RecyclerView.Adapter<QuestionWidgetViewHolder>() {
    private var questions: Array<QuestionWidget>? = null
    private var totalItems = 0
    private var showEditableNotes = false
    private val viewTypeToQuestion = ArrayMap<Int, QuestionWidget>()

    /**
     * Set the array of questions to show in the RecyclerView
     *
     * @param questions List of QuestionWidgets
     */
    fun setItems(questions: Array<QuestionWidget>?) {
        this.questions = questions
        showEditableNotes = false

        this.questions?.forEach { question -> question.allItemViewTypes.forEach { viewType -> registerViewType(viewType, question) } }

        updateItemPositions()
        notifyDataSetChanged()
    }

    /**
     * Filter responses to questions to only show responses that match searchPattern
     *
     * @param filterPattern Search text to use as filter
     */
    fun filterResponses(filterPattern: String) {
        questions?.let{
            for (qw in it) qw.filterResponses(filterPattern)
            updateItemPositions()
        }
    }

    override fun getItemViewType(position: Int): Int {
        val (question, posInQuestion) = getQuestionAtPosition(position)
        return question.getItemViewType(posInQuestion)
    }

    override fun onCreateViewHolder(viewGroup: ViewGroup, viewType: Int): QuestionWidgetViewHolder {
        val  question = viewTypeToQuestion[viewType] as QuestionWidget
        return question.onCreateViewHolder(viewGroup, viewType)
    }

    override fun onBindViewHolder(viewHolder: QuestionWidgetViewHolder, position: Int) {
        val (question, posInQuestion) = getQuestionAtPosition(position)

        // Make sure everything is enabled since QuestionnaireFragment disables views
        // to prevent double taps while engine is running
        Util.enableViewAndChildren(viewHolder.itemView)
        question.onBindViewHolder(viewHolder, posInQuestion, nextPageListener,
            showEditableNotes)
    }

    override fun getItemCount(): Int {
        return totalItems
    }

    override fun onViewRecycled(holder: QuestionWidgetViewHolder) {
        super.onViewRecycled(holder)
        holder.onRecycled()
    }

    fun toggleEditNotes(firstVisibleItem: Int) {
        showEditableNotes = !showEditableNotes
        questions?.let {
            var pos = 0
            for (qw in it) {
                if (showEditableNotes && !qw.field.isMirror && firstVisibleItem >= pos && firstVisibleItem < pos + qw.itemCount) qw.setFocusToNotes()
                for (i in 0 until qw.itemCount) {
                    if (qw.getItemViewType(i) == QuestionWidget.VIEW_TYPE_QUESTION_COMMON_PARTS && !qw.field.isMirror) {
                        notifyItemChanged(pos)
                    }
                    ++pos
                }
            }
        }
    }

    /**
     * Register which QuestionWidget to use to create ViewHolders for
     * a particular viewType
     *
     * @param viewType Code for type of view, must be unique for each type of view
     * @param questionWidget Factory to create ViewHolders for the view type
     */
    private fun registerViewType(viewType: Int, questionWidget: QuestionWidget) {
        viewTypeToQuestion[viewType] = questionWidget
    }

    /**
     * Get the QuestionWidget that manages the view at a given position in the RecyclerView
     *
     * @param position RecyclerView position
     * @return QuestionWidget at position and first RecyclerView position managed by QuestionWidget
     */
    private fun getQuestionAtPosition(position: Int): Pair<QuestionWidget, Int> {
        questions?.let {
            var questionStartPos = 0
            for (m_item in it) {
                val nextQuestionStartPos = questionStartPos + m_item.itemCount
                if (position < nextQuestionStartPos) return Pair(m_item, position - questionStartPos)
                questionStartPos = nextQuestionStartPos
            }
        }
        throw AssertionError("Can't find question at position $position")
    }

    private fun updateItemPositions() {
        questions?.let {
            totalItems = 0
            for (question in it) {
                question.setRecyclerViewStartPosition(totalItems)
                totalItems += question.itemCount
            }
        }
    }
}