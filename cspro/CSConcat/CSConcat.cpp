#include "StdAfx.h"
#include "CSConcat.h"
#include "CScatDlg.h"


/////////////////////////////////////////////////////////////////////////////
// The one and only CCSConcatApp object
CCSConcatApp theApp;


CCSConcatApp::CCSConcatApp()
    :   m_hAccelerators(nullptr)
{
    InitializeCSProEnvironment();

    EnableHtmlHelp();
}


BOOL CCSConcatApp::InitInstance()
{
    CWinApp::InitInstance();

    AfxEnableControlContainer();

    SetRegistryKey(_T("U.S. Census Bureau"));

    CCommandLineInfo cmdInfo;
    ParseCommandLine(cmdInfo);

    CCSConcatDlg dlg;

    if( cmdInfo.m_strFileName.IsEmpty() )
    {
        // add the accelerators to the dialog
        m_hAccelerators = LoadAccelerators(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDR_CSCONCAT_ACCEL));

        m_pMainWnd = &dlg;
        dlg.DoModal();
    }

    else
    {
        dlg.RunBatch(CS2WS(cmdInfo.m_strFileName));
    }

    return FALSE;
}


BOOL CCSConcatApp::ProcessMessageFilter(const int iCode, LPMSG lpMsg)
{
    if( iCode >= 0 && m_pMainWnd != nullptr && m_hAccelerators != nullptr )
    {
        if( ::TranslateAccelerator(m_pMainWnd->m_hWnd, m_hAccelerators, lpMsg) )
            return TRUE;
    }

    return CWinApp::ProcessMessageFilter(iCode, lpMsg);
}
