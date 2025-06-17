#pragma once

#include <zUtilO/ConnectionStringFileSimulator.h>
#include <zBridgeO/PifDlg.h>

class CExportDoc;


class CExportApp : public CWinApp
{
public:
    CExportApp();

    ConnectionStringFileSimulator& GetConnectionStringFileSimulator() { return m_connectionStringFileSimulator; }

    void DeletePifInfos();
    void ManageLanguageDlgBar() const;

protected:
    DECLARE_MESSAGE_MAP()

    BOOL InitInstance() override;

    void OnAppAbout();
    void OnFileOpen();

public:
    CArray<PIFINFO*,PIFINFO*> m_arrPifInfo;

private:
    CExportDoc* m_pExportDoc;
    ConnectionStringFileSimulator m_connectionStringFileSimulator;
};
