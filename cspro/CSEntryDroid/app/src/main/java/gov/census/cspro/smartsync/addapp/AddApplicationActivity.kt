package gov.census.cspro.smartsync.addapp

import android.app.Activity
import android.app.AlertDialog
import android.content.Intent
import android.media.MediaScannerConnection
import android.os.Bundle
import android.widget.Toast
import androidx.appcompat.app.AppCompatActivity
import com.dropbox.core.android.Auth
import gov.census.cspro.camera.BarcodeCaptureActivity
import gov.census.cspro.csentry.R
import gov.census.cspro.csentry.ui.EntryActivity
import gov.census.cspro.engine.*
import gov.census.cspro.engine.functions.AuthorizeDropboxFunction
import gov.census.cspro.smartsync.addapp.ChooseApplicationFragment.OnAddAppChooseApplicationListener
import gov.census.cspro.smartsync.addapp.ChooseApplicationSourceFragment.OnAddAppChooseSourceListener
import gov.census.cspro.smartsync.addapp.ServerDetailsFragment.OnAddAppServerDetailsListener
import gov.census.cspro.util.Constants
import java.io.File
import java.io.FileOutputStream
import java.io.IOException
import java.io.OutputStream
import java.net.URI
import java.util.*

class AddApplicationActivity : AppCompatActivity(), OnAddAppChooseSourceListener, OnAddAppServerDetailsListener, OnAddAppChooseApplicationListener {

    private var m_state = 0
    private var m_appSource = -1
    private var m_serverUrl: String? = null
    private var m_applicationPackages: ArrayList<DeploymentPackage?>? = null
    private var m_applicationDownloaderFragment: ApplicationDownloadFragment? = null

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_add_application)
        if (savedInstanceState == null) {
            // First time - start at the first state
            gotoState(CHOOSE_SOURCE)
        } else {
            // Restore state - UI will get restored from fragments
            m_state = savedInstanceState.getInt(STATE_CURRENT_STATE)
            m_appSource = savedInstanceState.getInt(STATE_APP_SOURCE)
            m_serverUrl = savedInstanceState.getString(STATE_SERVER_URL)
            m_applicationPackages = savedInstanceState.getParcelableArrayList(STATE_APPLICATION_PACKAGES)
        }

        // The application package downloader is stashed inside a fragment with setRetainInstance
        // set to true so that it won't get nuked on screen rotation.
        val fragmentManager = supportFragmentManager
        m_applicationDownloaderFragment = fragmentManager.findFragmentByTag(TAG_APPLICATION_DOWNLOAD_FRAGMENT) as ApplicationDownloadFragment?
        if (m_applicationDownloaderFragment == null) {
            // Must be first time, not a config change.
            m_applicationDownloaderFragment = ApplicationDownloadFragment()
            m_applicationDownloaderFragment?.let{
                fragmentManager.beginTransaction().add(it, TAG_APPLICATION_DOWNLOAD_FRAGMENT).commit()
            }
        }
    }

    override fun onSaveInstanceState(outState: Bundle) {
        outState.putInt(STATE_CURRENT_STATE, m_state)
        outState.putInt(STATE_APP_SOURCE, m_appSource)
        outState.putString(STATE_SERVER_URL, m_serverUrl)
        outState.putParcelableArrayList(STATE_APPLICATION_PACKAGES, m_applicationPackages)
        super.onSaveInstanceState(outState)
    }

    override fun onDestroy() {
        if (m_applicationDownloaderFragment?.deploymentPackageDownloader != null) {
            val downloader = m_applicationDownloaderFragment?.deploymentPackageDownloader
            m_applicationDownloaderFragment?.deploymentPackageDownloader = null
            Messenger.getInstance().sendMessage(
                object : EngineMessage() {
                    override fun run() {
                        downloader?.Disconnect()
                    }
                })
        }
        super.onDestroy()
    }

    override fun onSourceSelected(source: Int) {
        m_appSource = source
        when (m_appSource) {
            ChooseApplicationSourceFragment.DROPBOX_SOURCE -> {
                m_serverUrl = "Dropbox"
                downloadApplicationList()
            }
            ChooseApplicationSourceFragment.CSWEB_SOURCE, ChooseApplicationSourceFragment.FTP_SOURCE -> gotoState(SERVER_DETAILS)
            ChooseApplicationSourceFragment.SCAN_SOURCE -> openBarcodeCapture()
            ChooseApplicationSourceFragment.EXAMPLE_SOURCE -> installExampleApplication()
        }
    }

    private fun openBarcodeCapture() {
        //open Camera with QR code();
        val intent = Intent(this, BarcodeCaptureActivity::class.java)
        startActivityForResult(intent, Constants.INTENT_STARTACTIVITY_DEEPLINK)
    }

    override fun onServerDetailsSelected(uri: URI) {
        m_serverUrl = uri.toString()
        downloadApplicationList()
    }

    override fun onApplicationChosen(application: DeploymentPackage) {
        class DownloadApplicationMessage(activity: Activity, completedListener: IEngineMessageCompletedListener) : EngineMessage(activity, completedListener) {
            var succeeded = false
            var applicationPackage: DeploymentPackage? = null
            override fun run() {
                val fullInstall = application.installStatus == DeploymentPackage.UP_TO_DATE
                succeeded = m_applicationDownloaderFragment?.deploymentPackageDownloader?.InstallPackage(application.name, fullInstall) == DeploymentPackageDownloader.resultOk
                if (succeeded) {
                    applicationPackage = application
                    m_applicationDownloaderFragment?.deploymentPackageDownloader?.Disconnect()
                }
            }
        }

        val activity: Activity = this
        val msg: EngineMessage = DownloadApplicationMessage(this,
            IEngineMessageCompletedListener { msg ->
                val downloadMsg = msg as DownloadApplicationMessage
                if (downloadMsg.succeeded) {
                    if (downloadMsg.applicationPackage?.currentInstalledBuildTime == null) {
                        Toast.makeText(activity, String.format(getString(R.string.add_app_install_success), downloadMsg.applicationPackage?.name), Toast.LENGTH_LONG).show()
                    } else {
                        Toast.makeText(activity, String.format(getString(R.string.add_app_update_success), downloadMsg.applicationPackage?.name), Toast.LENGTH_LONG).show()
                    }
                    // App was installed, go back to app listing
                    finish()
                }
            })
        Messenger.getInstance().sendMessage(msg)
    }

    private fun gotoState(newState: Int) {
        // Update the UI fragment for the new state.
        when (newState) {
            CHOOSE_SOURCE -> {
                val transaction = supportFragmentManager.beginTransaction()
                transaction.replace(R.id.step_fragment, ChooseApplicationSourceFragment.newInstance())
                transaction.commit()
                m_state = newState
            }
            SERVER_DETAILS -> {
                val transaction = supportFragmentManager.beginTransaction()
                transaction.replace(R.id.step_fragment, ServerDetailsFragment.newInstance(m_appSource))
                transaction.addToBackStack(TAG_SERVER_DETAILS_FRAGMENT)
                transaction.commit()
                m_state = newState
            }
            CHOOSE_APPLICATION -> {
                val transaction = supportFragmentManager.beginTransaction()
                transaction.replace(R.id.step_fragment, ChooseApplicationFragment.newInstance(m_applicationPackages))
                transaction.addToBackStack(TAG_CHOOSE_APPLICATION_FRAGMENT)
                transaction.commit()
            }
        }
    }

    private fun downloadApplicationList() {
        class DownloadApplicationListMessage(activity: Activity, completedListener: IEngineMessageCompletedListener) : EngineMessage(activity, completedListener) {
            var success = false
            override fun run() {
                if (m_applicationDownloaderFragment?.deploymentPackageDownloader == null) m_applicationDownloaderFragment?.deploymentPackageDownloader = DeploymentPackageDownloader()
                m_applicationPackages = ArrayList()
                success = m_applicationDownloaderFragment?.deploymentPackageDownloader?.ConnectToServer(m_serverUrl) == DeploymentPackageDownloader.resultOk &&
                    m_applicationDownloaderFragment?.deploymentPackageDownloader?.ListPackages(m_applicationPackages ?: ArrayList()) == DeploymentPackageDownloader.resultOk
            }
        }

        val msg: EngineMessage = DownloadApplicationListMessage(this,
            IEngineMessageCompletedListener { msg ->
                val downloadMsg = msg as DownloadApplicationListMessage
                if (downloadMsg.success) {
                    if (m_applicationPackages == null || m_applicationPackages?.size == 0) {
                        val builder = AlertDialog.Builder(this@AddApplicationActivity)
                        builder.setMessage(R.string.add_app_no_apps).setPositiveButton(getString(R.string.modal_dialog_helper_ok_text)
                        ) { _, _ -> }
                        builder.create().show()
                    } else {
                        gotoState(CHOOSE_APPLICATION)
                    }
                }
            })
        Messenger.getInstance().sendMessage(msg)
    }

    override fun onResume() {
        super.onResume()
        if(AuthorizeDropboxFunction.isAuthenticating()){
            AuthorizeDropboxFunction.setAuthenticationComplete()
            var dbxCredential: String? = ""
            val credential = Auth.getDbxCredential()
            if (credential != null) {
                dbxCredential = credential.toString()
            }
            Messenger.getInstance().engineFunctionComplete(dbxCredential)
        }
    }

    @Deprecated("Deprecated in Java")
    override fun onActivityResult(requestCode: Int, resultCode: Int, intent: Intent?) {
        super.onActivityResult(requestCode, resultCode, intent)

        // If activity is returning from Barcode, finish this and take user back to
        if (requestCode == Constants.INTENT_STARTACTIVITY_DEEPLINK) {
            val server: String? = intent?.getStringExtra("xserver")
            val app: String? = intent?.getStringExtra("xapp")
            val cred: String? = intent?.getStringExtra("xcred")

            downloadSurvey(server, app, cred)
        }
    }

    private fun downloadSurvey(server: String?, app:String?, cred:String?) {
        class DownloadApplicationListMessage(
            activity: Activity,
            completedListener: IEngineMessageCompletedListener
        ) : EngineMessage(activity, completedListener) {
            var success = false
            override fun run() {
                if (m_applicationDownloaderFragment?.deploymentPackageDownloader == null) m_applicationDownloaderFragment?.deploymentPackageDownloader =
                    DeploymentPackageDownloader()

                if (cred != null) {
                    success =
                        m_applicationDownloaderFragment?.deploymentPackageDownloader?.ConnectToServerCredentials(
                            server, app, cred
                        ) == DeploymentPackageDownloader.resultOk
                } else {
                    success =
                        m_applicationDownloaderFragment?.deploymentPackageDownloader?.ConnectToServer(
                            server
                        ) == DeploymentPackageDownloader.resultOk
                }

                m_applicationDownloaderFragment?.deploymentPackageDownloader?.InstallPackage(
                    app,
                    false
                )

            }
        }

        val msg: EngineMessage = DownloadApplicationListMessage(this,
            IEngineMessageCompletedListener { msg ->
                val downloadMsg = msg as DownloadApplicationListMessage
                if (downloadMsg.success) {
                    finish()
                }
            })
        Messenger.getInstance().sendMessage(msg)

    }

    private fun installExampleApplication() {
        try {
            val destinationPath = EngineInterface.getInstance().csEntryDirectory.path + "/Simple CAPI"
            val dir = File(destinationPath)
            if (!dir.exists()) dir.mkdir()

            // GHM 20140213 modified from http://stackoverflow.com/questions/4447477/android-how-to-copy-files-in-assets-to-sdcard
            val sourcePath = "examples"
            val assetManager = assets
            val files = assetManager.list(sourcePath)
            if (files != null) {
                for (filename in files) {
                    val `in` = assetManager.open("$sourcePath/$filename")
                    val out: OutputStream = FileOutputStream(File(destinationPath, filename))
                    Util.copyAndCloseStreams(`in`, out)
                }
            }

            // Force rescan so that files show up when connected to PC
            val filesToScan = files?.let { arrayOfNulls<String>(it.size) }
            if (files != null) {
                for (i in files.indices) {
                    filesToScan?.set(i, Util.combinePath(destinationPath, files[i]))
                }
            }
            MediaScannerConnection.scanFile(this, filesToScan, null, null)
            Toast.makeText(this, String.format(getString(R.string.add_app_install_success), getString(R.string.add_app_source_example)),
                Toast.LENGTH_LONG).show()

            // App was installed, go back to app listing
            finish()
        } catch (e: IOException) {
            Toast.makeText(this, e.message, Toast.LENGTH_LONG).show()
        }
    }

    companion object {
        // States for state machine
        private const val CHOOSE_SOURCE = 1
        private const val SERVER_DETAILS = 2
        private const val CHOOSE_APPLICATION = 3

        // For saving instance state
        private const val STATE_CURRENT_STATE = "m_state"
        private const val STATE_APP_SOURCE = "m_appSource"
        private const val STATE_SERVER_URL = "m_serverUrl"
        private const val STATE_APPLICATION_PACKAGES = "m_applicationPackages"

        // Tags for fragments so that we can identify them on back stack
        private const val TAG_APPLICATION_DOWNLOAD_FRAGMENT = "ApplicationDownloadFragment"
        private const val TAG_SERVER_DETAILS_FRAGMENT = "ServerDetailsFragment"
        private const val TAG_CHOOSE_APPLICATION_FRAGMENT = "ChooseApplicationFragment"
    }
}