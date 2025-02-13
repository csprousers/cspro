package gov.census.cspro.csentry.ui

import android.graphics.Rect
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.TextView
import androidx.recyclerview.widget.LinearLayoutManager
import androidx.recyclerview.widget.RecyclerView
import gov.census.cspro.csentry.R
import gov.census.cspro.csentry.ui.CaseTreeAdapter.BaseViewHolder
import gov.census.cspro.engine.Util
import gov.census.cspro.form.CaseTreeNode
import gov.census.cspro.form.CaseTreeUpdate
import java.util.*
import java.util.regex.Pattern

/**
 * RecyclerView adapter for displaying case tree
 */
class CaseTreeAdapter : RecyclerView.Adapter<BaseViewHolder>() {

    private var m_tree: CaseTreeNode? = null
    private val m_flatTree = ArrayList<FlatNode>()
    private var m_onClickFieldNodeListener: OnClickFieldNodeListener? = null
    private var m_onLongClickNodeListener: OnLongClickNodeListener? = null
    var m_showLabels = true
    var m_showSkippedFields = false
    private var m_recyclerView: RecyclerView? = null
    private var m_filter: Pattern? = null

    var tree: CaseTreeNode?
        get() = m_tree
        set(root) {
            m_tree = root
            onTreeChanged()
        }

    fun updateTree(updates: Array<CaseTreeUpdate>) {
        for (update in updates) {
            when (update.type) {
                CaseTreeUpdate.NODE_ADDED -> {
                    val parent = m_tree?.let{findNodeById(it, update.parentNodeId)}
                    parent?.insertChild(update.childIndex, update.node!!)
                }
                CaseTreeUpdate.NODE_MODIFIED -> {
                    val newNode = update.node
                    val currentNode = m_tree?.let { findNodeById(it, newNode!!.id)}
                    currentNode?.copyContent(newNode!!)
                }
                CaseTreeUpdate.NODE_REMOVED -> {
                    val parent = m_tree?.let { findNodeById(it, update.parentNodeId)}
                    parent?.removeChild(update.childIndex)
                }
            }
        }
        onTreeChanged()
    }

    override fun onAttachedToRecyclerView(recyclerView: RecyclerView) {
        m_recyclerView = recyclerView
    }

    override fun onDetachedFromRecyclerView(recyclerView: RecyclerView) {
        m_recyclerView = null
    }

    private fun findNodeById(node: CaseTreeNode, nodeId: Int): CaseTreeNode? {
        if (node.id == nodeId) {
            return node
        }
        for (child in node.children) {
            val foundDescendant = findNodeById(child, nodeId)
            if (foundDescendant != null) return foundDescendant
        }
        return null
    }

    private fun findCurrentNode(node: CaseTreeNode): CaseTreeNode? {
        if (node.color == CaseTreeNode.FRM_FIELDCOLOR_CURRENT) return node
        for (child in node.children) {
            val foundDescendant = findCurrentNode(child)
            if (foundDescendant != null) return foundDescendant
        }
        return null
    }

    private fun expandToNode(root: CaseTreeNode, nodeToExpand: CaseTreeNode): Boolean {
        if (root == nodeToExpand) return true
        var found = false
        for (child in root.children) found = found or expandToNode(child, nodeToExpand)
        if (m_filter != null && root.matchesFilter) found = true
        root.expanded = found
        return found
    }

    private fun scrollToNode(node: CaseTreeNode) {
        if (m_recyclerView == null) return
        val currentNodePos = findFlatPos(node)
        if (currentNodePos >= 0) {
            val layoutMgr = m_recyclerView?.layoutManager as LinearLayoutManager?
            val offset = centerOffset
            layoutMgr?.scrollToPositionWithOffset(currentNodePos, offset)
        }
    }

    private val centerOffset: Int
        get() {
            val r = Rect()
            m_recyclerView?.getGlobalVisibleRect(r)
            return r.height() / 2
        }

    private fun findFlatPos(node: CaseTreeNode): Int {
        for (i in m_flatTree.indices) {
            if (m_flatTree[i].m_treeNode == node) return i
        }
        return -1
    }

    /**
     * Redraw case tree
     */
    fun onTreeChanged() {
        val tree : CaseTreeNode = m_tree ?: return
        val currentNode = findCurrentNode(tree)
        if (currentNode != null) expandToNode(tree, currentNode)
        flatten()
        currentNode?.let { scrollToNode(it) }
    }

    fun search(searchText: String?) {
        if (Util.stringIsNullOrEmptyTrim(searchText)) {
            clearSearch()
        } else {
            m_filter = Pattern.compile(searchText, Pattern.CASE_INSENSITIVE or Pattern.LITERAL)
            m_tree?.let { filter(it) }
            onTreeChanged()
        }
    }

    fun clearSearch() {
        m_filter = null
        onTreeChanged()
    }

    private fun filter(node: CaseTreeNode): Boolean {
        var childMatches = false
        for (child in node.children) {
            childMatches = childMatches or filter(child)
        }
        val matches = childMatches || m_filter?.matcher(node.label)?.find() == true
        node.matchesFilter = matches
        return matches
    }

    interface OnClickFieldNodeListener {
        fun onClickFieldNode(node: CaseTreeNode?)
    }

    fun setOnClickFieldNodeListener(listener: OnClickFieldNodeListener?) {
        m_onClickFieldNodeListener = listener
    }

    interface OnLongClickNodeListener {
        fun onLongClickNode(node: CaseTreeNode?, view: View?)
    }

    fun setOnLongClickNodeListener(listener: OnLongClickNodeListener?) {
        m_onLongClickNodeListener = listener
    }

    private fun flatten() {
        m_flatTree.clear()
        m_tree?.let{
            // We don't create a node for the root, just the children of the root
            for (child in it.children) {
                flatten(child, 0, m_flatTree)
            }
        }
        notifyDataSetChanged()
    }

    private fun flatten(treeNode: CaseTreeNode, level: Int, flatList: MutableList<FlatNode>) {
        if (treeNode.visible && (m_filter == null || treeNode.matchesFilter)) {
            flatList.add(FlatNode(treeNode, level))
            if (treeNode.expanded) {
                val childLevel = level + 1
                for (child in treeNode.children) {
                    if (m_showSkippedFields || child.color != CaseTreeNode.FRM_FIELDCOLOR_SKIPPED) flatten(child, childLevel, flatList)
                }
            }
        }
    }

    class FlatNode internal constructor(var m_treeNode: CaseTreeNode, var m_level: Int)

    private fun onClickFieldNode(position: Int) {
        val node = m_flatTree[position]
        m_onClickFieldNodeListener?.onClickFieldNode(node.m_treeNode)
    }

    private fun onClickGroupNode(position: Int) {
        val node = m_flatTree[position]
        if (node.m_treeNode.children.size > 0) {
            if (node.m_treeNode.expanded) {
                collapseNode(node, position)
            } else {
                expandToNode(node, position)
            }
        }
    }

    private fun onLongClickNode(position: Int, view: View) {
        val node = m_flatTree[position]
        m_onLongClickNodeListener?.onLongClickNode(node.m_treeNode, view)
    }

    private fun collapseNode(node: FlatNode, position: Int) {
        val flatChildren: List<FlatNode> = ArrayList()
        node.m_treeNode.expanded = false
        notifyItemChanged(position)

        // Find all nodes at child level and above and remove them
        val firstChildPos = position + 1
        var lastChildPos = firstChildPos
        while (lastChildPos < m_flatTree.size && m_flatTree[lastChildPos].m_level > node.m_level) ++lastChildPos
        if (lastChildPos > firstChildPos) {
            m_flatTree.subList(firstChildPos, lastChildPos).clear()
            notifyItemRangeRemoved(firstChildPos, lastChildPos - firstChildPos)
        }
    }

    private fun expandToNode(node: FlatNode, position: Int) {
        val flatChildren: MutableList<FlatNode> = ArrayList()
        node.m_treeNode.expanded = true
        notifyItemChanged(position)
        val nextLevel = node.m_level + 1
        for (child in node.m_treeNode.children) {
            flatten(child, nextLevel, flatChildren)
        }
        m_flatTree.addAll(position + 1, flatChildren)
        notifyItemRangeInserted(position + 1, flatChildren.size)
    }

    override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): BaseViewHolder {
        return if (viewType == VIEW_TYPE_FIELD) {
            val view = LayoutInflater.from(parent.context).inflate(R.layout.case_tree_field_node, parent, false)
            FieldNodeViewHolder(view)
        } else {
            val view = LayoutInflater.from(parent.context).inflate(R.layout.case_tree_group_node, parent, false)
            GroupNodeViewHolder(view)
        }
    }

    override fun onBindViewHolder(holder: BaseViewHolder, position: Int) {
        holder.bind(m_flatTree[position])
    }

    override fun getItemCount(): Int {
        return m_flatTree.size
    }

    override fun getItemViewType(position: Int): Int {
        return if (m_flatTree[position].m_treeNode.type == CaseTreeNode.ITEM_FIELD) {
            VIEW_TYPE_FIELD
        } else {
            VIEW_TYPE_GROUP
        }
    }

    abstract class BaseViewHolder(itemView: View) : RecyclerView.ViewHolder(itemView) {
        abstract fun bind(node: FlatNode)
    }

    inner class FieldNodeViewHolder internal constructor(itemView: View) : BaseViewHolder(itemView) {
        private val m_textViewLabel: TextView
        private val m_textViewValue: TextView

        init {
            m_textViewLabel = itemView.findViewById(R.id.text_view_label)
            m_textViewValue = itemView.findViewById(R.id.text_view_value)
            itemView.setOnClickListener { if (adapterPosition != RecyclerView.NO_POSITION) onClickFieldNode(adapterPosition) }
            itemView.setOnLongClickListener { view ->
                if (adapterPosition != RecyclerView.NO_POSITION) onLongClickNode(adapterPosition, view)
                true
            }
        }

        override fun bind(node: FlatNode) {
            m_textViewLabel.text = if (m_showLabels) node.m_treeNode.label else node.m_treeNode.name

            // replaces newlines in the value with spaces
            m_textViewValue.text = node.m_treeNode.value.replace('\n', ' ')

            when (node.m_treeNode.color) {
                CaseTreeNode.FRM_FIELDCOLOR_NEVERVISITED -> itemView.setBackgroundColor(0xFFEFEFEF.toInt())
                CaseTreeNode.FRM_FIELDCOLOR_SKIPPED -> itemView.setBackgroundColor(0xFF696969.toInt())
                CaseTreeNode.FRM_FIELDCOLOR_VISITED -> itemView.setBackgroundColor(0xFFD3D3D3.toInt())
                CaseTreeNode.FRM_FIELDCOLOR_CURRENT -> itemView.setBackgroundColor(0xFFA1A2B3.toInt())
                CaseTreeNode.FRM_FIELDCOLOR_PROTECTED -> itemView.setBackgroundColor(0xFFC0C0C0.toInt())
            }
        }
    }

    inner class GroupNodeViewHolder internal constructor(itemView: View) : BaseViewHolder(itemView) {
        private val m_textViewLabel: TextView
        private val m_textViewOccurrences: TextView

        init {
            m_textViewLabel = itemView.findViewById(R.id.text_view_label)
            m_textViewOccurrences = itemView.findViewById(R.id.text_view_occurrences)
            itemView.setOnClickListener { if (adapterPosition != RecyclerView.NO_POSITION) onClickGroupNode(adapterPosition) }
            itemView.setOnLongClickListener { view ->
                if (adapterPosition != RecyclerView.NO_POSITION) onLongClickNode(adapterPosition, view)
                true
            }
        }

        override fun bind(node: FlatNode) {
            m_textViewLabel.text = if (m_showLabels) node.m_treeNode.label else node.m_treeNode.name
            if (node.m_treeNode.expanded) {
                m_textViewLabel.setCompoundDrawablesWithIntrinsicBounds(R.drawable.ic_expand_less_grey_24dp, 0, 0, 0)
            } else {
                m_textViewLabel.setCompoundDrawablesWithIntrinsicBounds(R.drawable.ic_expand_more_grey_24dp, 0, 0, 0)
            }
            if (node.m_treeNode.type == CaseTreeNode.ITEM_GROUP_PLACEHOLDER) {
                m_textViewOccurrences.visibility = View.VISIBLE
                m_textViewOccurrences.text = String.format(Locale.US, "%d", getVisibleOccurrences(node.m_treeNode))
            } else {
                m_textViewOccurrences.visibility = View.GONE
            }
        }

        private fun getVisibleOccurrences(node: CaseTreeNode): Int {
            return  node.children.count { it.visible}
        }

    }

    companion object {
        private const val VIEW_TYPE_GROUP = 1
        private const val VIEW_TYPE_FIELD = 2
    }
}