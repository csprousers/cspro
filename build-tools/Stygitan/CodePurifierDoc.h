#pragma once

#include <Stygitan/CodePurifierData.h>


class CodePurifierDoc : public CDocument
{
    DECLARE_DYNCREATE(CodePurifierDoc)

protected:
    CodePurifierDoc();

public:
    ~CodePurifierDoc();

    // Starts or stops a worker thread that will handle Git-related changes.
    void StartGitProcessing(CWnd* wnd_for_updates);
    void StopGitProcessing();

    // Returns the repository's working directory.
    const std::string& GetRepositoryWorkingDirectory() const noexcept { return m_repoWorkingDirectory; }

    // The shared pointers returned may be replaced at any point by a worker thread,
    // so any user should create a copy of the shared pointer while it is being used.
    std::shared_ptr<const CP::BranchDetails> GetBranchDetails() const noexcept { return m_branchDetails; }

    std::shared_ptr<const std::map<std::string, GitBranch>> GetBranchCopies() const noexcept { return m_branchCopies; }

    std::shared_ptr<const GitCommit> GetCleanCommit() const noexcept { return m_cleanCommit; }

    std::shared_ptr<const std::vector<GitCommit>> GetRecentCommits() const noexcept { return m_recentCommits; }

    std::shared_ptr<const std::vector<CP::ModifiedFile>> GetModifiedFiles() const noexcept { return m_modifiedFiles; }

    // The public methods below, which are intended to be called from the UI thread,
    // suspend the worker thread to perform the action and can throw exceptions on error.

    // Creates a copy of the branch.
    void CreateBranchCopy();

    // Deletes all branch copies, prompting the user for confirmation to delete
    // copies created prior to this session.
    void DeleteBranchCopies();

    // Sets the clean commit override.
    void SetCleanCommitOverride(const GitCommit& commit);

    // Resets the current commit to the clean commit, optionally creating a
    // branch copy prior to the operation.
    void ResetBranchToCleanCommit(bool create_branch_copy_before_reset);

protected:
    void SetTitle(LPCTSTR lpszTitle) override;
    void SetPathName(LPCTSTR lpszPathName, BOOL bAddToMRU = TRUE) override;

    BOOL OnOpenDocument(LPCTSTR lpszPathName) override;
    void OnCloseDocument() override;

private:
    enum class RefreshStartAction { UpdateBranches, LocateCleanCommit, LoadRecentCommits, IdentifyModifiedFiles };
    CP::RefreshDataChanges RefreshData(RefreshStartAction action);

    bool RefreshBranchDetails();
    void EnumerateBranchCopies();
    bool LocateCleanCommit();
    bool LoadRecentCommits();
    bool IdentifyModifiedFiles();

private:
    GitRepository m_repo;
    std::string m_repoWorkingDirectory;

    CWnd* m_wndForGitUpdates;

    std::shared_ptr<const CP::BranchDetails> m_branchDetails;
    std::shared_ptr<std::map<std::string, GitBranch>> m_branchCopies;
    std::set<std::string> m_createdBranchCopyNames;

    std::shared_ptr<const GitCommit> m_cleanCommit;
    std::optional<GitObjectId> m_cleanCommitOverride;

    std::shared_ptr<const std::vector<GitCommit>> m_recentCommits;

    std::shared_ptr<const std::vector<CP::ModifiedFile>> m_modifiedFiles;
};
