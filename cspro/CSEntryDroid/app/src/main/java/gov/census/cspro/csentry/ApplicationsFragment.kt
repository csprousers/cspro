package gov.census.cspro.csentry

import android.content.Context
import android.os.Bundle
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.TextView
import androidx.fragment.app.Fragment
import androidx.lifecycle.lifecycleScope
import androidx.recyclerview.widget.LinearLayoutManager
import androidx.recyclerview.widget.RecyclerView
import gov.census.cspro.bridge.CNPifFile
import gov.census.cspro.engine.EngineInterface
import gov.census.cspro.engine.Util
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext
import java.util.*

class ApplicationsFragment() : Fragment() {
    data class AppListNameItem(
        val filename: String,
        val description: String,
        val entryAppType: Boolean
    )

    private var m_showHiddenApplications = false
    override fun onCreateView(inflater: LayoutInflater, container: ViewGroup?, savedInstanceState: Bundle?): View? {
        return inflater.inflate(R.layout.fragment_applications_layout, container, false)
    }

    fun launchSingleEntryApp() {
        // automatically launch the application if there is only one pff (and CSEntry isn't launched via another application)
        if (requireActivity().intent.data == null &&
            EngineInterface.GetSystemSettingBoolean(SystemSettings.SettingLaunchSingleApplicationAutomatically, false)) {
            view?.findViewById<RecyclerView>(R.id.applicationsListView)?.let{
                val listAdapter = it.adapter as ApplicationsListAdapter?
                if (listAdapter?.itemCount == 1) listAdapter.onItemClick(0)
            }
        }
    }

    override fun onAttach(context: Context) {
        super.onAttach(context)
        val parent = context as ApplicationsListActivity
        parent.setApplicationsFragment(this)
    }

    override fun onResume() {
        super.onResume()
        val applicationsListActivity = activity as ApplicationsListActivity?
        val pff_filename = EngineInterface.getExecPffParameter()
        if (pff_filename != null) {
            EngineInterface.setExecPffParameter(null)
            val pifFile = CNPifFile(pff_filename)
            if (pifFile.IsValid() && applicationsListActivity != null) {
                applicationsListActivity.openApplication(pff_filename,
                    pifFile.GetDescription(), pifFile.IsAppTypeEntry())
            }
        }
    }

    fun displayApplications(showHiddenApplications: Boolean) {
        m_showHiddenApplications = showHiddenApplications
        //this lifecyclescope to make sure when activity destroyed, the thread will stop
        lifecycleScope.launchWhenStarted {
            //get data on background thread
            withContext(Dispatchers.IO) {
                //running on background thread to get the items
                val appNameListItems = getApplications()

                // sort the list alphabetically
                Collections.sort(appNameListItems, Comparator { a, b -> a.description.compareTo((b.description), ignoreCase = true) })

                //and then update ui on ui thread
                withContext(Dispatchers.Main){
                    //this is on ui thread.
                    if (appNameListItems.size > 0) {

                        // Hide the message that there are no apps
                        val tv = view?.findViewById<TextView>(R.id.noAppsMessageTextView)
                        tv?.visibility = View.GONE

                        // Show the recycler view with list of apps
                        val rv: RecyclerView? = view?.findViewById(R.id.applicationsListView)
                        rv?.visibility = View.VISIBLE
                        rv?.layoutManager = LinearLayoutManager(context)
                        val listAdapter = ApplicationsListAdapter(appNameListItems,
                            object : ApplicationsListAdapter.OnItemClickListener {
                                override fun OnClick(item: AppListNameItem) {
                                    val activity = activity as ApplicationsListActivity?
                                    activity?.openApplication(item.filename, item.description, item.entryAppType)
                                }
                            })
                        rv?.adapter = listAdapter
                    } else {
                        // No applications - hide apps list view and show message
                        val rv: RecyclerView? = view?.findViewById(R.id.applicationsListView)
                        rv?.visibility = View.GONE
                        val tv = view?.findViewById<TextView>(R.id.noAppsMessageTextView)
                        tv?.visibility = View.VISIBLE
                    }
                }
            }
        }
    }
    //i moved to this function to get the array
    fun getApplications() : ArrayList<AppListNameItem>{
        val m_applicationFiles = Util.getApplicationsInDirectory(EngineInterface.getInstance().csEntryDirectory.path)
        val m_appNameListItems = ArrayList<AppListNameItem>()
        if (m_applicationFiles != null) {
            for (m_applicationFile: String? in m_applicationFiles) {
                if(m_applicationFile!=null) {
                    val pifFile = CNPifFile(m_applicationFile)
                    if (pifFile.IsValid() && pifFile.ShouldShowInApplicationListing(m_showHiddenApplications)) {
                        val appListItem = AppListNameItem(m_applicationFile, pifFile.GetDescription().trim(), pifFile.IsAppTypeEntry())
                        m_appNameListItems.add(appListItem)
                    }
                }
            }
        }
        return m_appNameListItems;
    }

    /**
     * RecyclerViewAdapter for application list
     */
    internal class ApplicationsListAdapter(private val m_items: ArrayList<AppListNameItem>,
                                           private val m_onItemClickListener: OnItemClickListener) : RecyclerView.Adapter<ApplicationsListAdapter.ViewHolder>() {
        internal interface OnItemClickListener {
            fun OnClick(item: AppListNameItem)
        }

        inner class ViewHolder(v: View) : RecyclerView.ViewHolder(v) {
            var m_textView: TextView

            init {
                v.setOnClickListener { onItemClick(adapterPosition) }
                m_textView = v.findViewById(android.R.id.text1)
            }
        }

        override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): ViewHolder {
            val v = LayoutInflater.from(parent.context)
                .inflate(R.layout.application_list_item_layout, parent, false)
            return ViewHolder(v)
        }

        override fun onBindViewHolder(holder: ViewHolder, position: Int) {
            holder.m_textView.text = m_items.get(position).description
        }

        override fun getItemCount(): Int {
            return m_items.size
        }

        fun onItemClick(position: Int) {
            val item = m_items[position]
            m_onItemClickListener.OnClick(item)
        }
    }

    companion object {
        val APP_DESCRIPTION_PARAM = "appDescription"
    }
}