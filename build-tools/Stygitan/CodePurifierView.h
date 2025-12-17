#pragma once

#include <Stygitan/CodePurifierDoc.h>
#include <zUtilF/SortListCtrl.h>


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

    void OnCommitsCustomDraw(NMHDR* pNMHDR, LRESULT* pResult);
    void OnCommitsRightClick(NMHDR* pNMHDR, LRESULT* pResult);
    void OnSetCleanCommit();

private:
    CodePurifierDoc& GetDoc() { return *assert_cast<CodePurifierDoc*>(GetDocument()); }

    void RefreshDataAndUpdateUI(WPARAM wParam);

private:
    int64_t m_lastFullRefreshTime;
    CListBox m_branchCopiesListBox;
    CSortListCtrl m_commitsListCtrl;
    int m_cleanCommitIndex;
};
