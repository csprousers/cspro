package gov.census.cspro.csentry.ui

import android.app.Activity
import android.content.Intent
import android.media.MediaMetadataRetriever
import android.net.Uri
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import androidx.activity.result.ActivityResultLauncher
import androidx.activity.result.contract.ActivityResultContracts
import androidx.recyclerview.widget.RecyclerView
import gov.census.cspro.csentry.CSEntry
import gov.census.cspro.csentry.databinding.QuestionWidgetAudioBinding
import gov.census.cspro.engine.EngineInterface
import gov.census.cspro.engine.Util
import gov.census.cspro.form.CDEField
import gov.census.cspro.media.player.AudioPlayerActivity
import gov.census.cspro.media.recording.interactive.RecordingActivity
import gov.census.cspro.media.util.AudioState
import gov.census.cspro.util.Constants
import gov.census.cspro.util.formatMillisecondsAsHoursMinutesSeconds
import timber.log.Timber
import java.io.File
import java.io.FileOutputStream
import java.io.IOException


class QuestionWidgetAudio(field: CDEField, adapter: RecyclerView.Adapter<QuestionWidgetViewHolder>, setFocus: Boolean, emitNextPage: Boolean, showCodes: Boolean): QuestionWidget(field, adapter, setFocus, emitNextPage, showCodes) {

    private var currentFieldValue: File? = null

    override val itemCount = 2

    override val allItemViewTypes = intArrayOf(VIEW_TYPE_QUESTION_COMMON_PARTS, VIEW_TYPE_AUDIO)

    override fun getItemViewType(position: Int) =
        if (position == 0) VIEW_TYPE_QUESTION_COMMON_PARTS else VIEW_TYPE_AUDIO

    override fun copyResponseToField() {
        // TODO
    }

    override fun supportsResponseFilter() = false

    override fun filterResponses(filterPattern: String) {
    }

    override val initialScrollPosition = 0


    override fun onCreateViewHolder(viewGroup: ViewGroup, viewType: Int) =
        when (viewType) {
            VIEW_TYPE_QUESTION_COMMON_PARTS -> {
                createCommonPartsViewHolder(viewGroup)
            }
            VIEW_TYPE_AUDIO -> {
                createAudioViewHolder(viewGroup)
            }
            else -> {
                throw AssertionError("Invalid view type")
            }
        }

    override fun onBindViewHolder(viewHolder: QuestionWidgetViewHolder, position: Int, nextPageListener: NextPageListener?, editNotes: Boolean) {
        if (position == 0)
            bindCommonParts(viewHolder, editNotes)
        else
            bindAudio(viewHolder, nextPageListener)
    }

    private fun createAudioViewHolder(parent: ViewGroup): QuestionWidgetViewHolder {
        val inflater = LayoutInflater.from(parent.context)
        val binding = QuestionWidgetAudioBinding.inflate(inflater, parent, false)
        return AudioViewHolder(binding)
    }

    private fun bindAudio(baseViewHolder: QuestionWidgetViewHolder, nextPageListener: NextPageListener?) {
        val audioViewHolder = baseViewHolder as AudioViewHolder
        audioViewHolder.bind(field, this, nextPageListener)
    }

    class AudioViewHolder(private val binding: QuestionWidgetAudioBinding) : QuestionWidgetViewHolder(binding.root) {

        fun bind(field: CDEField, questionWidget: QuestionWidgetAudio, nextPageListener: NextPageListener?) {

            updateUI(field, questionWidget)

            val recorderActivityLauncher = registerForActivityResult(ActivityResultContracts.StartActivityForResult()) {
                onRecorderActivityResult(field, questionWidget, nextPageListener, it.resultCode, it.data)
            }

            binding.buttonRecord.setOnClickListener {
                recordAudio(recorderActivityLauncher, field.label)
            }

            val chooseFileActivityLauncher = registerForActivityResult(ActivityResultContracts.StartActivityForResult()) {
                onChooseFileResult(field, questionWidget, nextPageListener, it.resultCode, it.data)
            }

            binding.buttonChoose.setOnClickListener {
                chooseAudio(chooseFileActivityLauncher)
            }

            val playerActivityLauncher = registerForActivityResult(ActivityResultContracts.StartActivityForResult()) {}
            binding.buttonPlay.setOnClickListener {
                playAudio(questionWidget.currentFieldValue!!, field.label, playerActivityLauncher)
            }

            binding.buttonDelete.setOnClickListener {
                questionWidget.currentFieldValue = null
                updateUI(field, questionWidget)
            }
        }

        private fun playAudio(file: File, label: String, startForResult: ActivityResultLauncher<Intent>) {
            val intent = Intent(CSEntry.context, AudioPlayerActivity::class.java)
            intent.putExtra(Constants.EXTRA_RECORDING_FILE_URL_KEY, file.path)
            if (!Util.stringIsNullOrEmpty(label))
                intent.putExtra(Constants.EXTRA_USER_MESSAGE_KEY, label)
            intent.putExtra(Constants.EXTRA_AUTOSTART, true)
            startForResult.launch(intent)
        }

        private fun recordAudio(startForResult: ActivityResultLauncher<Intent>, label: String) {
            val intent = Intent(CSEntry.context, RecordingActivity::class.java)
            val tempFile = File.createTempFile("audio", null, itemView.context.cacheDir)
            tempFile.deleteOnExit()
            intent.putExtra(Constants.EXTRA_RECORDING_FILE_URL_KEY, tempFile.path)
            // TODO: get sampling rate from field properties
            //  intent.putExtra(Constants.EXTRA_RECORDING_SAMPLING_RATE, samplingRate)
            if (!Util.stringIsNullOrEmpty(label))
                intent.putExtra(Constants.EXTRA_USER_MESSAGE_KEY, label)
            intent.action = AudioState.Start.toString()
            startForResult.launch(intent)
        }

        private fun chooseAudio(startForResult: ActivityResultLauncher<Intent>) {
            val intent = Intent(Intent.ACTION_GET_CONTENT).apply {
                type = "audio/*"
            }
            startForResult.launch(intent)
        }

        private fun updateUI(field: CDEField, questionWidget: QuestionWidgetAudio) {
            if (questionWidget.currentFieldValue != null) {
                binding.textViewDuration.visibility = View.VISIBLE
                val duration = questionWidget.currentFieldValue?.let { getDuration(it) }
                if (duration != null)
                   binding.textViewDuration.text = duration
                binding.buttonPlay.visibility = View.VISIBLE
            } else {
                binding.textViewDuration.visibility = View.GONE
                binding.buttonPlay.visibility = View.GONE
            }

            if (field.isReadOnly) {
                binding.buttonRecord.visibility = View.GONE
                binding.buttonChoose.visibility = View.GONE
                binding.buttonDelete.visibility = View.GONE
            } else {
                binding.buttonRecord.visibility = View.VISIBLE
                binding.buttonChoose.visibility = View.VISIBLE
                binding.buttonDelete.visibility =
                    if (questionWidget.currentFieldValue != null)
                        View.VISIBLE
                    else
                        View.GONE
            }
        }

        private fun getDuration(file: File): String? {
            val mediaMetadataRetriever = MediaMetadataRetriever()
            mediaMetadataRetriever.setDataSource(file.absolutePath)
            return mediaMetadataRetriever.extractMetadata(MediaMetadataRetriever.METADATA_KEY_DURATION)
                ?.toInt()
                ?.formatMillisecondsAsHoursMinutesSeconds()
        }

        private fun onRecorderActivityResult(field: CDEField, questionWidget: QuestionWidgetAudio, nextPageListener: NextPageListener?, resultCode: Int, data: Intent?) {
            if (data != null && resultCode != Activity.RESULT_CANCELED) {
                val newFieldValue = File(data.getStringExtra(Constants.EXTRA_RECORDING_FILE_URL_KEY)!!)
                questionWidget.currentFieldValue = newFieldValue
                updateUI(field, questionWidget)
                if (nextPageListener != null && questionWidget.emitNextPage && EngineInterface.getInstance().autoAdvanceOnSelectionFlag)
                    nextPageListener.OnNextPage()
            }
        }

        private fun onChooseFileResult(field: CDEField, questionWidget: QuestionWidgetAudio, nextPageListener: NextPageListener?, resultCode: Int, data: Intent?) {
            if (data != null && resultCode != Activity.RESULT_CANCELED) {
                val fullAudioUri = data.data
                if (fullAudioUri != null) {
                    val tempFile = File.createTempFile("audio", null, itemView.context.cacheDir)
                    try {
                        copyFromContentURI(fullAudioUri, tempFile)
                        questionWidget.currentFieldValue = tempFile
                        updateUI(field, questionWidget)
                        if (nextPageListener != null && questionWidget.emitNextPage && EngineInterface.getInstance().autoAdvanceOnSelectionFlag)
                            nextPageListener.OnNextPage()

                    } catch (e: Exception) {
                        Timber.e(e, "Error copying picked audio $fullAudioUri")
                        tempFile.delete()
                    }
                }
            }
        }

        private fun copyFromContentURI(uri: Uri, tempFile: File) {
            val inputStream = itemView.context.contentResolver.openInputStream(uri)
                ?: throw IOException("Failed to open content URI $uri")
            inputStream.use {
                val outputStream = FileOutputStream(tempFile)
                outputStream.use {
                    Util.copyStreams(inputStream, outputStream)
                }
            }
        }
    }
}