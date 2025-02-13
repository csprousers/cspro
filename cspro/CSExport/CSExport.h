#pragma once

#include <ZBRIDGEO/PifDlg.h>

class CExportDoc;


class CExportApp : public CWinApp
{
public:
    CExportApp();

    void DeletePifInfos();
    void ManageLanguageDlgBar() const;

    BOOL InitInstance() override;

protected:
    DECLARE_MESSAGE_MAP()

    afx_msg void OnAppAbout();
    afx_msg void OnFileOpen();

public:
    CArray<PIFINFO*,PIFINFO*> m_arrPifInfo;

    CIMSAString m_csModuleName;
    HICON m_hIcon;
    CIMSAString m_csWndClassName;

private:
    CExportDoc* m_pExportDoc;
};
