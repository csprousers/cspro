#include "StdAfx.h"
#include "ClickableDictionaryTreeCtrl.h"


BEGIN_MESSAGE_MAP(ClickableDictionaryTreeCtrl, DictionaryTreeCtrl)
    ON_NOTIFY_REFLECT(TVN_KEYDOWN, OnKeyDown)
    ON_NOTIFY_REFLECT(NM_CLICK, OnClick)
END_MESSAGE_MAP()


namespace
{
    namespace CheckState
    {
        constexpr int Unchecked     = 1;
        constexpr int Checked       = 2;
        constexpr int Indeterminate = 3;
    }
}


ClickableDictionaryTreeCtrl::ClickableDictionaryTreeCtrl()
    :   m_allowSeparateSelectionOfOccurrenceLessEntryOfItemsWithOccurrences(false),
        m_clickableDictionaryTreeActionResponder(nullptr)
{
}


void ClickableDictionaryTreeCtrl::Initialize(std::shared_ptr<const CDataDict> dictionary, const bool include_ids, const bool include_value_sets,
                                             const bool allow_separate_selection_of_occurrence_less_entry_of_items_with_occurrences,
                                             ClickableDictionaryTreeActionResponder* dictionary_tree_action_responder)
{
    m_allowSeparateSelectionOfOccurrenceLessEntryOfItemsWithOccurrences = allow_separate_selection_of_occurrence_less_entry_of_items_with_occurrences;
    m_clickableDictionaryTreeActionResponder = dictionary_tree_action_responder;
    ASSERT(m_clickableDictionaryTreeActionResponder != nullptr);

    DictionaryTreeCtrl::Initialize(std::move(dictionary), include_ids, include_value_sets, dictionary_tree_action_responder);

    // load the state icons
    if( m_stateImageList == nullptr )
    {
        m_stateImageList = std::make_unique<CImageList>();
        m_stateImageList->Create(IDB_SELECTION_STATES, 16, 0, RGB(255, 255, 255));
        SetImageList(m_stateImageList.get(), TVSIL_STATE);
    }

    // update the tree's states
    UpdateStates(GetRootItem());
}


bool ClickableDictionaryTreeCtrl::IsCheckable(const DictionaryTreeNode& dictionary_tree_node) const
{
    bool checkable_item = ( dictionary_tree_node.GetDictElementType() == DictElementType::Item ||
                            dictionary_tree_node.GetDictElementType() == DictElementType::ValueSet );

    // an item may not be checkable if not allowing the separate selection of the occurrence-less node of items with occurrences
    if( checkable_item && !m_allowSeparateSelectionOfOccurrenceLessEntryOfItemsWithOccurrences )
    {
        const auto& item_occurrence_info = dictionary_tree_node.GetDictItemOccurrenceInfo();

        if( std::get<0>(item_occurrence_info)->GetItemSubitemOccurs() > 1 && !std::get<1>(item_occurrence_info).has_value() )
            checkable_item = false;
    }

    return checkable_item;
}


void ClickableDictionaryTreeCtrl::UpdateStates(const HTREEITEM hItem, std::shared_ptr<StateCounter> state_counter/* = nullptr*/)
{
    const DictionaryTreeNode* dictionary_tree_node = ( hItem != nullptr ) ? reinterpret_cast<const DictionaryTreeNode*>(GetItemData(hItem))  : nullptr;

    if( dictionary_tree_node == nullptr )
        return;

    if( state_counter == nullptr )
        state_counter = std::make_shared<StateCounter>(StateCounter { });

    // if this is a checkable node, determine if it is checked 
    if( IsCheckable(*dictionary_tree_node) )
    {
        bool checked = m_clickableDictionaryTreeActionResponder->IsDictionaryTreeNodeChecked(*dictionary_tree_node);

        ++state_counter->eligible;
        state_counter->checked += checked ? 1 : 0;

        const int state = checked ? CheckState::Checked :
                                    CheckState::Unchecked;
        SetItemState(hItem, INDEXTOSTATEIMAGEMASK(state), TVIS_STATEIMAGEMASK);
    }

    // otherwise process all child nodes
    else
    {
        auto initial_state_counter = std::make_shared<StateCounter>(*state_counter);

        HTREEITEM hChildItem = GetChildItem(hItem);

        while( hChildItem != nullptr )
        {
            auto children_state_counter = std::make_shared<StateCounter>(*initial_state_counter);

            UpdateStates(hChildItem, children_state_counter);

            state_counter->eligible += children_state_counter->eligible;
            state_counter->checked += children_state_counter->checked;

            hChildItem = GetNextItem(hChildItem, TVGN_NEXT);
        }

        const int state = ( state_counter->eligible == state_counter->checked ) ? CheckState::Checked :
                          ( state_counter->checked > 0 )                        ? CheckState::Indeterminate :
                                                                                  CheckState::Unchecked;
        SetItemState(hItem, INDEXTOSTATEIMAGEMASK(state), TVIS_STATEIMAGEMASK);
    }
}


void ClickableDictionaryTreeCtrl::OnKeyDown(NMHDR* pNMHDR, LRESULT* pResult)
{
    const NMTVKEYDOWN* ptvkd = reinterpret_cast<const NMTVKEYDOWN*>(pNMHDR);

    if( ptvkd->wVKey == VK_SPACE )
    {
        const HTREEITEM hItem = GetSelectedItem();

        if( hItem != nullptr )
            ProcessStateChange(hItem);
    }

    *pResult = 0;
}


void ClickableDictionaryTreeCtrl::OnClick(NMHDR* /*pNMHDR*/, LRESULT* pResult)
{
    const DWORD message_pos = GetMessagePos();
    CPoint message_point(LOWORD(message_pos), HIWORD(message_pos));
    ScreenToClient(&message_point);

    UINT hit_test_flags = 0;
    HTREEITEM hItem = HitTest(message_point, &hit_test_flags);

    if( hItem != nullptr && hit_test_flags == TVHT_ONITEMSTATEICON )
        ProcessStateChange(hItem);

    *pResult = 0;
}


void ClickableDictionaryTreeCtrl::ProcessStateChange(HTREEITEM hItem)
{
    ASSERT(hItem != nullptr);
    const int state = GetItemState(hItem, TVIS_STATEIMAGEMASK) >> 12;
    const bool new_checked_value = ( state == CheckState::Unchecked );

    std::function<void(HTREEITEM)> toggle_items = [&](HTREEITEM hThisItem)
    {
        const DictionaryTreeNode* dictionary_tree_node = reinterpret_cast<const DictionaryTreeNode*>(GetItemData(hThisItem));

        if( dictionary_tree_node == nullptr )
            return;

        // if this is a checkable node, toggle its state
        if( IsCheckable(*dictionary_tree_node) )
        {
            ASSERT(GetChildItem(hThisItem) == nullptr);
            m_clickableDictionaryTreeActionResponder->OnDictionaryTreeNodeCheck(*dictionary_tree_node, new_checked_value);
        }

        // otherwise toggle the state of all its children
        else
        {
            HTREEITEM hChildItem = GetChildItem(hThisItem);

            while( hChildItem != nullptr )
            {
                toggle_items(hChildItem);
                hChildItem = GetNextItem(hChildItem, TVGN_NEXT);
            }
        }
    };

    toggle_items(hItem);

    UpdateStates(GetRootItem());
}
