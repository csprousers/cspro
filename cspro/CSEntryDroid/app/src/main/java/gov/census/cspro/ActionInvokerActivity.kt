package gov.census.cspro

import android.content.Intent
import android.os.Bundle
import android.widget.TextView
import androidx.appcompat.app.AppCompatActivity
import gov.census.cspro.csentry.R
import gov.census.cspro.engine.EngineInterface
import gov.census.cspro.engine.EngineMessage
import gov.census.cspro.engine.IEngineMessageCompletedListener
import gov.census.cspro.engine.Messenger


class ActionInvokerActivity: AppCompatActivity(), IEngineMessageCompletedListener {
    private val actionInvokerResult = Intent()

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_non_entry_application)

        // instantiate the application interface
        EngineInterface.CreateEngineInterfaceInstance(application)

        // the title can be specified in the intent
        val textviewApplicationName: TextView = findViewById(R.id.textview_application_name)
        textviewApplicationName.text = intent.getStringExtra(TITLE) ?: "Action Invoker"

        val action = intent.getStringExtra(ACTION)
        val accessToken = intent.getStringExtra(ACCESS_TOKEN)
        val refreshToken = intent.getStringExtra(REFRESH_TOKEN)
        val abortOnException = intent.getBooleanExtra(ABORT_ON_EXCEPTION, true)

        // the result will contain all of the input's extras
        intent.extras?.let {
            actionInvokerResult.putExtras(it)
        }

        Messenger.getInstance().sendMessage(
            object: EngineMessage(this, this) {
                override fun run() {
                    val result = EngineInterface.getInstance().runActionInvoker(activity.callingPackage, action, accessToken, refreshToken, abortOnException)
                    actionInvokerResult.putExtra(RESULT, result.result)
                    actionInvokerResult.putExtra(REFRESH_TOKEN, result.refreshToken)
                }
            })
    }

    override fun onMessageCompleted(msg: EngineMessage) {
        setResult(RESULT_OK, actionInvokerResult)
        finish()
    }

    private companion object {
        const val ABORT_ON_EXCEPTION = "ABORT_ON_EXCEPTION"
        const val ACCESS_TOKEN = "ACCESS_TOKEN"
        const val ACTION = "ACTION"
        const val REFRESH_TOKEN = "REFRESH_TOKEN"
        const val RESULT = "ABORT_ON_EXCEPTION"
        const val TITLE = "TITLE"
    }
}