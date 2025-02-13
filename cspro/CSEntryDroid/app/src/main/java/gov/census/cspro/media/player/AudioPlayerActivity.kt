package gov.census.cspro.media.player

import android.os.Bundle
import android.view.View
import android.widget.SeekBar
import androidx.activity.viewModels
import androidx.appcompat.app.AppCompatActivity
import androidx.core.content.ContextCompat
import androidx.databinding.DataBindingUtil
import gov.census.cspro.csentry.R
import gov.census.cspro.csentry.databinding.ActivityAudioPlayerBinding
import gov.census.cspro.media.util.AudioState
import gov.census.cspro.util.Constants

class AudioPlayerActivity : AppCompatActivity(), SeekBar.OnSeekBarChangeListener {

    private lateinit var binding: ActivityAudioPlayerBinding

    private val viewModel: AudioPlayerViewModel by viewModels {
        val audioFilePath = intent.getStringExtra(Constants.EXTRA_PLAYER_FILE_URL_KEY) ?: ""
        AudioPlayerViewModelFactory(audioFilePath)
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        setResult(RESULT_OK)

        binding = DataBindingUtil.setContentView(this, R.layout.activity_audio_player)
        binding.lifecycleOwner = this

        binding.viewModel = viewModel

        val title = intent.getStringExtra(Constants.EXTRA_USER_MESSAGE_KEY)
        if (title.isNullOrBlank())
            binding.titleTextview.visibility = View.GONE
        else
            binding.titleTextview.text = title

        binding.btnPlayPause.setOnClickListener {
            viewModel.togglePlayPause()
        }

        binding.seekBar.setOnSeekBarChangeListener(this)

        viewModel.audioState.observe(this, {
            if (it == AudioState.Running)
                binding.visualizer.visualizer?.apply { enabled = true }
            else
                binding.visualizer.visualizer?.apply { enabled = false }
        })

        binding.visualizer.setColor(ContextCompat.getColor(this, R.color.cspro_green))
        viewModel.audioSessionId.observe(this, {
            if (it > 0)
                binding.visualizer.setPlayer(it)
        })

        if (intent.getBooleanExtra(Constants.EXTRA_AUTOSTART, false))
            viewModel.audioState.value = AudioState.Running
    }

    override fun onDestroy() {
        super.onDestroy()

        if (binding.visualizer.visualizer != null)
            binding.visualizer.release()
    }

    override fun onProgressChanged(p0: SeekBar?, progress: Int, fromUser: Boolean) {
        // only call seekTo if was from user interaction, otherwise was fired from code and you shouldn't call player.seekTo()
        if (fromUser) {
            viewModel.seekTo(progress)
            binding.seekBar.progress = progress
        }
    }

    override fun onStartTrackingTouch(p0: SeekBar?) {}

    override fun onStopTrackingTouch(p0: SeekBar?) {}

}