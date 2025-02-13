package gov.census.cspro.csentry

import android.annotation.TargetApi
import android.app.Activity
import android.content.Context
import android.os.Bundle
import android.view.KeyEvent
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.view.inputmethod.EditorInfo
import android.widget.Button
import android.widget.EditText
import android.widget.TextView
import android.widget.TextView.OnEditorActionListener
import androidx.fragment.app.Fragment

/**
 * A [Fragment] subclass for entering operator id.
 * Activities that contain this fragment must implement the
 * [GetOperatorIdFragment.OnFragmentInteractionListener] interface
 * to handle interaction events.
 * Use the [GetOperatorIdFragment.newInstance] factory method to
 * create an instance of this fragment.
 */
class GetOperatorIdFragment : Fragment() {

    private var m_listener: OnFragmentInteractionListener? = null
    private lateinit var m_editText: EditText

    public override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
    }

    public override fun onCreateView(inflater: LayoutInflater, container: ViewGroup?,
                                     savedInstanceState: Bundle?): View? {
        // Inflate the layout for this fragment
        return inflater.inflate(R.layout.fragment_get_operator_id, container, false)
    }

    public override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        m_editText = view.findViewById(R.id.edittext_operator_id)

        m_editText.setOnEditorActionListener { v, actionId, event ->
            if ((actionId and EditorInfo.IME_MASK_ACTION) == EditorInfo.IME_ACTION_DONE) {
                onOkButtonPressed()
                true
            } else {
                false
            }
        }

        val okButton: Button = view.findViewById(R.id.button_ok)
        okButton.setOnClickListener { onOkButtonPressed() }
    }

    fun onOkButtonPressed() {
        val opId: String = m_editText.text.toString()
        if (opId.isEmpty()) {
            m_editText.error = getString(R.string.operator_id_blank)
        } else {
            if (m_listener != null) {
                m_listener?.onOperatorIdEntered(opId)
            }
        }
    }

    @TargetApi(23)
    public override fun onAttach(context: Context) {
        super.onAttach(context)
        attachToContext(context)
    }

    @Deprecated("Deprecated in Java")
    public override fun onAttach(activity: Activity) {
        super.onAttach(activity)
        attachToContext(activity)
    }

    private fun attachToContext(context: Context) {
        if (context is OnFragmentInteractionListener) {
            m_listener = context
        } else {
            throw RuntimeException((context.toString()
                + " must implement GetOperatorIdFragment"))
        }
    }

    public override fun onDetach() {
        super.onDetach()
        m_listener = null
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
        fun onOperatorIdEntered(operatorId: String)
    }

    companion object {
        /**
         * Use this factory method to create a new instance of
         * this fragment using the provided parameters.
         *
         * @return A new instance of fragment GetOperatorIdFragment.
         */
        fun newInstance(): GetOperatorIdFragment {
            return GetOperatorIdFragment()
        }
    }
}