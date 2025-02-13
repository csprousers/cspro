package gov.census.cspro.media.player

import android.media.AudioAttributes
import android.media.MediaPlayer
import androidx.lifecycle.MutableLiveData
import androidx.lifecycle.ViewModel
import androidx.lifecycle.ViewModelProvider
import gov.census.cspro.media.util.AudioState
import gov.census.cspro.util.formatMillisecondsAsHoursMinutesSeconds
import timber.log.Timber
import java.util.concurrent.atomic.AtomicBoolean


class AudioPlayerViewModelFactory(private val audioFileName: String) : ViewModelProvider.Factory {
    override fun <T : ViewModel> create(modelClass: Class<T>): T {
        if (modelClass.isAssignableFrom(AudioPlayerViewModel::class.java)) {
            @Suppress("UNCHECKED_CAST")
            return AudioPlayerViewModel(audioFileName) as T
        }
        throw IllegalArgumentException("Unknown ViewModel class")
    }
}

class AudioPlayerViewModel(audioFileName: String) : ViewModel() {

    val playerProgress: MutableLiveData<Int> = MutableLiveData(0)
    val playerDuration: MutableLiveData<String> = MutableLiveData("")
    val playerCurrTime: MutableLiveData<String> = MutableLiveData("")
    val audioSessionId : MutableLiveData<Int> = MutableLiveData(-1)
    val audioState: MutableLiveData<AudioState> = MutableLiveData(AudioState.Paused)

    private val mediaPlayer = MediaPlayer()
    private val observer = MediaObserver()

    init {
        try {
			mediaPlayer.setAudioAttributes(
				AudioAttributes.Builder()
					.setContentType(AudioAttributes.CONTENT_TYPE_SPEECH)
					.build()
			)

            mediaPlayer.setDataSource(audioFileName)
            mediaPlayer.setOnCompletionListener {
                audioState.value = AudioState.Stopped
            }
            mediaPlayer.setOnPreparedListener {
                it.seekTo(0)
                audioSessionId.value = it.audioSessionId
                playerDuration.value = mediaPlayer.duration.formatMillisecondsAsHoursMinutesSeconds()
                if (audioState.value == AudioState.Running)
                    mediaPlayer.start()
                Thread(observer).start()
            }
            mediaPlayer.prepareAsync()
        } catch (e: Exception) {
            Timber.e(e)
            audioState.value = AudioState.Stopped
        }
    }

    fun updateProgress() {

        playerProgress.postValue(
            (mediaPlayer.currentPosition.toDouble() / mediaPlayer.duration.toDouble() * 100).toInt()
        )

        playerCurrTime.postValue(mediaPlayer.currentPosition.formatMillisecondsAsHoursMinutesSeconds())
    }

    fun seekTo(position: Int) {
        val v = (position.toDouble() / 100.0 * mediaPlayer.duration.toDouble()).toInt()
        mediaPlayer.seekTo(v)
    }

    fun togglePlayPause() {
        if (audioState.value == AudioState.Running) {
            if (mediaPlayer.isPlaying)
                mediaPlayer.pause()
            audioState.value = AudioState.Paused
        } else {
            if (!mediaPlayer.isPlaying)
                mediaPlayer.start()
            audioState.value = AudioState.Running
        }
    }

    fun rewind() {
        mediaPlayer.seekTo(0)
    }

    override fun onCleared() {
        super.onCleared()
        observer.stop()
        mediaPlayer.release()
    }

    inner class MediaObserver : Runnable {
        private val stop: AtomicBoolean = AtomicBoolean(false)
        fun stop() {
            stop.set(true)
        }

        override fun run() {
            while (!stop.get()) {
                updateProgress()
                try {
                    Thread.sleep(200)
                } catch (ex: Exception) {
                }
            }
        }
    }
}