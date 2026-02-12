#include "StdAfx.h"
#include "RepositoryComparerView.h"
#include "Syncer.h"


IMPLEMENT_DYNCREATE(RepositoryComparerView, CFormView)


BEGIN_MESSAGE_MAP(RepositoryComparerView, CFormView)
    ON_COMMAND(IDC_COMPARE, OnCompare)
END_MESSAGE_MAP()


RepositoryComparerView::RepositoryComparerView()
    :   CFormView(IDD_REPO_COMPARER),
        m_controller(Controller::GetInstance())
{
}


void RepositoryComparerView::OnInitialUpdate()
{
    __super::OnInitialUpdate();

    ResizeParentToFit(FALSE);
}


void RepositoryComparerView::DoDataExchange(CDataExchange* const pDX)
{
    __super::DoDataExchange(pDX);

    DDX_Text(pDX, IDC_COMMIT_PRIVATE, m_privateCommit, true);
    DDX_Text(pDX, IDC_COMMIT_OPEN_SOURCE, m_openSourceCommit, true);
}


void RepositoryComparerView::OnCompare()
{
    UpdateData(TRUE);

    m_controller.RunOperation(GetParentFrame(),
        [this, private_commit_sha = m_privateCommit, open_source_commit_sha = m_openSourceCommit]
        (Controller& controller)
        {
            OnCompare(controller, *private_commit_sha, *open_source_commit_sha);
        });
}


void RepositoryComparerView::OnCompare(Controller& controller, const std::string& private_commit_sha,
                                       const std::string& open_source_commit_sha)
{
    GitRepository& private_repo = controller.GetPrivateRepo();
    GitRepository& open_source_repo = controller.GetOpenSourceRepo();
    Syncer& syncer = controller.GetSyncer();

    const GitCommit cs_commit =
        private_commit_sha.empty() ? private_repo.LookupCommit(private_repo.GetCurrentBranch()) :
                                     private_repo.LookupCommit(private_commit_sha);

    const GitCommit os_commit =
        open_source_commit_sha.empty() ? open_source_repo.LookupCommit(open_source_repo.GetCurrentBranch()) :
                                         open_source_repo.LookupCommit(open_source_commit_sha);

    syncer.CompareRepositories(cs_commit, os_commit, true);
}
