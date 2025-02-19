#include "stdafx.h"
#include "ParadataConcat.h"
#include "ParadataConcatDlg.h"


// The one and only ParadataConcatApp object
ParadataConcatApp theApp;


ParadataConcatApp::ParadataConcatApp()
    :   m_hAccelerators(nullptr)
{
    InitializeCSProEnvironment();

    EnableHtmlHelp();
}


BOOL ParadataConcatApp::InitInstance()
{
    CWinApp::InitInstance();

    AfxEnableControlContainer();

    SetRegistryKey(L"U.S. Census Bureau");

    // add the accelerators to the dialog
    m_hAccelerators = LoadAccelerators(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDR_PARADATACONCAT_ACCEL));

    RunProgram();

    return FALSE;
}


BOOL ParadataConcatApp::ProcessMessageFilter(const int iCode, LPMSG lpMsg)
{
    if( iCode >= 0 && m_pMainWnd != nullptr && m_hAccelerators != nullptr )
    {
        if( ::TranslateAccelerator(m_pMainWnd->m_hWnd, m_hAccelerators, lpMsg) )
            return TRUE;
    }

    return CWinApp::ProcessMessageFilter(iCode, lpMsg);
}


void ParadataConcatApp::RunProgram()
{
    CCommandLineInfo cmd_info;
    ParseCommandLine(cmd_info);
    const std::string pff_file_path = TC::ToUtf8(cmd_info.m_strFileName);

    if( !pff_file_path.empty() ) // just run the PFF
    {
        try
        {
            PFF pff(UTF8_TODO::GetCString(pff_file_path));

            if( !pff.LoadPifFile() || pff.GetAppType() != PARADATA_CONCAT_TYPE )
                throw CSProException("The PFF was not a Paradata Concatenator PFF");

            Paradata::GuiConcatenator::Run(pff);

            pff.ExecuteOnExitPff();
        }

        catch( const CSProException& exception )
        {
            ErrorMessage::Display(exception);
        }

        return;
    }

    // if here, show the UI
    ParadataConcatDlg dlg;
    m_pMainWnd = &dlg;
    dlg.DoModal();
}
