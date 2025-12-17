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

private:
    GitRepository m_repo;
    std::optional<const GitBranch> m_currentBranch;
    std::unique_ptr<const GitBranch> m_remoteBranch;

    std::map<std::string, GitBranch> m_branchCopies;
    std::optional<size_t> m_initialNumberOfBranchCopies;
};
