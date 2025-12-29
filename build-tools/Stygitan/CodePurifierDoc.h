#pragma once


class CodePurifierDoc : public CDocument
{
    friend class CodePurifierView;

    DECLARE_DYNCREATE(CodePurifierDoc)

protected:
    CodePurifierDoc();

protected:
    void SetTitle(LPCTSTR lpszTitle) override;
    void SetPathName(LPCTSTR lpszPathName, BOOL bAddToMRU = TRUE) override;

    BOOL OnOpenDocument(LPCTSTR lpszPathName) override;

private:
    void RefreshData();

    void EnumerateBranchCopies();
    void LocateCleanCommit();
    void LoadRecentCommits();
    void LoadModifiedFiles();

private:
    GitRepository m_repo;
    std::optional<const GitBranch> m_currentBranch;
    std::unique_ptr<const GitBranch> m_remoteBranch;

    std::map<std::string, GitBranch> m_branchCopies;
    std::optional<size_t> m_initialNumberOfBranchCopies;

    std::optional<GitCommit> m_cleanCommit;
    std::optional<GitObjectId> m_cleanCommitOverride;

    std::vector<GitCommit> m_recentCommits;

    std::vector<std::tuple<std::string, unsigned int>> m_modifiedFiles; // file path + difference flag
};
