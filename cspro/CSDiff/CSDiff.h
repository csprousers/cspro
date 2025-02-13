#pragma once


class CCSDiffApp : public CWinApp
{
public:
    CCSDiffApp();

protected:
    DECLARE_MESSAGE_MAP()

    BOOL InitInstance() override;

    void OnAppAbout();
    void OnFileOpen();

private:
    HICON m_hIcon;
};
