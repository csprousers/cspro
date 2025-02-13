#include "StdAfx.h"
#include "CSBatch.h"
#include "SelectApplicationDlg.h"
#include <ZBRIDGEO/PifDlg.h>
#include <zEngineF/PifInfoPopulator.h>
#include <zBatchF/BatchExecutor.h>


// The one and only CSBatchApp object
CSBatchApp theApp;


BEGIN_MESSAGE_MAP(CSBatchApp, CWinApp)
    ON_COMMAND(ID_HELP, OnHelp)
END_MESSAGE_MAP()


CSBatchApp::CSBatchApp()
{
    InitializeCSProEnvironment();

    EnableHtmlHelp();
}


BOOL CSBatchApp::InitInstance()
{
    AfxOleInit();

    AfxEnableControlContainer();

    // Standard initialization
    SetRegistryKey(_T("U.S. Census Bureau"));

    // open a named event only if one doesn't already exist
    HANDLE event_handle = OpenEvent(EVENT_ALL_ACCESS, FALSE, CSPRO_WNDCLASS_BATCHWND);

    if( event_handle != event_handle )
    {
        CloseHandle(event_handle);
        event_handle = nullptr;
    }

    else
        event_handle = CreateEvent(NULL, TRUE, TRUE, CSPRO_WNDCLASS_BATCHWND);

    // parse the command line
    CIMSACommandLineInfo cmd_info;
    ParseCommandLine(cmd_info);

    CString filename = cmd_info.m_strFileName;

    if( !filename.IsEmpty() )
    {
        // evaluate the full path
        CString current_directory;
        GetCurrentDirectory(_MAX_PATH, current_directory.GetBuffer(_MAX_PATH));
        current_directory.ReleaseBuffer();

        filename = WS2CS(MakeFullPath(current_directory, CS2WS(filename)));
    }

    // run the program
    try
    {
        BatchExecutor batch_executor(this);
        batch_executor.Run(filename);
    }

    catch( const CSProException& exception )
    {
        ErrorMessage::Display(exception);
    }

    // close the named event
    if( event_handle != nullptr )
        CloseHandle(event_handle);

    // inform other programs that batch is finised
    IMSASendMessage(IMSA_WNDCLASS_CSPRO, WM_IMSA_TABCONVERT);

    return FALSE;
}


bool CSBatchApp::QueryForFilename(CString& pff_or_batch_filename)
{
    SelectApplicationDlg dlg;

    if( dlg.DoModal() != IDOK )
        return false;

    pff_or_batch_filename = dlg.GetApplicationFilename();

    return true;
}


bool CSBatchApp::QueryForFileAssociations(CNPifFile& pff, const EngineData& engine_data)
{
    PifInfoPopulator pif_info_populator(engine_data, pff);

    CPifDlg pif_dlg(pif_info_populator.GetPifInfo(), pff.GetEvaluatedAppDescription());
    pif_dlg.m_pPifFile = &pff;

    if( pif_dlg.DoModal() != IDOK )
        return false;

    pff.Save();

    return true;
}
