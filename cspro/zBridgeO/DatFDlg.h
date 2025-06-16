#pragma once


class CDatFDlg : public CFileDialog // CONNECTION_TODO remove class
{
public:
    CDatFDlg(bool allowCSProExtensions,
             BOOL bOpenFileDialog, // TRUE for FileOpen, FALSE for FileSaveAs
             LPCTSTR lpszDefExt = NULL,
             LPCTSTR lpszFileName = NULL,
             DWORD dwFlags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
             LPCTSTR lpszFilter = NULL,
             CWnd* pParentWnd = NULL);

protected:
    BOOL OnFileNameOK() override;

private:
    bool m_bAllowCSProExtensions;
};
