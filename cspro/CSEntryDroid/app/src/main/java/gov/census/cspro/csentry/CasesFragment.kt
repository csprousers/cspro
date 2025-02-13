package gov.census.cspro.csentry

import android.annotation.TargetApi
import android.app.Activity
import android.app.AlertDialog
import android.content.Context
import android.graphics.Color
import android.os.Bundle
import android.view.*
import android.widget.EditText
import android.widget.PopupMenu
import android.widget.SearchView
import android.widget.Toast
import androidx.appcompat.app.AppCompatActivity
import androidx.coordinatorlayout.widget.CoordinatorLayout
import androidx.fragment.app.Fragment
import androidx.lifecycle.Observer
import androidx.lifecycle.ViewModelProvider
import androidx.recyclerview.widget.LinearLayoutManager
import androidx.recyclerview.widget.RecyclerView
import com.google.android.material.floatingactionbutton.FloatingActionButton
import gov.census.cspro.data.CaseSummary
import gov.census.cspro.data.GetTrimmedKeyForDisplay
import gov.census.cspro.engine.*
import gov.census.cspro.maps.*

class CasesFragment : Fragment(), SearchView.OnQueryTextListener, IMapEventListener {
    private var m_listAdapter: CaseListAdapter? = null
    private var m_applicationName: String? = null
    private lateinit var m_searchMenuItem: MenuItem
    private var m_listener: OnFragmentInteractionListener? = null
    private var m_viewModel: CasesViewModel? = null
    private var m_casesListView: RecyclerView? = null

    /**
     * This interface must be implemented by activities that contain this
     * fragment to allow an interaction in this fragment to be communicated
     * to the activity and potentially other fragments contained in that
     * activity.
     *
     *
     * See the Android Training lesson [Communicating with Other Fragments](http://developer.android.com/training/basics/fragments/communicating.html) for more information.
     */
    internal interface OnFragmentInteractionListener {
        fun onAddCase()
        fun onViewCase(caseSummary: CaseSummary?)
        fun onInsertCase(caseSummary: CaseSummary?)
        fun onModifyCase(caseSummary: CaseSummary?)
    }

    @TargetApi(23)
    override fun onAttach(context: Context) {
        super.onAttach(context)
        attachToContext(context)
    }

    @Deprecated("Deprecated in Java")
    override fun onAttach(activity: Activity) {
        super.onAttach(activity)
        attachToContext(activity)
    }

    private fun attachToContext(context: Context) {
        if (context is OnFragmentInteractionListener) {
            m_listener = context
        } else {
            throw RuntimeException(context.toString()
                + " must implement CasesFragment.OnFragmentInteractionListener")
        }
    }

    override fun onDetach() {
        super.onDetach()
        m_listener = null
    }

    override fun onCreateView(inflater: LayoutInflater, container: ViewGroup?, savedInstanceState: Bundle?): View? {
        m_applicationName = EngineInterface.getInstance().applicationDescription
        val fragmentsView: View = inflater.inflate(R.layout.fragment_caseslist_layout,
            container,
            false)
        setHasOptionsMenu(true)
        val fab: FloatingActionButton = fragmentsView.findViewById(R.id.fab)
        if (EngineInterface.getInstance().addLockFlag) {
            // Hide floating action button for adding new case
            fab.hide()
        } else {
            fab.show()
            fab.setOnClickListener { addCase() }
        }
        m_listAdapter = CaseListAdapter()
        m_casesListView = fragmentsView.findViewById(R.id.cases_listview)
        m_casesListView?.layoutManager = LinearLayoutManager(context)
        m_casesListView?.adapter = m_listAdapter
        m_listAdapter?.setItemOnClickListener(object : CaseListAdapter.OnItemClickListener {
            override fun OnClick(caseSummary: CaseSummary, view: View?) {
                modifyCase(caseSummary)
            }
        })
        m_listAdapter?.setItemOnLongClickListener(object : CaseListAdapter.OnItemClickListener {
            override fun OnClick(caseSummary: CaseSummary, view: View?) {
                val popup = PopupMenu(context, view)
                popup.setOnMenuItemClickListener(CaseItemPopupClickListener(caseSummary))
                popup.inflate(R.menu.menu_cases_list_popup)
                val caseId: String? = m_viewModel?.showCaseLabels?.let { caseSummary.GetTrimmedKeyForDisplay(it) }
                val menu: Menu = popup.menu

                // view
                if (EngineInterface.getInstance().viewLockFlag) {
                    menu.removeItem(R.id.menu_cases_list_view_questionnaire) 
                } else { 
                    menu.findItem(R.id.menu_cases_list_view_questionnaire).title = String.format(getString(R.string.menu_cases_list_view_questionnaire), caseId)
                }

                // delete
                if (EngineInterface.getInstance().deleteLockFlag) {
                    menu.removeItem(R.id.menu_cases_list_delete_case) 
                } else {
                    menu.findItem(R.id.menu_cases_list_delete_case).title = String.format(getString(R.string.menu_cases_list_delete_case), caseId)
                }

                // insert
                if (EngineInterface.getInstance().addLockFlag) {
                    menu.removeItem(R.id.menu_cases_list_insert_case) 
                } else { 
                    menu.findItem(R.id.menu_cases_list_insert_case).title = String.format(getString(R.string.menu_cases_list_insert_case), caseId)
                }

                popup.show()
            }
        })
        m_viewModel = activity?.let { ViewModelProvider(it).get(CasesViewModel::class.java) }
        m_viewModel?.cases?.observe(viewLifecycleOwner) {
            it?.let {cases->
                m_listAdapter?.setCases(cases)
                updateTitleInformation(cases.size)
            }

        }
        if (m_viewModel?.mappingOptions?.isMappingEnabled == true) {
            m_viewModel?.mapLiveData?.observe(viewLifecycleOwner, Observer { mapData: MapData? ->
                this.mapFragment?.setData(mapData)
            })
            // Initially the fragment layout just has a frame layout as a placeholder
            // for the map so they we don't load the map when mapping is not enabled.
            // This swaps out the empty frame layout for the actual MapFragment.
            childFragmentManager.beginTransaction()
                .replace(R.id.map_fragment, MapFragment())
                .commitNow()
            mapFragment?.setEventListener(this)
        } else {
            // No mapping - hide the map fragment
            fragmentsView.findViewById<View>(R.id.map_fragment).visibility = View.GONE

            // Remove the bottom sheet behavior so list is displayed normally rather
            // than sliding out from bottom of map
            m_casesListView?.layoutParams?.let {
                if (it is CoordinatorLayout.LayoutParams) it.behavior = null
            }
        }
        return fragmentsView
    }

    @Deprecated("Deprecated in Java")
    override fun onCreateOptionsMenu(menu: Menu, inflater: MenuInflater) {
        super.onCreateOptionsMenu(menu, inflater)
        inflater.inflate(R.menu.menu_cases_list, menu)

        // setup the searchview

        // noinspection ConstantConditions
        val searchView = SearchView((activity as AppCompatActivity?)?.supportActionBar?.themedContext)
        m_searchMenuItem = menu.findItem(R.id.menuitem_cases_action_search)
        m_searchMenuItem.actionView = searchView

        // set the search text color
        val searchEditTextId: Int = searchView.context.resources.getIdentifier("android:id/search_src_text", null, null)
        val searchEditText: EditText = searchView.findViewById(searchEditTextId)
        searchEditText.setTextColor(Color.LTGRAY)
        searchView.setOnQueryTextListener(this)
        menu.findItem(R.id.menuitem_cases_sync).isVisible = EngineInterface.getInstance().HasSync()
        menu.findItem(R.id.menuitem_cases_help).isVisible = EngineInterface.GetSystemSettingBoolean(SystemSettings.MenuHelp, true)
    }

    @Deprecated("Deprecated in Java")
    override fun onOptionsItemSelected(item: MenuItem): Boolean {
        when (item.itemId) {
            R.id.menuitem_cases_sort -> {
                item.isChecked = !item.isChecked
                m_viewModel?.showAlphabetically = item.isChecked
                m_viewModel?.showAlphabetically?.let { item.setChecked(it) }
                return true
            }
            R.id.menuitem_cases_partial_saves -> {
                item.isChecked = !item.isChecked
                m_viewModel?.setShowOnlyPartials(item.isChecked)
                return true
            }
            R.id.menuitem_cases_case_labels -> {
                val showLabels: Boolean = !item.isChecked
                m_viewModel?.showCaseLabels = showLabels
                item.isChecked = showLabels
                m_listAdapter?.setShowLabels(showLabels)
                return true
            }
            R.id.menuitem_cases_sync -> {
                syncApp()
                return true
            }
            R.id.menuitem_cases_help -> {
                activity?.let { SystemSettings.LaunchHelp(it) }
                return true
            }
            else -> return super.onOptionsItemSelected(item)
        }
    }

    override fun onQueryTextChange(newText: String): Boolean {

        m_viewModel?.setSearchText(newText)

        return m_viewModel?.cases?.value != null && m_viewModel?.cases?.value?.isNotEmpty() == true

    }

    override fun onQueryTextSubmit(query: String): Boolean {
        m_searchMenuItem.collapseActionView()
        return true
    }

    override fun onResume() {
        super.onResume()
        m_viewModel?.refreshCases()
    }

    private fun deleteCase(caseSummary: CaseSummary) {
        val trimmedKey: String? = m_viewModel?.showCaseLabels?.let { caseSummary.GetTrimmedKeyForDisplay(it) }
        val builder: AlertDialog.Builder = AlertDialog.Builder(activity)
        builder.setMessage(String.format(getString(R.string.menu_cases_list_delete_case_prompt), trimmedKey))
            .setPositiveButton(getString(R.string.modal_dialog_helper_yes_text)) { _, _ ->
                val message: String = if (EngineInterface.getInstance().deleteCase(caseSummary.positionInRepository)) {
                    m_viewModel?.refreshCases()
                    String.format(getString(R.string.menu_cases_list_delete_case_success), trimmedKey)
                } else String.format(getString(R.string.menu_cases_list_delete_case_error), trimmedKey)
                Toast.makeText(activity, message, Toast.LENGTH_LONG).show()
            }
            .setNegativeButton(getString(R.string.modal_dialog_helper_no_text), null)
            .show()
    }

    private fun updateTitleInformation(caseCount: Int) {
        val title: String = String.format(if (caseCount == 1) getString(R.string.title_case_listing_single) else getString(R.string.title_case_listing_plural), m_applicationName, caseCount)
        activity?.title = title
    }

    internal inner class CaseItemPopupClickListener constructor(private val m_caseSummary: CaseSummary) : PopupMenu.OnMenuItemClickListener {
        override fun onMenuItemClick(item: MenuItem): Boolean {
            return when (item.itemId) {
                R.id.menu_cases_list_view_questionnaire -> {
                    viewCase(m_caseSummary)
                    true
                }
                R.id.menu_cases_list_insert_case -> {
                    insertCase(m_caseSummary)
                    true
                }
                R.id.menu_cases_list_delete_case -> {
                    deleteCase(m_caseSummary)
                    true
                }
                else -> false
            }
        }
    }

    private fun addCase() {
        saveMapCamera()
        if (m_listener != null) m_listener?.onAddCase()
    }

    private fun viewCase(caseSummary: CaseSummary) {
        saveMapCamera()
        if (m_listener != null) m_listener?.onViewCase(caseSummary)
    }

    private fun insertCase(caseSummary: CaseSummary) {
        saveMapCamera()
        if (m_listener != null) m_listener?.onInsertCase(caseSummary)
    }

    private fun modifyCase(caseSummary: CaseSummary?) {
        saveMapCamera()
        if (m_listener != null) m_listener?.onModifyCase(caseSummary)
    }

    private fun saveMapCamera() {
        val map: MapFragment? = mapFragment
        if (map != null) m_viewModel?.setMapCamera(map.cameraPosition)
    }

    private fun syncApp() {
        class SyncAppCompletedListener : IEngineMessageCompletedListener {
            override fun onMessageCompleted(msg: EngineMessage) {
                if (msg.result != 0L) {
                    m_viewModel?.refreshCases()
                    showAppSyncCompleted()
                }
            }
        }

        val msg: EngineMessage = object : EngineMessage(activity, SyncAppCompletedListener()) {
            override fun run() {
                val result: Boolean = EngineInterface.getInstance().SyncApp()
                setResult(if (result) 1 else 0.toLong())
            }
        }
        Messenger.getInstance().sendMessage(msg)
    }

    private fun showAppSyncCompleted() {
        context?.let {
            //fu android sutdio
            Toast.makeText(it, String.format(it.getString(R.string.sync_success), ""), Toast.LENGTH_LONG).show()
        }
    }

    private val mapFragment: MapFragment?
        get() {
            return childFragmentManager.findFragmentById(R.id.map_fragment) as MapFragment?
        }

    override fun onMapEvent(e: MapEvent): Boolean {

        when (e.eventCode) {
            MapEvent.MARKER_CLICKED ->
            {
                // for marker click scroll the list view to show the marker
                (m_casesListView?.layoutManager as LinearLayoutManager?)?.scrollToPositionWithOffset(e.markerId, 0)
                return false
            }

            MapEvent.MARKER_INFO_WINDOW_CLICKED ->
            {
                // Marker info window launches modify case
                val cases: List<CaseSummary>? = m_viewModel?.cases?.value
                if (cases != null) modifyCase(cases[e.markerId])
                    return true
            }
        }
        return false
    }

    companion object {
        fun newInstance(): Fragment {
            val fragment = CasesFragment()
            val args = Bundle()
            fragment.arguments = args
            return fragment
        }
    }
}