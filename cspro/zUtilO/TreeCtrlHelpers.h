#pragma once


class TreeCtrlHelpers
{
public:
    // Expands all nodes.
    static void ExpandAllNodes(CTreeCtrl& tree_ctrl);
    static void ExpandAllNodes(CTreeCtrl& tree_ctrl, HTREEITEM node);

    // Executes for callback function for every node in the tree.
    template<typename CF>
    static void ForeachNode(CTreeCtrl& tree_ctrl, const CF& callback_function);

    // Executes for callback function for the specified node, its children, and conditionally its siblings.
    template<typename CF>
    static void ForeachNode(CTreeCtrl& tree_ctrl, HTREEITEM hItem, bool include_siblings, const CF& callback_function);

    // The find callback should return true when the item is found.
    template<typename CF>
    static bool FindInTree(const CTreeCtrl& tree_ctrl, HTREEITEM hItem, bool include_children, const CF& find_callback);

    // Iterates over the tree, executing the callback function for visible nodes.
    template<typename CF>
    static void IterateOverVisibleNodes(const CTreeCtrl& tree_ctrl, HTREEITEM hItem, const CF& callback,
                                        bool assume_starting_item_is_visible = true);
};



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

inline void TreeCtrlHelpers::ExpandAllNodes(CTreeCtrl& tree_ctrl)
{
    ExpandAllNodes(tree_ctrl, tree_ctrl.GetRootItem());
}


inline void TreeCtrlHelpers::ExpandAllNodes(CTreeCtrl& tree_ctrl, const HTREEITEM node)
{
    if( node == nullptr )
        return;

    tree_ctrl.Expand(node, TVE_EXPAND);

    // expand child nodes
    HTREEITEM next_node = tree_ctrl.GetChildItem(node);
    ExpandAllNodes(tree_ctrl, next_node);

    // expand sibling nodes
    next_node = tree_ctrl.GetNextSiblingItem(node);

    while( next_node != nullptr )
    {
        tree_ctrl.Expand(next_node, TVE_EXPAND);
        next_node = tree_ctrl.GetNextItem(next_node, TVGN_NEXT);
    }
}


template<typename CF>
void TreeCtrlHelpers::ForeachNode(CTreeCtrl& tree_ctrl, const CF& callback_function)
{
    ForeachNode(tree_ctrl, tree_ctrl.GetRootItem(), true, callback_function);
}


template<typename CF>
void TreeCtrlHelpers::ForeachNode(CTreeCtrl& tree_ctrl, HTREEITEM hItem, const bool include_siblings, const CF& callback_function)
{
    while( hItem != nullptr )
    {
        callback_function(hItem);

        // process children
        ForeachNode(tree_ctrl, tree_ctrl.GetChildItem(hItem), true, callback_function);

        if( !include_siblings )
            return;

        // continue on to the next sibling
        hItem = tree_ctrl.GetNextSiblingItem(hItem);
    }
}


template<typename CF>
bool TreeCtrlHelpers::FindInTree(const CTreeCtrl& tree_ctrl, HTREEITEM hItem, const bool include_children, const CF& find_callback)
{
    while( hItem != nullptr )
    {
        if( find_callback(hItem) )
            return true;

        // traverse children
        if( include_children && FindInTree(tree_ctrl, tree_ctrl.GetChildItem(hItem), include_children, find_callback) )
            return true;

        // continue on to the next sibling
        hItem = tree_ctrl.GetNextSiblingItem(hItem);
    }

    return false;
}


template<typename CF>
void TreeCtrlHelpers::IterateOverVisibleNodes(const CTreeCtrl& tree_ctrl, HTREEITEM hItem, const CF& callback,
                                              const bool assume_starting_item_is_visible/* = true*/)
{
    while( hItem != nullptr )
    {
        callback(hItem);

        // traverse children
        if( assume_starting_item_is_visible || ( tree_ctrl.GetItemState(hItem, TVIS_EXPANDED) & TVIS_EXPANDED ) != 0 )
            IterateOverVisibleNodes(tree_ctrl, tree_ctrl.GetChildItem(hItem), callback, false);

        // continue on to the next sibling
        hItem = tree_ctrl.GetNextSiblingItem(hItem);
    }
}
