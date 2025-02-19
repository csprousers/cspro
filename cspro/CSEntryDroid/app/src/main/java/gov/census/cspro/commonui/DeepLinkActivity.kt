package gov.census.cspro.commonui

import android.content.Intent
import android.net.Uri
import android.os.Bundle
import android.webkit.URLUtil
import android.widget.Toast
import androidx.appcompat.app.AppCompatActivity
import androidx.lifecycle.lifecycleScope
import gov.census.cspro.bridge.CNPifFile
import gov.census.cspro.csentry.CaseListActivity
import gov.census.cspro.csentry.NonEntryApplicationActivity
import gov.census.cspro.csentry.R
import gov.census.cspro.engine.EngineInterface
import gov.census.cspro.smartsync.addapp.DeploymentPackageDownloader
import gov.census.cspro.util.Constants
import kotlinx.coroutines.launch
import java.io.File
import java.util.*


class DeepLinkListener : AppCompatActivity() {

    public override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        // instantiate the application interface
        EngineInterface.CreateEngineInterfaceInstance(application)

        try {
            var data: Uri? = intent?.data

            if(data == null) {
                val barcodeLink: String? = intent.getStringExtra(Constants.BARCODE_URI)
                data = Uri.parse(barcodeLink)
            } else if(data.path?.startsWith(getString(R.string.deeplink_pff_path_prefix)) == true) {
                runApplication(data)
            }

            downloadApplication(data)
        }
        catch(e: Exception) {
            Toast.makeText(this, e.message, Toast.LENGTH_LONG).show()
            finish()
        }
    }

    private fun downloadApplication(data: Uri?) {
        val server: String? = data?.getQueryParameter("server")
        val app: String? = data?.getQueryParameter("app")
        val cred: String? = data?.getQueryParameter("cred")

        if (data != null
            && server != null
            && (URLUtil.isValidUrl(server)
                || server.lowercase(Locale.ROOT).contains("ftp")
                || server.lowercase(Locale.ROOT).contains("dropbox"))
            && app != null) {
            val context = this
            lifecycleScope.launch {

                val result = runEngine {
                    val downloader = DeploymentPackageDownloader()
                    val connectResult = when {
                        (cred != null) -> downloader.ConnectToServerCredentials(server, app, cred)
                        else -> downloader.ConnectToServer(server)
                    }

                    if (connectResult != DeploymentPackageDownloader.resultOk) {
                        return@runEngine connectResult
                    }
                    try {
                        downloader.InstallPackage(app, false)
                    } finally {
                        downloader.Disconnect()
                    }
                }

                if (result == DeploymentPackageDownloader.resultOk)
                    Toast.makeText(context, String.format(getString(R.string.add_app_install_success), app), Toast.LENGTH_LONG).show()

                finish()
            }
        } else {
            finish()
        }
    }

    private fun runApplication(deepLinkUri: Uri) {
        assert(deepLinkUri.pathSegments.isNotEmpty() && deepLinkUri.pathSegments[0] == "pff")

        val pffFilePath = CNPifFile.CreatePffFromDeepLinkUrl(deepLinkUri.toString())
        val pff = CNPifFile(pffFilePath)

        // open entry PFFs with CaseListActivity and other PFFs with NonEntryApplicationActivity
        val intent = Intent(this, if( pff.IsAppTypeEntry() ) { CaseListActivity::class.java } else { NonEntryApplicationActivity::class.java }).apply {
            action = Intent.ACTION_VIEW
            data = Uri.fromFile(File(pffFilePath))
        }

        startActivity(intent)
        finish()
    }
}
