/*

 CSEntry for Android

 Module:		QuestionnaireFragment.java

 Description: UI/Layout presentation and handler for CSEntry for Android.
 This class loads the questionnare layout from resources
 and presents the views and controls the user interacts with
 in responding to questions.

 */
package gov.census.cspro.csentry.ui

import android.animation.ObjectAnimator
import android.annotation.TargetApi
import android.app.Activity
import android.content.Context
import android.content.SharedPreferences
import android.content.res.Configuration
import android.graphics.Color
import android.graphics.Point
import android.os.Bundle
import android.view.*
import android.view.View.OnFocusChangeListener
import android.view.inputmethod.EditorInfo
import android.widget.*
import android.widget.TextView.OnEditorActionListener
import androidx.appcompat.app.ActionBar
import androidx.appcompat.app.AppCompatActivity
import androidx.constraintlayout.widget.ConstraintLayout
import androidx.constraintlayout.widget.Guideline
import androidx.fragment.app.Fragment
import androidx.recyclerview.widget.LinearLayoutManager
import androidx.recyclerview.widget.RecyclerView
import com.bumptech.glide.Glide
import gov.census.cspro.csentry.R
import gov.census.cspro.csentry.SystemSettings
import gov.census.cspro.csentry.ui.NavigationFragment.OnNavigationButtonClickedListener
import gov.census.cspro.engine.EngineInterface
import gov.census.cspro.engine.Util
import gov.census.cspro.form.CDEField
import gov.census.cspro.form.EntryPage
import gov.census.cspro.form.OnFormNavigatedListener

class QuestionnaireFragment : Fragment(), View.OnClickListener, OnEditorActionListener, OnFocusChangeListener, OnFormNavigatedListener, SearchView.OnQueryTextListener {

    private var navControlsMovedOnStartup: Boolean = false
    private var navControlsShown: Boolean = true
    private var buttonNavigationListener: OnNavigationButtonClickedListener? = null
    private var searchMenuItem: MenuItem? = null
    private val questionWidgetFactory: QuestionWidgetFactory = QuestionWidgetFactory()
    private var pageQuestions: Array<QuestionWidget?>? = null
    private lateinit var occurrenceLabelTextView: TextView
    private lateinit var blockQuestionTextWebView: QuestionTextView
    private lateinit var blockHelpTextWebView: QuestionTextView
    private lateinit var divider: View
    private lateinit var toggleBlockHelpTextButton: ImageButton
    private lateinit var questionListRecyclerView: RecyclerView
    private lateinit var questionListAdapter: QuestionListAdapter
    private var fieldNoteUpdateListener: FieldNoteUpdateListener? = null

    override fun onCreateView(inflater: LayoutInflater, container: ViewGroup?, savedInstanceState: Bundle?): View {
        val view: View = inflater.inflate(R.layout.fragment_questionnaire_layout, container, false)
        occurrenceLabelTextView = view.findViewById(R.id.occurrence_label)
        blockQuestionTextWebView = view.findViewById(R.id.block_question_text)
        blockHelpTextWebView = view.findViewById(R.id.block_help_text)
        toggleBlockHelpTextButton = view.findViewById(R.id.button_toggle_block_help_text)
        toggleBlockHelpTextButton.setOnClickListener { blockHelpTextWebView.visibility = if (blockHelpTextWebView.visibility == View.VISIBLE) View.GONE else View.VISIBLE }
        divider = view.findViewById(R.id.divider)
        questionListRecyclerView = view.findViewById(R.id.question_list_recycler_view)
        questionListAdapter = QuestionListAdapter(object : NextPageListener {
            override fun OnNextPage() {
                buttonNavigationListener?.onNavigationNextButtonClicked()
            }
        })
        questionListRecyclerView.adapter = questionListAdapter
        questionListRecyclerView.layoutManager = LinearLayoutManager(activity)

        // wireup the button events
        val prevButton: Button = view.findViewById(R.id.questionnaire_fragment_prev_button)
        prevButton.setOnClickListener(this)
        val nextButton: Button = view.findViewById(R.id.questionnaire_fragment_next_button)
        nextButton.setOnClickListener(this)

        // wireup this class for menuitem events, we need to handle the
        // user making questionnaire actions
        setHasOptionsMenu(true)
        if (activity != null) activity?.resources?.configuration?.orientation?.let { adjustPrevNextButtons(view, it) }
        return view
    }

    private fun adjustPrevNextButtons(view: View?, screenOrientation: Int) {
        if (activity != null) {
            val display: Display? = activity?.windowManager?.defaultDisplay
            val size = Point()
            display?.getSize(size)
            // In landscape mode the keyboard is pretty big so can't
            val fractionOfScreenHeightFromTop: Double = if (screenOrientation == Configuration.ORIENTATION_PORTRAIT) 0.25 else 0.1
            val buttonVerticalPos: Int = (size.y * fractionOfScreenHeightFromTop).toInt()
            val guideLine: Guideline? = view?.findViewById(R.id.guideline_prev_next_buttons)
            val params: ConstraintLayout.LayoutParams = guideLine?.layoutParams as ConstraintLayout.LayoutParams
            params.guideBegin = buttonVerticalPos
            guideLine.layoutParams = params
        }
    }

    override fun onConfigurationChanged(newConfig: Configuration) {
        super.onConfigurationChanged(newConfig)
        adjustPrevNextButtons(view, newConfig.orientation)
    }

    @TargetApi(23)
    override fun onAttach(context: Context) {
        super.onAttach(context)
        attachToContext(context)
    }

    @Deprecated("Deprecated in Java")
    override fun onAttach(activity: Activity) {
        @Suppress("DEPRECATION")
        super.onAttach(activity)
        attachToContext(activity)
    }

    private fun attachToContext(context: Context) {
        if (context is EntryActivity) {
            val activity: EntryActivity = context
            activity.addOnFormNavigatedListener(this)
            buttonNavigationListener = activity
            fieldNoteUpdateListener = activity
        } else {
            throw RuntimeException("QuestionnaireFragment must be hosted in EntryActivity")
        }
    }

    override fun onDetach() {
        (this.activity as EntryActivity?)?.removeOnFormNavigatedListener(this)
        super.onDetach()
    }

    override fun onClick(v: View) {
        val viewId: Int = v.id
        if (viewId == R.id.questionnaire_fragment_prev_button) {
            buttonNavigationListener?.onNavigationPreviousButtonClicked()
        } else if (viewId == R.id.questionnaire_fragment_next_button) {
            // set the focus to the next button
            buttonNavigationListener?.onNavigationNextButtonClicked()
        }
    }

    @Deprecated("Deprecated in Java")
    override fun onCreateOptionsMenu(menu: Menu, inflater: MenuInflater) {
        super.onCreateOptionsMenu(menu, inflater)

        // toggle the navigation controls ... the initial state can be defined as a system setting in the common store,
        // or serialized as a preference on the Android side
        val navMenuItem: MenuItem? = menu.findItem(R.id.questionnaire_menuitem_toggle_nav_controls)
        var navState = true
        val navigationSystemSetting = EngineInterface.GetSystemSettingString(SystemSettings.SettingShowNavigationControls, "")
        if (navigationSystemSetting != "") {
            navState = navigationSystemSetting.equals("Yes", true)

            // if the setting was changed in logic, toggle the controls
            if (navControlsMovedOnStartup && navControlsShown != navState) toggleNavigationControls(navMenuItem)
        } else if (activity != null) {
            val navStateProperty: String = getString(R.string.menu_questionnaire_save_nav_control_state)
            navState = activity?.getPreferences(Context.MODE_PRIVATE)?.getBoolean(navStateProperty, true) == true
        }
        if (!navControlsMovedOnStartup) {
            // the menu may be invalidated when the application first starts
            // because that may happen, this toggle will
            // show or unshow the navigation controls based upon this flag
            if (!navState) {
                // show/hide the controls
                toggleNavigationControls(navMenuItem)
            }
            navControlsMovedOnStartup = true
        } else {
            // the menu was invalidated and we need to update the display
            // with the proper title
            if (navMenuItem != null) {
                navMenuItem.title = getString(if (navState) R.string.menu_questionnaire_hide_nav_controls else R.string.menu_questionnaire_show_nav_controls)
            }
        }

        // case tree state
        val entryActivity: EntryActivity? = activity as EntryActivity?
        val caseTreeVisible: Boolean = entryActivity == null || entryActivity.isCasetreeVisible
        val casetreeMenuItem: MenuItem? = menu.findItem(R.id.questionnaire_menuitem_toggle_casetree)
        if (casetreeMenuItem != null) {
            casetreeMenuItem.title = getString(if (caseTreeVisible) R.string.menu_questionnaire_hide_casetree else R.string.menu_questionnaire_show_casetree)
        }

        // setup the searchview
        if (activity != null) {
            val actionBar: ActionBar? = (activity as AppCompatActivity?)?.supportActionBar
            if (actionBar != null) {
                val searchView = SearchView(actionBar.themedContext)
                searchMenuItem = menu.findItem(R.id.questionnaire_menuitem_action_search)
                searchMenuItem?.actionView = searchView
                searchView.isEnabled = false

                // set the search text color
                val searchEditTextId: Int = searchView.context.resources.getIdentifier("android:id/search_src_text", null, null)
                val searchEditText: EditText = searchView.findViewById(searchEditTextId)
                searchEditText.setTextColor(Color.LTGRAY)
                searchView.setOnQueryTextListener(this)
            }
        }
    }

    override fun onEditorAction(v: TextView, actionId: Int, event: KeyEvent): Boolean {
        var result = false
        if (actionId == EditorInfo.IME_ACTION_DONE || actionId == EditorInfo.IME_ACTION_NEXT) {
            // set the focus to the next button
            buttonNavigationListener?.onNavigationNextButtonClicked()
            result = true
        }
        return result
    }

    override fun onFocusChange(v: View, hasFocus: Boolean) {
        if (hasFocus && activity != null) {
            activity?.window?.setSoftInputMode(WindowManager.LayoutParams.SOFT_INPUT_STATE_ALWAYS_VISIBLE)
        }
    }

    override fun onFormNavigated(page: EntryPage) {
        // update the UI with the field info
        updateFieldDisplay(page)
    }

    override fun onQueryTextChange(newText: String): Boolean {
        questionListAdapter.filterResponses(newText)
        return true
    }

    override fun onQueryTextSubmit(query: String): Boolean {
        searchMenuItem?.collapseActionView()
        return true
    }

    fun toggleNavigationControls(item: MenuItem?) {
        if (view == null) return
        val prevButton: Button = requireView().findViewById(R.id.questionnaire_fragment_prev_button)
        val nextButton: Button = requireView().findViewById(R.id.questionnaire_fragment_next_button)
        val prevanim: ObjectAnimator
        val nextanim: ObjectAnimator
        val buttonWidth: Int = prevButton.width
        val prevXpos: Float = prevButton.x
        val nextXpos: Float = nextButton.x
        val distance: Float = buttonWidth + BUTTON_OFF_SCREEN_EXTRA
        val menuItemText: String
        if (prevXpos < 0.0) {
            // the buttons are off the screen, move them back
            prevanim = ObjectAnimator.ofFloat(prevButton, "x", -distance, 0f)
            nextanim = ObjectAnimator.ofFloat(nextButton, "x", nextXpos, nextXpos - distance)
            menuItemText = getString(R.string.menu_questionnaire_hide_nav_controls)
            navControlsShown = true
        } else {
            // the buttons are on the screen, move them off
            prevanim = ObjectAnimator.ofFloat(prevButton, "x", 0f, -distance)
            nextanim = ObjectAnimator.ofFloat(nextButton, "x", nextXpos, nextXpos + distance)
            menuItemText = getString(R.string.menu_questionnaire_show_nav_controls)
            navControlsShown = false
        }
        prevanim.duration = 1000
        nextanim.duration = 1000
        prevanim.start()
        nextanim.start()
        item?.title = menuItemText
        if (activity != null) {
            val sharedPref: SharedPreferences = requireActivity().getPreferences(Context.MODE_PRIVATE)
            val editor: SharedPreferences.Editor = sharedPref.edit()
            editor.putBoolean(getString(R.string.menu_questionnaire_save_nav_control_state), navControlsShown)
            editor.apply()
        }
    }

    /****
     * @author wmapp
     * {@docRoot} This function may run in a different thread, so we should have this method
     * run in the UI thread at all times
     * @param page Page containing fields to display
     */
    private fun updateFieldDisplay(page: EntryPage) {
        // a field was entered, set the current data here

        // clear any search query that has been entered and collapse to just the icon
        if (searchMenuItem != null) {
            val searchView: SearchView = searchMenuItem?.actionView as SearchView
            val searchEditTextId: Int = searchView.context.resources.getIdentifier("android:id/search_src_text", null, null)
            val searchEditText: EditText = searchView.findViewById(searchEditTextId)
            // Temporarily turn off handling search text changes since
            // filtering the responses here when the field is changing can cause
            // problems.
            searchView.setOnQueryTextListener(null)
            searchEditText.setText("")
            searchView.setOnQueryTextListener(this)
            searchView.isIconified = true
            searchMenuItem?.collapseActionView()
        }

        if (Util.stringIsNullOrEmpty(page.GetOccurrenceLabel())) {
            occurrenceLabelTextView.visibility = View.GONE
        } else {
            occurrenceLabelTextView.text = page.GetOccurrenceLabel()
            occurrenceLabelTextView.visibility = View.VISIBLE
        }

        val questionTextUrl = page.GetBlockQuestionTextUrl()
        if (questionTextUrl == null) {
            blockQuestionTextWebView.visibility = View.GONE
            blockQuestionTextWebView.clear()
        } else {
            blockQuestionTextWebView.loadUrl(questionTextUrl)
            blockQuestionTextWebView.visibility = View.VISIBLE
        }

        val helpTextUrl = page.GetBlockHelpTextUrl()
        blockHelpTextWebView.visibility = View.GONE
        blockHelpTextWebView.clear()
        if (helpTextUrl == null) {
            toggleBlockHelpTextButton.visibility = View.GONE
        } else {
            toggleBlockHelpTextButton.visibility = View.VISIBLE
            blockHelpTextWebView.loadUrl(helpTextUrl)
        }

        if (!Util.stringIsNullOrEmpty(page.GetOccurrenceLabel()) || questionTextUrl != null || helpTextUrl != null) {
            divider.visibility = View.VISIBLE
        } else {
            divider.visibility = View.GONE
        }

        val pageFields: Array<CDEField> = page.GetPageFields()
        pageQuestions = arrayOfNulls(pageFields.size)
        val showCodes = EngineInterface.getInstance().displayCodesAlongsideLabelsFlag
        var foundFirstFocus = false
        for (iField in pageFields.indices) {
            val field: CDEField = pageFields[iField]
            var setFocus = false
            if (!foundFirstFocus && !field.isReadOnly) {
                setFocus = true
                foundFirstFocus = true
            }
            val emitNextPage: Boolean = (iField == pageFields.size - 1)
            val widget: QuestionWidget = questionWidgetFactory.createWidgetForField(field, questionListAdapter, Glide.with(this),
                setFocus, emitNextPage, showCodes)
            pageQuestions?.let {
                it[iField] = widget
            }
        }
        questionListAdapter.setItems(pageQuestions?.filterNotNull()?.toTypedArray())

        // If there is only field and it is radio button/checkbox then scroll to
        // first selected item
        val scrollPos: Int = pageQuestions?.let {
            if (it.size == 1) {
                it[0]?.initialScrollPosition ?: 0
            } else 0

        } ?: 0
        questionListRecyclerView.layoutManager?.scrollToPosition(scrollPos)

        // Update options menu to show/hide search, highlight notes icon...
        if (activity != null) {
            requireActivity().invalidateOptionsMenu()
        }
    }

    fun applyCurrentFieldValues() {
        for (m_pageQuestion: QuestionWidget? in pageQuestions ?: emptyArray()) {
            m_pageQuestion?.save()
        }
    }

    @Deprecated("Deprecated in Java")
    override fun onPrepareOptionsMenu(menu: Menu) {

        // Only enable search if there is a single field that supports it
        searchMenuItem?.isVisible = pageQuestions?.let {
            (it.size == 1) && it[0]?.supportsResponseFilter() == true
        } == true
        super.onPrepareOptionsMenu(menu)
    }

    fun hasFieldNote(): Boolean = pageQuestions?.any { it!!.hasNote() } == true

    fun toggleEditNotes() {
        val layoutManager: LinearLayoutManager? = (questionListRecyclerView.layoutManager as LinearLayoutManager?)
        if (layoutManager != null) {
            val firstVisibleItem: Int = layoutManager.findFirstVisibleItemPosition()
            questionListAdapter.toggleEditNotes(firstVisibleItem)
            fieldNoteUpdateListener?.noteStateChanged()
        }
    }

    fun disable() {
        Util.disableViewAndChildren(view)
    }

    fun enable() {
        Util.enableViewAndChildren(view)
    }

    companion object {
        private const val BUTTON_OFF_SCREEN_EXTRA: Float = 5.0f
    }
}