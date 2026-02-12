#pragma once


class FeatureBranchSyncerView : public CFormView
{
    DECLARE_DYNCREATE(FeatureBranchSyncerView)

protected:
    FeatureBranchSyncerView();

protected:
    DECLARE_MESSAGE_MAP()

    void OnInitialUpdate() override;
    void DoDataExchange(CDataExchange* pDX) override;

    void OnFindUnsyncedMergeCommits();
    LRESULT OnUpdateUnsyncedMergeCommits(WPARAM wParam, LPARAM lParam);

    void OnSyncFeatureBranches();

private:
    static GitBranch ValidateTargetBranch(GitRepository& open_source_repo, const std::string& target_branch_name);

    void OnSyncFeatureBranches(Controller& controller, const std::string& target_branch_name,
                               const std::string& oldest_commit_sha, const std::string& newest_commit_sha);
private:
    Controller& m_controller;

    SharableString m_targetBranchName;
    SharableString m_oldestCommit;
    SharableString m_newestCommit;

    std::unique_ptr<std::tuple<GitCommit, GitCommit>> m_foundUnsyncedMergeCommits;
};
