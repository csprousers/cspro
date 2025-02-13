#pragma once
// CSTab.h : main header file for the PROJECT_NAME application
//

#ifndef __AFXWIN_H__
    #error include 'stdafx.h' before including this file for PCH
#endif


// CSTabApp:
// See CSTab.cpp for the implementation of this class
//
#include <CSTab/TabDlg.h>
class CSTabApp : public CWinApp
{
   friend class CSTabDlg;
public:
    CSTabApp();
    bool InitNCompileApp();

public:
    //PROCESS  m_eProcess;
private:
    CNPifFile* m_pPIFFile;
    void CleanFiles();
    void PrepareTabRun();
// Overrides
    public:
    virtual BOOL InitInstance();
    virtual int ExitInstance();

// Implementation

    DECLARE_MESSAGE_MAP()
};

extern CSTabApp theApp;
