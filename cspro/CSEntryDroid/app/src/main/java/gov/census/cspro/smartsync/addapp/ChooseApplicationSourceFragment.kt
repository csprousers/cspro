package gov.census.cspro.smartsync.addapp

import android.annotation.TargetApi
import android.app.Activity
import android.content.Context
import android.os.Bundle
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.AdapterView
import android.widget.BaseAdapter
import android.widget.ListView
import android.widget.TextView
import androidx.fragment.app.Fragment
import gov.census.cspro.csentry.R

/**
 * A simple [Fragment] subclass.
 * Activities that contain this fragment must implement the
 * [ChooseApplicationSourceFragment.OnAddAppChooseSourceListener] interface
 * to handle interaction events.
 * Use the [ChooseApplicationSourceFragment.newInstance] factory method to
 * create an instance of this fragment.
 */
class ChooseApplicationSourceFragment : Fragment(), AdapterView.OnItemClickListener {

    private var mListener: OnAddAppChooseSourceListener? = null

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
    }

    override fun onCreateView(inflater: LayoutInflater, container: ViewGroup?,
                              savedInstanceState: Bundle?): View? {
        // Inflate the layout for this fragment
        return inflater.inflate(R.layout.fragment_choose_application_source, container, false)
    }

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        val listView = view.findViewById<View>(R.id.appSourcesListView) as ListView
        val adapter = AppSourceAdapter(activity)
        listView.adapter = adapter
        listView.onItemClickListener = this
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
        mListener = if (context is OnAddAppChooseSourceListener) {
            context
        } else {
            throw RuntimeException(context.toString()
                + " must implement OnAddAppChooseSourceListener")
        }
    }

    override fun onDetach() {
        super.onDetach()
        mListener = null
    }

    override fun onItemClick(adapterView: AdapterView<*>?, view: View, i: Int, l: Long) {
        if (mListener != null) {
            mListener?.onSourceSelected(i)
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
    internal interface OnAddAppChooseSourceListener {
        fun onSourceSelected(source: Int)
    }

    private inner class AppSourceAdapter(context: Context?) : BaseAdapter() {
        private val m_labels: Array<String?>?
        private val m_icons: IntArray
        private val m_inflater: LayoutInflater

        init {
            m_inflater = LayoutInflater.from(context)
            m_labels = arrayOfNulls(5)
            m_icons = IntArray(5)
            m_labels[CSWEB_SOURCE] = getString(R.string.add_app_source_csweb)
            m_icons[CSWEB_SOURCE] = R.drawable.ic_csweb
            m_labels[DROPBOX_SOURCE] = getString(R.string.add_app_source_dropbox)
            m_icons[DROPBOX_SOURCE] = R.drawable.ic_dropbox
            m_labels[FTP_SOURCE] = getString(R.string.add_app_source_ftp)
            m_icons[FTP_SOURCE] = R.drawable.ic_ftp_server
            m_labels[SCAN_SOURCE] = getString(R.string.add_app_source_scan)
            m_icons[SCAN_SOURCE] = R.drawable.ic_qr_code
            m_labels[EXAMPLE_SOURCE] = getString(R.string.add_app_source_example)
            m_icons[EXAMPLE_SOURCE] = R.drawable.ic_example_app
        }

        override fun getCount(): Int {
            return m_labels?.size ?: 0
        }

        override fun getItem(position: Int): Any? {
            return if (m_labels == null) null else m_labels[position]
        }

        override fun getItemId(position: Int): Long {
            return position.toLong()
        }


        override fun getView(position: Int, convertView: View?, parent: ViewGroup?): View? {
            m_labels?.let {
                var newView = convertView
                if (newView == null) {
                    newView = m_inflater.inflate(android.R.layout.simple_list_item_1, null)
                }
                val labelView = newView?.findViewById<View>(android.R.id.text1) as TextView
                labelView.text = it[position]
                labelView.setCompoundDrawablesWithIntrinsicBounds(m_icons[position], 0, 0, 0)
                labelView.compoundDrawablePadding = 10
                return newView
            }
            return null
        }

    }

    companion object {
        const val CSWEB_SOURCE = 0
        const val DROPBOX_SOURCE = 1
        const val FTP_SOURCE = 2
        const val SCAN_SOURCE = 3
        const val EXAMPLE_SOURCE = 4

        /**
         * Use this factory method to create a new instance of
         * this fragment using the provided parameters.
         *
         * @return A new instance of fragment ChooseApplicationSourceFragment.
         */
        fun newInstance(): ChooseApplicationSourceFragment {
            return ChooseApplicationSourceFragment()
        }
    }
}