package gov.census.cspro.media.recording.background

import android.media.MediaRecorder
import android.os.Build
import gov.census.cspro.media.util.AudioState
import gov.census.cspro.media.util.DependencyInjection
import timber.log.Timber

object BackgroundRecorder {

    private lateinit var mediaRecorder: MediaRecorder
    private var audioState: AudioState = AudioState.Stopped
    private lateinit var outputFilePath: String
    private var samplingRate: Int = -1
    private var maxRecordingTimeMs: Int? = null

    fun startRecording(outputFilePath: String, samplingRate: Int, maxRecordingTimeMs: Int?, timeoutCallback: (()->Unit)?) {

        try {
            this.outputFilePath = outputFilePath
            this.maxRecordingTimeMs = maxRecordingTimeMs
            this.samplingRate = samplingRate

            mediaRecorder = DependencyInjection.provideMediaPlayerObject(samplingRate)
            DependencyInjection.prepareRecorderPath(outputFilePath)
            maxRecordingTimeMs?.let { mediaRecorder.setMaxDuration(it) }
            timeoutCallback?.let { mediaRecorder.setOnInfoListener { _, what, _ ->
                    if (what == MediaRecorder.MEDIA_RECORDER_INFO_MAX_DURATION_REACHED)
                        timeoutCallback()
            } }
            mediaRecorder.prepare()
            mediaRecorder.start()

            audioState = AudioState.Running
            Timber.d("mediaRecorder is started")

        } catch (e: Exception) {
            Timber.e(e)
            audioState = AudioState.Stopped
        }
    }

    fun stopRecording() {

        if (audioState != AudioState.Stopped)
        {
            mediaRecorder.stop()
            mediaRecorder.release()
            audioState = AudioState.Stopped
        }

    }

    fun resumeRecording() {

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.N)
        {
            mediaRecorder.resume()
            audioState = AudioState.Running
        }
        else {
            startRecording(outputFilePath, samplingRate, maxRecordingTimeMs, null)
        }
    }


}