package gov.census.cspro.csentry

import android.os.Bundle
import android.widget.TextView
import androidx.appcompat.app.AppCompatActivity
import gov.census.cspro.engine.*

class ViewQuestionnaireActivity : AppCompatActivity(), IEngineMessageCompletedListener {

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_non_entry_application)

        // instantiate the application interface
        EngineInterface.CreateEngineInterfaceInstance(application)

        val textviewApplicationName: TextView = findViewById(R.id.textview_application_name)
        textviewApplicationName.text = String.format(getString(R.string.menu_cases_list_view_questionnaire), intent.getStringExtra(KEY))

        Messenger.getInstance().sendMessage(
            object : EngineMessage(this, this) {
                override fun run() {
                    EngineInterface.getInstance().viewCase(intent.getDoubleExtra(POSITION_IN_REPOSITORY, -1.0))
                }
            })
    }

    override fun onMessageCompleted(msg: EngineMessage) {
        finish()
    }

    companion object {
        const val KEY = "KEY"
        const val POSITION_IN_REPOSITORY = "POSITION_IN_REPOSITORY"
    }
}
