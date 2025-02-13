package gov.census.cspro.csentry

import android.os.Bundle
import android.widget.TextView
import androidx.appcompat.app.AppCompatActivity
import gov.census.cspro.engine.*
import java.io.File

class NonEntryApplicationActivity : AppCompatActivity(), IEngineMessageCompletedListener {

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_non_entry_application)

        // instantiate the application interface
        EngineInterface.CreateEngineInterfaceInstance(application)

        if( intent.data?.path != null ) {
            val textviewApplicationName: TextView = findViewById(R.id.textview_application_name)
            textviewApplicationName.text = "Running " + File(intent.data!!.path).name

            Messenger.getInstance().sendMessage(
                object : EngineMessage(this, this) {
                    public override fun run() {
                        EngineInterface.getInstance().runNonEntryApplication(intent.data?.path)
                    }
                })
        }
    }

    override fun onMessageCompleted(msg: EngineMessage) {
        finish()
    }
}
