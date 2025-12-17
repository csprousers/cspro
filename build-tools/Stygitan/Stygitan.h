#pragma once

#include <zUtilO/FileFreeDocManager.h>


class StygitanApp : public CWinApp
{
public:
    StygitanApp();

protected:
    DECLARE_MESSAGE_MAP()

    BOOL InitInstance() override;

    void OnOpenCodePurifier();
    void OnCreateMessageFromFilteredCommits();

    void OnAppAbout();

private:
    void OnOpenCodePurifier(std::string directory);

private:
    FileFreeDocManager* m_fileFreeDocManager;
    CDocTemplate* m_codePurifierDocTemplate;
};
