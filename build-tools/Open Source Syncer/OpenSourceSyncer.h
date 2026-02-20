#pragma once

#include <zUtilO/FileFreeDocManager.h>


class OpenSourceSyncerApp : public CWinApp
{
public:
    OpenSourceSyncerApp();

    Controller& GetController() { return *m_controller; }

    void OpenLog();

protected:
    DECLARE_MESSAGE_MAP()

    BOOL InitInstance() override;

    void OnSyncFeatureBranches();

    void OnManuallyMirrorCommits();

    void OnManageReleases();

    void OnSyncTags();

    void OnManageLibraries();

    void OnCompareRepositories();

    void OnSettings();
    void OnUpdateSettings(CCmdUI* pCmdUI);

    void OnAppAbout();

private:
    std::optional<Controller> m_controller;
    FileFreeDocManager* m_fileFreeDocManager;
};
