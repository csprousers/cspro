package gov.census.cspro.util

import android.view.View
import androidx.activity.enableEdgeToEdge
import androidx.appcompat.app.AppCompatActivity
import androidx.core.view.ViewCompat
import androidx.core.view.WindowInsetsCompat

object EdgeToEdgeUtils {

    /**
     * Sets up window insets handling. Call this after setContentView().
     *
     * @param activity The activity to apply insets handling to
     * @param rootViewId Optional custom root view ID. Defaults to android.R.id.content
     */
    fun setupWindowInsets(activity: AppCompatActivity, rootViewId: Int = android.R.id.content) {
        val root = activity.findViewById<View>(rootViewId)
        //Handle padding for system bars
        if (root != null) {
            ViewCompat.setOnApplyWindowInsetsListener(root) { view, insets ->
                val systemBars = insets.getInsets(WindowInsetsCompat.Type.systemBars())
                view.setPadding(systemBars.left, systemBars.top, systemBars.right, systemBars.bottom)
                insets
            }
        }
    }

    /**
     * Complete setup for simple cases where setContentView and setupWindowInsets are both called.
     * This is a convenience method that handles both steps.
     */
    fun setupEdgeToEdge(
        activity: AppCompatActivity,
        layoutId: Int,
        rootViewId: Int = android.R.id.content
    ) {
        activity.enableEdgeToEdge()
        activity.setContentView(layoutId)
        setupWindowInsets(activity, rootViewId)
    }
}