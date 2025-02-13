package gov.census.cspro.csentry.ui

import android.view.View
import androidx.activity.ComponentActivity
import androidx.activity.result.ActivityResultCallback
import androidx.activity.result.ActivityResultLauncher
import androidx.activity.result.contract.ActivityResultContract
import androidx.recyclerview.widget.RecyclerView

open class QuestionWidgetViewHolder(viewItem: View): RecyclerView.ViewHolder(viewItem) {

    private val activityResultLaunchers: MutableList<ActivityResultLauncher<*>> = mutableListOf()


    /**
     * Create a launcher that lets you launch an activity and get a result using AndroidX Activity Result APIs
     * Should be called when ViewHolder is bound to view (e.g. in the bind method)
     * then use the return value to launch the activity from a click or other event by calling launch().
     * For example:
     *      val launcher = registerForActivityResult(ActivityResultContracts.StartActivityForResult()) { // handle activity result here }
     *      mybutton.setOnClickListener { launcher.launch(intent) }
     */
    fun <I, O> registerForActivityResult(contract: ActivityResultContract<I, O>, callback: ActivityResultCallback<O>): ActivityResultLauncher<I> {
        val registry = (itemView.context as ComponentActivity).activityResultRegistry
        val launcherNumber = activityResultLaunchers.size
        val launcher = registry.register("QuestionWidgetViewHolder-$adapterPosition-$launcherNumber", contract, callback)
        activityResultLaunchers.add(launcher)
        return launcher
    }

    fun onRecycled() {
        activityResultLaunchers.forEach { it.unregister() }
        activityResultLaunchers.clear()
    }
}