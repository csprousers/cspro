#include "stdafx.h"
#include "ParadataConcat.h"
#include "ParadataConcatDlg.h"


// The one and only CParadataConcatApp object
CParadataConcatApp theApp;

// CParadataConcatApp initialization

CParadataConcatApp::CParadataConcatApp()
    :   m_hAccelerators(nullptr)
{
    InitializeCSProEnvironment();

    EnableHtmlHelp();
}


BOOL CParadataConcatApp::InitInstance()
{
    CWinApp::InitInstance();

    AfxEnableControlContainer();

    SetRegistryKey(_T("U.S. Census Bureau"));

    // add the accelerators to the dialog
    m_hAccelerators = LoadAccelerators(AfxGetInstanceHandle(),MAKEINTRESOURCE(IDR_PARADATACONCAT_ACCEL));

    RunProgram();

    return FALSE;
}


BOOL CParadataConcatApp::ProcessMessageFilter(int iCode,LPMSG lpMsg)
{
    if( ( iCode >= 0 ) && ( m_pMainWnd != nullptr ) && ( m_hAccelerators != nullptr ) )
    {
        if( ::TranslateAccelerator(m_pMainWnd->m_hWnd,m_hAccelerators,lpMsg) )
            return TRUE;
    }

    return CWinApp::ProcessMessageFilter(iCode,lpMsg);
}


void CParadataConcatApp::RunProgram()
{
    CCommandLineInfo cmdInfo;
    ParseCommandLine(cmdInfo);
    CString pff_filename = cmdInfo.m_strFileName;

    if( !pff_filename.IsEmpty() ) // just run the PFF
    {
        try
        {
            PFF pff(pff_filename);

            if( !pff.LoadPifFile() || pff.GetAppType() != PARADATA_CONCAT_TYPE )
                throw CSProException("The PFF was not a Paradata Concatenator PFF");

            Paradata::GuiConcatenatorPffWrapper pff_wrapper(pff);
            Paradata::GuiConcatenator::Run(pff_wrapper);

            pff.ExecuteOnExitPff();
        }

        catch( const CSProException& exception )
        {
            ErrorMessage::Display(exception);
        }

        return;
    }

    // if here, show the UI
    CParadataConcatDlg dlg;
    m_pMainWnd = &dlg;
    dlg.DoModal();
}
