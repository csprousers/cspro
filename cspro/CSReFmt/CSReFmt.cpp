#include "StdAfx.h"
#include "CSReFmt.h"
#include "ReformatDlg.h"
#include <zReformatO/ToolReformatter.h>


/////////////////////////////////////////////////////////////////////////////
// The one and only CReFmtApp object
CReFmtApp theApp;


CReFmtApp::CReFmtApp()
    :   m_hAccelerators(nullptr)
{
    InitializeCSProEnvironment();

    EnableHtmlHelp();
}


BOOL CReFmtApp::InitInstance()
{
    CWinApp::InitInstance();

    AfxEnableControlContainer();

    SetRegistryKey(_T("U.S. Census Bureau"));

    // Parse command line for standard shell commands, DDE, file open
    CCommandLineInfo cmdInfo;
    ParseCommandLine(cmdInfo);

    try
    {
        if( SO::EqualsNoCase(PortableFunctions::PathGetFileExtension(cmdInfo.m_strFileName), FileExtensions::Pff) )
        {
            PFF pff;
            pff.SetPifFileName(cmdInfo.m_strFileName);

            if( !pff.LoadPifFile() || pff.GetAppType() != REFORMAT_TYPE )
            {
                throw CSProException(_T("PFF file '%s' was not read correctly. Check the file for parameters invalid to the Reformat Data tool."),
                                     cmdInfo.m_strFileName.GetString());
            }

            ToolReformatter().Run(pff, true);

            pff.ExecuteOnExitPff();
        }

        else
        {
            // add the accelerators to the dialog
            m_hAccelerators = LoadAccelerators(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDR_REFORMAT_ACCEL));

            ReformatDlg dlg;
            m_pMainWnd = &dlg;
            dlg.DoModal();
        }
    }

    catch( const CSProException& exception )
    {
        ErrorMessage::Display(exception);
    }

    return FALSE;
}


BOOL CReFmtApp::ProcessMessageFilter(const int iCode, LPMSG lpMsg)
{
    if( iCode >= 0 && m_pMainWnd != nullptr && m_hAccelerators != nullptr )
    {
        if( ::TranslateAccelerator(m_pMainWnd->m_hWnd, m_hAccelerators, lpMsg) )
            return TRUE;
    }

    return CWinApp::ProcessMessageFilter(iCode, lpMsg);
}
