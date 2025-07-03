#pragma once


class GitHelpersDlg : public CDialog
{
public:
    GitHelpersDlg(CWnd* pParent = nullptr);
    ~GitHelpersDlg();

protected:
    DECLARE_MESSAGE_MAP()

    void DoDataExchange(CDataExchange* pDX) override;

    void OnCreateCommitTextForFilteredCommits();

private:
    std::string m_filteredRepositoryDirectory;
    std::string m_filteredRepositoryCommit;
    std::string m_destinationDirectory;
    std::string m_destinationBranchName;
};
