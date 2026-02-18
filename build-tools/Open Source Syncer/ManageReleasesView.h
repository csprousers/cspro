#pragma once

#include <zUtilF/SortListCtrl.h>


class ManageReleasesView : public CFormView
{
    DECLARE_DYNCREATE(ManageReleasesView)

protected:
    ManageReleasesView();

public:
    ~ManageReleasesView();

protected:
    DECLARE_MESSAGE_MAP()

    void OnInitialUpdate() override;
    void DoDataExchange(CDataExchange* pDX) override;

    LRESULT OnUpdateReleases(WPARAM wParam, LPARAM lParam);

    void OnRefreshReleases();

    void OnViewRelease();

    void OnCreateRelease();

private:
    struct GitHubTag;
    std::vector<GitHubTag> ParseGitHubTags() const;

    struct GitHubRelease;
    std::vector<GitHubRelease> ParseGitHubReleases() const;

    struct ReleaseOption;
    void PopulateReleaseOptions();

    void DoWithReleaseOption(std::function<void(const ReleaseOption&)> callback_function);

private:
    Controller& m_controller;

    CSortListCtrl m_releasesListCtrl;

    std::string m_tagsJsonText;
    std::string m_releasesJsonText;
    std::vector<ReleaseOption> m_releaseOptions;
};
