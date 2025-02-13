package gov.census.cspro.csentry.ui

import android.annotation.SuppressLint
import android.app.Activity
import android.app.AlertDialog
import android.content.Intent
import android.content.pm.ActivityInfo
import android.content.res.Configuration
import android.graphics.drawable.Drawable
import android.os.Bundle
import android.view.*
import android.view.GestureDetector.SimpleOnGestureListener
import android.webkit.WebView
import android.widget.*
import androidx.appcompat.app.ActionBarDrawerToggle
import androidx.appcompat.app.AppCompatActivity
import androidx.core.view.GestureDetectorCompat
import androidx.drawerlayout.widget.DrawerLayout
import androidx.fragment.app.DialogFragment
import com.dropbox.core.android.Auth
import gov.census.cspro.commonui.ErrorDialogFragment
import gov.census.cspro.commonui.ErrorDialogFragment.OnErrorFragmentDismissed
import gov.census.cspro.csentry.AppLoadingFragment
import gov.census.cspro.csentry.CSEntry
import gov.census.cspro.csentry.R
import gov.census.cspro.csentry.SystemSettings
import gov.census.cspro.csentry.ui.EntryEngineMessage.EntryMessageRequestType
import gov.census.cspro.csentry.ui.NavigationFragment.OnNavigationButtonClickedListener
import gov.census.cspro.engine.*
import gov.census.cspro.engine.functions.AuthorizeDropboxFunction
import gov.census.cspro.engine.functions.GPSFunction
import gov.census.cspro.form.OnFormNavigatedListener
import gov.census.cspro.util.Constants
import timber.log.Timber
import java.util.*
import java.util.concurrent.Semaphore
import kotlin.math.abs

@SuppressLint("DefaultLocale")
class EntryActivity: AppCompatActivity(), IEngineMessageCompletedListener, OnNavigationButtonClickedListener, AppLoadingFragment.OnFragmentInteractionListener, OnErrorFragmentDismissed, FieldNoteUpdateListener {

    private var m_appStarted = false
    private var m_drawerToggler: ActionBarDrawerToggle ? = null
    private var m_drawerLayout: DrawerLayout ? = null
    private var m_useDrawerForCaseTree = false
    private val m_formNavigatedListeners: ArrayList<OnFormNavigatedListener>? = ArrayList()
    private var m_gestureDetector: GestureDetectorCompat? = null
    private var m_qsfrag: QuestionnaireFragment? = null
    private var m_navFrag: NavigationFragment? = null
    private var m_menu: Menu? = null
    var isCasetreeVisible : Boolean = true
        get() =
            if(m_useDrawerForCaseTree){
                val fragView = m_navFrag?.view
                if(fragView!=null) {
                    m_drawerLayout?.isDrawerOpen(fragView) == true
                }else{
                    false
                }
            }else{
                field
            }
    set(value){
        field = value
    }

    private var m_bProcessingEntryEngineMsg = false
    private var m_iconNoteFilled: Drawable? = null
    private var m_iconNoNote: Drawable? = null

    private fun applyCurrentFieldValues() {
        // Clear current focus before copying widget values to fields
        // This flushes any in progress edits before getting values like editing year text in DatePicker
        clearFocus()
        m_qsfrag?.applyCurrentFieldValues()
    }

    private fun clearFocus() = currentFocus?.clearFocus()


    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        // instantiate the application interface
        EngineInterface.CreateEngineInterfaceInstance(application)

        // set the screen orientation
        if (resources.getBoolean(R.bool.portrait_only)) requestedOrientation = ActivityInfo.SCREEN_ORIENTATION_PORTRAIT
        m_iconNoNote = resources.getDrawable(R.drawable.ic_menu_edit_note)
        m_iconNoteFilled = resources.getDrawable(R.drawable.ic_menu_edit_note_filled)
        if (!EngineInterface.getInstance().isApplicationOpen) {
            val appLoadingDialog = AppLoadingFragment.newInstance(intent)
            appLoadingDialog.show(supportFragmentManager, "AppLoadingDialog")
        } else {
            applicationLoaded()
        }
    }

    override fun applicationLoaded() {
        if( !EngineInterface.getInstance().useHtmlDialogs() ) {
            var operatorId = intent.getStringExtra(OPERATOR_ID_PARAM)
            if (Util.stringIsNullOrEmpty(operatorId)) operatorId = EngineInterface.getInstance().opIDFromPff
            EngineInterface.getInstance().setOperatorId(operatorId)
        }

        val fm = supportFragmentManager
        val showCaseTreeInOverlaySetting = EngineInterface.GetSystemSettingString(SystemSettings.SettingShowCaseTreeInOverlay, "BasedOnScreenSize")
        if (showCaseTreeInOverlaySetting.equals("No", ignoreCase = true)) {
            m_useDrawerForCaseTree = false
        } else if (showCaseTreeInOverlaySetting.equals("Yes", ignoreCase = true)) {
            m_useDrawerForCaseTree = true
        } else {
            m_useDrawerForCaseTree = !CSEntry.isTablet
        }
        if (m_useDrawerForCaseTree) {
            setContentView(R.layout.activity_entry_application_with_drawer)
            m_qsfrag = fm.findFragmentById(R.id.fragment_questionnaire_layout) as QuestionnaireFragment?

            // create the drawer up front
            createDrawerLayout()
        } else {
            setContentView(R.layout.activity_entry_application)
            m_qsfrag = fm.findFragmentById(R.id.fragment_questionnaire_layout_withnav) as QuestionnaireFragment?
        }
        m_navFrag = fm.findFragmentById(R.id.fragment_navigation_layout) as NavigationFragment?
        m_navFrag?.setNavigationButtonClickedListener(this)

        // Initial visibility of case tree (if show case tree is set to "Never" or "Only on Desktop"
        // in data entry options.
        if (!m_useDrawerForCaseTree && !EngineInterface.getInstance().showCaseTree()) {
            hideCaseTree()
        }
        m_gestureDetector = GestureDetectorCompat(this,
            QuestionSwipeGestureListener())

        // set the activity title
        updateWindowTitle()

        // will still need this call to start, it's just that the components
        // are created first and accessed via the FragmentManager
        // after the engine starts and the first field is navigated
        // the event triggered through the getNextField change
        // will force an update to the nav and questionnaire fragments
        val msg = EntryEngineMessage(this, this, EntryMessageRequestType.START_APPLICATION)
        SendEntryEngineMessage(msg)
        dismissAppLoadingDialog()
    }

    override fun applicationLoadFailed(filename: String) {
        dismissAppLoadingDialog()
        val errorDialog = ErrorDialogFragment.newInstance(
            getString(R.string.app_startup_failure_msg_title), String.format(getString(R.string.app_startup_failure_msg), filename),
            R.string.app_startup_failure_msg)
        errorDialog.show(supportFragmentManager, "errorDialog")
    }

    private fun dismissAppLoadingDialog() {
        val frag = supportFragmentManager.findFragmentByTag("AppLoadingDialog")
        if (frag != null) {
            (frag as DialogFragment).dismiss()
        }
    }

    override fun errorDialogDismissed(requestCode: Int) {
        when (requestCode) {
            R.string.app_startup_failure_msg ->                // If we get an error from app load go back to previous activity
                finish()
        }
    }

    override fun dispatchTouchEvent(ev: MotionEvent): Boolean {
        return if (m_gestureDetector?.onTouchEvent(ev) == true) true else super.dispatchTouchEvent(ev)
    }

    private fun createDrawerLayout() {
        m_drawerLayout = findViewById(R.id.activity_entry_application_with_drawer)
        m_drawerToggler = object : ActionBarDrawerToggle(this, m_drawerLayout, R.string.drawer_open, R.string.drawer_close) {
            // Called when a drawer has settled in a completely closed state.
            override fun onDrawerClosed(view: View) {
                updateWindowTitle()
            }

            // Called when a drawer has settled in a completely open state.
            override fun onDrawerOpened(drawerView: View) {
                supportActionBar?.title = "Questionnaire"
                m_navFrag?.populateCaseTree()
                Util.hideInputMethod(this@EntryActivity)
            }
        }
        m_drawerLayout?.addDrawerListener(m_drawerToggler as ActionBarDrawerToggle)

        // noinspection ConstantConditions
        supportActionBar?.setDisplayHomeAsUpEnabled(true)
        supportActionBar?.setHomeButtonEnabled(true)

        // enable tap and hold clicks for this view
        m_drawerLayout?.isLongClickable = true
    }

    override fun onPostCreate(savedInstanceState: Bundle?) {
        super.onPostCreate(savedInstanceState)
        // Sync the toggle state after onRestoreInstanceState has occurred.
        if (m_drawerToggler != null) {
            m_drawerToggler?.syncState()
        }
    }

    public override fun onDestroy() {
        super.onDestroy()

        // turn off the GPS if it is still on
        GPSFunction.close()
        val paradataDriver = EngineInterface.getInstance().paradataDriver
        paradataDriver?.stopGpsLocationUpdates()
    }

    override fun onConfigurationChanged(newConfig: Configuration) {
        super.onConfigurationChanged(newConfig)
        if (m_drawerToggler != null) {
            m_drawerToggler?.onConfigurationChanged(newConfig)
        }
    }

    override fun onCreateOptionsMenu(menu: Menu): Boolean {
        menuInflater.inflate(R.menu.menu_questionnaire_options, menu)
        m_menu = menu
        if (m_appStarted) // update the application-specific options
        {
            // remove options that aren't relevant
            val engineInterface = EngineInterface.getInstance()
            if (!engineInterface.containsMultipleLanguages()) menu.removeItem(R.id.questionnaire_menuitem_change_language)
            if (engineInterface.isSystemControlled) {
                menu.removeItem(R.id.questionnaire_menuitem_end_group)
                menu.removeItem(R.id.questionnaire_menuitem_end_level)
            }
            if (!engineInterface.isSystemControlled ||
                !EngineInterface.GetSystemSettingBoolean(SystemSettings.MenuAdvanceToEnd, true)) {
                menu.removeItem(R.id.questionnaire_menuitem_advance_to_end)
            }
            if (!engineInterface.allowsPartialSave()) menu.removeItem(R.id.questionnaire_menuitem_partial_save)
            if (engineInterface.showsRefusalsAutomatically() || !EngineInterface.GetSystemSettingBoolean(SystemSettings.MenuShowRefusals, true)) menu.removeItem(R.id.questionnaire_menuitem_show_refusals)
            if (!EngineInterface.GetSystemSettingBoolean(SystemSettings.MenuReviewAllNotes, true)) menu.removeItem(R.id.questionnaire_menuitem_review_notes)
            if (!engineInterface.hasPersistentFields()) menu.removeItem(R.id.questionnaire_menuitem_previous_persistent)
            if (EngineInterface.GetSystemSettingString(SystemSettings.SettingShowNavigationControls, "") != "") menu.removeItem(R.id.questionnaire_menuitem_toggle_nav_controls)
            if (m_useDrawerForCaseTree || !EngineInterface.GetSystemSettingBoolean(SystemSettings.MenuShowCaseTree, true)) menu.removeItem(R.id.questionnaire_menuitem_toggle_casetree)
            if (!EngineInterface.GetSystemSettingBoolean(SystemSettings.MenuHelp, true)) menu.removeItem(R.id.questionnaire_menuitem_help)

            // set up the click listeners for the note button
            val editNoteMenuItem = menu.findItem(R.id.questionnaire_menuitem_edit_field_note)
            editNoteMenuItem.setActionView(R.layout.questionnaire_menuitem_edit_note_layout)
            val editNoteActionView = editNoteMenuItem.actionView
            val noteMenuImageButton = editNoteActionView?.findViewById<ImageButton>(R.id.questionnaire_menuitem_edit_field_note)
            noteMenuImageButton?.setImageDrawable(if (m_qsfrag != null && m_qsfrag?.hasFieldNote() == true) m_iconNoteFilled else m_iconNoNote)

            editNoteActionView?.setOnClickListener { editNotes() }
            editNoteActionView?.setOnLongClickListener { parentView ->
                val popup = PopupMenu(parentView.context, parentView)
                popup.setOnMenuItemClickListener { item ->
                    when (item.itemId) {
                        R.id.questionnaire_menuitem_edit_field_note -> {
                            editNotes()
                            true
                        }
                        R.id.questionnaire_menuitem_edit_case_note -> {
                            editCaseNote()
                            true
                        }
                        R.id.questionnaire_menuitem_review_notes -> {
                            reviewNotes()
                            true
                        }
                        else -> false
                    }
                }
                popup.inflate(R.menu.menu_edit_note_popup)
                if (!EngineInterface.GetSystemSettingBoolean(SystemSettings.MenuReviewAllNotes, true)) popup.menu.removeItem(R.id.questionnaire_menuitem_review_notes)
                popup.show()
                true
            }
            EngineInterface.getInstance().userbarHandler.creatingMenu(menu.findItem(R.id.questionnaire_menuitem_userbar))
        }
        return true
    }

    private fun SendEntryEngineMessage(msg: EntryEngineMessage) {
        // Ignore messages if already processing one since a msg
        // may change UI and the click causing the second msg to be sent
        // can be invalid.
        if (!m_bProcessingEntryEngineMsg) {
            m_bProcessingEntryEngineMsg = true
            m_qsfrag?.disable()
            m_navFrag?.disable()
            Messenger.getInstance().sendMessage(msg)
        }
    }

    override fun onMessageCompleted(msg: EngineMessage) {
        if (msg is EntryEngineMessage) {
            m_bProcessingEntryEngineMsg = false
            when (msg.entryMessageRequestType) {
                EntryMessageRequestType.START_APPLICATION -> {
                    m_appStarted = (msg.result == 1L)
                    if (m_appStarted) processStartApplication() else startApplicationFailed(msg.errorMessage)
                }
                EntryMessageRequestType.END_APPLICATION -> finish()
                EntryMessageRequestType.GOTO_FIELD, EntryMessageRequestType.GOTO_NOTE_FIELD, EntryMessageRequestType.DELETE_OCC, EntryMessageRequestType.INSERT_OCC, EntryMessageRequestType.INSERT_OCC_AFTER -> {
                    closeCaseTreeDrawer()
                    processCurrentField()
                }
                EntryMessageRequestType.ADVANCE_TO_END, EntryMessageRequestType.END_GROUP, EntryMessageRequestType.END_LEVEL, EntryMessageRequestType.END_LEVEL_OCC, EntryMessageRequestType.NEXT_FIELD, EntryMessageRequestType.PREVIOUS_FIELD, EntryMessageRequestType.PREVIOUS_PERSISTENT_FIELD, EntryMessageRequestType.CHANGE_LANGUAGE, EntryMessageRequestType.REVIEW_NOTES, EntryMessageRequestType.VIEW_CURRENT_CASE, EntryMessageRequestType.USER_TRIGGERED_STOP -> processCurrentField()
                EntryMessageRequestType.SHOW_REFUSALS -> if (msg.result == 0L) Toast.makeText(this, getString(R.string.refusals_none_to_show), Toast.LENGTH_LONG).show() else processCurrentField()
                else -> {
                }
            }

            // Fragment UI was disabled when message was sent, reenable UI except on end application
            // message in which case keep it disabled so we don't get extra UI events after activity
            // is finished. You would think Android would not allow that but alas it does.
            if (msg.entryMessageRequestType != EntryMessageRequestType.END_APPLICATION) {
                m_qsfrag?.enable()
                m_navFrag?.enable()
            }
        } else if (msg is UserbarHandler.UserbarMessage) {
            processCurrentField()
        }
    }

    override fun onKeyDown(keyCode: Int, event: KeyEvent): Boolean {
        if (keyCode == KeyEvent.KEYCODE_BACK) {
            if (closeCaseTreeDrawer()) return true
            applyCurrentFieldValues()
            val msg = EntryEngineMessage(this, this, EntryMessageRequestType.USER_TRIGGERED_STOP)
            SendEntryEngineMessage(msg)
            return true
        }
        return super.onKeyDown(keyCode, event)
    }

    override fun onNavigationNextButtonClicked() {
        initiateFieldMovement(EntryMessageRequestType.NEXT_FIELD)
    }

    override fun onNavigationPreviousButtonClicked() {
        initiateFieldMovement(EntryMessageRequestType.PREVIOUS_FIELD)
    }

    fun GoToField(fieldSymbol: Int, index1: Int, index2: Int, index3: Int) {
        // set the current field value on the screen
        applyCurrentFieldValues()
        val msg = EntryEngineMessage(this, this, EntryMessageRequestType.GOTO_FIELD)
        msg.wParam = fieldSymbol.toLong()
        msg.setObject(intArrayOf(index1, index2, index3))
        SendEntryEngineMessage(msg)
    }

    override fun onResume() {
        super.onResume()
        if(AuthorizeDropboxFunction.isAuthenticating()){
            AuthorizeDropboxFunction.setAuthenticationComplete()
            var dbxCredential: String? = ""
            val credential = Auth.getDbxCredential()
            if (credential != null) {
                dbxCredential = credential.toString()
            }
            Messenger.getInstance().engineFunctionComplete(dbxCredential)
        }
    }
    //public static final int BarcodeCode = Constants.INTENT_STARTACTIVITY_BARCODE;
    @Deprecated("Deprecated in Java")
    override fun onActivityResult(requestCode: Int, resultCode: Int, intent: Intent?) {
        if (requestCode == BluetoothDiscoverableCode) {
            Timber.d("Bluetooth discoverable complete")
            // Bluetooth discoverable activity doesn't return RESULT_OK
            // It returns length of discoverable period or RESULT_CANCELED if denied.
            Messenger.getInstance().engineFunctionComplete(if (resultCode == RESULT_CANCELED) 0 else 1.toLong())
        } else if (requestCode == BluetoothChooseDeviceCode) {
            Timber.d("Choose Bluetooth device complete")
            if (resultCode == RESULT_OK) {
                val nameAndAddress = intent?.extras?.getString(ChooseBluetoothDeviceActivity.EXTRA_DEVICE_NAME) + "\n" + intent?.extras?.getString(ChooseBluetoothDeviceActivity.EXTRA_DEVICE_ADDRESS)
                Messenger.getInstance().engineFunctionComplete(nameAndAddress)
            } else {
                Messenger.getInstance().engineFunctionComplete(null)
            }
        } else if (requestCode == ReviewNotesCode) {
            val extras = if (resultCode == RESULT_OK) intent?.extras else null
            if (extras != null && extras.containsKey(ReviewNotesActivity.GotoFieldNoteIndex)) {
                // set the current field value on the screen
                val msg = EntryEngineMessage(this, this, EntryMessageRequestType.GOTO_NOTE_FIELD)
                msg.wParam = extras.getLong(ReviewNotesActivity.GotoFieldNoteIndex)
                SendEntryEngineMessage(msg)
            } else if (m_qsfrag != null) // update the field display in case note was deleted
                processCurrentField()
        } else if (requestCode == EnableLocationInSettings) {
            // Returned from location settings, inform GPS reader
            GPSFunction.getReader().onSettingsResult(this)
        } else if (requestCode == SystemAppCode) {
            // Returned from external app - null result is error, so if called app doesn't return anything use an empty bundle
            val result = if (intent != null && intent.extras != null) intent.extras else Bundle()
            if (intent != null && intent.data != null) {
                // Add data to bundle so that it can be retrieved in CSPro logic
                result?.putString("DATA", intent.data.toString())
            }
            Messenger.getInstance().engineFunctionComplete(result)
        } else if (requestCode == Constants.INTENT_STARTACTIVITY_BARCODE) {
            var barcodeReadValue: String? = ""
            if (intent != null) {
                barcodeReadValue = intent.getStringExtra(Constants.EXTRA_BARCODE_DISPLAYVALUE_KEY)
            }
            Messenger.getInstance().engineFunctionComplete(barcodeReadValue)
        } else {
            // call the private cleanup member here
            super.onActivityResult(requestCode, resultCode, intent)
        }
    }

    private fun toggleCaseTree(item: MenuItem) {
        //Hide Or Show CaseTree
        if (m_useDrawerForCaseTree) return
        if (item.title.toString().compareTo(getString(R.string.menu_questionnaire_hide_casetree)) == 0) {
            hideCaseTree()
            item.title = getString(R.string.menu_questionnaire_show_casetree)
        } else if (item.title.toString().compareTo(getString(R.string.menu_questionnaire_show_casetree)) == 0) {
            showCaseTree()
            item.title = getString(R.string.menu_questionnaire_hide_casetree)
        }
    }

    private fun hideCaseTree() {
        m_navFrag?.let{
            val fm = supportFragmentManager
            fm.beginTransaction() //.setCustomAnimations(android.R.animator.fade_in, android.R.animator.fade_out)
                .hide(it)
                .commit()
        }
        isCasetreeVisible = false
    }

    private fun showCaseTree() {
        m_navFrag?.let {
            val fm = supportFragmentManager
            fm.beginTransaction() // .setCustomAnimations(android.R.animator.fade_in, android.R.animator.fade_out)
                .show(it)
                .commit()
        }
        isCasetreeVisible = true
        m_navFrag?.populateCaseTree()
    }

    override fun onOptionsItemSelected(item: MenuItem): Boolean {
        if (m_drawerToggler?.onOptionsItemSelected(item) == true) return true
        if (m_useDrawerForCaseTree) {
            if (m_drawerLayout != null) {
                //Close the drawer if anything else other than the drawer toggle is clicked
                val navView = m_navFrag?.view
                if ((navView != null) && m_drawerLayout?.isDrawerOpen(navView) == true && (item.itemId != android.R.id.home)) {    //if drawer is open close it.
                    m_drawerLayout?.closeDrawer(navView)
                }
            }
        }
        when (item.itemId) {
            R.id.questionnaire_menuitem_change_language -> {
                changeLanguage()
                return true
            }
            R.id.questionnaire_menuitem_show_refusals -> {
                showRefusals()
                return true
            }
            R.id.questionnaire_menuitem_review_notes -> {
                reviewNotes()
                return true
            }
            R.id.questionnaire_menuitem_end_group -> {
                initiateFieldMovement(EntryMessageRequestType.END_GROUP)
                return true
            }
            R.id.questionnaire_menuitem_end_level -> {
                initiateFieldMovement(EntryMessageRequestType.END_LEVEL)
                return true
            }
            R.id.questionnaire_menuitem_help -> {
                SystemSettings.LaunchHelp(this)
                return true
            }
            R.id.questionnaire_menuitem_advance_to_end -> {
                initiateFieldMovement(EntryMessageRequestType.ADVANCE_TO_END)
                return true
            }
            R.id.questionnaire_menuitem_toggle_nav_controls -> {
                toggleNavigationControls()
                return true
            }
            R.id.questionnaire_menuitem_toggle_casetree -> {
                toggleCaseTree(item)
                return true
            }
            R.id.questionnaire_menuitem_userbar -> {
                userbarClicked()
                return true
            }
            R.id.questionnaire_menuitem_partial_save -> {
                savePartial()
                return true
            }
            R.id.questionnaire_menuitem_previous_persistent -> {
                initiateFieldMovement(EntryMessageRequestType.PREVIOUS_PERSISTENT_FIELD)
                return true
            }
            R.id.questionnaire_menuitem_select_style -> {
                val styleIntent = Intent(this, SelectStyleActivity::class.java)
                startActivity(styleIntent)
                return true
            }
            R.id.questionnaire_menuitem_view_questionnaire -> {
				viewCurrentCase()
                return true
            }
        }
        return super.onOptionsItemSelected(item)
    }

    fun addOnFormNavigatedListener(listener: OnFormNavigatedListener) {
        if (m_formNavigatedListeners?.contains(listener) == false) {
            m_formNavigatedListeners.add(listener)
        }
    }

    fun removeOnFormNavigatedListener(listener: OnFormNavigatedListener?) {
        m_formNavigatedListeners?.remove(listener)
    }

    private fun closeCaseTreeDrawer(): Boolean {
        //returns true if the case tree drawer was closed (on a phone)
        if (m_useDrawerForCaseTree && m_navFrag != null) {
            // close the drawer if it is open
            val navView = m_navFrag?.view
            if ((m_drawerLayout != null) && (navView != null) && m_drawerLayout?.isDrawerOpen(navView) == true) {
                m_drawerLayout?.closeDrawer(navView)
                return true
            }
        }
        return false
    }

    fun initiateFieldMovement(requestType: EntryMessageRequestType) {
        // first update the current edit text values to the field data
        applyCurrentFieldValues()
        val msg = EntryEngineMessage(this, this, requestType)
        SendEntryEngineMessage(msg)
    }

    private fun processCurrentField(processPossibleRequests: Boolean = false) {
        val page = EngineInterface.getInstance().getCurrentPage(processPossibleRequests)
        if (page != null) {
            if (m_formNavigatedListeners != null) {
                for (listener: OnFormNavigatedListener in m_formNavigatedListeners) listener.onFormNavigated(page)
            }
        } else {
            // the end of the case was reached
            stopEntryApplication()
        }
    }

    private fun stopEntryApplication() {
        val msg = EntryEngineMessage(this, this, EntryMessageRequestType.END_APPLICATION)
        SendEntryEngineMessage(msg)
    }

    private fun processStartApplication() {
        invalidateOptionsMenu()

        // display the field information
        if (m_formNavigatedListeners != null) processCurrentField()
    }

    private fun startApplicationFailed(errorMessage: String?) {
        var errorMessage: String? = errorMessage
        if (errorMessage == null) errorMessage = String.format(getString(R.string.app_startup_failure_msg), EngineInterface.getInstance().windowTitle)
        val builder = AlertDialog.Builder(this)
        builder.setMessage(errorMessage)
            .setTitle(getString(R.string.app_startup_failure_msg_title))
            .setCancelable(false)
            .setPositiveButton(getString(R.string.modal_dialog_helper_ok_text)) { dialog, id -> finish() }
        builder.show()
    }

    override fun noteStateChanged() {
        invalidateOptionsMenu()
    }

    internal inner class QuestionSwipeGestureListener() : SimpleOnGestureListener() {
        override fun onDown(e: MotionEvent): Boolean {
            return false
        }

        override fun onScroll(e1: MotionEvent, e2: MotionEvent, distanceX: Float, distanceY: Float): Boolean {
            // we need to pass this event along so that additional touches
            // are detected down the line for the fling
            return false
        }

        override fun onFling(e1: MotionEvent, e2: MotionEvent, velocityX: Float, velocityY: Float): Boolean {
            // this should really be automatic but android makes us implement this
            // ourselves
            // compute the delta from the current point and use the velocity to
            // indicate motion off the screen

            // check to see if the menu drawer is open
            // if it is, then we don't want to fling anything
            // the user is simply closing the drawer
            // also, if the user is sliding from the left edge of the screen
            // across then they simply want to open the drawer
            // don't fling
            val parentView: View
            if (m_useDrawerForCaseTree) {
                parentView = findViewById(R.id.activity_entry_application_with_drawer)
                // check to make sure the user isn't swiping too close to the left edge
                // we don't want them to accidentally activate the nav drawer
                if (e1.x < (SWIPE_MIN_DISTANCE / 2) || e1.x > (parentView.width - 60)) {
                    return false
                }
            }

            // if the user swiped vertically all around the place, do nothing
            if (abs(e1.y - e2.y) > SWIPE_MAX_OFF_PATH) return false

            // right to left swipe
            if (e1.x - e2.x > SWIPE_MIN_DISTANCE && abs(velocityX) > SWIPE_THRESHOLD_VELOCITY) {
                val runnable = Runnable { onNavigationNextButtonClicked() }
                runOnUiThread(runnable)
            } else if (e2.x - e1.x > SWIPE_MIN_DISTANCE && Math.abs(velocityX) > SWIPE_THRESHOLD_VELOCITY) {
                val runnable = Runnable { onNavigationPreviousButtonClicked() }
                runOnUiThread(runnable)
            }
            return false
        }
    }

    private fun changeLanguage() {
        applyCurrentFieldValues()
        val msg = EntryEngineMessage(this, this, EntryMessageRequestType.CHANGE_LANGUAGE)
        SendEntryEngineMessage(msg)
    }

    private fun showRefusals() {
        applyCurrentFieldValues()
        val msg = EntryEngineMessage(this, this, EntryMessageRequestType.SHOW_REFUSALS)
        SendEntryEngineMessage(msg)
    }

    fun editNotes() {
        m_qsfrag?.toggleEditNotes()
    }

    private fun editCaseNote() {
        class EditCaseNoteMessage(activity: Activity, listener: IEngineMessageCompletedListener) : EngineMessage(activity, listener) {
            override fun run() {
                EngineInterface.getInstance().editCaseNote()
            }
        }
        Messenger.getInstance().sendMessage(EditCaseNoteMessage(this, this))
    }

    private fun reviewNotes() {
        applyCurrentFieldValues()
        if (EngineInterface.getInstance().useHtmlDialogs()) {
            val msg = EntryEngineMessage(this, this, EntryMessageRequestType.REVIEW_NOTES)
            SendEntryEngineMessage(msg)
        } else {
            startActivityForResult(Intent(this, ReviewNotesActivity::class.java), ReviewNotesCode)
        }
    }

    private fun userbarClicked() {
        applyCurrentFieldValues()
        EngineInterface.getInstance().userbarHandler.clicked(this)
    }

    private fun savePartial() {
        applyCurrentFieldValues()
        Messenger.getInstance().sendMessage(object : EngineMessage(this, this) {
            override fun run() {
                EngineInterface.getInstance().savePartial()
            }
        })
    }

    private fun viewCurrentCase() {
        applyCurrentFieldValues()
        val msg = EntryEngineMessage(this, this, EntryMessageRequestType.VIEW_CURRENT_CASE)
        SendEntryEngineMessage(msg)
    }

    fun updateWindowTitle() {
        supportActionBar?.title = EngineInterface.getInstance().windowTitle
    }

    fun toggleNavigationControls() {
        val item = m_menu?.findItem(R.id.questionnaire_menuitem_toggle_nav_controls)
        m_qsfrag?.toggleNavigationControls(item)
    }

    fun ShowLabels() {
        m_navFrag?.ShowLabels()
    }

    fun ShowOrHideSkippedFields() {
        m_navFrag?.ShowOrHideSkippedFields()
    }

    override fun onFieldItemClicked(fieldSymbol: Int, index1: Int, index2: Int, index3: Int) {
        GoToField(fieldSymbol, index1, index2, index3)
    }

    private inner class QuestionTextActionInvokerListener(webView: WebView): ActionInvokerListener(webView) {
        override fun onEngineProgramControlExecuted(): Boolean {
            runOnUiThread {
                // update the field if there were any requests
                processCurrentField(true)
            }
            return true
        }
    }

    private inner class QuestionTextActionInvokerMessage(private val actionInvoker: ActionInvoker,
                                                         private val message: String,
                                                         private val oldCSProObjectRunAsyncHandler: ActionInvoker.OldCSProObjectRunAsyncHandler?): EngineMessage(this, this) {
        override fun run() {
            actionInvoker.runAsyncWorker(message, oldCSProObjectRunAsyncHandler)
        }
    }

    inner class QuestionTextActionInvoker(webView: WebView): ActionInvoker(webView, null, QuestionTextActionInvokerListener(webView)) {
        override fun runSync(message: String): String {
            // the current field values have to be updated on the UI thread
            val mutex = Semaphore(0)
            runOnUiThread {
                applyCurrentFieldValues()
                mutex.release()
            }

            try {
                mutex.acquire()
            } catch (e: Exception) {
            }

            return EngineInterface.getInstance().actionInvokerProcessMessage(getWebControllerKey(), listener, message, false, false)
        }

        override fun runAsync(message: String, oldCSProObjectRunAsyncHandler: OldCSProObjectRunAsyncHandler?) {
            runOnUiThread {
                // update the current field values
                applyCurrentFieldValues()

                // use the Messenger to process the message
                Messenger.getInstance().sendMessage(QuestionTextActionInvokerMessage(this, message, oldCSProObjectRunAsyncHandler))
            }
        }
    }

    fun createQuestionTextActionInvoker(webView: WebView) : ActionInvoker {
        return QuestionTextActionInvoker(webView)
    }

    companion object {
        @JvmField
        val START_MODE_PARAM = "StartMode"
        val OPERATOR_ID_PARAM = "OperatorID"
        val APP_DESCRIPTION_PARAM = "Description"
        @JvmField
        val CASEPOS_PARAM = "StartCasePosition"
        val PFF_FILENAME_PARAM = "PffFilename"
        @JvmField
        val KEEP_APP_OPEN_ON_FINISH_PARAM = "KeepAppOpenOnFinish"
        private val SWIPE_MIN_DISTANCE = 120
        private val SWIPE_MAX_OFF_PATH = 250
        private val SWIPE_THRESHOLD_VELOCITY = 200
        @JvmField
        val ReviewNotesCode = 2
        @JvmField
        val BluetoothDiscoverableCode = 3
        @JvmField
        val BluetoothChooseDeviceCode = 4
        @JvmField
        val AuthorizeDropboxCode = 5
        @JvmField
        val EnableLocationInSettings = 6
        val SystemAppCode = 7
    }
}