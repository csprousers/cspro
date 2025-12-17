#pragma once

#include <Stygitan/CodePurifierDoc.h>


class CodePurifierView : public CFormView
{
    DECLARE_DYNCREATE(CodePurifierView)

protected:
    CodePurifierView();

protected:
    DECLARE_MESSAGE_MAP()

    void OnInitialUpdate() override;
    void DoDataExchange(CDataExchange* pDX) override;

    LRESULT OnAppActivated(WPARAM wParam, LPARAM lParam);
    LRESULT OnUpdateUI(WPARAM wParam, LPARAM lParam);

    void OnWorkingDirectoryClick(NMHDR* pNMHDR, LRESULT* pResult);

    void OnCreateBranchCopy();
    void OnDeleteBranchCopies();

private:
    CodePurifierDoc& GetDoc() { return *assert_cast<CodePurifierDoc*>(GetDocument()); }

private:
    int64_t m_lastRefreshTime;
    CListBox m_branchCopiesListBox;
};
