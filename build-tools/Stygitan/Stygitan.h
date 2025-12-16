#pragma once

#include <zUtilO/FileFreeDocManager.h>


class StygitanApp : public CWinApp
{
protected:
    DECLARE_MESSAGE_MAP()

    BOOL InitInstance() override;

    void OnAppAbout();

private:
    FileFreeDocManager* m_fileFreeDocManager = nullptr;
};
