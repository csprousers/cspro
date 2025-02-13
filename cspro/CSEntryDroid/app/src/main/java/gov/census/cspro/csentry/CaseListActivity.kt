package gov.census.cspro.csentry

import android.content.Intent
import android.os.Bundle
import android.view.KeyEvent
import android.widget.Toast
import androidx.appcompat.app.AppCompatActivity
import androidx.fragment.app.Fragment
import androidx.fragment.app.FragmentManager
import androidx.fragment.app.FragmentTransaction
import com.dropbox.core.android.Auth
import gov.census.cspro.commonui.ErrorDialogFragment
import gov.census.cspro.commonui.ErrorDialogFragment.OnErrorFragmentDismissed
import gov.census.cspro.csentry.ui.EntryActivity
import gov.census.cspro.data.CaseSummary
import gov.census.cspro.engine.*
import gov.census.cspro.engine.functions.AuthorizeDropboxFunction

class CaseListActivity : AppCompatActivity(), CasesFragment.OnFragmentInteractionListener, AppLoadingFragment.OnFragmentInteractionListener, GetOperatorIdFragment.OnFragmentInteractionListener, OnErrorFragmentDismissed {
    private var mState: Int = 0
    private var mOperatorId: String? = null
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        // instantiate the application interface
        EngineInterface.CreateEngineInterfaceInstance(application)

        // If savedInstanceState is null but an app is open then this a second copy of activity
        // being opened which we do not allow.
        if (EngineInterface.getInstance().isApplicationOpen && savedInstanceState == null) {
            Toast.makeText(this, getString(R.string.app_cannot_run_multiple_programs), Toast.LENGTH_LONG).show()
            finish()
            return
        }
        setContentView(R.layout.activity_case_list)
        if (savedInstanceState == null) {
            // No savedInstanceState so this is the first time the activity has been created
            // i.e. this is not from a config change. When first created we start with app loading.
            showAppLoadingFragment()
        } else {
            mState = savedInstanceState.getInt(STATE)
            mOperatorId = savedInstanceState.getString(OPERATOR_ID)
            if (!EngineInterface.getInstance().isApplicationOpen) {
                // This is not first time activity is started but since last time activity was shown
                // the engine got closed so need to reload the application as if this was first
                // time shown
                if (mState != STATE_APP_LOADING) {
                    showAppLoadingFragment()
                }
            }
        }
    }

    public override fun onSaveInstanceState(savedInstanceState: Bundle) {
        super.onSaveInstanceState(savedInstanceState)
        savedInstanceState.putString(OPERATOR_ID, mOperatorId)
        savedInstanceState.putInt(STATE, mState)
    }

    private val currentFragment: Fragment?
        get() {
            return supportFragmentManager.findFragmentById(R.id.fragment_placeholder)
        }

    private fun showAppLoadingFragment() {
        mState = STATE_APP_LOADING
        if (intent.data == null) throw AssertionError("Intent is missing path to pff file")
        val pffFileName: String = intent.data?.path
                ?: throw AssertionError("Intent is missing path to pff file")
        val appDescription: String? = intent.getStringExtra(APP_DESCRIPTION)
        val fm: FragmentManager = supportFragmentManager
        fm.beginTransaction()
            .replace(R.id.fragment_placeholder, AppLoadingFragment.newInstance(pffFileName, appDescription))
            .commitNow()
    }

    private fun showGetOperatorIdFragment() {
        mState = STATE_OPERATOR_ID
        val fm: FragmentManager = supportFragmentManager
        fm.beginTransaction()
            .replace(R.id.fragment_placeholder, GetOperatorIdFragment.newInstance())
            .commitNow()
    }

    private fun showCasesFragment() {
        mState = STATE_CASE_LIST
        val transaction: FragmentTransaction = supportFragmentManager.beginTransaction()
        transaction.replace(R.id.fragment_placeholder, CasesFragment.newInstance())
        transaction.commit()
    }

    private fun clearFragment() {
        mState = STATE_END_ENTRY_REQUESTED
        val fm: FragmentManager = supportFragmentManager
        val currentFrag: Fragment? = currentFragment
        if (currentFrag != null) fm.beginTransaction().remove(currentFrag).commitNow()
    }

    private fun appFullyLoadedAndReady() {
        // App has now been loaded and operator id has been entered (if needed)
        // now we are ready to either show the case list or immediately
        // launch entry if pff parameters say to do so
        if (EngineInterface.getInstance().startInEntry) {
            launchEntry()
            return
        }
        if (EngineInterface.getInstance().caseListingLockFlag) {
            // If the case listing is locked and we didn't start case from the pff
            // file above then show an error since you have to specify a start case or add
            // mode if case listing is locked
            val errorDialog: ErrorDialogFragment = ErrorDialogFragment.newInstance(
                getString(R.string.app_startup_failure_msg_title),
                getString(R.string.app_startup_caselisting_locked),
                R.string.app_startup_caselisting_locked)
            errorDialog.show(supportFragmentManager, "errorDialog")
        } else {
            // No lock or start mode in the pff file so show the case list
            showCasesFragment()
        }
    }

    public override fun onResume() {
        super.onResume()
        val appiface: EngineInterface = EngineInterface.getInstance()
        if (appiface.isApplicationOpen) {
            // Application already open - show case list
            if (appiface.stopCode == 1) // this will come from a stop(1) executed in logic
            {
                endEntryApplication()
            }
        }
        else if(AuthorizeDropboxFunction.isAuthenticating()){
            AuthorizeDropboxFunction.setAuthenticationComplete()
            var dbxCredential: String? = ""
            val credential = Auth.getDbxCredential()
            if (credential != null) {
                dbxCredential = credential.toString()
            }
            Messenger.getInstance().engineFunctionComplete(dbxCredential)
        }
        // Ensure that fragment shown matches the state - the two can get out of sync
        // when we resume after EntryActivity if we started EntryActivity directly from
        // AppLoading or GetOperatorId rather than from CasesFragment.
        ensureFragmentMatchesState()
    }

    private fun ensureFragmentMatchesState() {
        // If we launched entry directly from app loading because of a start mode
        // in pff file then when we return the fragment will still be app loading
        // but we want to show the case listing. In this situation the state will have
        // been set to case list and it will be different from fragment so we
        // need to switch to case list.
        if (mState == STATE_CASE_LIST && !(currentFragment is CasesFragment)) {
            if (EngineInterface.getInstance().doNotShowCaseListing()) {
                // Case listing is locked so go back to AppListing
                endEntryApplication()
            } else {
                showCasesFragment()
            }
        } else if (mState == STATE_END_ENTRY_REQUESTED && currentFragment != null) {
            // Don't show any fragment once end entry has been requested
            clearFragment()
        }
    }

    override fun applicationLoaded() {
        // get the operator id if necessary
        if( EngineInterface.getInstance().useHtmlDialogs() ) {
            appFullyLoadedAndReady()
            return
        }
        if (Util.stringIsNullOrEmpty(mOperatorId)) mOperatorId = EngineInterface.getInstance().opIDFromPff
        if (EngineInterface.getInstance().askOpIDFlag) {
            if (Util.stringIsNullOrEmpty(mOperatorId)) {
                showGetOperatorIdFragment()
            } else {
                EngineInterface.getInstance().setOperatorId(mOperatorId)
                appFullyLoadedAndReady()
            }
        } else {
            appFullyLoadedAndReady()
        }
    }

    override fun onOperatorIdEntered(operatorId: String) {
        mOperatorId = operatorId
        EngineInterface.getInstance().setOperatorId(operatorId)
        appFullyLoadedAndReady()
    }

    override fun applicationLoadFailed(filename: String) {
        val errorDialog: ErrorDialogFragment = ErrorDialogFragment.newInstance(
            getString(R.string.app_startup_failure_msg_title), String.format(getString(R.string.app_startup_failure_msg), filename),
            R.string.app_startup_failure_msg)
        errorDialog.show(supportFragmentManager, "errorDialog")
    }

    override fun errorDialogDismissed(requestCode: Int) {
        when (requestCode) {
            R.string.app_startup_failure_msg ->                 // If we get an error from app load go back to previous activity
                finish()
            R.string.app_startup_caselisting_locked ->                 // If case listing is locked and no case to load then close current app
                // which will in turn close this activity
                endEntryApplication()
        }
    }

    override fun onKeyDown(keyCode: Int, event: KeyEvent): Boolean {
        if (keyCode == KeyEvent.KEYCODE_BACK) {
            endEntryApplication()
            return true
        }
        return super.onKeyDown(keyCode, event)
    }

    private fun endEntryApplication() {
        if (mState == STATE_END_ENTRY_REQUESTED) return

        // Hide fragments so that we don't get any case list refreshes
        // while closing app
        clearFragment()
        class EndApplicationListener : IEngineMessageCompletedListener {
            override fun onMessageCompleted(msg: EngineMessage) {
                finish()
            }
        }

        val msg: EngineMessage = object : EngineMessage(this, EndApplicationListener()) {
            override fun run() {
                EngineInterface.getInstance().endApplication()
            }
        }
        Messenger.getInstance().sendMessage(msg)
    }

    override fun onAddCase() {
        launchEntry("Add")
    }

    override fun onViewCase(caseSummary: CaseSummary?) {
        if (caseSummary != null) {
            val intent = Intent(this, ViewQuestionnaireActivity::class.java)
            intent.putExtra(ViewQuestionnaireActivity.KEY, caseSummary.key)
            intent.putExtra(ViewQuestionnaireActivity.POSITION_IN_REPOSITORY, caseSummary.positionInRepository)
            startActivity(intent)
        }
    }

    override fun onInsertCase(caseSummary: CaseSummary?) {
        launchEntry("Insert", caseSummary)
    }

    override fun onModifyCase(caseSummary: CaseSummary?) {
        if (EngineInterface.getInstance().modifyLockFlag) {
            Toast.makeText(this, getString(R.string.app_startup_modify_locked), Toast.LENGTH_LONG).show()
        } else {
            launchEntry("Modify", caseSummary)
        }
    }

    private fun launchEntry(startMode: String? = null, caseSummary: CaseSummary? = null) {
        mState = STATE_CASE_LIST // ensure that we come back to case list after entry
        val intent = Intent(this, EntryActivity::class.java)
        if (startMode != null) intent.putExtra(EntryActivity.START_MODE_PARAM, startMode)
        if (caseSummary != null) intent.putExtra(EntryActivity.CASEPOS_PARAM, caseSummary.positionInRepository)
        val pffFileName: String? = getIntent().data?.path
        intent.putExtra(EntryActivity.PFF_FILENAME_PARAM, pffFileName)
        intent.putExtra(EntryActivity.APP_DESCRIPTION_PARAM,
            getIntent().getStringExtra(ApplicationsFragment.APP_DESCRIPTION_PARAM))
        intent.putExtra(EntryActivity.OPERATOR_ID_PARAM, mOperatorId)
        intent.putExtra(EntryActivity.KEEP_APP_OPEN_ON_FINISH_PARAM, true)
        startActivity(intent)
    }

    @Deprecated("Deprecated in Java")
    public override fun onActivityResult(requestCode: Int, resultCode: Int, intent: Intent?) {
        super.onActivityResult(requestCode, resultCode, intent)
    }

    companion object {
        const val APP_DESCRIPTION: String = "APP_DESCRIPTION"
        const val OPERATOR_ID: String = "OPERATOR_ID"
        const val STATE: String = "STATE"
        private const val STATE_APP_LOADING: Int = 1
        private const val STATE_OPERATOR_ID: Int = 2
        private const val STATE_CASE_LIST: Int = 3
        private const val STATE_END_ENTRY_REQUESTED: Int = 4
    }
}