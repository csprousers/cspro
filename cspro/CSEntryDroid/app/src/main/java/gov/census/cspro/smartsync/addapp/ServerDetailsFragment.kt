package gov.census.cspro.smartsync.addapp

import android.annotation.TargetApi
import android.app.Activity
import android.content.Context
import android.os.Bundle
import android.text.Editable
import android.text.TextWatcher
import android.view.KeyEvent
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.view.inputmethod.EditorInfo
import android.widget.*
import android.widget.TextView.OnEditorActionListener
import androidx.fragment.app.Fragment
import gov.census.cspro.csentry.R
import gov.census.cspro.engine.Util
import java.net.URI
import java.net.URISyntaxException

/**
 * A simple [Fragment] subclass.
 * Activities that contain this fragment must implement the
 * [ServerDetailsFragment.OnAddAppServerDetailsListener] interface
 * to handle interaction events.
 * Use the [ServerDetailsFragment.newInstance] factory method to
 * create an instance of this fragment.
 */
class ServerDetailsFragment : Fragment(), TextWatcher, OnEditorActionListener, View.OnClickListener {

     private val mServerType: Int by lazy {
        requireArguments().getInt(ARG_SERVER_TYPE)
    }

    private var mListener: OnAddAppServerDetailsListener? = null

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
    }

    override fun onCreateView(inflater: LayoutInflater, container: ViewGroup?,
                              savedInstanceState: Bundle?): View? {
        // Inflate the layout for this fragment
        return inflater.inflate(R.layout.fragment_server_details, container, false)
    }

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        val serverEditText = view.findViewById<View>(R.id.edittext_add_app_server_uri) as EditText
        val connectButton = view.findViewById<View>(R.id.button_add_app_connect) as Button

        // set up edit text handling and ime
        serverEditText.addTextChangedListener(this)
        connectButton.setOnClickListener(this)
        connectButton.isEnabled = false
        if (mServerType == ChooseApplicationSourceFragment.FTP_SOURCE) serverEditText.setHint(R.string.add_app_uri_hint_ftp) else serverEditText.setHint(R.string.add_app_uri_hint_http)
        populatePreviousConnectionInfo()
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
        mListener = if (context is OnAddAppServerDetailsListener) {
            context
        } else {
            throw RuntimeException(context.toString()
                + " must implement OnAddAppServerDetailsListener")
        }
    }

    override fun onDetach() {
        super.onDetach()
        mListener = null
    }

    override fun beforeTextChanged(charSequence: CharSequence, i: Int, i1: Int, i2: Int) {}
    override fun onTextChanged(charSequence: CharSequence, i: Int, i1: Int, i2: Int) {}
    override fun afterTextChanged(editable: Editable) {
        activateConnectButton()
    }

    override fun onClick(view: View) {
        connectClicked()
    }

    private fun connectClicked() {
        saveConnectionInfo()
        if (mListener != null) {
            val serverEditText = requireView().findViewById<View>(R.id.edittext_add_app_server_uri) as EditText
            var serverURI: URI
            try {
                serverURI = URI(serverEditText.text.toString().trim())
                if (serverURI.scheme == null) {
                    val scheme = if (mServerType == ChooseApplicationSourceFragment.FTP_SOURCE) "ftp://" else "http://"
                    serverURI = URI(scheme + serverEditText.text.toString())
                }
            } catch (e: URISyntaxException) {
                serverEditText.error = "Invalid URL"
                return
            }
            mListener?.onServerDetailsSelected(serverURI)
        }
    }

    private fun activateConnectButton() {
        var enabled = false
        val connectButton = requireView().findViewById<View>(R.id.button_add_app_connect) as Button
        val serverEditText = requireView().findViewById<View>(R.id.edittext_add_app_server_uri) as EditText
        if (serverEditText.text.isNotEmpty()) {
            try {
                val url = serverEditText.text.toString().trim()
                val uri = URI(url)

                // Enable if there is either host or path (if they forget scheme then host will
                // be null and host will be interpreted as path).
                if (!Util.stringIsNullOrEmpty(uri.host) || !Util.stringIsNullOrEmpty(uri.path)) {
                    enabled = true
                }
            } catch (ex: Exception) {
            }
        }
        connectButton.isEnabled = enabled
    }

    private fun saveConnectionInfo() {
        val serverEditText = requireView().findViewById<View>(R.id.edittext_add_app_server_uri) as EditText
        val sharedPref = requireActivity().getPreferences(Context.MODE_PRIVATE)
        val editor = sharedPref.edit()
        if (mServerType == ChooseApplicationSourceFragment.FTP_SOURCE) editor.putString(FTP_SERVER_URI_PREF, serverEditText.text.toString()) else editor.putString(CSWEB_SERVER_URI_PREF, serverEditText.text.toString())
        editor.apply()
    }

    private fun populatePreviousConnectionInfo() {
        val sharedPref = requireActivity().getPreferences(Context.MODE_PRIVATE)
        val previousServerUri = if (mServerType == ChooseApplicationSourceFragment.FTP_SOURCE) sharedPref.getString(FTP_SERVER_URI_PREF, null) else sharedPref.getString(CSWEB_SERVER_URI_PREF, null)
        if (!Util.stringIsNullOrEmptyTrim(previousServerUri)) {
            val pathEditText = requireView().findViewById<View>(R.id.edittext_add_app_server_uri) as EditText
            pathEditText.setText(previousServerUri)
        }
    }

    override fun onEditorAction(textView: TextView, actionId: Int, keyEvent: KeyEvent): Boolean {
        if (actionId == EditorInfo.IME_ACTION_GO) {
            val connectButton = requireView().findViewById<View>(R.id.button_add_app_connect) as Button
            if (connectButton.isEnabled) {
                connectClicked()
                return true
            }
        }
        return false
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
    internal interface OnAddAppServerDetailsListener {
        fun onServerDetailsSelected(uri: URI)
    }

    companion object {
        private const val ARG_SERVER_TYPE = "serverType"
        private const val FTP_SERVER_URI_PREF = "ftpServerUriPref"
        private const val CSWEB_SERVER_URI_PREF = "CSWebServerUriPref"

        /**
         * Use this factory method to create a new instance of
         * this fragment using the provided parameters.
         *
         * @param serverType Type of server (csweb or ftp).
         * @return A new instance of fragment ServerDetailsFragment.
         */
        fun newInstance(serverType: Int): ServerDetailsFragment {
            val fragment = ServerDetailsFragment()
            val args = Bundle()
            args.putInt(ARG_SERVER_TYPE, serverType)
            fragment.arguments = args
            return fragment
        }
    }
}