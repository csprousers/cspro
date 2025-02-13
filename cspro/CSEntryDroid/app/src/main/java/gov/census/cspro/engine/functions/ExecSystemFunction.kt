package gov.census.cspro.engine.functions

import android.app.Activity
import android.content.Intent
import android.net.Uri
import gov.census.cspro.csentry.ui.GenericWebViewActivity
import gov.census.cspro.signature.SignatureActivity
import android.webkit.MimeTypeMap
import androidx.activity.result.ActivityResult
import gov.census.cspro.camera.PictureCaptureActivity
import gov.census.cspro.csentry.CSEntry
import gov.census.cspro.engine.Messenger
import gov.census.cspro.engine.Util
import gov.census.cspro.util.Constants
import timber.log.Timber
import java.io.File
import java.lang.Exception
import java.util.*

class ExecSystemFunction(private val m_command: String, private val m_wait: Boolean) : EngineFunction {

    override fun runEngineFunction(activity: Activity) {
        try {
            val (action, argument) = parseCommand(m_command)
            val intent: Intent = when (action) {
                "app" -> launchAppIntent(activity, argument)
                "call" -> dialerIntent(argument)
                "sms" -> smsIntent(argument)
                "browse" -> browserIntent(argument)
                "html" -> webViewIntent(activity, argument)
                "signature" -> signatureIntent(activity, argument)
                "view" -> viewFileIntent(activity, argument)
                "camera" -> pictureIntent(activity, argument)
                "gps" -> pointOnMapIntent(argument)
                else -> throw Exception("Invalid command $action in execsystem")
            }
            launchIntent(activity, intent)
        } catch (e: Exception) {
            Timber.e(e,"Error launching execsystem $m_command")
            endFunction(0)
        }
    }

    private fun viewFileIntent(activity: Activity, fileName: String): Intent {
        val extension = fileName.substring(fileName.lastIndexOf('.') + 1)
        val mimeType = MimeTypeMap.getSingleton().getMimeTypeFromExtension(extension)
        return Intent(Intent.ACTION_VIEW).apply {
            setDataAndType(Util.getShareableUriForFile(File(fileName), activity), mimeType)
            addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION)
        }
    }

    private fun launchAppIntent(activity: Activity, packageName: String) =
        activity.packageManager.getLaunchIntentForPackage(packageName)
                ?: throw Exception("No launch intent found for package $packageName")

    private fun dialerIntent(phoneNumber: String) =
        Intent(Intent.ACTION_DIAL).apply { data = Uri.parse("tel:$phoneNumber") }

    private fun smsIntent(numberAndMessage: String): Intent {
        val commaPos = numberAndMessage.indexOf(',')
        val phoneNumber = if (commaPos >= 0) numberAndMessage.substring(0, commaPos) else numberAndMessage
        val intent = Intent(Intent.ACTION_VIEW, Uri.parse("sms:" + phoneNumber.trim()))
        if (commaPos >= 0) intent.putExtra("sms_body", numberAndMessage.substring(commaPos + 1).trim())
        return intent
    }

    private fun browserIntent(url: String)
            = Intent(Intent.ACTION_VIEW).apply { data = Uri.parse(url) }

    private fun webViewIntent(activity: Activity, url: String) =
        Intent(activity, GenericWebViewActivity::class.java).apply { putExtra(GenericWebViewActivity.URL_PARAM, url) }

    private fun signatureIntent(activity: Activity, argument: String): Intent {
        val (file, msg) = parsePipeDelimited(argument)
        return Intent(activity, SignatureActivity::class.java).apply {
            putExtra(Constants.EXTRA_SIGNATURE_FILE_URL_KEY, file)
            msg?.let { putExtra(Constants.EXTRA_SIGNATURE_MESSAGE_KEY, msg) }
        }

    }

    private fun pictureIntent(activity: Activity, argument: String): Intent {
        val (file, msg) = parsePipeDelimited(argument)
        return Intent(activity, PictureCaptureActivity::class.java).apply {
            putExtra(Constants.EXTRA_CAPTURE_IMAGE_FILE_URL_KEY, file)
            msg?.let { putExtra(Constants.EXTRA_USER_MESSAGE_KEY, it) }
        }
    }

    private fun pointOnMapIntent(latLongName: String): Intent {
        val commaPos1 = latLongName.indexOf(',')
        val commaPos2 = latLongName.indexOf(',', commaPos1 + 1)
        val latitude = latLongName.substring(0, commaPos1).trim().toDouble()
        val longitude = (if (commaPos2 >= 0) latLongName.substring(commaPos1 + 1, commaPos2) else latLongName.substring(commaPos1 + 1)).trim().toDouble()
        var geoString = String.format(CSEntry.CS_LOCALE, "geo:%1$.9f,%2$.9f?q=%1$.9f,%2$.9f", latitude, longitude)
        if (commaPos2 >= 0) geoString = String.format("%s(%s)", geoString, latLongName.substring(commaPos2 + 1))
        return Intent(Intent.ACTION_VIEW).apply { data= Uri.parse(geoString) }
    }

    private fun launchIntent(activity: Activity, intent: Intent?) {
        if (m_wait)
            Messenger.getInstance().startActivityForResultFromEngineFunction(activity,
                { result: ActivityResult -> endFunction(if (result.resultCode == Activity.RESULT_OK) 1 else 0) }, intent)
        else {
            activity.startActivity(intent)
            endFunction(1)
        }
    }

    private fun parseCommand(command: String): Pair<String, String> {
        val colonPos = command.indexOf(':')
        if (colonPos == -1) throw Exception("Invalid execsystem command: missing colon")
        val action = command.substring(0, colonPos).toLowerCase(Locale.ROOT)
        val argument = command.substring(colonPos + 1).trim()
        return Pair(action, argument)
    }

    private fun parsePipeDelimited(s: String): Pair<String, String?> {
        val delimiterPos = s.indexOf('|')
        return if (delimiterPos >= 0) {
            Pair(s.substring(0, delimiterPos), s.substring(delimiterPos + 1))
        } else {
            Pair(s, null)
        }
    }

    private fun endFunction(retVal: Int) {
        Messenger.getInstance().engineFunctionComplete(retVal.toLong())
    }
}