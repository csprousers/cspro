#pragma once


class CSViewApp : public CWinAppEx
{
public:
    CSViewApp();

protected:
    DECLARE_MESSAGE_MAP()

    BOOL InitInstance() override;

    void OnAppAbout();
};
