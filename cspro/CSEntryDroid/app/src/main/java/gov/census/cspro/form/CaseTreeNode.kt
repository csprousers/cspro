package gov.census.cspro.form

import java.util.*

class CaseTreeNode(var id: Int,
                   var name: String,
                   var label: String,
                   var value: String,
                   var type: Int,
                   var color: Int,
                   var fieldSymbol: Int,
                   var fieldIndex: IntArray,
                   visible: Boolean) {
    private val m_children: MutableList<CaseTreeNode>
    var visible: Boolean
    var expanded: Boolean
    var matchesFilter: Boolean

    // called from C++
    fun addChild(node: CaseTreeNode) {
        m_children.add(node)
    }

    fun insertChild(childIndex: Int, node: CaseTreeNode) {
        m_children.add(childIndex, node)
    }

    fun removeChild(childIndex: Int) {
        m_children.removeAt(childIndex)
    }

    val children: List<CaseTreeNode>
        get() = m_children

    fun copyContent(newNode: CaseTreeNode) {
        id = newNode.id
        name = newNode.name
        label = newNode.label
        value = newNode.value
        type = newNode.type
        color = newNode.color
        fieldSymbol = newNode.fieldSymbol
        fieldIndex = newNode.fieldIndex
        expanded = true
        visible = newNode.visible
    }

    val isCurrentGroup: Boolean
        get() {
            for (c in m_children) {
                if (c.type == ITEM_FIELD) {
                    if (c.color == FRM_FIELDCOLOR_CURRENT) return true
                }
                if (c.isCurrentGroup) return true
            }
            return false
        }

    companion object {
        // called from C++
        const val ITEM_LEVEL = 0

        // called from C++
        const val ITEM_GROUP = 1
        const val ITEM_GROUP_PLACEHOLDER = 2
        const val ITEM_GROUP_OCCURRENCE = 3
        const val ITEM_FIELD = 4

        // called from C++
        const val FRM_FIELDCOLOR_PARENT = -1 //only valid for fields
        const val FRM_FIELDCOLOR_NEVERVISITED = 0
        const val FRM_FIELDCOLOR_SKIPPED = 1
        const val FRM_FIELDCOLOR_VISITED = 2
        const val FRM_FIELDCOLOR_CURRENT = 3
        const val FRM_FIELDCOLOR_PROTECTED = 4
    }

    init {
        m_children = ArrayList()
        expanded = false
        matchesFilter = false
        this.visible = visible
    }
}