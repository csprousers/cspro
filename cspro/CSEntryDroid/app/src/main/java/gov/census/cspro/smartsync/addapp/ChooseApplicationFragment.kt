package gov.census.cspro.smartsync.addapp

import android.annotation.TargetApi
import android.app.Activity
import android.content.Context
import android.os.Bundle
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.ListView
import androidx.fragment.app.Fragment
import androidx.recyclerview.widget.RecyclerView
import gov.census.cspro.csentry.R
import gov.census.cspro.smartsync.addapp.ChooseApplicationFragment.OnAddAppChooseApplicationListener
import java.util.*

/**
 * A simple [Fragment] subclass.
 * Activities that contain this fragment must implement the
 * [OnAddAppChooseApplicationListener] interface
 * to handle interaction events.
 * Use the [ChooseApplicationFragment.newInstance] factory method to
 * create an instance of this fragment.
 */
class ChooseApplicationFragment : Fragment() {

    private var mAppList: ArrayList<DeploymentPackage>? = null
    private var mListener: OnAddAppChooseApplicationListener? = null

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        if (arguments != null) {
            mAppList = arguments?.getParcelableArrayList(ARG_APP_LIST)
        }
    }

    override fun onCreateView(inflater: LayoutInflater, container: ViewGroup?,
                              savedInstanceState: Bundle?): View? {
        // Inflate the layout for this fragment
        return inflater.inflate(R.layout.fragment_choose_bundle, container, false)
    }

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        val listView: RecyclerView = view.findViewById(R.id.add_app_choose_app_list)
        val adapter = DeploymentPackageListAdapter({ p: DeploymentPackage ->
            mListener?.onApplicationChosen(p)
        }, true)
        listView.adapter = adapter
        adapter.submitList(mAppList)
        adapter.notifyDataSetChanged()
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
        mListener = if (context is OnAddAppChooseApplicationListener) {
            context
        } else {
            throw RuntimeException(context.toString()
                + " must implement OnAddAppChooseApplicationListener")
        }
    }

    override fun onDetach() {
        super.onDetach()
        mListener = null
    }

    fun onClickedAppInstall(v: View) {
        if (mListener != null) {
            val parentRow = v.parent as View
            val listView = parentRow.parent as ListView
            val position = listView.getPositionForView(parentRow)
            mAppList?.get(position)?.let { mListener?.onApplicationChosen(it) }
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
    internal interface OnAddAppChooseApplicationListener {
        fun onApplicationChosen(application: DeploymentPackage)
    }

    companion object {
        private const val ARG_APP_LIST = "appList"

        /**
         * Use this factory method to create a new instance of
         * this fragment using the provided parameters.
         *
         * @param appList List of bundles to choose from.
         * @return A new instance of fragment ChooseApplicationFragment.
         */
        // TODO: Rename and change types and number of parameters
        fun newInstance(appList: ArrayList<DeploymentPackage?>?): ChooseApplicationFragment {
            val fragment = ChooseApplicationFragment()
            val args = Bundle()
            args.putParcelableArrayList(ARG_APP_LIST, appList)
            fragment.arguments = args
            return fragment
        }
    }
}