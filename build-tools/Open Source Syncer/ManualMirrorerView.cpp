#include "StdAfx.h"
#include "ManualMirrorerView.h"
#include "Syncer.h"


IMPLEMENT_DYNCREATE(ManualMirrorerView, CFormView)


BEGIN_MESSAGE_MAP(ManualMirrorerView, CFormView)
    ON_COMMAND(IDC_MIRROR_COMMIT, OnMirrorCommit)
    ON_COMMAND(IDC_MIRROR_MERGE_COMMIT, OnMirrorMergeCommit)
END_MESSAGE_MAP()


ManualMirrorerView::ManualMirrorerView()
    :   CFormView(IDD_MANUAL_MIRRORER),
        m_controller(Controller::GetInstance())
{
}


void ManualMirrorerView::OnInitialUpdate()
{
    __super::OnInitialUpdate();

    ResizeParentToFit(FALSE);
}


void ManualMirrorerView::DoDataExchange(CDataExchange* const pDX)
{
    __super::DoDataExchange(pDX);

    DDX_Text(pDX, IDC_BRANCH_NAME, m_branchName, true);
    DDX_Text(pDX, IDC_COMMIT_OLDEST, m_oldestCommit, true);
    DDX_Text(pDX, IDC_COMMIT_NEWEST, m_newestCommit, true);

    DDX_Text(pDX, IDC_TARGET_BRANCH_NAME, m_targetBranchName, true);
    DDX_Text(pDX, IDC_MERGE_COMMIT, m_mergeCommit, true);
    DDX_Text(pDX, IDC_PARENT_COMMIT1, m_parentCommit1, true);
    DDX_Text(pDX, IDC_PARENT_COMMIT2, m_parentCommit2, true);
}


void ManualMirrorerView::OnMirrorCommit()
{
    UpdateData(TRUE);

    m_controller.RunOperation(GetParentFrame(),
        [this, branch_name = m_branchName, oldest_commit_sha = m_oldestCommit, newest_commit_sha = m_newestCommit]
        (Controller& controller)
        {
            OnMirrorCommit(controller, *branch_name, *oldest_commit_sha, *newest_commit_sha);
        });
}


void ManualMirrorerView::OnMirrorCommit(Controller& controller, const std::string& branch_name,
                                        const std::string& oldest_commit_sha, const std::string& newest_commit_sha)
{
    GitRepository& private_repo = controller.GetPrivateRepo();
    GitRepository& open_source_repo = controller.GetOpenSourceRepo();
    Syncer& syncer = controller.GetSyncer();

    // validate the branch
    if( branch_name.empty() )
        throw CSProException("Specify the branch name.");

    std::optional<GitBranch> os_branch;

    try
    {
        os_branch = open_source_repo.LookupBranch(branch_name);
    }

    catch(...)
    {
        // try creating the branch if it does not exist
        const GitBranch current_branch = open_source_repo.GetCurrentBranch();

        os_branch = open_source_repo.CreateBranch(
            branch_name,
            open_source_repo.LookupCommit(current_branch)
        );
    }

    // validate the commits
    std::optional<GitCommit> cs_oldest_commit;
    std::optional<GitCommit> cs_commit_newest;

    if( !oldest_commit_sha.empty() )
    {
        cs_oldest_commit = private_repo.LookupCommit(oldest_commit_sha);
    }

    else if( newest_commit_sha.empty() )
    {
        throw CSProException("Specify at least one commit.");
    }

    if( !newest_commit_sha.empty() )
    {
        cs_commit_newest = private_repo.LookupCommit(newest_commit_sha);

        // when only one commit is specified, make it the oldest
        if( !cs_oldest_commit.has_value() )
            std::swap(cs_oldest_commit, cs_commit_newest);
    }

    ASSERT(cs_oldest_commit.has_value());

    // mirror the merge commits
    syncer.MirrorCommits(
        *os_branch,
        *cs_oldest_commit,
        cs_commit_newest.has_value() ? &*cs_commit_newest : nullptr
    );
}


void ManualMirrorerView::OnMirrorMergeCommit()
{
    UpdateData(TRUE);

    m_controller.RunOperation(GetParentFrame(),
        [this, target_branch_name = m_targetBranchName, merge_commit_sha = m_mergeCommit,
         parent_commit_sha1 = m_parentCommit1, parent_commit_sha2 = m_parentCommit2]
        (Controller& controller)
        {
            OnMirrorMergeCommit(controller, *target_branch_name, *merge_commit_sha,
                                *parent_commit_sha1, *parent_commit_sha2);
        });
}


void ManualMirrorerView::OnMirrorMergeCommit(Controller& controller,
                                             const std::string& target_branch_name, const std::string& merge_commit_sha,
                                             const std::string& parent_commit_sha1, const std::string& parent_commit_sha2)
{
    GitRepository& private_repo = controller.GetPrivateRepo();
    GitRepository& open_source_repo = controller.GetOpenSourceRepo();
    Syncer& syncer = controller.GetSyncer();

    // validate the branch
    if( target_branch_name.empty() )
        throw CSProException("Specify the open source branch target.");

    const GitBranch os_branch = open_source_repo.LookupBranch(target_branch_name);

    // validate the commits
    if( merge_commit_sha.empty() )
        throw CSProException("Specify the merge commit.");

    const GitCommit cs_merge_commit = private_repo.LookupCommit(merge_commit_sha);

    if( parent_commit_sha1.empty() ||
        parent_commit_sha2.empty() )
    {
        throw CSProException("Specify the two parent commits.");
    }

    // mirror the merge commit
    syncer.MirrorMergeCommit(
        os_branch,
        cs_merge_commit,
        open_source_repo.LookupCommit(parent_commit_sha1),
        open_source_repo.LookupCommit(parent_commit_sha2)
    );
}
