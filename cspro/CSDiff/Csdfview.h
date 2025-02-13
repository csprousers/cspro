#pragma once

#include <zInterfaceF/ClickableDictionaryTreeCtrl.h>

class CCSDiffDoc;


class CCSDiffView : public CFormView, public ClickableDictionaryTreeActionResponder
{
    DECLARE_DYNCREATE(CCSDiffView)

public:
    enum { IDD = IDD_DIFF_FORM };

    CCSDiffView();

    const CCSDiffDoc* GetDocument() const { return assert_cast<const CCSDiffDoc*>(m_pDocument); }
    CCSDiffDoc* GetDocument()             { return assert_cast<CCSDiffDoc*>(m_pDocument); }

    static std::wstring CreateWindowTitle(const std::wstring& spec_filename, const CDataDict* dictionary);

protected:
    DECLARE_MESSAGE_MAP()

    void DoDataExchange(CDataExchange* pDX) override;
    void OnInitialUpdate() override;

    void OnSize(UINT nType, int cx, int cy);
    void OnToggle();
    void OnUpdateToggle(CCmdUI* pCmdUI);

    // ClickableDictionaryTreeActionResponder overrides
    bool IsDictionaryTreeNodeChecked(const DictionaryTreeNode& dictionary_tree_node) const override;
    void OnDictionaryTreeNodeCheck(const DictionaryTreeNode& dictionary_tree_node, bool checked) override;

private:
    ClickableDictionaryTreeCtrl m_dictionaryTreeCtrl;
};
