package gov.census.cspro.form

data class CaseTreeUpdate(
    val type: Int,
    val node: CaseTreeNode?,
    val parentNodeId: Int,
    val childIndex: Int) {

    companion object {
        const val NODE_MODIFIED = 1
        const val NODE_ADDED = 2
        const val NODE_REMOVED = 3
    }
}