package gov.census.cspro.csentry

import android.app.Activity
import android.content.Intent
import android.webkit.MimeTypeMap
import android.widget.Toast
import gov.census.cspro.csentry.ui.GenericWebViewActivity
import gov.census.cspro.engine.EngineInterface
import gov.census.cspro.engine.Util
import java.io.File
import java.net.URL

object SystemSettings {

    // for multiple activities
    const val MenuHelp: String = "CSEntry.Menu.Help"
    const val SettingHelpUrl: String = "CSEntry.Setting.HelpUrl"

    // for the application listing activity
    const val SettingLaunchSingleApplicationAutomatically: String = "CSEntry.Setting.LaunchSingleApplicationAutomatically"
    const val MenuAddApplication: String = "CSEntry.Menu.AddApplication"
    const val MenuUpdateApplications: String = "CSEntry.Menu.UpdateApplications"
    const val MenuSettings: String = "CSEntry.Menu.Settings"
    const val MenuShowHiddenApplications: String = "CSEntry.Menu.ShowHiddenApplications"

    // for the entry activity
    const val MenuShowRefusals: String = "CSEntry.Menu.ShowRefusals"
    const val MenuReviewAllNotes: String = "CSEntry.Menu.ReviewAllNotes"
    const val MenuAdvanceToEnd: String = "CSEntry.Menu.AdvanceToEnd"
    const  val SettingShowNavigationControls: String = "CSEntry.Setting.ShowNavigationControls"
    const  val MenuShowCaseTree: String = "CSEntry.Menu.ShowCaseTree"

    // for the case tree
    const  val MenuAddInsertOccurrence: String = "CSEntry.Menu.AddInsertOccurrence"
    const  val MenuDeleteOccurrence: String = "CSEntry.Menu.DeleteOccurrence"
    const  val SettingShowCaseTreeInOverlay: String = "CSEntry.Setting.ShowCaseTreeInOverlay"

    //for camera aspect ratio
    const val CameraAspectRatio: String = "CSEntry.Setting.CameraAspectRatio"

    fun LaunchHelp(activity: Activity) {
        val help_url: String = EngineInterface.GetSystemSettingString(SettingHelpUrl, activity.getString(R.string.url_main_help))
        try {
            // check if this is a website; if not, it will throw an exception
            URL(help_url).toURI()
            val intent = Intent(activity, GenericWebViewActivity::class.java)
            intent.putExtra(GenericWebViewActivity.URL_PARAM, help_url)
            activity.startActivity(intent)
            return
        } catch (exception: Exception) {
        }
        try {
            // if not a website, try to open the help URL as if it is a file
            if (File(help_url).exists()) {
                val extension: String = help_url.substring(help_url.lastIndexOf('.') + 1)
                val mimeType: String? = MimeTypeMap.getSingleton().getMimeTypeFromExtension(extension)
                val intent = Intent(Intent.ACTION_VIEW)
                intent.setDataAndType(Util.getShareableUriForFile(File(help_url), activity), mimeType)
                intent.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION)
                activity.startActivity(intent)
                return
            }
        } catch (exception: Exception) {
        }
        Toast.makeText(activity, String.format(activity.getString(R.string.menu_help_invalid_url), help_url), Toast.LENGTH_LONG).show()
    }
}