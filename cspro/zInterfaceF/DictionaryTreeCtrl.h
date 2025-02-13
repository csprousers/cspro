#pragma once

#include <zInterfaceF/zInterfaceF.h>
#include <zInterfaceF/DictionaryTreeActionResponder.h>
#include <zInterfaceF/DictionaryTreeNode.h>


class CLASS_DECL_ZINTERFACEF DictionaryTreeCtrl : public CTreeCtrl
{
public:
    DictionaryTreeCtrl();
    virtual ~DictionaryTreeCtrl();

    void Initialize(std::shared_ptr<const CDataDict> dictionary, bool include_ids, bool include_value_sets,
                    DictionaryTreeActionResponder* dictionary_tree_action_responder);

protected:
    DECLARE_MESSAGE_MAP()

    afx_msg void OnGetDisplayInfo(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnSelectionChanged(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnDoubleClick(NMHDR* pNMHDR, LRESULT* pResult);

private:
    void BuildTree();

private:
    std::shared_ptr<const CDataDict> m_dictionary;
    bool m_includeIds;
    bool m_includeValueSets;
    DictionaryTreeActionResponder* m_dictionaryTreeActionResponder;

    std::unique_ptr<CImageList> m_imageList;
    std::vector<std::shared_ptr<DictionaryTreeNode>> m_dictionaryTreeNodes;

public:
    bool show_binary_items = false; // BINARY_TYPES_TO_ENGINE_TODO for 8.0, binary items aren't generally added to the tree
};
