#include "StdAfx.h"
#include "ManageLibrariesView.h"
#include "LibraryManager.h"


IMPLEMENT_DYNCREATE(ManageLibrariesView, CFormView)


BEGIN_MESSAGE_MAP(ManageLibrariesView, CFormView)
    ON_MESSAGE(UWM::OpenSourceSyncer::UpdateUI, OnUpdateLibrariesIds)
    ON_COMMAND(IDC_REFRESH_LIBRARIES_IDS, OnRefreshLibrariesIds)
    ON_COMMAND(IDC_VIEW_RELEASE, OnViewRelease)
    ON_COMMAND(IDC_CREATE_LIBRARY_RELEASE, OnCreateLibraryRelease)
    ON_COMMAND(IDC_VIEW_LIBRARY_INPUTS, OnViewLibraryInputs)
END_MESSAGE_MAP()


ManageLibrariesView::ManageLibrariesView()
    :   CFormView(IDD_MANAGE_LIBRARIES),
        m_controller(Controller::GetInstance()),
        m_releasesJsonText(m_controller.GetSettingsDb().ReadOrDefault<std::string>(SettingsKeys::ThirdPartyLibrariesGitHubReleases_sv)),
        m_librariesIdRegex(R"(Libraries ID: \*?([[:xdigit:]]+))")
{
}


void ManageLibrariesView::OnInitialUpdate()
{
    __super::OnInitialUpdate();

    ResizeParentToFit(FALSE);

    m_librariesIdsListCtrl.SetExtendedStyle(LVS_EX_FULLROWSELECT);
    m_librariesIdsListCtrl.SetHeadings(L"Tag Name,150;Libraries ID,400;Assets,45;");
    m_librariesIdsListCtrl.LoadColumnInfo();

    PostMessage(UWM::OpenSourceSyncer::UpdateUI);
}


void ManageLibrariesView::DoDataExchange(CDataExchange* const pDX)
{
    __super::DoDataExchange(pDX);

    DDX_Control(pDX, IDC_LIBRARIES_IDS, m_librariesIdsListCtrl);
}


LRESULT ManageLibrariesView::OnUpdateLibrariesIds(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    m_librariesIdsListCtrl.DeleteAllItems();

    try
    {
        LibraryManager& library_manager = m_controller.GetLibraryManager();

        const std::string cfwd_libraries_id = library_manager.GetLibrariesId(LibraryManager::LibrariesIdType::CalculatedFromWorkingDirectory);
        const std::string ilj_libraries_id = library_manager.GetLibrariesId(LibraryManager::LibrariesIdType::InLibrariesJson);

        if( cfwd_libraries_id != ilj_libraries_id )
        {
            m_librariesIdsListCtrl.AddItem(
                L"<current working directory>",
                TC::ToWide(cfwd_libraries_id).c_str(),
                L""
            );
        }

        m_librariesIdsListCtrl.AddItem(
            L"<current libraries.json>",
            TC::ToWide(ilj_libraries_id).c_str(),
            L""
        );

        if( m_releasesJsonText.empty() )
            return 1;

        const std::vector<GitHubRelease> releases = Json::Parse(m_releasesJsonText).GetArray().GetVector<GitHubRelease>();

        for( const GitHubRelease& release : releases )
        {
            ASSERT(release.tag_name == release.name);

            m_librariesIdsListCtrl.AddItem(
                TC::ToWide(release.tag_name).c_str(),
                TC::ToWide(GetLibrariesIdFromReleaseNotes(release.release_notes)).c_str(),
                TC::ToWide(IntToString(release.assets_count)).c_str()
            );
        }
    }

    catch( const CSProException& exception )
    {
        ErrorMessage::Display(exception);
    }

    return 1;
}


std::string ManageLibrariesView::GetLibrariesIdFromReleaseNotes(const std::string& release_notes) const
{
    std::smatch matches;

    if( std::regex_search(release_notes, matches, m_librariesIdRegex) )
        return matches.str(1);

    throw ProgrammingErrorException();
}


void ManageLibrariesView::OnRefreshLibrariesIds()
{
    m_controller.RunOperation(GetParentFrame(),
        [this](Controller& /*controller*/)
        {
            RefreshLibrariesIds();
        });
}


void ManageLibrariesView::RefreshLibrariesIds()
{
    LibraryManager& library_manager = m_controller.GetLibraryManager();
    GitHubRepositoryConnection& gh_connection = library_manager.GetThirdPartyLibrariesGitHubRepositoryConnection();

    m_controller.LogText("Refreshing the libraries IDs from GitHub.");

    m_releasesJsonText = gh_connection.GetReleases<std::string>();
    m_controller.GetSettingsDb().Write(SettingsKeys::ThirdPartyLibrariesGitHubReleases_sv, m_releasesJsonText);

    PostMessage(UWM::OpenSourceSyncer::UpdateUI);
}


void ManageLibrariesView::OnViewRelease()
{
    try
    {
        const int index = m_librariesIdsListCtrl.GetSelectionMark();
        std::string tag_name;

        if( index >= 0 && m_librariesIdsListCtrl.GetSelectedCount() == 1 )
        {
            // the assets are only set for valid releases
            if( !m_librariesIdsListCtrl.GetItemText(index, 2).IsEmpty() )
                tag_name = TC::ToUtf8(m_librariesIdsListCtrl.GetItemText(index, 0));
        }

        if( tag_name.empty() )
            throw CSProException("Select a release.");

        Viewer().ViewHtmlUrl(
            "https://github.com/csprousers/cspro-libraries-third-party/releases/tag/" + tag_name
        );
    }

    catch( const CSProException& exception )
    {
        ErrorMessage::Display(exception);
    }
}


void ManageLibrariesView::OnCreateLibraryRelease()
{
    m_controller.RunOperation(GetParentFrame(),
        [this](Controller& controller)
        {
            LibraryManager& library_manager = controller.GetLibraryManager();
            library_manager.CreateLibraryRelease();

            RefreshLibrariesIds();
        });
}


void ManageLibrariesView::OnViewLibraryInputs()
{
    m_controller.RunOperation(GetParentFrame(),
        [](Controller& controller)
        {
            LibraryManager& library_manager = controller.GetLibraryManager();
            const std::set<std::string> file_paths = library_manager.GetThirdPartyFilePaths();

            controller.LogText("There are %zu files included in the library:\n", file_paths.size());

            for( const std::string& file_path : file_paths )
                controller.LogText(file_path);
        });
}
