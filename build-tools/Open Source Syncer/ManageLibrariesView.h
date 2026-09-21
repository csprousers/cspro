#pragma once

#include <zUtilF/SortListCtrl.h>


class ManageLibrariesView : public CFormView
{
    DECLARE_DYNCREATE(ManageLibrariesView)

protected:
    ManageLibrariesView();

protected:
    DECLARE_MESSAGE_MAP()

    void OnInitialUpdate() override;
    void DoDataExchange(CDataExchange* pDX) override;

    LRESULT OnUpdateLibrariesIds(WPARAM wParam, LPARAM lParam);

    void OnRefreshLibrariesIds();

    void OnViewRelease();

    void OnCreateLibraryRelease();

    void OnViewLibraryInputs();

private:
    std::string GetLibrariesIdFromReleaseNotes(const std::string& release_notes) const;

    void RefreshLibrariesIds();

private:
    Controller& m_controller;

    CSortListCtrl m_librariesIdsListCtrl;

    std::string m_releasesJsonText;
    std::regex m_librariesIdRegex;
};
