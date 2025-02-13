#pragma once

#include <zInterfaceF/zInterfaceF.h>
#include <zInterfaceF/DictionaryTreeCtrl.h>


class CLASS_DECL_ZINTERFACEF ClickableDictionaryTreeCtrl : public DictionaryTreeCtrl
{
public:
    ClickableDictionaryTreeCtrl();

    void Initialize(std::shared_ptr<const CDataDict> dictionary, bool include_ids, bool include_value_sets,
                    bool allow_separate_selection_of_occurrence_less_entry_of_items_with_occurrences,
                    ClickableDictionaryTreeActionResponder* dictionary_tree_action_responder);

protected:
    DECLARE_MESSAGE_MAP()

    afx_msg void OnKeyDown(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnClick(NMHDR* pNMHDR, LRESULT* pResult);

private:
    bool IsCheckable(const DictionaryTreeNode& dictionary_tree_node) const;

    struct StateCounter
    {
        size_t eligible;
        size_t checked;
    };

    void UpdateStates(HTREEITEM hItem, std::shared_ptr<StateCounter> state_counter = nullptr);

    void ProcessStateChange(HTREEITEM hItem);

private:
    bool m_allowSeparateSelectionOfOccurrenceLessEntryOfItemsWithOccurrences;
    ClickableDictionaryTreeActionResponder* m_clickableDictionaryTreeActionResponder;

    std::unique_ptr<CImageList> m_stateImageList;
};

