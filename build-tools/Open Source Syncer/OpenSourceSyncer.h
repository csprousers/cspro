#pragma once

#include <zUtilO/FileFreeDocManager.h>


class OpenSourceSyncerApp : public CWinApp
{
public:
    OpenSourceSyncerApp();

protected:
    DECLARE_MESSAGE_MAP()

    BOOL InitInstance() override;

    void OnOpenSyncer();

    void OnAppAbout();

private:
    FileFreeDocManager* m_fileFreeDocManager;
};
