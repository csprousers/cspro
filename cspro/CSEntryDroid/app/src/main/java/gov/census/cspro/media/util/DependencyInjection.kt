package gov.census.cspro.media.util

import android.media.MediaRecorder
import timber.log.Timber
import java.io.File
import java.io.IOException

object DependencyInjection {
    private var mediaRecorder: MediaRecorder? = null

    fun provideMediaPlayerObject(samplingRate: Int): MediaRecorder {
        try {
            mediaRecorder = MediaRecorder()
            mediaRecorder?.setAudioSource(MediaRecorder.AudioSource.MIC)
            mediaRecorder?.setOutputFormat(MediaRecorder.OutputFormat.MPEG_4)
            mediaRecorder?.setAudioEncoder(MediaRecorder.AudioEncoder.AAC)
            if (samplingRate > 0)
                mediaRecorder?.setAudioSamplingRate(samplingRate)

            //mediaRecorder?.setAudioSamplingRate(16)
            //mediaRecorder?.setAudioEncodingBitRate(44100)
            //mediaRecorder?.setOnErrorListener(errorListener)
        } catch (e: Exception) {
            Timber.d("provideMediaPlayerObject exception $e")
        }
        return mediaRecorder!!
    }

     fun prepareRecorderPath(outputFilePath: String) {
        val directory: String = outputFilePath.substringBeforeLast("/")

        try {
            // Creating  Directory for saving Audio recording
            val file = File(directory)
            if (!file.exists()) {
                val result = file.mkdirs()
                Timber.d("MAKE DIR: $result")
            }

        } catch (e: IOException) {
            Timber.e(e.toString())
        }

        mediaRecorder?.setOutputFile(outputFilePath)
        Timber.i("outputFilePath.value: %s", outputFilePath)

    }

}