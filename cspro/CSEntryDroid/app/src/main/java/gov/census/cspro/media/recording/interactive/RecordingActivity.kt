package gov.census.cspro.media.recording.interactive

import android.Manifest
import android.app.*
import android.content.Intent
import android.content.res.ColorStateList
import android.os.Build
import android.os.Bundle
import android.view.View
import android.view.View.GONE
import androidx.appcompat.app.AppCompatActivity
import androidx.core.app.NotificationCompat
import androidx.core.app.NotificationManagerCompat
import androidx.core.content.ContextCompat
import androidx.core.widget.ImageViewCompat
import androidx.databinding.DataBindingUtil
import androidx.lifecycle.ViewModelProvider
import com.karumi.dexter.Dexter
import com.karumi.dexter.MultiplePermissionsReport
import com.karumi.dexter.PermissionToken
import com.karumi.dexter.listener.PermissionRequest
import com.karumi.dexter.listener.multi.CompositeMultiplePermissionsListener
import com.karumi.dexter.listener.multi.DialogOnAnyDeniedMultiplePermissionsListener
import com.karumi.dexter.listener.multi.MultiplePermissionsListener
import gov.census.cspro.csentry.R
import gov.census.cspro.csentry.databinding.ActivityAudioRecorderBinding
import gov.census.cspro.media.util.AudioState
import gov.census.cspro.util.Constants
import kotlinx.coroutines.GlobalScope
import kotlinx.coroutines.launch
import timber.log.Timber


class RecordingActivity : AppCompatActivity(), RecorderCallback {

    companion object {
        private const val NOTIFICATION_ID = 2422
    }

    private lateinit var binding: ActivityAudioRecorderBinding
    private lateinit var mAudioFilePath: String
    private lateinit var audioState: AudioState
    private lateinit var recorderViewModel: InteractiveRecorderViewModel
    private lateinit var notificationManager: NotificationManagerCompat
    private var isExit = false

    val permissions = listOf(
        Manifest.permission.RECORD_AUDIO
    )

    //allPermissionsListener is to CompositePermissionListener to compound multiple listeners into one:
    private var allPermissionsListener: MultiplePermissionsListener? = null
    private var systemDialogPermissionsListener: MultiplePermissionsListener? = object : MultiplePermissionsListener {
        override fun onPermissionsChecked(report: MultiplePermissionsReport) {
            binding.btnStartRecording.isEnabled = report.areAllPermissionsGranted() == true
        }

        override fun onPermissionRationaleShouldBeShown(permissions: List<PermissionRequest?>?, token: PermissionToken?) {
            //Dexter will call the method onPermissionRationaleShouldBeShown implemented in your listener with a PermissionToken.
            // It's important to keep in mind that the request process will pause until the token is used,
            // therefore, you won't be able to call Dexter again or request any other permissions if the token has not been used.
            token?.continuePermissionRequest()
        }
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        mAudioFilePath = intent.getStringExtra(Constants.EXTRA_RECORDING_FILE_URL_KEY)!!
        audioState = intent.action?.let { AudioState.valueOf(it) }!!

        binding = DataBindingUtil.setContentView(this, R.layout.activity_audio_recorder)
        binding.lifecycleOwner = this

        recorderViewModel = ViewModelProvider(this)[InteractiveRecorderViewModel::class.java]
        binding.viewModel = recorderViewModel
        recorderViewModel.audioState.value = this.audioState

        initUI()
        addObserver()
        initNotification()
    }

    private fun initUI() {
        val dialogMultiplePermissionsListener: MultiplePermissionsListener =
            DialogOnAnyDeniedMultiplePermissionsListener.Builder
                .withContext(this)
                .withTitle(getString(R.string.modal_dialog_helper_audio_storage_title))
                .withMessage(getString(R.string.modal_dialog_helper_audio_storage_text))
                .withButtonText(getString(R.string.modal_dialog_helper_ok_text))
                .build()
        allPermissionsListener = CompositeMultiplePermissionsListener(systemDialogPermissionsListener, dialogMultiplePermissionsListener)

        Dexter.withContext(this)
            .withPermissions(permissions)
            .withListener(allPermissionsListener)
            .withErrorListener { error -> Timber.e("There was an permission error: $error") }
            .check()

        val title = intent.getStringExtra(Constants.EXTRA_USER_MESSAGE_KEY)
        if (title.isNullOrBlank()) {
            binding.titleTextview.visibility = GONE
        }
        else {
            binding.titleTextview.text = title
        }

        recorderViewModel.outputFilePath.value = mAudioFilePath
        recorderViewModel.samplingRate = intent.getIntExtra(Constants.EXTRA_RECORDING_SAMPLING_RATE, -1)

        binding.btnStartRecording.setOnClickListener { btnRecordingClick() }
        if (Build.VERSION.SDK_INT > Build.VERSION_CODES.M) {
            binding.btnStop.visibility = View.VISIBLE
            binding.btnStop.setOnClickListener { stopRecording() }
        }

        //clear button only not visible when started only else always visible
        binding.btnClear.isEnabled = recorderViewModel.audioState.value != AudioState.Start
        binding.btnClear.setOnClickListener { clearRecordingDialog() }

        recorderViewModel.callback = this

    }

    private fun initNotification() {
        notificationManager = NotificationManagerCompat.from(this)
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            val channel = NotificationChannel(
                "2020",
                getString(R.string.app_name),
                NotificationManager.IMPORTANCE_DEFAULT
            )
            channel.setSound(null, null)
            notificationManager.createNotificationChannel(channel)
        }
    }

    private fun createNotification(): Notification {
        // Create notification builder.
        val builder = NotificationCompat.Builder(
            this,
            "2020"
        )
        builder.setSmallIcon(R.drawable.ic_launcher)
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.N) {
            builder.priority = NotificationManager.IMPORTANCE_MAX
        } else {
            @Suppress("DEPRECATION")
            builder.priority = Notification.PRIORITY_MAX
        }

        val intent = Intent(applicationContext, RecordingActivity::class.java)
        intent.flags = Intent.FLAG_ACTIVITY_PREVIOUS_IS_TOP
        val pendingIntent = PendingIntent.getActivity(this, 0, intent, PendingIntent.FLAG_UPDATE_CURRENT or PendingIntent.FLAG_IMMUTABLE)
        builder.setContentIntent(pendingIntent)
        builder.setContentTitle(getString(R.string.app_name))
        builder.setOnlyAlertOnce(true)
        builder.setDefaults(0)
        builder.setSound(null)
        return builder.build()
    }


    private fun addObserver() {

        recorderViewModel.audioState.observe(this) {
            //Timber.e( "audioState changed -> $it")
            if (it == AudioState.Running) {
                notificationManager.notify(NOTIFICATION_ID, createNotification())
                if (Build.VERSION.SDK_INT > Build.VERSION_CODES.M) {
                    binding.btnStartRecording.setImageResource(R.drawable.record_pause)
                } else {
                    binding.btnStartRecording.setImageResource(R.drawable.save)
                    ImageViewCompat.setImageTintList(binding.btnStartRecording, ColorStateList.valueOf(ContextCompat.getColor(this, R.color.cspro_green)))
                }

                binding.btnClear.isEnabled = true
            } else if (it == AudioState.Start) {
                notificationManager.cancelAll()
                if (isExit) {
                    exit()
                }
            } else if (it == AudioState.Stopped || it == AudioState.Cleared) {
                notificationManager.cancelAll()
                when {
                    isExit -> {
                        exit()
                    }
                    it == AudioState.Stopped -> {
                        exit()
                    }
                    it == AudioState.Cleared -> {
                        binding.btnClear.isEnabled = false
                        binding.btnStartRecording.setImageResource(R.drawable.record_mic)
                        ImageViewCompat.setImageTintList(binding.btnStartRecording,
                            ColorStateList.valueOf(ContextCompat.getColor(this, R.color.colorAccent)))
                    }
                }
            } else if (it == AudioState.Paused) {
                binding.btnStartRecording.setImageResource(R.drawable.record_mic)
                ImageViewCompat.setImageTintList(binding.btnStartRecording,
                    ColorStateList.valueOf(ContextCompat.getColor(this, R.color.colorAccent)))
                binding.btnClear.isEnabled = true
            }
        }
    }

    private fun btnRecordingClick() {

        when (recorderViewModel.audioState.value) {
            AudioState.Start, AudioState.Cleared, AudioState.Stopped -> {
                startRecording()
            }
            AudioState.Paused -> {
                resumeRecording()
            }
            AudioState.Running -> {
                if (Build.VERSION.SDK_INT > Build.VERSION_CODES.M) {
                    pauseRecording()
                } else {
                    stopRecording()
                }

            }
            else -> {}
        }

    }

    private fun startRecording() {

        //testing
        /*
        val mediaRecorder = MediaRecorder()
        if(mediaRecorder==null){
            Log.e(javaClass.simpleName, "Test mediarecorder null")
        }else{
            Log.e(javaClass.simpleName, "Test mediarecorder not null")

        }
        */

        //this will be the problem if state is set here
        //recorderViewModel.audioState.postValue(AudioState.Running)

        Thread {
            kotlin.run {
                recorderViewModel.startRecording()
            }
        }.start()

    }

    private fun stopRecording() {
        if(recorderViewModel.audioState.value == AudioState.Start
            || recorderViewModel.audioState.value == AudioState.Cleared
            || recorderViewModel.audioState.value == AudioState.Stopped)
        {
            return
        }
        GlobalScope.launch {
            recorderViewModel.stopRecording()
        }

    }

    private fun pauseRecording() {

        recorderViewModel.pauseRecording()

    }

    private fun resumeRecording() {

        recorderViewModel.resumeRecording()

        binding.btnStartRecording.setImageResource(R.drawable.record_pause)
        ImageViewCompat.setImageTintList(binding.btnStartRecording,
            ColorStateList.valueOf(ContextCompat.getColor(this, R.color.dark)))
        binding.btnClear.isEnabled = true

    }

    private fun clearRecording() {
        recorderViewModel.clearRecording()
        resetVisualizer()

    }

    private fun clearRecordingDialog() {

        if(recorderViewModel.audioState.value == AudioState.Start
            || recorderViewModel.audioState.value == AudioState.Cleared)
        {
            return
        }

        // build alert dialog
        val dialogBuilder = AlertDialog.Builder(this)

        // set message of alert dialog
        dialogBuilder.setMessage(getString(R.string.dialog_clear_recording_title))

            // if the dialog is cancelable
            .setCancelable(true)

            // positive button text and action
            .setPositiveButton(getString(R.string.modal_dialog_helper_ok_text)) { _, _ ->
                clearRecording()
            }

            // negative button text and action
            .setNegativeButton(getString(R.string.modal_dialog_helper_cancel_text)) { dialog, _ ->
                dialog.cancel()
            }

        // create dialog box
        val alert = dialogBuilder.create()
        // show alert dialog
        alert.show()
    }


    override fun onAmplitudeChanged(maxAmplitude: Int) {
        runOnUiThread {
            binding.visualizer.update(maxAmplitude)
        }
    }

    private fun resetVisualizer() {
        runOnUiThread {
            binding.visualizer.recreate()
        }
    }

    private fun stopRecordingDialog() {
        // build alert dialog
        val dialogBuilder = AlertDialog.Builder(this)

        // set message of alert dialog
        dialogBuilder.setMessage(getString(R.string.dialog_save_recording_title))

            // if the dialog is cancelable
            .setCancelable(true)

            .setPositiveButton(getString(R.string.save)) { _, _ ->
                isExit = true
                stopRecording()
                recorderViewModel.stopTimer()
            }

            // negative button text and action to clear
            .setNegativeButton(getString(R.string.modal_dialog_helper_cancel_text)) { _, _ ->
                isExit = true
                clearRecording()
            }

        // create dialog box
        val alert = dialogBuilder.create()
        // show alert dialog
        alert.show()
    }

    override fun onBackPressed() {
        when (recorderViewModel.audioState.value) {
            AudioState.Running, AudioState.Paused -> {
                stopRecordingDialog()
            }

            AudioState.Start, AudioState.Stopped, AudioState.Cleared -> {
                super.onBackPressed()
            }
            else -> {}
        }

    }

    override fun onDestroy() {
        notificationManager.cancel(NOTIFICATION_ID)
        super.onDestroy()
    }

    private fun exit() {
        if (recorderViewModel.hasRecording())
            setResult(Activity.RESULT_OK, Intent().apply { putExtra(Constants.EXTRA_RECORDING_FILE_URL_KEY, mAudioFilePath) })
        else
            setResult(Activity.RESULT_CANCELED)
        finish()
    }
}
