#pragma once

#include <zGit/GitBranch.h>
#include <zGit/GitRepository.h>


class CodePurifierDoc : public CDocument
{
    DECLARE_DYNCREATE(CodePurifierDoc)

protected:
    CodePurifierDoc();

public:
    GitRepository& GetRepository() { return m_repo; }

protected:
    void SetTitle(LPCTSTR lpszTitle) override;
    void SetPathName(LPCTSTR lpszPathName, BOOL bAddToMRU = TRUE) override;

    BOOL OnOpenDocument(LPCTSTR lpszPathName) override;

private:
    GitRepository m_repo;
    std::optional<GitBranch> m_branch;
};
