package gov.census.cspro.csentry

import android.content.Intent
import android.os.Build
import android.os.Bundle
import android.text.Html
import android.text.method.LinkMovementMethod
import android.view.View
import android.widget.ImageView
import android.widget.TextView
import androidx.appcompat.app.AppCompatActivity
import gov.census.cspro.csentry.ui.GenericWebViewActivity
import gov.census.cspro.engine.EngineInterface
import timber.log.Timber

class AboutActivity : AppCompatActivity(), View.OnClickListener {
    /***
     * Create the activity view and controls
     */
    override fun onCreate(savedInstanceState: Bundle?) {

        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_about_csentry)

        try {
            // pull the about text and get the HTML links working
            val aboutText: String = getString(R.string.about_creators) + "<br><br>" + getString(R.string.about_disclaimer)
            val textviewCreators: TextView = findViewById(R.id.textView_about_creators)
            textviewCreators.text = Html.fromHtml(aboutText)
            textviewCreators.movementMethod = LinkMovementMethod.getInstance()

            // add the version
            val textviewVersion: TextView = findViewById(R.id.textview_about_version)
            textviewVersion.text = String.format(getText(R.string.about_version).toString(), EngineInterface.getVersionDetailedString())

            // add the release date
            val textviewReleaseDate: TextView = findViewById(R.id.textview_about_release_date)
            textviewReleaseDate.text = String.format(getString(R.string.about_release_date), EngineInterface.getReleaseDateString())

            // display the android version
            val textviewAndroid: TextView = findViewById(R.id.textview_about_android_version)
            textviewAndroid.text = String.format(getString(R.string.about_android_version), Build.VERSION.RELEASE)

            // setup the licenses
            val textviewLicenses: TextView = findViewById(R.id.textview_about_licenses)
            textviewLicenses.setOnClickListener(this)

            // setup the globe icon
            val globeImageView: ImageView = findViewById(R.id.about_globe_icon)
            globeImageView.setOnClickListener(this)
        } catch (ex: Exception) {
            Timber.e(ex, "An Error Occurred While Compiling Version Information.")
        }
    }

    /***
     * Handlers for icon clicks
     */
    override fun onClick(v: View) {
        try {
            val viewId: Int = v.id
            var url: String? = null
            if (viewId == R.id.about_globe_icon) url = getString(R.string.about_csentry_web_url) else if (viewId == R.id.textview_about_licenses) url = "file:///android_asset/Licenses.html"

            if (url != null) {
                val intent = Intent(this, GenericWebViewActivity::class.java)
                intent.putExtra(GenericWebViewActivity.URL_PARAM, url)
                startActivity(intent)
            }
        } catch (exception: Exception) {
        }
    }
}