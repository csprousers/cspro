//***************************************************************************
//  File name: CSIndex.cpp
//
//  Description:
//       Application framework for CSIndex; process PFF code here
//
//  History:      Date       Author       Comment
//                -------------------------------
//    (created)   2009-10-07  GHM
//    (rewritten) 2019-12-05
//
//***************************************************************************

#include "stdafx.h"
#include "CSIndex.h"
#include "IndexDlg.h"
#include "ToolIndexer.h"


/////////////////////////////////////////////////////////////////////////////
// The one and only CCSIndexApp object
CCSIndexApp theApp;


CCSIndexApp::CCSIndexApp()
    :   m_hAccelerators(nullptr)
{
    InitializeCSProEnvironment();

    EnableHtmlHelp();
}


BOOL CCSIndexApp::InitInstance()
{
    CWinApp::InitInstance();

    AfxEnableControlContainer();

    SetRegistryKey(L"U.S. Census Bureau");

    // Parse command line for standard shell commands, DDE, file open
    CCommandLineInfo cmdInfo;
    ParseCommandLine(cmdInfo);

    try
    {
        std::string dictionary_file_path;
        bool show_dialog = true;

        if( !cmdInfo.m_strFileName.IsEmpty() )
        {
            std::string file_path = TC::ToUtf8(cmdInfo.m_strFileName);
            const std::string extension = PortableFunctions::PathGetFileExtension(file_path);

            // execute a PFF
            if( SO::EqualsNoCase(extension, FileExtensions::Pff) )
            {
                show_dialog = false;

                PFF pff;
                pff.SetPifFileName(UTF8_TODO::GetCString(file_path));

                if( !pff.LoadPifFile() || pff.GetAppType() != INDEX_TYPE )
                {
                    throw CSProException("PFF file '%s' was not read correctly. Check the file for parameters invalid to CSIndex.",
                                         file_path.c_str());
                }

                ToolIndexer().Run(pff, true);

                pff.ExecuteOnExitPff();
            }

            // prefill the dictionary name
            else if( SO::EqualsNoCase(extension, FileExtensions::Dictionary) )
            {
                dictionary_file_path = std::move(file_path);
            }

            else
            {
                throw CSProException("The parameter '%s' is invalid. It must be a file with the extension PFF.",
                                     file_path.c_str());
            }
        }

        if( show_dialog )
        {
            // add the accelerators to the dialog
            m_hAccelerators = LoadAccelerators(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDR_CSINDEX_ACCEL));

            IndexDlg dlg(std::move(dictionary_file_path));
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


BOOL CCSIndexApp::ProcessMessageFilter(const int iCode, LPMSG lpMsg)
{
    if( iCode >= 0 && m_pMainWnd != nullptr && m_hAccelerators != nullptr )
    {
        if( ::TranslateAccelerator(m_pMainWnd->m_hWnd, m_hAccelerators, lpMsg) )
            return TRUE;
    }

    return CWinApp::ProcessMessageFilter(iCode, lpMsg);
}
