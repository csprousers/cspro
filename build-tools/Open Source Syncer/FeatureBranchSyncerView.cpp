#include "StdAfx.h"
#include "FeatureBranchSyncerView.h"
#include "Syncer.h"


namespace
{
    constexpr std::string_view TargetBranchNameKey_sv = "feature-branch-syncer-branch-name";
}


IMPLEMENT_DYNCREATE(FeatureBranchSyncerView, CFormView)


BEGIN_MESSAGE_MAP(FeatureBranchSyncerView, CFormView)
    ON_COMMAND(IDC_FIND_UNSYNCED_MERGE_COMMITS, OnFindUnsyncedMergeCommits)
    ON_MESSAGE(UWM::OpenSourceSyncer::UpdateUI, OnUpdateUnsyncedMergeCommits)
    ON_COMMAND(IDC_SYNC_FEATURE_BRANCHES, OnSyncFeatureBranches)
END_MESSAGE_MAP()


FeatureBranchSyncerView::FeatureBranchSyncerView()
    :   CFormView(IDD_FEATURE_BRANCH_SYNCER),
        m_controller(Controller::GetInstance()),
        m_targetBranchName(m_controller.GetSettingsDb().ReadOrDefault<std::string>(TargetBranchNameKey_sv))
{
}


void FeatureBranchSyncerView::OnInitialUpdate()
{
    __super::OnInitialUpdate();

    ResizeParentToFit(FALSE);
}


void FeatureBranchSyncerView::DoDataExchange(CDataExchange* const pDX)
{
    __super::DoDataExchange(pDX);

    DDX_Text(pDX, IDC_TARGET_BRANCH_NAME, m_targetBranchName, true);
    DDX_Text(pDX, IDC_COMMIT_OLDEST, m_oldestCommit, true);
    DDX_Text(pDX, IDC_COMMIT_NEWEST, m_newestCommit, true);

    if( pDX->m_bSaveAndValidate )
        m_controller.GetSettingsDb().Write(TargetBranchNameKey_sv, *m_targetBranchName);
}


GitBranch FeatureBranchSyncerView::ValidateTargetBranch(GitRepository& open_source_repo, const std::string& target_branch_name)
{
    if( target_branch_name.empty() )
        throw CSProException("Specify the open source branch target.");

    return open_source_repo.LookupBranch(target_branch_name);
}


void FeatureBranchSyncerView::OnFindUnsyncedMergeCommits()
{
    UpdateData(TRUE);

    m_controller.RunOperation(GetParentFrame(),
        [this, target_branch_name = m_targetBranchName]
        (Controller& controller)
        {
            GitRepository& open_source_repo = controller.GetOpenSourceRepo();
            Syncer& syncer = controller.GetSyncer();

            const GitBranch os_merge_branch = ValidateTargetBranch(open_source_repo, *target_branch_name);

            m_foundUnsyncedMergeCommits = std::make_unique<std::tuple<GitCommit, GitCommit>>(
                syncer.FindUnsyncedMergeCommits(os_merge_branch)
            );

            m_controller.LogText(
                "Found unsynced merge commits:\n\n"
                "Last synced (%s): %s\n\n%s\n\n"
                "Newest eligible for syncing (%s): %s\n\n%s",
                std::get<0>(*m_foundUnsyncedMergeCommits).GetAuthor().GetWhen().GetLocalDateTimeString().c_str(),
                std::get<0>(*m_foundUnsyncedMergeCommits).GetObjectId().GetHexHash().c_str(),
                std::get<0>(*m_foundUnsyncedMergeCommits).GetMessage().c_str(),
                std::get<1>(*m_foundUnsyncedMergeCommits).GetAuthor().GetWhen().GetLocalDateTimeString().c_str(),
                std::get<1>(*m_foundUnsyncedMergeCommits).GetObjectId().GetHexHash().c_str(),
                std::get<1>(*m_foundUnsyncedMergeCommits).GetMessage().c_str()
            );

            PostMessage(UWM::OpenSourceSyncer::UpdateUI);
        });
}


LRESULT FeatureBranchSyncerView::OnUpdateUnsyncedMergeCommits(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    const std::unique_ptr<const std::tuple<GitCommit, GitCommit>> found_unsynced_merge_commits = std::move(m_foundUnsyncedMergeCommits);
    ASSERT(found_unsynced_merge_commits != nullptr);

    m_oldestCommit = std::get<0>(*found_unsynced_merge_commits).GetObjectId().GetHexHash();
    m_newestCommit = std::get<1>(*found_unsynced_merge_commits).GetObjectId().GetHexHash();

    UpdateData(FALSE);

    return 1;
}


void FeatureBranchSyncerView::OnSyncFeatureBranches()
{
    UpdateData(TRUE);

    m_controller.RunOperation(GetParentFrame(),
        [this, target_branch_name = m_targetBranchName, oldest_commit_sha = m_oldestCommit, newest_commit_sha = m_newestCommit]
        (Controller& controller)
        {
            OnSyncFeatureBranches(controller, *target_branch_name, *oldest_commit_sha, *newest_commit_sha);
        });
}


void FeatureBranchSyncerView::OnSyncFeatureBranches(Controller& controller, const std::string& target_branch_name,
                                                    const std::string& oldest_commit_sha, const std::string& newest_commit_sha)
{
    GitRepository& private_repo = controller.GetPrivateRepo();
    GitRepository& open_source_repo = controller.GetOpenSourceRepo();
    Syncer& syncer = controller.GetSyncer();

    const GitBranch os_merge_branch = ValidateTargetBranch(open_source_repo, target_branch_name);

    // validate the commits
    if( oldest_commit_sha.empty() )
        throw CSProException("Specify the oldest merge commit.");

    const GitCommit cs_oldest_merge_commit = private_repo.LookupCommit(oldest_commit_sha);

    if( newest_commit_sha.empty() )
        throw CSProException("Specify the newest merge commit.");

    const GitCommit cs_newest_merge_commit = private_repo.LookupCommit(newest_commit_sha);

    // mirror the feature branches
    syncer.MirrorFeatureBranches(os_merge_branch, cs_oldest_merge_commit, cs_newest_merge_commit);
}
