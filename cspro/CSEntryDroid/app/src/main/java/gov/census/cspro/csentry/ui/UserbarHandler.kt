package gov.census.cspro.csentry.ui

import android.app.Activity
import android.app.AlertDialog
import android.view.MenuItem
import gov.census.cspro.engine.EngineInterface
import gov.census.cspro.engine.EngineMessage
import gov.census.cspro.engine.IEngineMessageCompletedListener
import gov.census.cspro.engine.Messenger
import gov.census.cspro.engine.functions.EngineFunction

class UserbarHandler : EngineFunction {
    private var m_menuItem: MenuItem? = null
    private var m_visibility = false
    private var m_buttonTexts: Array<String?>? = null

    fun setParameters(visibility: Boolean, buttonTexts: Array<String?>) {
        m_visibility = visibility
        m_buttonTexts = buttonTexts.clone()
    }

    override fun runEngineFunction(activity: Activity) {
        if (m_menuItem != null) m_menuItem?.isVisible = m_visibility
        Messenger.getInstance().engineFunctionComplete(null)
    }

    fun creatingMenu(menuItem: MenuItem?) {
        m_menuItem = menuItem
        m_menuItem?.isVisible = m_visibility
    }

    fun clicked(activity: Activity?) {
        if (m_buttonTexts == null || m_buttonTexts?.size == 0) return
        val builder = AlertDialog.Builder(activity)
        builder.setItems(m_buttonTexts) { _, which -> executeButton(activity, which) }
        builder.show()
    }

    internal inner class UserbarMessage(activity: Activity?, listener: IEngineMessageCompletedListener?, private val m_index: Int) : EngineMessage(activity, listener) {
        override fun run() {
            EngineInterface.getInstance().runUserBarFunction(m_index)
        }
    }

    fun executeButton(activity: Activity?, index: Int) {
        Messenger.getInstance().sendMessage(UserbarMessage(activity, activity as IEngineMessageCompletedListener?, index))
    }
}