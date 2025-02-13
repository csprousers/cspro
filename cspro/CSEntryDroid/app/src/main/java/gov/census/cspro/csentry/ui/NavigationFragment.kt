package gov.census.cspro.csentry.ui

import android.annotation.TargetApi
import android.app.Activity
import android.content.Context
import android.content.SharedPreferences
import android.os.Bundle
import android.view.*
import android.widget.PopupMenu
import android.widget.SearchView
import androidx.fragment.app.Fragment
import androidx.recyclerview.widget.DividerItemDecoration
import androidx.recyclerview.widget.LinearLayoutManager
import androidx.recyclerview.widget.RecyclerView
import gov.census.cspro.csentry.R
import gov.census.cspro.csentry.SystemSettings
import gov.census.cspro.csentry.ui.EntryEngineMessage.EntryMessageRequestType
import gov.census.cspro.engine.EngineInterface
import gov.census.cspro.engine.EngineMessage
import gov.census.cspro.engine.IEngineMessageCompletedListener
import gov.census.cspro.engine.Messenger
import gov.census.cspro.form.CaseTreeNode
import gov.census.cspro.form.CaseTreeUpdate
import gov.census.cspro.form.EntryPage
import gov.census.cspro.form.OnFormNavigatedListener

class NavigationFragment : Fragment(), OnFormNavigatedListener, SearchView.OnQueryTextListener {

    private lateinit var m_caseTreeAdapter: CaseTreeAdapter
    private var m_enabled: Boolean = true
    private var m_buttonNavigationListener: OnNavigationButtonClickedListener? = null

    fun enable() {
        m_enabled = true
    }

    fun disable() {
        m_enabled = false
    }

    override fun onCreateView(inflater: LayoutInflater, container: ViewGroup?, savedInstanceState: Bundle?): View {
        val view: View = inflater.inflate(R.layout.fragment_navigation_layout, container, false)
        val caseTreeRecyclerView: RecyclerView = view.findViewById(R.id.case_tree_recycler_view)
        m_caseTreeAdapter = CaseTreeAdapter()
        m_caseTreeAdapter.setOnClickFieldNodeListener(object: CaseTreeAdapter.OnClickFieldNodeListener{
            override fun onClickFieldNode(node: CaseTreeNode?) {
                if (m_enabled && node != null) {
                    m_buttonNavigationListener?.onFieldItemClicked(
                        node.fieldSymbol,
                        node.fieldIndex[0],
                        node.fieldIndex[1],
                        node.fieldIndex[2])
                }
            }
        })
        m_caseTreeAdapter.setOnLongClickNodeListener(object: CaseTreeAdapter.OnLongClickNodeListener {
            override fun onLongClickNode(node: CaseTreeNode?, view: View?) {
                if (m_enabled && node!=null && view!=null) {
                    onItemLongClick(node, view)
                }
            }

        })
        if (activity != null) {
            val sharedPref: SharedPreferences = requireActivity().getPreferences(Context.MODE_PRIVATE)
            val showLabelsStateProperty: String = getString(R.string.menu_casetree_save_show_labels_state)
            m_caseTreeAdapter.m_showLabels = sharedPref.getBoolean(showLabelsStateProperty, true)

            // Only hide skipped fields in system control mode. In operator control mode hiding
            // skipped fields causes items to be hidden as you move back due to different way
            // nodes are colored in op control.
            if (EngineInterface.getInstance().isSystemControlled) {
                val showSkippedStateProperty: String = getString(R.string.menu_casetree_save_show_skipped_state)
                m_caseTreeAdapter.m_showSkippedFields = sharedPref.getBoolean(showSkippedStateProperty, false)
            } else {
                m_caseTreeAdapter.m_showSkippedFields = true
            }
        }

        caseTreeRecyclerView.adapter = m_caseTreeAdapter
        val layoutManager = LinearLayoutManager(requireContext())
        caseTreeRecyclerView.layoutManager = layoutManager
        val decoration = DividerItemDecoration(requireContext(), layoutManager.orientation)
        decoration.setDrawable(resources.getDrawable(R.drawable.case_tree_divider))
        caseTreeRecyclerView.addItemDecoration(decoration)

        // wireup event handling for the search view
        val searchView: SearchView = view.findViewById(R.id.searchview_casetree)
        searchView.setOnQueryTextListener(this)
        return view
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
        if (context is EntryActivity) {
            val activity: EntryActivity = context
            activity.addOnFormNavigatedListener(this)
            m_buttonNavigationListener = activity
        } else {
            throw RuntimeException("Expected to attach to EntryActivity but was attached to $context")
        }
    }

    override fun onDetach() {
        (requireActivity() as EntryActivity).removeOnFormNavigatedListener(this)
        super.onDetach()
    }

    /**
     * Full update of case tree
     */
    fun populateCaseTree() {
        class GetCaseTreeMessage(activity: Activity?, completedListener: IEngineMessageCompletedListener) : EngineMessage(activity, completedListener) {
            var tree: CaseTreeNode? = null
            override fun run() {
                tree = EngineInterface.getInstance().caseTree
            }
        }

        val msg: EngineMessage = GetCaseTreeMessage(activity) { msg ->
            val getMsg: GetCaseTreeMessage = msg as GetCaseTreeMessage
            if (getMsg.tree != null) {
                m_caseTreeAdapter.tree = getMsg.tree
            }
        }
        val activity: EntryActivity? = activity as EntryActivity?
        if (activity != null && activity.isCasetreeVisible) {
            Messenger.getInstance().sendMessage(msg)
        }
    }

    /**
     * Partial update of case tree
     */
    private fun updateCaseTree() {
        if (m_caseTreeAdapter.tree == null) {
            // Need to do full update
            populateCaseTree()
            return
        }
        class UpdateCaseTreeMessage(activity: Activity?, completedListener: IEngineMessageCompletedListener) : EngineMessage(activity, completedListener) {
            var updates: Array<CaseTreeUpdate>? = null
            override fun run() {
                updates = EngineInterface.getInstance().updateCaseTree()
            }
        }

        val msg: EngineMessage = UpdateCaseTreeMessage(activity) { msg ->
            val updateMsg: UpdateCaseTreeMessage = msg as UpdateCaseTreeMessage
            updateMsg.updates?.let {
                m_caseTreeAdapter.updateTree(it)
            }
        }
        val activity: EntryActivity? = activity as EntryActivity?
        if (activity != null && activity.isCasetreeVisible) {
            Messenger.getInstance().sendMessage(msg)
        }
    }

    fun setNavigationButtonClickedListener(navigationButtonListener: OnNavigationButtonClickedListener?) {
        m_buttonNavigationListener = navigationButtonListener
    }

    interface OnNavigationButtonClickedListener {
        fun onNavigationNextButtonClicked()
        fun onNavigationPreviousButtonClicked()
        fun onFieldItemClicked(fieldSymbol: Int, index1: Int, index2: Int, index3: Int)
    }

    private fun onItemLongClick(node: CaseTreeNode, view: View) {
        val popup = PopupMenu(context, view)
        popup.inflate(R.menu.menu_casetree_list_popup)
        val menu: Menu = popup.menu
        val namesOrLabels: Int = if (m_caseTreeAdapter.m_showLabels) R.string.menu_casetree_list_show_names else R.string.menu_casetree_list_show_labels
        menu.findItem(R.id.menu_casetree_list_show_labels).title = getString(namesOrLabels)

        //show or hide skipped fields
        if (EngineInterface.getInstance().isSystemControlled) {
            if (activity != null) {
                val skippedMenuItem: MenuItem = menu.findItem(R.id.menu_casetree_list_hide_skipped)
                val sharedPref: SharedPreferences = requireActivity().getPreferences(Context.MODE_PRIVATE)
                val showSkippedStateProperty: String = getString(R.string.menu_casetree_save_show_skipped_state)
                val showSkippedState: Boolean = sharedPref.getBoolean(showSkippedStateProperty, false)
                val showSkippedMenu: Int = if (showSkippedState) R.string.menu_casetree_list_hide_skipped else R.string.menu_casetree_list_show_skipped
                skippedMenuItem.title = getString(showSkippedMenu)
            }
        } else {
            // Don't show "show skipped items" entry in op controlled mode since we always show
            // skipped items in op control.
            menu.removeItem(R.id.menu_casetree_list_hide_skipped)
        }
        var showAddInsertOcc = false
        var showDeleteOcc = false
        if (node.type == CaseTreeNode.ITEM_GROUP_OCCURRENCE && node.isCurrentGroup) {
            showAddInsertOcc = EngineInterface.GetSystemSettingBoolean(SystemSettings.MenuAddInsertOccurrence, true)
            showDeleteOcc = EngineInterface.GetSystemSettingBoolean(SystemSettings.MenuDeleteOccurrence, true)
        }
        if (!showAddInsertOcc) {
            menu.removeItem(R.id.menu_casetree_list_add_occ)
            menu.removeItem(R.id.menu_casetree_list_insert_occ)
        }
        if (!showDeleteOcc) {
            menu.removeItem(R.id.menu_casetree_list_delete_occ)
        }
        popup.setOnMenuItemClickListener(object : PopupMenu.OnMenuItemClickListener {
            override fun onMenuItemClick(item: MenuItem): Boolean {
                val activity: EntryActivity? = activity as EntryActivity?
                if (activity != null) {
                    when (item.itemId) {
                        R.id.menu_casetree_list_show_labels -> {
                            ShowLabels()
                            return true
                        }
                        R.id.menu_casetree_list_hide_skipped -> {
                            ShowOrHideSkippedFields()
                            return true
                        }
                        R.id.menu_casetree_list_insert_occ -> {
                            activity.initiateFieldMovement(EntryMessageRequestType.INSERT_OCC)
                            return true
                        }
                        R.id.menu_casetree_list_delete_occ -> {
                            activity.initiateFieldMovement(EntryMessageRequestType.DELETE_OCC)
                            return true
                        }
                        R.id.menu_casetree_list_add_occ -> {
                            activity.initiateFieldMovement(EntryMessageRequestType.INSERT_OCC_AFTER)
                            return true
                        }
                    }
                }
                return false
            }
        })
        popup.show()
    }

    fun ShowLabels() {
        // toggle the state to prefs
        m_caseTreeAdapter.m_showLabels = !m_caseTreeAdapter.m_showLabels
        m_caseTreeAdapter.notifyDataSetChanged()
        if (activity != null) {
            val sharedPref: SharedPreferences = requireActivity().getPreferences(Context.MODE_PRIVATE)
            val showLabelsStateProperty: String = getString(R.string.menu_casetree_save_show_labels_state)
            val editor: SharedPreferences.Editor = sharedPref.edit()
            m_caseTreeAdapter.m_showLabels.let { editor.putBoolean(showLabelsStateProperty, it) }
            editor.apply()
        }
    }

    fun ShowOrHideSkippedFields() {
        // Only hide skipped fields in system control mode
        if (EngineInterface.getInstance().isSystemControlled) {
            // toggle the state to prefs
            m_caseTreeAdapter.m_showSkippedFields = !m_caseTreeAdapter.m_showSkippedFields
            if (activity != null) {
                val sharedPref: SharedPreferences = requireActivity().getPreferences(Context.MODE_PRIVATE)
                val showSkippedStateProperty: String = getString(R.string.menu_casetree_save_show_skipped_state)
                val editor: SharedPreferences.Editor = sharedPref.edit()
                m_caseTreeAdapter.m_showSkippedFields.let { editor.putBoolean(showSkippedStateProperty, it) }
                editor.apply()
            }
            m_caseTreeAdapter.onTreeChanged()
        }
    }

    override fun onFormNavigated(page: EntryPage) {
        //Log.d("NavigationForm", "Next Field Navigated.");
        val activity: EntryActivity? = activity as EntryActivity?
        if (activity != null && activity.isCasetreeVisible) updateCaseTree()
    }

    override fun onQueryTextChange(newText: String): Boolean {
        if (newText.isEmpty()) {
            m_caseTreeAdapter.clearSearch()
        }
        return false
    }

    override fun onQueryTextSubmit(newText: String): Boolean {
        m_caseTreeAdapter.search(newText)
        return true
    }
}