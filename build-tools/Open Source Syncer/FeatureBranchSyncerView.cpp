#include "StdAfx.h"
#include "FeatureBranchSyncerView.h"
#include "Syncer.h"


namespace
{
    constexpr std::string_view TargetBranchNameKey_sv = "feature-branch-syncer-branch-name";
}


IMPLEMENT_DYNCREATE(FeatureBranchSyncerView, CFormView)


BEGIN_MESSAGE_MAP(FeatureBranchSyncerView, CFormView)
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

    // validate the branch
    if( target_branch_name.empty() )
        throw CSProException("Specify the open source branch target.");

    const GitBranch os_branch = open_source_repo.LookupBranch(target_branch_name);

    // validate the commits
    if( oldest_commit_sha.empty() )
        throw CSProException("Specify the oldest merge commit.");

    const GitCommit cs_oldest_merge_commit = private_repo.LookupCommit(oldest_commit_sha);

    if( newest_commit_sha.empty() )
        throw CSProException("Specify the newest merge commit.");

    const GitCommit cs_newest_merge_commit = private_repo.LookupCommit(newest_commit_sha);

    // mirror the feature branches
    syncer.MirrorFeatureBranches(os_branch, cs_oldest_merge_commit, cs_newest_merge_commit);
}
