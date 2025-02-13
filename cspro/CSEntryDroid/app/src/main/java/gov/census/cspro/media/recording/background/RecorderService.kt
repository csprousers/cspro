package gov.census.cspro.media.recording.background

import android.app.*
import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent
import android.os.Build
import android.os.IBinder
import android.widget.RemoteViews
import androidx.core.app.NotificationCompat
import androidx.core.app.NotificationManagerCompat
import androidx.localbroadcastmanager.content.LocalBroadcastManager
import gov.census.cspro.csentry.R
import gov.census.cspro.csentry.ui.EntryActivity
import gov.census.cspro.media.util.AudioState
import gov.census.cspro.util.Constants
import timber.log.Timber


class RecorderService : Service() {

    private lateinit var notificationManager: NotificationManagerCompat
    private lateinit var notification: Notification
    var started = false

    override fun onBind(p0: Intent?): IBinder? {
        return null
    }

    override fun onStartCommand(intent: Intent?, flags: Int, startId: Int): Int {
        intent?.let {
            val action = intent.action
            action?.let {
                when (action) {
                    AudioState.Start.toString() -> {
                        val outputFilePath = intent.getStringExtra(Constants.EXTRA_RECORDING_SERVICE_FILE_URL_KEY)!!
                        val maxRecordingTimeSeconds = intent.extras?.getDouble(Constants.EXTRA_RECORDING_MAX_TIME_SECONDS, -1.0)
                        val maxRecordingTimeMs =
                            if (maxRecordingTimeSeconds != null && maxRecordingTimeSeconds > 0)
                                (maxRecordingTimeSeconds * 1000).toInt()
                            else
                                null

                        val samplingRate = intent.getIntExtra(Constants.EXTRA_RECORDING_SAMPLING_RATE, -1)

                        startForegroundService(outputFilePath, maxRecordingTimeMs, samplingRate)
                    }
                    AudioState.Stopped.toString() -> {
                        BackgroundRecorder.stopRecording()
                        stopForegroundService()
                    }

                    AudioState.Running.toString() -> {
                        BackgroundRecorder.resumeRecording()
                        updateNotificationPlay()
                    }
                }
                LocalBroadcastManager.getInstance(this).sendBroadcast(intent)
            }

        }
        return super.onStartCommand(intent, flags, startId)

    }


    private fun startForegroundService(outputFilePath: String, maxRecordingTimeMs: Int?, samplingRate: Int) {
        if (maxRecordingTimeMs == null)
            BackgroundRecorder.startRecording(outputFilePath, samplingRate, null, null)
        else
            BackgroundRecorder.startRecording(outputFilePath, samplingRate, maxRecordingTimeMs) {
                BackgroundRecorder.stopRecording()
                stopForegroundService()
            }
        initNotification()
    }

    private fun initNotification() {

        notificationManager = NotificationManagerCompat.from(this.applicationContext)
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            val channel = createNotificationChannel()
            channel?.let {
                notificationManager.createNotificationChannel(channel)
            }
        }

        // Create notification builder.
        val builder = NotificationCompat.Builder(
            this,
            "2020"
        )

        builder.setWhen(System.currentTimeMillis())
        builder.setSmallIcon(R.drawable.ic_launcher)
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.N) {
            builder.priority = NotificationManager.IMPORTANCE_MAX
        } else {
            @Suppress("DEPRECATION")
            builder.priority = Notification.PRIORITY_MAX
        }
        builder.setContentIntent(createContentIntent())
        builder.setCustomContentView(getRemoteViews())
        builder.setContentTitle(getString(R.string.app_name))
        builder.setOnlyAlertOnce(true)
        builder.setDefaults(0)
        builder.setSound(null)
//        try {
//            builder?.setSound(RingtoneManager.getDefaultUri(RingtoneManager.TYPE_NOTIFICATION))
//        } catch (e:Exception) {
//            builder?.setSound(null)
//        }
        notification = builder.build()
        startForeground(NOTIF_ID, notification)
        started = true
        updateNotificationPlay()

    }

    private fun createContentIntent(): PendingIntent {
        val intent = Intent(applicationContext, EntryActivity::class.java)
        intent.flags = Intent.FLAG_ACTIVITY_PREVIOUS_IS_TOP
        return PendingIntent.getActivity(applicationContext, 0, intent, PendingIntent.FLAG_UPDATE_CURRENT or PendingIntent.FLAG_IMMUTABLE)
    }

    private fun stopForegroundService() {
        stopForeground(true)
        stopSelf()
        started = false
    }

    private fun updateNotificationPlay() {
        if (started) {
            notificationManager.notify(
                NOTIF_ID,
                notification
            )
        }
    }

    private fun createNotificationChannel(): NotificationChannel? {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            val channel = NotificationChannel(
                "2020",
                getString(R.string.app_name),
                NotificationManager.IMPORTANCE_DEFAULT
            )
            channel.setSound(null, null)
            return channel

        }
        return null
    }

    private fun getRemoteViews(): RemoteViews {
        return RemoteViews(packageName,
            R.layout.layout_record_notification_text
        )
    }

    //To stop  service when application is cleared from the recent task:
    //a. define an attribute stopWithTask for service in the manifest file
    //b. add stopSelf on onTaskRemoved()
    override fun onTaskRemoved(rootIntent: Intent?) {
        Timber.d("onTaskRemoved called")
        super.onTaskRemoved(rootIntent)
        // Stop foreground service and remove the notification.
        this.stopForeground(true)
        // Stop the foreground service.
        this.stopSelf()
    }

    companion object {
        private const val NOTIF_ID = 101

        class ServiceReceiver : BroadcastReceiver() {
            override fun onReceive(
                context: Context,
                intent: Intent
            ) {
                val serviceIntent =
                    Intent(context, RecorderService::class.java)
                serviceIntent.action = intent.action
                context.startService(serviceIntent)
            }
        }
    }
}