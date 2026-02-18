#include "StdAfx.h"
#include "ManageReleasesView.h"
#include "GitHubConnection.h"
#include <zUtilO/Viewers.h>


CREATE_JSON_KEY(assets)
CREATE_JSON_KEY(commit)
CREATE_JSON_KEY(draft)
CREATE_JSON_KEY(html_url)
CREATE_JSON_KEY(prerelease)
CREATE_JSON_KEY(sha)
CREATE_JSON_KEY(tag_name)


IMPLEMENT_DYNCREATE(ManageReleasesView, CFormView)


BEGIN_MESSAGE_MAP(ManageReleasesView, CFormView)
    ON_MESSAGE(UWM::OpenSourceSyncer::UpdateUI, OnUpdateReleases)
    ON_COMMAND(IDC_REFRESH_RELEASES, OnRefreshReleases)
    ON_COMMAND(IDC_VIEW_RELEASE, OnViewRelease)
    ON_COMMAND(IDC_CREATE_RELEASE, OnCreateRelease)
END_MESSAGE_MAP()


ManageReleasesView::ManageReleasesView()
    :   CFormView(IDD_MANAGE_RELEASES),
        m_controller(Controller::GetInstance()),
        m_tagsJsonText(m_controller.GetSettingsDb().ReadOrDefault<std::string>(SettingsKeys::GitHubTags_sv)),
        m_releasesJsonText(m_controller.GetSettingsDb().ReadOrDefault<std::string>(SettingsKeys::GitHubReleases_sv))
{
}


ManageReleasesView::~ManageReleasesView()
{
}


void ManageReleasesView::OnInitialUpdate()
{
    __super::OnInitialUpdate();

    ResizeParentToFit(FALSE);

    m_releasesListCtrl.SetExtendedStyle(LVS_EX_FULLROWSELECT);
    m_releasesListCtrl.SetHeadings(L"Tag Name,250;Release Name,250;Status,110;Assets,45;Commit,250");
    m_releasesListCtrl.LoadColumnInfo();

    PostMessage(UWM::OpenSourceSyncer::UpdateUI);
}


void ManageReleasesView::DoDataExchange(CDataExchange* const pDX)
{
    __super::DoDataExchange(pDX);

    DDX_Control(pDX, IDC_RELEASES, m_releasesListCtrl);
}


struct ManageReleasesView::GitHubTag
{
    std::string name;
    std::string commit_sha;
};


std::vector<ManageReleasesView::GitHubTag> ManageReleasesView::ParseGitHubTags() const
{
    std::vector<GitHubTag> tags;

    if( m_tagsJsonText.empty() )
        return tags;

    for( const JsonNode& json_node : Json::Parse(m_tagsJsonText).GetArray() )
    {
        tags.emplace_back(
            GitHubTag
            {
                json_node.Get<std::string>(JK::name),
                json_node.Get(JK::commit).Get<std::string>(JK::sha)
            }
        );
    }

    return tags;
}


struct ManageReleasesView::GitHubRelease
{
    std::string html_url;
    std::string tag_name;
    std::string name;
    bool draft;
    bool prerelease;
    size_t assets_count;

    const std::string& GetReleaseName() const;
    std::wstring GetStatus() const;
};


const std::string& ManageReleasesView::GitHubRelease::GetReleaseName() const
{
    // release names are not available in old releases
    return !name.empty() ? name :
                           tag_name;
}


std::wstring ManageReleasesView::GitHubRelease::GetStatus() const
{
    std::wstring status = prerelease ? L"Pre-Release" :
                                       L"Release";

    if( draft )
        status.insert(0, L"(Draft) ");

    return status;
}


std::vector<ManageReleasesView::GitHubRelease> ManageReleasesView::ParseGitHubReleases() const
{
    std::vector<GitHubRelease> releases;

    if( m_releasesJsonText.empty() )
        return releases;

    for( const JsonNode& json_node : Json::Parse(m_releasesJsonText).GetArray() )
    {
        releases.emplace_back(
            GitHubRelease
            {
                json_node.Get<std::string>(JK::html_url),
                json_node.Get<std::string>(JK::tag_name),
                json_node.Get<std::string>(JK::name),
                json_node.Get<bool>(JK::draft),
                json_node.Get<bool>(JK::prerelease),
                json_node.GetArray(JK::assets).size()
            }
        );
    }

    return releases;
}


struct ManageReleasesView::ReleaseOption : GitHubTag
{
    std::optional<GitHubRelease> release;
};


void ManageReleasesView::PopulateReleaseOptions()
{
    m_releaseOptions.clear();

    const std::vector<GitHubTag> tags = ParseGitHubTags();
    std::vector<GitHubRelease> releases = ParseGitHubReleases();

    for( const GitHubTag& tag : tags )
    {
        ReleaseOption& release_option = m_releaseOptions.emplace_back(ReleaseOption { tag });

        // see if there is a release for this tag
        const auto& lookup = std::find_if(releases.begin(), releases.end(),
            [&](const GitHubRelease& release) { return ( release.tag_name == tag.name ); }
        );

        if( lookup != releases.end() )
            release_option.release = std::move(*lookup);
    }
}


LRESULT ManageReleasesView::OnUpdateReleases(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    try
    {
        m_releasesListCtrl.DeleteAllItems();

        PopulateReleaseOptions();

        for( const ReleaseOption& release_option : m_releaseOptions )
        {
            const bool is_release = release_option.release.has_value();

            m_releasesListCtrl.AddItem(
                TC::ToWide(release_option.name).c_str(),
                is_release ? TC::ToWide(release_option.release->GetReleaseName()).c_str() : L"—",
                is_release ? release_option.release->GetStatus().c_str() : L"—",
                is_release ? TC::ToWide(IntToString(release_option.release->assets_count)).c_str() : L"—",
                TC::ToWide(release_option.commit_sha).c_str()
            );
        }
    }

    catch( const CSProException& exception )
    {
        ErrorMessage::Display(exception);
    }

    return 1;
}


void ManageReleasesView::OnRefreshReleases()
{
    try
    {
        GitHubConnection gh_connection;

        m_tagsJsonText = gh_connection.RequestWithPagination<std::string>(
            gh_connection.CreateApiUrl("tags"),
            true
        );

        m_controller.GetSettingsDb().Write(SettingsKeys::GitHubTags_sv, m_tagsJsonText);

        m_releasesJsonText = gh_connection.RequestWithPagination<std::string>(
            gh_connection.CreateApiUrl("releases"),
            true
        );

        m_controller.GetSettingsDb().Write(SettingsKeys::GitHubReleases_sv, m_releasesJsonText);
    }

    catch( const CSProException& exception )
    {
        ErrorMessage::Display(exception);
    }

    PostMessage(UWM::OpenSourceSyncer::UpdateUI);
}


void ManageReleasesView::DoWithReleaseOption(std::function<void(const ReleaseOption&)> callback_function)
{
    ASSERT(callback_function);

    try
    {
        const int index = m_releasesListCtrl.GetSelectionMark();

        if( index < 0 || m_releasesListCtrl.GetSelectedCount() != 1 )
            throw CSProException("Select a release.");

        ASSERT(static_cast<size_t>(index) < m_releaseOptions.size());

        callback_function(m_releaseOptions[index]);
    }

    catch( const CSProException& exception )
    {
        ErrorMessage::Display(exception);
    }
}


void ManageReleasesView::OnViewRelease()
{
    DoWithReleaseOption(
        [](const ReleaseOption& release_option)
        {
            if( !release_option.release.has_value() )
                throw CSProException("No release exists for tag: " + release_option.name);

            Viewer().ViewHtmlUrl(release_option.release->html_url);
        });
}


void ManageReleasesView::OnCreateRelease()
{
    DoWithReleaseOption(
        [](const ReleaseOption& release_option)
        {
            if( release_option.release.has_value() )
                throw CSProException("A release has already been created: " + release_option.release->GetReleaseName());

            // OS_TODO
        });
}
