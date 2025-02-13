package gov.census.cspro.smartsync.addapp

import android.os.Bundle
import androidx.fragment.app.Fragment

/** This fragment has no UI. It's sole purpose is to
 * hold on to a syncClient reference so that it doesn't
 * get deleted on configuration changes (e.g. screen rotate)
 * when the parent activity is destroyed and recreated.
 * Seems like an aggregious hack to me but apparently
 * this is the recommended way for stuff that can't
 * be stuck in a Bundle and saved.
 */
class ApplicationDownloadFragment : Fragment() {

    var deploymentPackageDownloader: DeploymentPackageDownloader? = null

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        // This forces fragment to stay around.
        retainInstance = true
    }
}