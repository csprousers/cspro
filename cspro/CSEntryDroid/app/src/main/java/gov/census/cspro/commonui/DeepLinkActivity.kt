package gov.census.cspro.commonui

import android.net.Uri
import android.os.Bundle
import android.webkit.URLUtil
import android.widget.Toast
import androidx.appcompat.app.AppCompatActivity
import androidx.lifecycle.lifecycleScope
import gov.census.cspro.csentry.R
import gov.census.cspro.engine.EngineInterface
import gov.census.cspro.smartsync.addapp.DeploymentPackageDownloader
import gov.census.cspro.util.Constants
import kotlinx.coroutines.launch
import java.util.*


class DeepLinkListener : AppCompatActivity() {


    public override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        // instantiate the application interface
        EngineInterface.CreateEngineInterfaceInstance(application)

        val deepLinkIntent = intent
        var data: Uri? = deepLinkIntent?.data

        if (data == null) {
            val barcodeLink: String? = intent.getStringExtra(Constants.BARCODE_URI)
            data = Uri.parse(barcodeLink)
        }

        downloadSurvey(data)
    }

    private fun downloadSurvey(data: Uri?) {

        val server: String? = data?.getQueryParameter("server")
        val app: String? = data?.getQueryParameter("app")
        val cred: String? = data?.getQueryParameter("cred")

        if (data != null
            && server != null
            && (URLUtil.isValidUrl(server)
                || server.toLowerCase(Locale.ROOT).contains("ftp")
                || server.toLowerCase(Locale.ROOT).contains("dropbox"))
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


}