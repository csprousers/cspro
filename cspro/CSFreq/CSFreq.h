#pragma once


class CSFreqApp : public CWinApp
{
public:
    CSFreqApp();

    void ManageLanguageDlgBar();

    BOOL InitInstance() override;

protected:
    DECLARE_MESSAGE_MAP()

    afx_msg void OnAppAbout();
    afx_msg void OnFileOpen();

public:
    CIMSAString m_csModuleName;
    HICON m_hIcon;
    CIMSAString m_csWndClassName;
    int m_iReturnCode;
};
