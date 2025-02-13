@file:JvmName("ApplicationInterface")

package gov.census.cspro.engine

import android.Manifest
import android.annotation.SuppressLint
import android.app.Activity
import android.content.Context
import android.content.Intent
import android.media.MediaScannerConnection
import android.net.ConnectivityManager
import android.net.NetworkInfo
import android.net.Uri
import android.os.Bundle
import android.provider.Settings
import android.webkit.MimeTypeMap
import androidx.appcompat.app.AppCompatActivity
import androidx.lifecycle.LiveData
import androidx.lifecycle.MutableLiveData
import com.google.android.gms.maps.model.LatLng
import com.karumi.dexter.Dexter
import com.karumi.dexter.MultiplePermissionsReport
import com.karumi.dexter.PermissionToken
import com.karumi.dexter.listener.PermissionRequest
import com.karumi.dexter.listener.multi.CompositeMultiplePermissionsListener
import com.karumi.dexter.listener.multi.DialogOnAnyDeniedMultiplePermissionsListener
import com.karumi.dexter.listener.multi.MultiplePermissionsListener
import gov.census.cspro.camera.PictureCaptureActivity
import gov.census.cspro.csentry.CSEntry
import gov.census.cspro.csentry.R
import gov.census.cspro.csentry.ui.DialogWebViewFragment
import gov.census.cspro.csentry.ui.FieldNoteUpdateListener
import gov.census.cspro.csentry.ui.WebViewWithJavaScriptInterfaceActivity
import gov.census.cspro.engine.functions.*
import gov.census.cspro.maps.CapturePolygonActivity
import gov.census.cspro.maps.MapUI
import gov.census.cspro.maps.geojson.Polygon
import gov.census.cspro.media.player.AudioPlayerActivity
import gov.census.cspro.media.recording.background.BackgroundRecorder
import gov.census.cspro.media.recording.background.RecorderService
import gov.census.cspro.media.recording.interactive.RecordingActivity
import gov.census.cspro.media.util.AudioState
import gov.census.cspro.signature.SignatureActivity
import gov.census.cspro.util.Constants
import gov.census.cspro.util.Media
import timber.log.Timber
import java.io.File
import java.io.FileOutputStream
import java.io.IOException
import java.util.*

fun refreshNotes() {
    val activity = Messenger.getInstance().currentMessage.activity
    if (activity is FieldNoteUpdateListener) {
        (activity as FieldNoteUpdateListener).noteStateChanged()
    }
}

fun displayCSHtmlDlg(htmlFilename: String, actionInvokerAccessTokenOverride: String?): String? {
    return Messenger.getInstance().runStringEngineFunction(DisplayCSHtmlDlgFunction(htmlFilename, actionInvokerAccessTokenOverride))
}

fun displayHtmlDialogFunctionDlg(url: String, actionInvokerAccessTokenOverride: String?, displayOptionsJson: String?): Long {
    val threadWaitId = EngineInterface.getInstance().threadWaitId

    val engineFunction = EngineFunction { activity ->
        activity?.run {
            val dlgWebViewFragment = DialogWebViewFragment.newInstance(true, url, actionInvokerAccessTokenOverride, threadWaitId, displayOptionsJson)

            dlgWebViewFragment.show(
                (activity as AppCompatActivity).supportFragmentManager,
                "ModalDialogFragment"
            )
        }
    }

    // instead of using the messenger as typically used, this function will immediately return
    // and code in the JNI layer will wait for the activity to close; with this approach,
    // JavaScript interactions can call into CSPro logic that uses the messenger without
    // being blocked by the messenger waiting for this function to end
    Messenger.getInstance().runEngineFunctionDirectlyWithoutUsingMessengerQueue(engineFunction)
    return threadWaitId
}

fun showModalDialog(title: String?, message: String?, type: Int): Int {
    return Messenger.getInstance().runLongEngineFunction(ModalDialogFunction(title, message, type)).toInt()
}

fun exexecsystem(command: String, wait: Boolean): Long {
    return Messenger.getInstance().runLongEngineFunction(ExecSystemFunction(command, true))
}

fun choiceDialog(title: String?, list: Array<String?>?): Long {
    return Messenger.getInstance().runLongEngineFunction(ChoiceDialog(title, list))
}

fun exerrmsg(title: String?, message: String?, buttons: Array<String?>?): Long {
    return Messenger.getInstance().runLongEngineFunction(ErrmsgFunction(title, message, buttons))
}

fun exeditnote(note: String?, title: String?, caseNote: Boolean): String? {
    return Messenger.getInstance().runStringEngineFunction(EditNoteFunction(note, title, caseNote))
}

fun gpsRead(waitTime: Int, desiredAccuracy: Int, dialogText: String?): String? {
    return Messenger.getInstance().runStringEngineFunction(GPSFunction(GPSFunction.GPS_READ,
		waitTime, desiredAccuracy, dialogText, null))
}

fun gpsReadLast(): String? {
    return Messenger.getInstance().runStringEngineFunction(GPSFunction(GPSFunction.GPS_READLAST,
		0, 0, null, null))
}

fun gpsReadInteractive(readInteractiveMode: Boolean, baseMapSelection: BaseMapSelection, message: String, readDuration: Double): String? {
    return Messenger.getInstance().runStringEngineFunction(
        GPSFunction(
            if (readInteractiveMode) GPSFunction.GPS_READINTERACTIVE else GPSFunction.GPS_SELECT,
            readDuration.toInt() * 1000, 0, message, baseMapSelection
        )
    )
}

fun gpsOpen(): Boolean {
    return Messenger.getInstance().runStringEngineFunction(GPSFunction(GPSFunction.GPS_OPEN,
        0, 0, null, null)) == "1"
}

fun gpsClose(): Boolean {
    return Messenger.getInstance().runStringEngineFunction(GPSFunction(GPSFunction.GPS_CLOSE,
        0, 0, null, null)) == "1"
}

fun exuserbar(visibility: Boolean, button_texts: Array<String?>) {
    val userbarHandler = EngineInterface.getInstance().userbarHandler
    userbarHandler.setParameters(visibility, button_texts)
    Messenger.getInstance().runObjectEngineFunction(userbarHandler)
}

fun exshow(headers: Array<String?>?, rowTextColors: IntArray?, lines: Array<String?>?, headingText: String?): Long {
    return Messenger.getInstance().runLongEngineFunction(ShowListFunction(headers, rowTextColors, lines, headingText))
}

fun exselcase(headers: Array<String?>?, lines: Array<String?>?, headingText: String?, multipleSelection: Boolean): String? {
    return Messenger.getInstance().runStringEngineFunction(ShowListFunction(headers, lines, headingText, multipleSelection))
}

fun execPff(pffFilename: String?): Boolean {
    EngineInterface.setExecPffParameter(pffFilename)
    return true
}

@SuppressLint("HardwareIds")
fun exgetdeviceid(): String {
    val activity = Messenger.getInstance().currentMessage.activity
    return Settings.Secure.getString(activity.contentResolver, Settings.Secure.ANDROID_ID).lowercase(Locale.ENGLISH)
}

private fun isConnected(connMgr: ConnectivityManager, type: Int): Boolean {
	val networks = connMgr.allNetworks
	var networkInfo: NetworkInfo?
	for (mNetwork in networks) {
		networkInfo = connMgr.getNetworkInfo(mNetwork)
		if (networkInfo != null && networkInfo.type == type && networkInfo.isConnected) {
			return true
		}
	}
	return false
}

fun getMaxDisplaySize(width: Boolean): Int {
    val xMargin = 24
    val yMargin = 64
    val resources = Messenger.getInstance().currentMessage.activity.resources

    return if (width) {
        (resources.displayMetrics.widthPixels / resources.displayMetrics.density).toInt() - xMargin
    } else {
        (resources.displayMetrics.heightPixels  / resources.displayMetrics.density).toInt() - yMargin
    }
}

fun getMediaFilenames(mediaType: Int): Any {
	return Media.getMediaFilenames(mediaType, Messenger.getInstance().currentMessage.activity)
}

fun isNetworkConnected(connectionType: Int): Boolean {
    // the following constants come from engine/defines.h
    val CONNECTION_ANY = -0x1
    val CONNECTION_MOBILE = 0x00000001
    val CONNECTION_WIFI = 0x00000002
    val activity = Messenger.getInstance().currentMessage.activity
    val connectivityManager = activity.getSystemService(Context.CONNECTIVITY_SERVICE) as ConnectivityManager
    if ((connectionType == CONNECTION_ANY || connectionType == CONNECTION_MOBILE) &&
        isConnected(connectivityManager, ConnectivityManager.TYPE_MOBILE)) return true
    if ((connectionType == CONNECTION_ANY || connectionType == CONNECTION_WIFI) &&
        isConnected(connectivityManager, ConnectivityManager.TYPE_WIFI)) return true
    return false
}

fun exprompt(title: String?, initialValue: String?, numeric: Boolean, password: Boolean, upperCase: Boolean, multiline: Boolean): String? {
    return Messenger.getInstance().runStringEngineFunction(PromptFunction(title, initialValue, numeric, password, upperCase, multiline))
}

fun getProperty(parameter: String?): String? {
    return Messenger.getInstance().runStringEngineFunction(PropertyFunction(true, parameter, null))
}

fun setProperty(parameter: String?, value: String?) {
    Messenger.getInstance().runStringEngineFunction(PropertyFunction(false, parameter, value))
}

fun showProgressDialog(dialogText: String?) {
    Messenger.getInstance().runLongEngineFunction(ProgressDialogFunction(ProgressDialogFunction.COMMAND_SHOW,
        ProgressDialogFunction.PROGRESS_INDETERMINATE, dialogText))
}

fun hideProgressDialog() {
    Messenger.getInstance().runLongEngineFunction(ProgressDialogFunction(ProgressDialogFunction.COMMAND_HIDE))
}

data class ProgressUpdate(val progress: Int, val dialogText: String?)

private val m_progressUpdateLiveData = MutableLiveData<ProgressUpdate>()

val progressUpdateLiveData: LiveData<ProgressUpdate>
    get() = m_progressUpdateLiveData

fun updateProgressDialog(progress: Int, dialogText: String?) {
    Timber.d("Progress %d", progress)
    m_progressUpdateLiveData.postValue(ProgressUpdate(progress, dialogText))
}

fun chooseBluetoothDevice(): String? {
    return Messenger.getInstance().runStringEngineFunction(ChooseBluetoothDeviceFunction())
}

fun authorizeDropbox(): String? {
    return Messenger.getInstance().runStringEngineFunction(AuthorizeDropboxFunction())
}

fun loginDialog(server: String, showInvalidLoginError: Boolean): String? {
    return Messenger.getInstance().runStringEngineFunction(LoginDialogFunction(server, showInvalidLoginError))
}

fun storeCredential(attribute: String, secret_value: String?) {
    EngineInterface.getInstance().credentialStore.Store(attribute, secret_value)
}

fun retrieveCredential(attribute: String): String? {
    return EngineInterface.getInstance().credentialStore.Retrieve(attribute)
}

fun paradataDriverManager(queryType: Int, extra: Int): Any? {
    val applicationInstance = EngineInterface.getInstance()
    val activity = Messenger.getInstance().currentMessage.activity
    if (queryType == 0) { // closing the paradata log
        val paradataDriver = applicationInstance.paradataDriver
        paradataDriver?.stopGpsLocationUpdates()
        applicationInstance.paradataDriver = null
    } else if (queryType == 1) { // opening the paradata log
        applicationInstance.paradataDriver = ParadataDriver()
    } else if (queryType == 2) { // getting the cached events
        val paradataDriver = applicationInstance?.paradataDriver
        return paradataDriver?.cachedEvents
    } else if (queryType == 3) { // setting the background reading parameter
        val paradataDriver = applicationInstance.paradataDriver
        if (extra > 0) {
            paradataDriver.startGpsLocationRequest(extra, activity)
        } else {
            // Zero minutes means turn off background reading
            paradataDriver.stopGpsLocationUpdates()
        }
    }
    return null
}

fun paradataDeviceQuery(queryType: Int, values: Array<String>) {
    val activity = Messenger.getInstance().currentMessage.activity
    if (queryType == 1) ParadataDeviceQueryRunner.DeviceInfoQuery(activity, values) else if (queryType == 2) ParadataDeviceQueryRunner.ApplicationInstanceQuery(activity, values) else if (queryType == 3) ParadataDeviceQueryRunner.DeviceStateQuery(activity, values)
}

val localeLanguage: String
    get() {
        val activity = Messenger.getInstance().currentMessage.activity
        return Util.getLocaleLanguage(activity.resources)
    }

fun viewFile(path: String) {
    startActivityFromEngine { activity ->
        val intent = Intent(Intent.ACTION_VIEW)
		val extension = path.substring(path.lastIndexOf('.') + 1)
		val mimeType = MimeTypeMap.getSingleton().getMimeTypeFromExtension(extension)
		intent.setDataAndType(Util.getShareableUriForFile(File(path), activity), mimeType)
		intent.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION)
        intent
    }
}

fun viewWebPageWithJavaScriptInterface(title: String?, url: String, actionInvokerAccessTokenOverride: String?): Long {
    val threadWaitId = EngineInterface.getInstance().threadWaitId

    val engineFunction = EngineFunction { activity ->
        activity?.run {
            val intent = Intent(activity, WebViewWithJavaScriptInterfaceActivity::class.java)
            intent.putExtra(WebViewWithJavaScriptInterfaceActivity.TITLE, title)
            intent.putExtra(WebViewWithJavaScriptInterfaceActivity.URL, url)
            intent.putExtra(WebViewWithJavaScriptInterfaceActivity.ACCESS_TOKEN_OVERRIDE, actionInvokerAccessTokenOverride)
            intent.putExtra(WebViewWithJavaScriptInterfaceActivity.THREAD_WAIT_ID, threadWaitId)
            activity.startActivity(intent)
        }
    }

    // instead of using the messenger as typically used, this function will immediately return
    // and code in the JNI layer will wait for the activity to close; with this approach,
    // JavaScript interactions can call into CSPro logic that uses the messenger without
    // being blocked by the messenger waiting for this function to end
    Messenger.getInstance().runEngineFunctionDirectlyWithoutUsingMessengerQueue(engineFunction)
    return threadWaitId
}


fun mediaScanFiles(filePaths: Array<String>) {
    val activity = Messenger.getInstance().currentMessage.activity
    MediaScannerConnection.scanFile(activity, filePaths, null, null)
}

fun createSharableUri(path: String, addWritePermission: Boolean): String {
    val activity = Messenger.getInstance().currentMessage.activity

    val uri = Util.getShareableUriForFile(File(path), activity)

    // grant the calling package permissions to use the URI
    activity.callingPackage?.let {
        activity.grantUriPermission(it, uri,
            Intent.FLAG_GRANT_READ_URI_PERMISSION or
            ( if( addWritePermission ) Intent.FLAG_GRANT_WRITE_URI_PERMISSION else 0 ))
    }

    return uri.toString()
}

fun fileCopySharableUri(sharableUri: String, destinationPath: String) {
    val activity = Messenger.getInstance().currentMessage.activity
    val uri = Uri.parse(sharableUri)
    val inputStream = activity.contentResolver.openInputStream(uri)
        ?: throw IOException("Failed to open content URI '$uri'")
    inputStream.use {
        FileOutputStream(destinationPath, false).use {
            inputStream.copyTo(it)
        }
    }
}

fun getPassword(title: String, description: String, hideReenter: Boolean): String? {
    return Messenger.getInstance().runStringEngineFunction(PasswordQueryFunction(title, description, hideReenter))
}

fun barcodeRead(message: String?): String? {
    return Messenger.getInstance().runStringEngineFunction(BarcodeReadFunction(message))
}

fun runSystemApp(packageName: String, activityName: String?, arguments: Bundle): Bundle? {
    return Messenger.getInstance().runBundleEngineFunction(SystemAppEngineFunction(packageName, activityName, arguments))
}

fun audioPlay(filename: String, message: String?): Boolean {
    return startActivityFromEngine { activity ->
        val intent = Intent(activity, AudioPlayerActivity::class.java)
        intent.putExtra(Constants.EXTRA_RECORDING_FILE_URL_KEY, filename)
        intent.putExtra(Constants.EXTRA_USER_MESSAGE_KEY, message)
        intent.action = AudioState.Start.toString()
        intent
    } != 0L
}

fun audioRecordInteractive(filename: String, message: String?, samplingRate: Int): Boolean {
    return startActivityFromEngine { activity ->
        val intent = Intent(activity, RecordingActivity::class.java)
        intent.putExtra(Constants.EXTRA_USER_MESSAGE_KEY, message)
        intent.putExtra(Constants.EXTRA_RECORDING_FILE_URL_KEY, filename)
        intent.putExtra(Constants.EXTRA_RECORDING_SAMPLING_RATE, samplingRate)
        intent.action = AudioState.Start.toString()
        intent
    } != 0L
}

fun audioStartRecording(filename: String, seconds: Double, samplingRate: Int): Boolean {
    return Messenger.getInstance().runLongEngineFunction { activity ->
        try {

            val systemDialogPermissionsListener: MultiplePermissionsListener = object : MultiplePermissionsListener {
                override fun onPermissionsChecked(report: MultiplePermissionsReport) {
                    if (report.areAllPermissionsGranted()) {
                        val intent = Intent(activity, RecorderService::class.java)
                        intent.action = AudioState.Start.toString()
                        intent.putExtra(Constants.EXTRA_RECORDING_SERVICE_FILE_URL_KEY, filename)
                        intent.putExtra(Constants.EXTRA_RECORDING_MAX_TIME_SECONDS, seconds)
                        intent.putExtra(Constants.EXTRA_RECORDING_SAMPLING_RATE, samplingRate)
                        activity.startService(intent)
                        Messenger.getInstance().engineFunctionComplete(1)
                    }
                }

                override fun onPermissionRationaleShouldBeShown(permissions: List<PermissionRequest?>?, token: PermissionToken?) {
                    //Dexter will call the method onPermissionRationaleShouldBeShown implemented in your listener with a PermissionToken.
                    // It's important to keep in mind that the request process will pause until the token is used,
                    // therefore, you won't be able to call Dexter again or request any other permissions if the token has not been used.
                    token?.continuePermissionRequest()
                }
            }

            checkRecordingRelatedPermission(systemDialogPermissionsListener)

        } catch (e: Throwable) {
            Timber.e(e, "Error starting background recording service")
            Messenger.getInstance().engineFunctionComplete(0)
        }
    } != 0L
}

fun audioStopRecording(): Boolean {
    return Messenger.getInstance().runLongEngineFunction { activity ->
        try {

            // Make a synchronous call to stop recording so that we guarantee recorder
            // has released file before returning to C++ to avoid corrupt audio files
            BackgroundRecorder.stopRecording()

            // Stop the service
            val intent = Intent(activity, RecorderService::class.java)
            intent.action = AudioState.Stopped.toString()
            activity.startService(intent)

            Messenger.getInstance().engineFunctionComplete(1)
        } catch (e: Throwable) {
            Timber.e(e, "Error stopping background recording service")
            Messenger.getInstance().engineFunctionComplete(0)
        }
    } != 0L
}

fun takePhoto(overlayMessage: String?): String? {
    return Messenger.getInstance().runStringEngineFunction { activity ->
        val intent = Intent(activity, PictureCaptureActivity::class.java)
        overlayMessage?.let { intent.putExtra(Constants.EXTRA_USER_MESSAGE_KEY, it) }
        try {
            Messenger.getInstance().startActivityForResultFromEngineFunction(activity,
                { result ->
                    Messenger.getInstance().engineFunctionComplete(
                        if (result.resultCode == Activity.RESULT_OK)
                            result.data?.getStringExtra(Constants.EXTRA_CAPTURE_IMAGE_FILE_URL_KEY)
                        else null)
                },
                intent)
        } catch (e: Throwable) {
            Timber.e(e, "Error launching activity from engine thread $intent")
            Messenger.getInstance().engineFunctionComplete(0)
        }
    }
}

fun captureSignature(overlayMessage: String?): String? {
    return Messenger.getInstance().runStringEngineFunction { activity ->
        val intent = Intent(activity, SignatureActivity::class.java)
        overlayMessage?.let { intent.putExtra(Constants.EXTRA_SIGNATURE_MESSAGE_KEY, it) }
        try {
            Messenger.getInstance().startActivityForResultFromEngineFunction(activity,
                { result ->
                    Messenger.getInstance().engineFunctionComplete(
                        if (result.resultCode == Activity.RESULT_OK)
                            result.data?.getStringExtra(Constants.EXTRA_SIGNATURE_FILE_URL_KEY)
                        else null)
                },
                intent)
        } catch (e: Throwable) {
            Timber.e(e, "Error launching activity from engine thread $intent")
            Messenger.getInstance().engineFunctionComplete(0)
        }
    }
}

fun tracePolygon(existingPolygon: Polygon?, map: MapUI?): List<LatLng>? {
    return Messenger.getInstance().runObjectEngineFunction(CapturePolygonMapFunction(CapturePolygonActivity.PolygonCaptureMode.TRACE, existingPolygon, map?.mapData)) as List<LatLng>?
}

fun walkPolygon(existingPolygon: Polygon?, map: MapUI?): List<LatLng>? {
    return Messenger.getInstance().runObjectEngineFunction(CapturePolygonMapFunction(CapturePolygonActivity.PolygonCaptureMode.WALK, existingPolygon, map?.mapData)) as List<LatLng>?
}

fun clipboardGetText(): String? {
    return Messenger.getInstance().runObjectEngineFunction { activity ->
        val clipboardManager = activity.getSystemService(Context.CLIPBOARD_SERVICE) as android.content.ClipboardManager
        val item = clipboardManager.primaryClip?.getItemAt(0)
        val text: String? = item?.coerceToText(activity)?.toString()
        Messenger.getInstance().engineFunctionComplete(text)
    } as String?
}

fun clipboardPutText(text: String) {
    val activity = Messenger.getInstance().currentMessage.activity
    val clipboardManager = activity.getSystemService(Context.CLIPBOARD_SERVICE) as android.content.ClipboardManager
    clipboardManager.setPrimaryClip(android.content.ClipData.newPlainText("", text))
}

fun showSelectDocumentDialog(mimeTypes: Array<String>, multiple: Boolean): Array<String>? {
    return Messenger.getInstance().runObjectEngineFunction(SelectDocumentDialog(mimeTypes, multiple)) as Array<String>?
}

private fun startActivityFromEngine(intentBuilder: (Activity) -> Intent): Long {
    return Messenger.getInstance().runLongEngineFunction { activity ->
        val intent = intentBuilder(activity)
        try {
            Messenger.getInstance().startActivityForResultFromEngineFunction(activity,
                { result -> Messenger.getInstance().engineFunctionComplete(if (result.resultCode == Activity.RESULT_OK) 1 else 0) },
                intent)
        } catch (e: Throwable) {
            Timber.e(e, "Error launching activity from engine thread $intent")
            Messenger.getInstance().engineFunctionComplete(0)
        }
    }
}


private val permissionRecorder = listOf(
    Manifest.permission.RECORD_AUDIO
)

//allPermissionsListener is to CompositePermissionListener to compound multiple listeners into one:
private var allPermissionsListener: MultiplePermissionsListener? = null
fun checkRecordingRelatedPermission(systemDialogPermissionsListener: MultiplePermissionsListener) {

    val context: Context = CSEntry.context
    val dialogMultiplePermissionsListener: MultiplePermissionsListener =
        DialogOnAnyDeniedMultiplePermissionsListener.Builder
            .withContext(context)
            .withTitle(context.getString(R.string.modal_dialog_helper_audio_storage_title))
            .withMessage(context.getString(R.string.modal_dialog_helper_audio_storage_text))
            .withButtonText(context.getString(R.string.modal_dialog_helper_ok_text))
            .build()
    allPermissionsListener = CompositeMultiplePermissionsListener(systemDialogPermissionsListener, dialogMultiplePermissionsListener)

    Dexter.withContext(CSEntry.context)
        .withPermissions(permissionRecorder)
        .withListener(allPermissionsListener)
        .withErrorListener { error -> Timber.e("There was an permission error: $error") }
        .check()
}