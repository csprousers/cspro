package gov.census.cspro.engine.functions

import android.app.Activity
import androidx.appcompat.app.AppCompatActivity

import gov.census.cspro.csentry.ui.DialogWebViewFragment


class DisplayCSHtmlDlgFunction(private val url: String, private val actionInvokerAccessTokenOverride: String?): EngineFunction {
    override fun runEngineFunction(activity: Activity) {
        val dlgWebViewFragment = DialogWebViewFragment.newInstance(false, url, actionInvokerAccessTokenOverride, null, null)

        dlgWebViewFragment.show(
            (activity as AppCompatActivity).supportFragmentManager,
            "ModalDialogFragment"
        )
    }
}