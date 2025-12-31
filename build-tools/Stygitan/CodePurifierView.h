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
    void OnActivateView(BOOL bActivate, CView* pActivateView, CView* pDeactiveView) override;
    void OnDestroy();

    LRESULT OnUpdateUI(WPARAM wParam, LPARAM lParam);

    void UpdateBranchDetails();
    void UpdateBranchCopies();
    void UpdateCleanCommit();
    void UpdateRecentCommits();
    void UpdateModifiedFiles();

    void OnWorkingDirectoryClick(NMHDR* pNMHDR, LRESULT* pResult);

    void OnCreateBranchCopy();
    void OnDeleteBranchCopies();

    void OnCommitsCustomDraw(NMHDR* pNMHDR, LRESULT* pResult);
    void OnCommitsRightClick(NMHDR* pNMHDR, LRESULT* pResult);
    void OnSetCleanCommit();

    void OnResetBranchToCleanCommit();
    void OnCreateBranchCopyBeforeResetClick();

    void OnModifiedFilesDoubleOrRightClick(NMHDR* pNMHDR, LRESULT* pResult);
    void OnModifiedFileOpen();
    void OnModifiedFileOpenContainingFolder();
    void OnModifiedFileCopyPath();

private:
    CodePurifierDoc& GetDoc() { return *assert_cast<CodePurifierDoc*>(GetDocument()); }

    template<typename CF>
    void OnModifiedFile(const CF& callback_function);

private:
    SettingsDb m_settingsDb;

    CListBox m_branchCopiesListBox;
    CSortListCtrl m_commitsListCtrl;
    bool m_createBranchCopyBeforeReset;
    CSortListCtrl m_modifiedFilesListCtrl;

    std::shared_ptr<const std::vector<GitCommit>> m_recentCommits;
    int m_cleanCommitIndex;

    std::shared_ptr<const std::vector<CP::ModifiedFile>> m_modifiedFiles;
};
