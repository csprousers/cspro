#include "StdAfx.h"
#include "ManageLibrariesView.h"
#include "LibraryManager.h"


IMPLEMENT_DYNCREATE(ManageLibrariesView, CFormView)


BEGIN_MESSAGE_MAP(ManageLibrariesView, CFormView)
    ON_MESSAGE(UWM::OpenSourceSyncer::UpdateUI, OnUpdateLibraryIds)
    ON_COMMAND(IDC_REFRESH_LIBRARY_IDS, OnRefreshLibraryIds)
    ON_COMMAND(IDC_CREATE_BUILT_LIBRARY, OnCreateBuiltLibrary)
    ON_COMMAND(IDC_PREVIEW_BUILT_LIBRARY, OnPreviewBuiltLibrary)
    ON_COMMAND(IDC_VIEW_BUILT_LIBRARY_INPUTS, OnViewBuiltLibraryInputs)
END_MESSAGE_MAP()


ManageLibrariesView::ManageLibrariesView()
    :   CFormView(IDD_MANAGE_LIBRARIES),
        m_controller(Controller::GetInstance())
{
}


void ManageLibrariesView::OnInitialUpdate()
{
    __super::OnInitialUpdate();

    ResizeParentToFit(FALSE);

    m_libraryIdsListCtrl.SetExtendedStyle(LVS_EX_FULLROWSELECT);
    m_libraryIdsListCtrl.SetHeadings(L"Tag Name,130;Library ID,210;Local Hash,210");
    m_libraryIdsListCtrl.LoadColumnInfo();

    PostMessage(UWM::OpenSourceSyncer::UpdateUI);
}


void ManageLibrariesView::DoDataExchange(CDataExchange* const pDX)
{
    __super::DoDataExchange(pDX);

    DDX_Control(pDX, IDC_LIBRARY_IDS, m_libraryIdsListCtrl);
    DDX_Text(pDX, IDC_COMMIT, m_commit, true);
}


LRESULT ManageLibrariesView::OnUpdateLibraryIds(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    m_libraryIdsListCtrl.DeleteAllItems();

    const std::shared_ptr<const std::vector<LibraryManager::Build>> builds = m_controller.GetLibraryManager().GetBuilds();
    ASSERT(builds != nullptr);

    for( const LibraryManager::Build& build : *builds )
    {
        m_libraryIdsListCtrl.AddItem(
            TC::ToWide(build.tag_name).c_str(),
            TC::ToWide(build.library_id).c_str(),
            TC::ToWide(build.local_hash).c_str()
        );
    }

    return 1;
}


void ManageLibrariesView::OnRefreshLibraryIds()
{
    m_controller.RunOperation(GetParentFrame(),
        [this](Controller& controller)
        {
            LibraryManager& library_manager = controller.GetLibraryManager();
            library_manager.RefreshBuildsFromTags();

            PostMessage(UWM::OpenSourceSyncer::UpdateUI);
        });
}


void ManageLibrariesView::OnBuiltLibraryAction(const bool create)
{
    UpdateData(TRUE);

    m_controller.RunOperation(GetParentFrame(),
        [this, commit_sha = m_commit, create](Controller& controller)
        {
            LibraryManager& library_manager = controller.GetLibraryManager();
            GitRepository& private_repo = controller.GetPrivateRepo();

            const GitCommit cs_commit = commit_sha.empty() ? private_repo.LookupCommit(private_repo.GetCurrentBranch()) :
                                                             private_repo.LookupCommit(commit_sha);

            controller.LogText("Generating built library information for: " + cs_commit.GetObjectId().GetHexHash());

            const std::vector<LibraryManager::Input> inputs = library_manager.GetInputs(cs_commit);
            const std::string library_id = library_manager.CalculateCacheKey(inputs, false);
            const std::string local_cache_key = library_manager.CalculateCacheKey(inputs, true);

            controller.LogText("There are %zu files included in the built library.", inputs.size());
            controller.LogText("Library ID: " + library_id);
            controller.LogText("Local cache key: " + local_cache_key);

            if( create )
            {
                library_manager.CreateAndCommitBuild(cs_commit);
                PostMessage(UWM::OpenSourceSyncer::UpdateUI);
            }
        });
}


void ManageLibrariesView::OnViewBuiltLibraryInputs()
{
    m_controller.RunOperation(GetParentFrame(),
        [](Controller& controller)
        {
            LibraryManager& library_manager = controller.GetLibraryManager();
            std::vector<RepoFilePath> inputs = library_manager.GetInputs();

            controller.LogText("There are %zu files included in the built library:\n", inputs.size());

            // write out the paths in file path order
            std::sort(inputs.begin(), inputs.end(),
                [](const RepoFilePath& rfp1, const RepoFilePath& rfp2)
                {
                    return ( rfp1.file_path < rfp2.file_path );
                });

            for( const RepoFilePath& input : inputs )
                controller.LogText(input.file_path);
        });
}
