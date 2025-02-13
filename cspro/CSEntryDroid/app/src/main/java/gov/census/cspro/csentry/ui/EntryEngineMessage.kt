package gov.census.cspro.csentry.ui

import android.app.Activity
import gov.census.cspro.csentry.R
import gov.census.cspro.engine.EngineInterface
import gov.census.cspro.engine.EngineMessage
import gov.census.cspro.engine.IEngineMessageCompletedListener
import gov.census.cspro.engine.PffStartModeParameter

class EntryEngineMessage internal constructor(activity: Activity?,
                                              listener: IEngineMessageCompletedListener?, val entryMessageRequestType: EntryMessageRequestType) : EngineMessage(activity, listener) {
    enum class EntryMessageRequestType {
        MSG_NONE,
        START_APPLICATION,
        END_APPLICATION,
        END_LEVEL,
        END_LEVEL_OCC,
        END_GROUP,
        ADVANCE_TO_END,
        NEXT_FIELD,
        PREVIOUS_FIELD,
        PREVIOUS_PERSISTENT_FIELD,
        USER_TRIGGERED_STOP,
        GOTO_FIELD,
        GOTO_NOTE_FIELD,
        REVIEW_NOTES,
        INSERT_OCC,
        DELETE_OCC,
        INSERT_OCC_AFTER,
        CHANGE_LANGUAGE,
        SHOW_REFUSALS,
        VIEW_CURRENT_CASE
    }

    var errorMessage: String? = null
        private set

    override fun run() {
        when (entryMessageRequestType) {
            EntryMessageRequestType.START_APPLICATION -> setResult(if (startEntryApplication()) 1 else 0.toLong())
            EntryMessageRequestType.END_APPLICATION -> {
                EngineInterface.getInstance().stopApplication(this.activity)
                val keepOpenOnFinish = this.activity.intent.getBooleanExtra(EntryActivity.KEEP_APP_OPEN_ON_FINISH_PARAM, false)
                if (!keepOpenOnFinish) EngineInterface.getInstance().endApplication()
            }
            EntryMessageRequestType.ADVANCE_TO_END -> EngineInterface.getInstance().AdvanceToEnd()
            EntryMessageRequestType.END_LEVEL -> EngineInterface.getInstance().EndLevel()
            EntryMessageRequestType.END_LEVEL_OCC -> EngineInterface.getInstance().EndLevelOcc()
            EntryMessageRequestType.END_GROUP -> EngineInterface.getInstance().EndGroup()
            EntryMessageRequestType.NEXT_FIELD -> EngineInterface.getInstance().NextField()
            EntryMessageRequestType.PREVIOUS_FIELD -> EngineInterface.getInstance().PreviousField()
            EntryMessageRequestType.PREVIOUS_PERSISTENT_FIELD -> EngineInterface.getInstance().PreviousPersistentField()
            EntryMessageRequestType.GOTO_FIELD -> {
                val index = getObject() as IntArray
                EngineInterface.getInstance().goToField(wParam.toInt(), index[0], index[1], index[2])
            }
            EntryMessageRequestType.GOTO_NOTE_FIELD -> EngineInterface.getInstance().goToNoteField(wParam)
            EntryMessageRequestType.REVIEW_NOTES  -> EngineInterface.getInstance().reviewNotes()
            EntryMessageRequestType.USER_TRIGGERED_STOP -> EngineInterface.getInstance().runUserTriggedStop()
            EntryMessageRequestType.INSERT_OCC -> EngineInterface.getInstance().insertOcc()
            EntryMessageRequestType.DELETE_OCC -> EngineInterface.getInstance().deleteOcc()
            EntryMessageRequestType.INSERT_OCC_AFTER -> EngineInterface.getInstance().insertOccAfter()
            EntryMessageRequestType.CHANGE_LANGUAGE -> EngineInterface.getInstance().changeLanguage()
            EntryMessageRequestType.SHOW_REFUSALS -> setResult(if (EngineInterface.getInstance().showRefusals()) 1 else 0.toLong())
            EntryMessageRequestType.VIEW_CURRENT_CASE -> EngineInterface.getInstance().viewCurrentCase()
            else -> {
            }
        }
    }

    private fun startEntryApplication(): Boolean {
        var appStarted = false
        val engineInterface = EngineInterface.getInstance()
        var startMode = m_activity.intent.getStringExtra(EntryActivity.START_MODE_PARAM)
        var casePosition = m_activity.intent.getDoubleExtra(EntryActivity.CASEPOS_PARAM, -1.0)
        if (startMode == null) {
            // Start params not defined in the intent, use parameters from the pff file
            val pffStartMode = engineInterface.queryPffStartMode()
            when (pffStartMode.action) {
                PffStartModeParameter.ADD_NEW_CASE -> startMode = "Add"
                PffStartModeParameter.MODIFY_CASE -> {
                    startMode = "Modify"
                    casePosition = pffStartMode.modifyCasePosition
                }
                PffStartModeParameter.MODIFY_ERROR -> {
                    errorMessage = String.format(activity.getString(R.string.app_startup_failure_case_missing), engineInterface.startCaseKey)
                    return false
                }
                PffStartModeParameter.NO_ACTION -> {
                    errorMessage = activity.getString(R.string.app_startup_failure_start_mode_missing)
                    return false
                }
                else -> {
                    errorMessage = activity.getString(R.string.app_startup_failure_start_mode_missing)
                    return false
                }
            }
        }
        if (startMode.equals("Add", ignoreCase = true)) { // add new case
            appStarted = if (casePosition >= 0) {
                // If add mode includes a case position then launch existing case at that pos
                engineInterface.modifyCase(casePosition)
            } else {
                // No pos given so start new case
                engineInterface.start()
            }
        } else if (startMode.equals("Insert", ignoreCase = true)) // insert a case
            appStarted = engineInterface.insertCase(casePosition) else if (startMode.equals("Modify", ignoreCase = true)) { //modify a case
            appStarted = engineInterface.modifyCase(casePosition)
        } else {
            errorMessage = activity.getString(R.string.app_startup_failure_start_mode_missing)
        }
        return appStarted
    }
}