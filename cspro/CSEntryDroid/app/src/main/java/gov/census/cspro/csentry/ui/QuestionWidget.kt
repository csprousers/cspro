package gov.census.cspro.csentry.ui

import android.text.Editable
import android.text.TextWatcher
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.EditText
import android.widget.ImageButton
import android.widget.TextView
import androidx.recyclerview.widget.RecyclerView
import com.bumptech.glide.signature.ObjectKey
import gov.census.cspro.csentry.R
import gov.census.cspro.engine.Util
import gov.census.cspro.form.CDEField
import java.io.File

/***
 * Interface for UI widgets for different questions types (text box, radio buttons...)
 *
 * Multiple fields on screen are implemented using a RecyclerView. Each field is represented
 * by one or more items in the RecyclerView. For example a field with capture type text box will
 * have two items: an entry for the common question parts (label, CAPI text...) and another
 * for the text box. A radio button field however will have the common parts plus one item in the
 * RecyclerView for each possible response.
 *
 * A class that implements the QuestionWidget interface provides all the methods needed to
 * create and manage entries in the RecyclerView for a particular field type. This includes
 * inflating the view layouts, creating ViewHolders and binding ViewHolders to the views.
 * A question acts like a RecyclerView adapter although it manages only a subset of the
 * positions in the RecyclerView.
 *
 * To add support for a new field type do the following:
 * - Add a new VIEW_TYPE_XXX constant below
 * - Create a new class that implements the QuestionWidgetInterface
 * - Modify the QuestionWidgetFactory to create an instance of the new class
 *
 */


/**
 * Create a new QuestionWidget
 *
 * @param field Engine field that this question wraps
 * @param recyclerViewAdapter RecyclerView adapter that the question will be displayed in
 * @param setFocus Whether or not this widget should get focus when first displayed
 * @param emitNextPage Whether or not this widget should call next page listener when
 * the user finishes entering data in it.
 * @param showCodes Whether or not to display codes along with labels
 */
abstract class QuestionWidget(val field: CDEField,
                              private val recyclerViewAdapter: RecyclerView.Adapter<QuestionWidgetViewHolder>,
                              var setFocus: Boolean,
                              val emitNextPage: Boolean,
                              val showCodes: Boolean) {
    private var recyclerViewStartPosition = 0
    private var helpTextShowing = false
    private var fieldNoteText: String = field.note
    private var setFocusToNotes = false

    fun hasNote(): Boolean {
        return !Util.stringIsNullOrEmpty(fieldNoteText)
    }

    /**
     * Save the current note and field value to the CDEField object
     */
    fun save() {
        if (!field.isReadOnly) copyResponseToField()
        if (!field.isMirror) field.note = fieldNoteText
    }

    /**
     * Get the all viewType codes for views that this question type uses.
     * Should be array of the of VIEW_TYPE_XXX values above.
     */
    abstract val allItemViewTypes: IntArray

    /**
     * View type to display at given position.
     *
     * @param position Local position of view in the question
     * @return one the of VIEW_TYPE_XXX values above.
     */
    abstract fun getItemViewType(position: Int): Int

    /**
     * Set the value of the CDEField to the current value in the UI
     */
    protected abstract fun copyResponseToField()

    /**
     * Return true if the widget handles response filter in supportsResponseFilter()
     */
    abstract fun supportsResponseFilter(): Boolean

    /**
     * Filter displayed responses to only show those whose labels match filterPattern
     * @param filterPattern string against which responses are filtered
     */
    abstract fun filterResponses(filterPattern: String)

    /**
     * For widgets that show a list of items return the index of the first selected item
     * so that the system can scroll to make that item visible. For other widget types
     * or if no item is selected return 0. Only applies to widget types that need to
     * scrolled to selected item so applies to check box/radio button and does not apply to
     * combo box/dropdown
     *
     * @return zero based index of item in widget to scroll to when first shown
     */
    abstract val initialScrollPosition: Int

    /**
     * Create a ViewHolder for a view of the given type. See [RecyclerView.Adapter.onCreateViewHolder]
     *
     * @param viewGroup The ViewGroup into which the new View will be added after it is bound to an adapter position.
     * @param viewType Type of view to create. One the of VIEW_TYPE_XXX values above.
     * @return New ViewHolder
     */
    abstract fun onCreateViewHolder(viewGroup: ViewGroup, viewType: Int): QuestionWidgetViewHolder

    /**
     * Bind view to part of question at given position.
     * Called by QuestionListAdapter to display the data at the specified position.
     * This method should update the contents of the itemView to reflect the
     * part of the question at the given position.
     *
     * See [RecyclerView.Adapter.onBindViewHolder]
     * @param viewHolder ViewHolder created by a previous call to onCreateViewHolder
     * @param position Local position of view in the question
     * @param nextPageListener Optional listener for next page event generated from the view
     * @param editNotes Display editable notes view
     */
    abstract fun onBindViewHolder(viewHolder: QuestionWidgetViewHolder, position: Int,
                                  nextPageListener: NextPageListener?, editNotes: Boolean)

    /**
     * Get total number of views to display for this question.
     */
    abstract val itemCount: Int

    /**
     * Called by QuestionListAdapter to set the first RecyclerView position for views
     * managed by this question. This is used to map from local question positions
     * to RecyclerView positions.
     *
     * @param position Position in RecyclerView for the first view managed by this question.
     */
    fun setRecyclerViewStartPosition(position: Int) {
        recyclerViewStartPosition = position
    }

    /**
     * Tell RecyclerView that an item has changed so that it the corresponding view will be
     * updated. This will force a call to onBindViewHolder to update the contents of the view
     * from the updated data.
     *
     * @param itemPosition Local position of view in question.
     */
    fun notifyItemChanged(itemPosition: Int) {
        // Convert from local pos to RecyclerView pos
        val recyclerViewPos = recyclerViewStartPosition + itemPosition
        recyclerViewAdapter.notifyItemChanged(recyclerViewPos)
    }

    /**
     * Tell RecyclerView that a range of items were removed so that corresponding views will be
     * removed.
     *
     * @param positionStart Start position of range that was removed
     * @param itemCount Number of items in range removed
     */
    fun notifyItemRangeRemoved(positionStart: Int, itemCount: Int) {
        // Convert from local pos to RecyclerView pos
        val recyclerViewPos = recyclerViewStartPosition + positionStart
        recyclerViewAdapter.notifyItemRangeRemoved(recyclerViewPos, itemCount)
    }

    /**
     * Tell RecyclerView that a range of items were inserted so that correspondings view will be
     * added.
     *
     * @param positionStart Position that items were inserted at
     * @param itemCount Number of items that were inserted
     */
    fun notifyItemRangeInserted(positionStart: Int, itemCount: Int) {
        // Convert from local pos to RecyclerView pos
        val recyclerViewPos = recyclerViewStartPosition + positionStart
        recyclerViewAdapter.notifyItemRangeInserted(recyclerViewPos, itemCount)
    }

    fun setFocusToNotes() {
        setFocusToNotes = true
    }

    /**
     * ViewHolder for the parts of the question UI that is shared by all question types.
     * This includes label, question text, notes...
     */
    protected class CommonPartsViewHolder internal constructor(v: View) : QuestionWidgetViewHolder(v) {
        private var questionWidget: QuestionWidget? = null
        private val questionLabel: TextView = v.findViewById(R.id.questionLabel)
        private val questionText: QuestionTextView = v.findViewById(R.id.questionText)
        private val toggleHelpTextButton: ImageButton = v.findViewById(R.id.imagebutton_toggle_qsf_contents)
        private val helpText: QuestionTextView = v.findViewById(R.id.helpText)
        private val noteEditText: EditText = v.findViewById(R.id.note)
        private val notesLabel: TextView = v.findViewById(R.id.noteLabel)

        init {
            noteEditText.addTextChangedListener(object : TextWatcher {
                override fun beforeTextChanged(charSequence: CharSequence, i: Int, i1: Int, i2: Int) {}
                override fun onTextChanged(charSequence: CharSequence, i: Int, i1: Int, i2: Int) {}
                override fun afterTextChanged(editable: Editable) {
                    if (questionWidget != null && questionWidget?.field?.isMirror == false) {
                        questionWidget?.fieldNoteText = editable.toString()
                    }
                }
            })
        }

        fun bind(questionWidget: QuestionWidget, editNotes: Boolean) {
            // Set the field to null to temporarily disable the TextWatcher
            // while we set the initial note value
            this.questionWidget = null

            val label = questionWidget.field.label
            if (Util.stringIsNullOrEmpty(label)) {
                questionLabel.visibility = View.GONE
            } else {
                questionLabel.text = label
                questionLabel.visibility = View.VISIBLE
            }

            val questionTextUrl = questionWidget.field.questionTextUrl
            if (questionTextUrl == null) {
                this.questionText.visibility = View.GONE
                this.questionText.clear()
            } else {
                this.questionText.loadUrl(questionTextUrl)
                this.questionText.visibility = View.VISIBLE
            }

            val helpTextUrl = questionWidget.field.helpTextUrl
            if (helpTextUrl == null) {
                toggleHelpTextButton.visibility = View.GONE
                this.helpText.visibility = View.GONE
                this.helpText.clear()
            } else {
                toggleHelpTextButton.visibility = View.VISIBLE
                this.helpText.loadUrl(helpTextUrl)
                this.helpText.visibility = if (questionWidget.helpTextShowing) View.VISIBLE else View.GONE
                toggleHelpTextButton.setOnClickListener {
                    if (questionWidget.helpTextShowing) {
                        this.helpText.visibility = View.GONE
                    } else {
                        this.helpText.visibility = View.VISIBLE
                    }
                    questionWidget.helpTextShowing = !questionWidget.helpTextShowing
                }
            }

            if ((questionWidget.hasNote() || editNotes) && !questionWidget.field.isMirror) {
                noteEditText.visibility = View.VISIBLE
                notesLabel.visibility = View.VISIBLE
                noteEditText.setText(questionWidget.fieldNoteText)
                noteEditText.isEnabled = editNotes
                if (editNotes) {
                    noteEditText.minLines = 4
                    if (questionWidget.setFocusToNotes) {
                        questionWidget.setFocusToNotes = false // only do this first time flag is set
                        Util.setFocus(noteEditText)
                    }
                } else {
                    noteEditText.minLines = 0
                }
            } else {
                noteEditText.visibility = View.GONE
                notesLabel.visibility = View.GONE
            }

            this.questionWidget = questionWidget
        }
    }

    fun createCommonPartsViewHolder(viewGroup: ViewGroup): QuestionWidgetViewHolder {
        val inflater = LayoutInflater.from(viewGroup.context)
        val view = inflater.inflate(R.layout.question_layout, viewGroup, false)
        return CommonPartsViewHolder(view)
    }

    fun bindCommonParts(h: QuestionWidgetViewHolder, editNotes: Boolean) {
        val viewHolder = h as CommonPartsViewHolder
        viewHolder.bind(this, editNotes)
    }

    companion object {
        /**
         * View type codes for use in QuestionListAdapter
         */
        const val VIEW_TYPE_QUESTION_COMMON_PARTS = 1
        const val VIEW_TYPE_NUMERIC_TEXT_BOX = 2
        const val VIEW_TYPE_ALPHA_TEXT_BOX = 3
        const val VIEW_TYPE_RADIO_BUTTON_LIST_ITEM = 4
        const val VIEW_TYPE_CHECK_BOX_LIST_ITEM = 5
        const val VIEW_TYPE_DROP_DOWN = 6
        const val VIEW_TYPE_COMBO_BOX_NUMERIC = 7
        const val VIEW_TYPE_DATE_PICKER = 8
        const val VIEW_TYPE_COMBO_BOX_ALPHA = 9
        const val VIEW_TYPE_BARCODE_BUTTON_ALPHA = 10
        const val VIEW_TYPE_BARCODE_BUTTON_NUMERIC = 11
        const val VIEW_TYPE_TOGGLE_BUTTON_SLIDER = 12
        const val VIEW_TYPE_SLIDER_NUMERIC = 13
        const val VIEW_TYPE_PHOTO = 14
        const val VIEW_TYPE_SIGNATURE = 15
        const val VIEW_TYPE_AUDIO = 16

        @JvmStatic
        fun getFileSignature(imagePath: String?) : ObjectKey{
            val f = File(imagePath)
            if (f.exists() && f.isFile)
                return ObjectKey(f.lastModified())

            return ObjectKey(0L)
        }
    }

}