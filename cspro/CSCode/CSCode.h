#pragma once


class CSCodeApp : public CWinAppEx
{
public:
    CSCodeApp();

protected:
    DECLARE_MESSAGE_MAP()

    BOOL InitInstance() override;

    CDocument* OpenDocumentFile(LPCTSTR lpszFileName) override;
	CDocument* OpenDocumentFile(LPCTSTR lpszFileName, BOOL bAddToMRU) override;

    void OnAppAbout();
};
