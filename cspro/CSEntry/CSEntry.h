#pragma once
// CSEntry.h : main header file for the ENTRYRUN application
//

#include "BinaryCommandLineInfo.h"
#include <zBridgeO/NPff.h>

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
    ~CEntryrunApp();

    CNPifFile* m_pPifFile;
    CRunAplEntry* m_pRunAplEntry;

private:
    void CreatePenFile(const std::string& application_file_path);
    void OpenApplicationHelper(std::string application_file_path, bool force_show_file_associations = false);
    bool LoadApplication(const std::string& file_path, bool force_show_file_associations = false);
    void PostLoadApplicationOperations();
    void ProcessStartMode();

    bool InitNCompileApp();

    bool ShowPifDlg(bool save_pff);

    void ApplicationShutdown(bool csentry_closing = false);

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
    std::unique_ptr<CWindowFocusMgr> m_pWindowFocusMgr;
    CSEntryBinaryCommandLineInfo m_cmdInfo;
    bool m_pffLaunchedFromCommandLine;
    std::string m_currentApplicationFilePath;
};
