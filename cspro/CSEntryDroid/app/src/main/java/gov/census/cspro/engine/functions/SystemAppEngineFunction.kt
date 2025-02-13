package gov.census.cspro.engine.functions

import android.app.Activity
import android.content.ComponentName
import android.content.Intent
import android.net.Uri
import android.os.Bundle
import gov.census.cspro.csentry.ui.EntryActivity
import gov.census.cspro.engine.Messenger
import timber.log.Timber
import java.lang.Exception

fun Bundle.getAndRemoveIgnoreCase(key: String): String?
{
    val keyCorrectCase = keySet().firstOrNull { it.equals(key, true) }
    val value = getString(keyCorrectCase)
    remove(keyCorrectCase)
    return value
}

class SystemAppEngineFunction(private val packageName: String?, private val activityName: String?, private val arguments: Bundle) : EngineFunction {
    override fun runEngineFunction(activity: Activity?) {

        val packageManager = activity?.packageManager
        val intent = when {
            !packageName.isNullOrBlank() && !activityName.isNullOrBlank() -> Intent().apply {
                component = ComponentName(packageName, activityName)
            }
            !packageName.isNullOrBlank() && activityName.isNullOrBlank() -> packageManager?.getLaunchIntentForPackage(
                packageName
            ) ?: Intent()
            else -> Intent()
        }

        arguments.getAndRemoveIgnoreCase("data")?.let { intent.data = Uri.parse(it) }
        arguments.getAndRemoveIgnoreCase("action")?.let { intent.action = it }
        arguments.getAndRemoveIgnoreCase("type")?.let { intent.type = it }

        // first try to start the activity using the supplied package name
        if( packageManager?.let { intent.resolveActivity(it) } != null ) {
            intent.flags = 0 //this removes the NEW_TASK flag from the intent and prevents the activity returning result before it finished
            intent.putExtras(arguments)
            try {
                activity.startActivityForResult(intent, EntryActivity.SystemAppCode)
                return
            } catch (e: Exception) {
                Timber.e(e, "Error launching SystemApp using package: $packageName")
            }
        }

        // on failure, and when no activity name is specified, parse the "package name" as a URI and,
        // if parsable, view that URI; this allows deep links to be opened
        else if( packageName != null && activityName == null && activity != null ) {
            try {
                val viewIntent = Intent(Intent.ACTION_VIEW).apply {
                    data = Uri.parse(packageName)
                }
                activity.startActivityForResult(viewIntent, EntryActivity.SystemAppCode)
                return
            } catch( e: Exception ) {
                Timber.e(e, "Error launching SystemApp using view: $packageName")
            }
        }

        // fallback on error
        Messenger.getInstance().engineFunctionComplete(null)
    }
}