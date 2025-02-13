package gov.census.cspro.media.recording.interactive

import android.media.MediaRecorder
import android.os.Build
import androidx.lifecycle.MutableLiveData
import androidx.lifecycle.ViewModel
import gov.census.cspro.media.util.AudioState
import gov.census.cspro.media.util.DependencyInjection
import timber.log.Timber
import java.io.File
import java.util.*

interface RecorderCallback {
    fun onAmplitudeChanged(maxAmplitude: Int)
}

class InteractiveRecorderViewModel : ViewModel() {

    var outputFilePath: MutableLiveData<String> = MutableLiveData("")
    var samplingRate: Int = -1
    private var mediaRecorder: MediaRecorder? = null
    private var recordingTime: Long = 0
    private var timer = Timer()
    private var recorderVisualizerTimer = Timer()
    val recordingTimeString = MutableLiveData<String>()
    var callback: RecorderCallback? = null
    var audioState: MutableLiveData<AudioState> = MutableLiveData(AudioState.Stopped)

    fun hasRecording() = recordingTime > 0

    fun startRecording() {

        if(audioState.value == AudioState.Stopped ) {
            restartTimer()
        }

        try {
            mediaRecorder?.release()
            mediaRecorder?.reset()
        } catch (e: java.lang.Exception) {
            Timber.e("while releasing mediaRecorder in startRecording()%s", e)
        }

        try {

            mediaRecorder = DependencyInjection.provideMediaPlayerObject(samplingRate)
            DependencyInjection.prepareRecorderPath(outputFilePath.value!!)

            mediaRecorder?.prepare()

            mediaRecorder?.start()

            timer = Timer()
            startTimer()

            //Set state here so that at this point 100% it's started successfully
            audioState.postValue(AudioState.Running)
            Timber.d("mediaRecorder is started")

        } catch (e: Exception) {
            Timber.e(e)
            audioState.postValue(AudioState.Stopped)
        }


    }

    fun stopRecording() {

        if (audioState.value == AudioState.Running || audioState.value == AudioState.Paused) {
            mediaRecorder?.stop()
            mediaRecorder = null
            stopTimer()
            audioState.postValue(AudioState.Stopped)
        }
    }

    fun pauseRecording() {

        stopTimer()

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.N)
        {
            mediaRecorder?.pause()
            audioState.postValue(AudioState.Paused)
        } else {
            stopRecording()
        }
    }

    fun resumeRecording() {
        timer = Timer()
        startTimer()

        Thread {
            kotlin.run {

                if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.N)
                {
                    mediaRecorder?.resume()
                }
                else {
                    startRecording()
                }

                audioState.postValue(AudioState.Running)

            }
        }.start()
    }

    fun clearRecording() {

        if (audioState.value != AudioState.Cleared)
        {
            try {
                //Restarts the MediaRecorder to its idle state.
                mediaRecorder?.reset()

                // After calling release  method,
                // you will have to configure it again as if it had just been constructed.
                mediaRecorder?.release()
            } catch (e: IllegalStateException) {
                //do nothing
            }

            mediaRecorder = null

            stopTimer()
            resetTimer()

            val file = File(outputFilePath.value)
            if (file.exists()) {
                if (file.delete()) {
                    Timber.d("file Deleted : %s", outputFilePath)
                } else {
                    Timber.d("file not Deleted : %s", outputFilePath)
                }
            }

            audioState.value = AudioState.Cleared
        }
    }


    private fun startTimer() {
        timer.scheduleAtFixedRate(object : TimerTask() {
            override fun run() {
                recordingTime += 1
                updateDisplay()
            }
        }, 1000, 1000)

        recorderVisualizerTimer = Timer()
        recorderVisualizerTimer.schedule(object : TimerTask() {
            override fun run() {
                try {
                    val currentMaxAmplitude = mediaRecorder?.maxAmplitude
                    if (currentMaxAmplitude != null) {
                        callback?.onAmplitudeChanged(currentMaxAmplitude)
                    } //redraw view
                } catch (e: IllegalStateException) {
                    //do nothing for mediaRecorder visualizer IllegalStateException
                }
            }
        }, 0, 100)

    }
    private fun restartTimer() {
        recordingTime = 0

        recordingTimeString.postValue("00:00")

    }

    fun stopTimer() {
        timer.cancel()
        timer.purge()

        recorderVisualizerTimer.cancel()
        recorderVisualizerTimer.purge()
    }


    private fun resetTimer() {
        timer.cancel()
        recordingTime = 0

        recordingTimeString.postValue("00:00")

        recorderVisualizerTimer.cancel()
    }

    private fun updateDisplay() {
        val minutes = recordingTime / (60)
        val seconds = recordingTime % 60
        val str = String.format("%d:%02d", minutes, seconds)
        recordingTimeString.postValue(str)
    }

}