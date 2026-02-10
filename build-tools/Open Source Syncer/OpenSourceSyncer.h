#pragma once

#include <zUtilO/FileFreeDocManager.h>


class OpenSourceSyncerApp : public CWinApp
{
public:
    OpenSourceSyncerApp();

    Controller& GetController() { return *m_controller; }

protected:
    DECLARE_MESSAGE_MAP()

    BOOL InitInstance() override;

    void OnOpenSyncer();

    void OnSettings();

    void OnAppAbout();

private:
    std::optional<Controller> m_controller;
    FileFreeDocManager* m_fileFreeDocManager;
};
