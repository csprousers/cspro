package gov.census.cspro.csentry

import android.content.Context
import android.content.SharedPreferences
import androidx.multidex.MultiDexApplication
import gov.census.cspro.commonui.CSStyle
import gov.census.cspro.commonui.OnCSStyleChangedListener
import gov.census.cspro.engine.Util
import timber.log.Timber
import timber.log.Timber.DebugTree
import java.util.*

//import gov.census.cspro.util.CrashReporter;
class CSEntry : MultiDexApplication() {

    override fun onCreate() {
        super.onCreate()
        context = applicationContext

        System.loadLibrary("CSEntry")

        if (BuildConfig.DEBUG) {
            Timber.plant(DebugTree())
        }

        // Uncomment this line to enable crash report logging for non Google Play builds.
        //CrashReporter.install(CONTEXT);
    }

    companion object {

        lateinit var context: Context
            private set
        private var CURRENT_STYLE: CSStyle? = null
        private const val STYLE_NAME_KEY: String = "Style.Name"
        private const val STYLE_PREFS_KEY: String = "Style.Preferences"
        private val m_styleChangedListeners: ArrayList<OnCSStyleChangedListener> = ArrayList()
        @JvmField
        val CS_LOCALE: Locale = Locale.US

        fun addCSStyleListener(listener: OnCSStyleChangedListener) {
            if (!m_styleChangedListeners.contains(listener)) {
                m_styleChangedListeners.add(listener)
            }
        }

        fun removeCSStyleListener(listener: OnCSStyleChangedListener?) {
            m_styleChangedListeners.remove(listener)
        }

        val isTablet: Boolean
            get() {
                return context.resources.getBoolean(R.bool.is_tablet)
            }// an error occurred during loading, or no style set// set the member

        // notify interested parties
        // save the style name to disk
        // save the name to disk
        // set the default and store it
        // could not load styles
        // load the style from preferences
        var style: CSStyle?
            get() {
                if (CURRENT_STYLE == null) {
                    // load the style from preferences
                    val prefs: SharedPreferences = context.getSharedPreferences(STYLE_PREFS_KEY, MODE_PRIVATE)
                    val stylename: String? = prefs.getString(STYLE_NAME_KEY, null)
                    if (!Util.stringIsNullOrEmpty(stylename)) {
                        try {
                            val styles: ArrayList<CSStyle> = CSStyle.getAllStyles(context)
                            for (csStyle: CSStyle in styles) {
                                if (stylename?.let { csStyle.name.compareTo(it) } == 0) {
                                    CURRENT_STYLE = csStyle
                                    break
                                }
                            }
                        } catch (ex: Exception) {
                            // could not load styles
                            Timber.d(ex, "Could not load styles from disk")
                        }
                    }
                    if (CURRENT_STYLE == null) {
                        // an error occurred during loading, or no style set
                        // set the default and store it
                        CURRENT_STYLE = CSStyle.defaultStyle()
                        style = CURRENT_STYLE
                    }
                }
                return CURRENT_STYLE
            }

            set(style) {
                // save the style name to disk
                val prefs: SharedPreferences = context.getSharedPreferences(STYLE_PREFS_KEY, MODE_PRIVATE)
                val editor: SharedPreferences.Editor = prefs.edit()
                editor.putString(STYLE_NAME_KEY, style?.name)
                // save the name to disk
                if (editor.commit()) {    // set the member
                    CURRENT_STYLE = style
                    // notify interested parties
                    for (listener: OnCSStyleChangedListener in m_styleChangedListeners) {
                        listener.onCSStyleChanged(style)
                    }
                }
            }
    }
}