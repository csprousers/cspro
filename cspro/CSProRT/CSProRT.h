#pragma once


class CSProRuntimeApp : public CWinAppEx
{
public:
    CSProRuntimeApp();

protected:
    DECLARE_MESSAGE_MAP()

    BOOL InitInstance() override;

    void OnAppAbout();
};
