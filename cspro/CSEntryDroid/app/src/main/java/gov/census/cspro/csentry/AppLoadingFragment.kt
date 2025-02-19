package gov.census.cspro.csentry

import android.annotation.TargetApi
import android.app.Activity
import android.content.Context
import android.content.Intent
import android.os.Bundle
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.TextView
import android.widget.Toast
import androidx.appcompat.app.AppCompatActivity
import androidx.fragment.app.DialogFragment
import gov.census.cspro.bridge.CNPifFile
import gov.census.cspro.csentry.ui.EntryActivity
import gov.census.cspro.engine.*
import java.io.File
import java.util.Locale

/**
 * A simple [Fragment] subclass that displays and app loading message.
 * Activities that contain this fragment must implement the
 * [AppLoadingFragment.OnFragmentInteractionListener] interface
 * to handle interaction events.
 * Use the [AppLoadingFragment.newInstance] factory method to
 * create an instance of this fragment.
 */
class AppLoadingFragment : DialogFragment() {

    private var mListener: OnFragmentInteractionListener? = null

    override fun onCreateView(inflater: LayoutInflater, container: ViewGroup?,
                                     savedInstanceState: Bundle?): View? {
        // Inflate the layout for this fragment
        return inflater.inflate(R.layout.fragment_app_loading, container, false)
    }

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        // noinspection ConstantConditions
        val appDescription: String? = arguments?.getString(ARG_APP_DESCRIPTION)
        val loadingText: TextView = view.findViewById(R.id.text_app_loading_message)
        loadingText.text = String.format(getText(R.string.starting_application).toString(), appDescription)
    }

    @Deprecated("Deprecated in Java")
    override fun onActivityCreated(savedInstanceState: Bundle?) {
        super.onActivityCreated(savedInstanceState)
        if (savedInstanceState == null) {
            // noinspection ConstantConditions
            val appFilename: String? = arguments?.getString(ARG_APP_FILENAME)
            openApplication(appFilename)
        }
    }

    /**
     * This interface must be implemented by activities that contain this
     * fragment to allow an interaction in this fragment to be communicated
     * to the activity and potentially other fragments contained in that
     * activity.
     *
     *
     * See the Android Training lesson [Communicating with Other Fragments](http://developer.android.com/training/basics/fragments/communicating.html) for more information.
     */
    interface OnFragmentInteractionListener {
        fun applicationLoaded()
        fun applicationLoadFailed(filename: String)
    }

    @TargetApi(23)
    override fun onAttach(context: Context) {
        super.onAttach(context)
        attachToContext(context)
    }

    @Deprecated("Deprecated in Java")
    override fun onAttach(activity: Activity) {
        @Suppress("DEPRECATION")
        super.onAttach(activity)
        attachToContext(activity)
    }

    private fun attachToContext(context: Context) {
        if (context is OnFragmentInteractionListener) {
            mListener = context
        } else {
            throw RuntimeException(context.toString()
                + " must implement AppLoadingFragment.OnFragmentInteractionListener")
        }
    }

    override fun onDetach() {
        super.onDetach()
        mListener = null
    }

    private fun openApplication(applicationFilename: String?) {
        Messenger.getInstance().sendMessage(OpenApplicationMessage(activity as AppCompatActivity?,
            { msg: EngineMessage ->
                if (mListener != null) {
                    val openAppMsg: OpenApplicationMessage = msg as OpenApplicationMessage
                    if (openAppMsg.mSuccess) {
                        mListener?.applicationLoaded()
                    } else {
                        openAppMsg.mApplicationFilename?.let {
                            mListener?.applicationLoadFailed(it)
                        }
                    }
                }
            }, applicationFilename))
    }

    private class OpenApplicationMessage(activity: AppCompatActivity?, listener: IEngineMessageCompletedListener, val mApplicationFilename: String?) : EngineMessage(activity, listener) {
        var mSuccess: Boolean = false
        override fun run() {
            mSuccess = EngineInterface.getInstance().openApplication(mApplicationFilename)
        }
    }

    companion object {
        private const val ARG_APP_FILENAME: String = "APP_FILENAME"
        private const val ARG_APP_DESCRIPTION: String = "APP_DESCRIPTION"

        /**
         * Use this factory method to create a new instance of
         * this fragment using the provided parameters.
         *
         * @param appFilename    Full path of pff file for application.
         * @param appDescription Full path of pff file for application.
         * @return A new instance of fragment AppLoadingFragment.
         */
        fun newInstance(appFilename: String, appDescription: String?): AppLoadingFragment {
            var description: String? = appDescription
            val fragment = AppLoadingFragment()
            val args = Bundle()
            args.putString(ARG_APP_FILENAME, appFilename)
            if (Util.stringIsNullOrEmpty(description)) {
                description = File(appFilename).name
            }
            args.putString(ARG_APP_DESCRIPTION, description)
            fragment.arguments = args
            return fragment
        }

        @JvmStatic
        fun newInstance(context: Context, intent: Intent): AppLoadingFragment? {
            val args = intent.extras
            try {
                var pffFilePath: String? = null

                args?.getString(EntryActivity.PFF_FILENAME_PARAM)?.let {
                    pffFilePath = getPffFullPath(it)
                }

                if( pffFilePath == null ) {
                    throw Exception("You must specify the file path of the PFF using: " + EntryActivity.PFF_FILENAME_PARAM)
                }

                val newPffFilePath: String = CNPifFile.CreatePffFromIntentExtras(pffFilePath, args)
                val description: String? = args!!.getString(EntryActivity.APP_DESCRIPTION_PARAM)
                return newInstance(newPffFilePath, description)
            }
            catch(e: Exception) {
                Toast.makeText(context, e.message, Toast.LENGTH_LONG).show()
                return null
            }
        }

        private fun getPffFullPath(pffName: String): String {

            if (File(pffName).isAbsolute) return pffName

            val lowercasePffFilePath: String = pffName.lowercase(Locale.getDefault())
            for (filename: String in Util.getApplicationsInDirectory(EngineInterface.getInstance().csEntryDirectory.path)) {
                if (filename.lowercase(Locale.getDefault()).contains(lowercasePffFilePath)) return filename
            }
            return pffName
        }
    }
}
