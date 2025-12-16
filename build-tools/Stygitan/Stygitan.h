#pragma once


class StygitanApp : public CWinApp
{
protected:
    DECLARE_MESSAGE_MAP()

    BOOL InitInstance() override;

    void OnAppAbout();
};
