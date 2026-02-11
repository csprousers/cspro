#pragma once


class ManualMirrorerView : public CFormView
{
    DECLARE_DYNCREATE(ManualMirrorerView)

protected:
    ManualMirrorerView();

protected:
    DECLARE_MESSAGE_MAP()

    void OnInitialUpdate() override;
    void DoDataExchange(CDataExchange* pDX) override;

    void OnMirrorCommit();
    void OnMirrorMergeCommit();

private:
    static void OnMirrorCommit(Controller& controller, const std::string& branch_name,
                               const std::string& oldest_commit_sha, const std::string& newest_commit_sha);

    static void OnMirrorMergeCommit(Controller& controller,
                                    const std::string& target_branch_name, const std::string& merge_commit_sha,
                                    const std::string& parent_commit_sha1, const std::string& parent_commit_sha2);

private:
    Controller& m_controller;

    SharableString m_branchName;
    SharableString m_oldestCommit;
    SharableString m_newestCommit;

    SharableString m_targetBranchName;
    SharableString m_mergeCommit;
    SharableString m_parentCommit1;
    SharableString m_parentCommit2;
};
