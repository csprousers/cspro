#pragma once

class GitCommit;


class FilteredCommitsMessageCreatorView : public CFormView
{
    DECLARE_DYNCREATE(FilteredCommitsMessageCreatorView)

protected:
    FilteredCommitsMessageCreatorView();

protected:
    DECLARE_MESSAGE_MAP()

    void DoDataExchange(CDataExchange* pDX) override;

    void OnCreateCommitTextForFilteredCommits();

private:
    // Returns the commit history from HEAD up to (and including) the commit specified.
    std::vector<GitCommit> ReadCommitHistory() const;

    // Returns the commit SHAs that match the commits, matched by author time and email.
    std::vector<std::string> LookupCommitSHAs(const std::vector<GitCommit>& commits) const;

private:
    SettingsDb m_settingsDb;
    std::string m_filteredRepositoryDirectory;
    std::string m_filteredRepositoryCommit;
    std::string m_destinationDirectory;
    std::string m_destinationBranchName;
};
