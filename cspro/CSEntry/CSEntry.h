#pragma once
// CSEntry.h : main header file for the ENTRYRUN application
//

#include "BinaryCommandLineInfo.h"
#include <ZBRIDGEO/npff.h>

/////////////////////////////////////////////////////////////////////////////
// CEntryrunApp:
// See entryrun.cpp for the implementation of this class
//

class CWindowFocusMgr;

extern TCHAR DECIMAL_CHAR;


class CEntryrunApp : public CWinApp
{
public:
    CEntryrunApp();
    virtual ~CEntryrunApp();

    CString m_csCurrentDocumentName;
    CNPifFile* m_pPifFile;
    CRunAplEntry* m_pRunAplEntry;

private:
    void CreatePenFile(const TCHAR* filename);
    void OpenApplicationHelper(CString csFilename,bool bForceShowPifDlg = false);
    bool LoadApplication(CString csFilename,bool bForceShowPifDlg = false);
    void PostLoadApplicationOperations();
    void ProcessStartMode();

    bool InitNCompileApp();

    bool ShowPifDlg(bool bSavePif);

    void ApplicationShutdown(bool bCSEntryClosing = false);

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CEntryrunApp)
public:
    virtual BOOL InitInstance();
    virtual int ExitInstance();
    virtual BOOL PreTranslateMessage(MSG* pMsg);
    //}}AFX_VIRTUAL

// Implementation
    //{{AFX_MSG(CEntryrunApp)
    afx_msg void OnAppAbout();
    afx_msg void OnFileOpen();
    afx_msg BOOL OnOpenRecentFile(UINT nID);
    afx_msg void OnOpenDatFile();
    afx_msg void OnUpdateOpenDatFile(CCmdUI* pCmdUI);
    afx_msg void OnUpdateRecentFileMenu(CCmdUI* pCmdUI);
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

private:
    CSEntryBinaryCommandLineInfo m_cmdInfo;
    bool m_bPffLaunchedFromCommandLine;
    CWindowFocusMgr* m_pWindowFocusMgr;
};
