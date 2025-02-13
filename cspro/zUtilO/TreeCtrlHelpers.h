#pragma once


namespace TreeCtrlHelpers
{
    // the find callback should return true when the item is found
    template<typename CF>
    bool FindInTree(const CTreeCtrl& tree_ctrl, HTREEITEM hItem, CF find_callback)
    {
        while( hItem != nullptr )
        {
            if( find_callback(hItem) )
                return true;

            // traverse children
            if( FindInTree(tree_ctrl, tree_ctrl.GetChildItem(hItem), find_callback) )
                return true;

            // continue on to the next sibling
            hItem = tree_ctrl.GetNextSiblingItem(hItem);
        }

        return false;
    }


    // iterates over the tree, executing the callback function for visible nodes
    template<typename CF>
    void IterateOverVisibleNodes(const CTreeCtrl& tree_ctrl, HTREEITEM hItem, CF callback,
                                 bool assume_starting_item_is_visible = true)
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
}
