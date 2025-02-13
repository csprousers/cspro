#pragma once

#include <zIndexO/Indexer.h>
#include <zHtml/HtmlViewCtrl.h>
#include <zCaseO/CaseToHtmlConverter.h>


class DuplicateChooserDlg : public CDialog
{
public:
    DuplicateChooserDlg(std::vector<DuplicateInfo>& case_duplicates, size_t duplicate_index, size_t number_duplicates, CWnd* pParent = nullptr);

    enum { IDD = IDD_CSINDEX_DELETE_DIALOG };

protected:
    DECLARE_MESSAGE_MAP()

    void DoDataExchange(CDataExchange* pDX) override;
    BOOL OnInitDialog() override;

    void OnOK()  override;

    afx_msg void OnHelp();

    afx_msg void OnCaseListSelectionChange(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnCaseListClick(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnCaseListKeydown(NMHDR* pNMHDR, LRESULT* pResult);

private:
    void ProcessCaseListClick();

    std::vector<DuplicateInfo>& m_caseDuplicates;
    CaseToHtmlConverter m_caseToHtmlConverter;

    std::wstring m_duplicateIndexText;
    CTreeCtrl m_caseList;
    HtmlViewCtrl m_caseContentsHtml;
};
