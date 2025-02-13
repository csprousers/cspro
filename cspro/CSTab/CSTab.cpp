// CSTab.cpp : Defines the class behaviors for the application.
//

#include "StdAfx.h"
#include "CSTab.h"
#include "TabDlg.h"
#include <zTableO/Table.h>
#include <zUtilO/CSProExecutables.h>


#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CSTabApp
#define _CALC_

BEGIN_MESSAGE_MAP(CSTabApp, CWinApp)
    ON_COMMAND(ID_HELP,OnHelp)
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////////
//
//  void CSTabApp::PrepareTabRun()
//
/////////////////////////////////////////////////////////////////////////////////
void CSTabApp::PrepareTabRun()
{
    //Fill the CNPifFile Object
    CString sFileName = m_pPIFFile->GetPifFileName();
    m_pPIFFile->ResetContents();
    m_pPIFFile->SetPifFileName(sFileName);
    m_pPIFFile->LoadPifFile();

    //Delete the .lst file if it exists
    CString sLSTFName = m_pPIFFile->GetListingFName();
    DeleteFile(sLSTFName);
}

/////////////////////////////////////////////////////////////////////////////
// CSTabApp construction

CSTabApp::CSTabApp()
{
    InitializeCSProEnvironment();

    EnableHtmlHelp();
    // TODO: add construction code here,
    // Place all significant initialization in InitInstance
}


// The one and only CSTabApp object

CSTabApp theApp;


// CSTabApp initialization

BOOL CSTabApp::InitInstance()
{
    // InitCommonControls() is required on Windows XP if an application
    // manifest specifies use of ComCtl32.dll version 6 or later to enable
    // visual styles.  Otherwise, any window creation will fail.
    InitCommonControls();


    AfxEnableControlContainer();

    CCommandLineInfo cmdInfo;
    ParseCommandLine(cmdInfo);

    CString sFileName = cmdInfo.m_strFileName;

    bool bPffLaunchedFromCommandLine = SO::EqualsNoCase(PortableFunctions::PathGetFileExtension(sFileName), FileExtensions::Pff);

    // Try to open the named event
    HANDLE ahEvent = OpenEvent( EVENT_ALL_ACCESS, FALSE, CSPRO_WNDCLASS_BATCHWND);
    if(ahEvent == NULL){
        //&&& TO DO define an event for tab //Single instance check in CSpro
        CreateEvent(NULL, TRUE, TRUE, CSPRO_WNDCLASS_BATCHWND );
    }

    m_pPIFFile = new CNPifFile(sFileName);
    m_pPIFFile->SetAppType(TAB_TYPE);

    CSTabDlg dlg;
    dlg.m_pPIFFile = m_pPIFFile;
    dlg.m_sFileName = sFileName;

    //m_pMainWnd = &dlg;
    m_pMainWnd = NULL;

    if( dlg.DoModal() != IDOK )
        return FALSE;

    PrepareTabRun();

    CleanFiles();

    Application& application = *m_pPIFFile->GetApplication();

    auto pDict = application.GetTabSpec()->GetSharedDictionary();
    application.SetEngineAppType(EngineAppType::Batch);

    auto pOrder = std::make_shared<CDEFormFile>();
    pOrder->CreateOrderFile(*pDict, true);
    pOrder->SetName(application.GetTabSpec()->GetName());
    pOrder->SetDictionary(pDict);
    application.AddRuntimeFormFile(pOrder);

    pOrder->UpdatePointers();

    DictionaryDescription* dictionary_description = application.AddDictionaryDescription(DictionaryDescription(CS2WS(pDict->GetName()), CS2WS(pOrder->GetName()), DictionaryType::Input));
    dictionary_description->SetDictionary(pDict.get());

    CRunTab runTab;
    runTab.InitRun(m_pPIFFile,m_pPIFFile->GetApplication());

    PROCESS eCurrentProcess = m_pPIFFile->GetTabProcess();
    bool bRet = runTab.Exec(false);

    application.GetRuntimeFormFiles().clear();
    application.SetDictionaryDescriptions({ });

    if(bRet && (eCurrentProcess == CS_PREP|| eCurrentProcess == ALL_STUFF) && m_pPIFFile->GetViewResultsFlag()) {
        //Save .TBW
        CString sTabSpecFName = application.GetTabSpecFilenames().front();
        CString sTbwFileName = m_pPIFFile->GetPrepOutputFName();
        if(sTbwFileName.IsEmpty()){
            ASSERT(FALSE);//Cannot be empty
            sTbwFileName =sTabSpecFName;
            PathRemoveExtension(sTbwFileName.GetBuffer(_MAX_PATH));
            sTbwFileName.ReleaseBuffer();
            sTbwFileName += FileExtensions::WithDot::Table;
            CString sDictFName = application.GetTabSpec()->GetDictFile();
            application.GetTabSpec()->Save(sTbwFileName,sDictFName); //Save .TBW
        }

        //Here launch .tbw
        CFileStatus fStatus;
        BOOL bTBWExists = CFile::GetStatus(sTbwFileName,fStatus);
        if(bTBWExists){
            const std::optional<std::wstring> tblview_exe =  CSProExecutables::GetExecutablePath(CSProExecutables::Program::TblView);
            if(tblview_exe.has_value()) {
                IMSASpawnApp(*tblview_exe, IMSA_WNDCLASS_TABLEVIEW, sTbwFileName, TRUE);
            }
        }
    }

    if( bRet && bPffLaunchedFromCommandLine )
        m_pPIFFile->ExecuteOnExitPff();

    // Since the dialog has been closed, return FALSE so that we exit the
    //  application, rather than start the application's message pump.
    return FALSE;
}

/////////////////////////////////////////////////////////////////////////////////
//
//  bool CSTabApp::InitNCompileApp()
//
/////////////////////////////////////////////////////////////////////////////////
bool CSTabApp::InitNCompileApp()
{
    bool bRet = false;
    ASSERT(m_pPIFFile);
    CString sPifFileName = m_pPIFFile->GetPifFileName();
    m_pPIFFile->SetViewListing(ONERROR);
    CString sTemp(sPifFileName);
    CString sExt = PathFindExtension(sTemp.GetBuffer(_MAX_PATH));
    sTemp.ReleaseBuffer();

    CString sFileName(sPifFileName);
    PathRemoveExtension(sFileName.GetBuffer(_MAX_PATH));
    sFileName.ReleaseBuffer();

    if(sExt.IsEmpty()) {
        sExt += FileExtensions::WithDot::TabulationApplication;
    }
    if(sExt.CompareNoCase(FileExtensions::WithDot::Pff) ==0) {
        //Check if the PffFile is valid
        sFileName += FileExtensions::WithDot::Pff;
        m_pPIFFile->ResetContents();
        m_pPIFFile->SetPifFileName(sFileName);
        m_pPIFFile->LoadPifFile();
    }
    else if (sExt.CompareNoCase(FileExtensions::WithDot::TabulationApplication) == 0) {
        CString sAppFName = sFileName + FileExtensions::WithDot::TabulationApplication;
        sPifFileName = sAppFName + FileExtensions::WithDot::Pff;
        m_pPIFFile->SetPifFileName(sPifFileName);
        m_pPIFFile->SetAppFName(sAppFName);
    }

    else {
        if(!sFileName.IsEmpty()){
            AfxMessageBox(_T("Invalid File Type"));
        }
        return false;
    }

    CWaitCursor wait1;
    if(m_pPIFFile->BuildAllObjects()){
        //for now &&
        bRet = true;
    }
    else {
        return bRet;
    }

    return bRet;
}



/////////////////////////////////////////////////////////////////////////////////
//
//  void CSTabApp::CleanFiles()
//
/////////////////////////////////////////////////////////////////////////////////
void CSTabApp::CleanFiles()
{
    //Get the lst file name
    CString sLSTFName = m_pPIFFile->GetListingFName();

    IMSASendMessage(IMSA_WNDCLASS_TEXTVIEW, WM_IMSA_FILECLOSE, sLSTFName);
    CFileStatus fStatus;
    BOOL bExists = CFile::GetStatus(sLSTFName,fStatus);
    if(bExists) {
        DeleteFile(sLSTFName);
    }

    CString sWriteFName = m_pPIFFile->GetWriteFName();
    if (!sWriteFName.IsEmpty() && m_pPIFFile->GetViewResultsFlag()) {
        IMSASendMessage(IMSA_WNDCLASS_TEXTVIEW, WM_IMSA_FILECLOSE, sWriteFName);
        bExists = CFile::GetStatus(sWriteFName,fStatus);
        if(bExists) {
            DeleteFile(sWriteFName);
        }
    }
}


/////////////////////////////////////////////////////////////////////////////////
//
//  int CSTabApp::ExitInstance()
//
/////////////////////////////////////////////////////////////////////////////////
int CSTabApp::ExitInstance()
{
    delete m_pPIFFile;
    return CWinApp::ExitInstance();
}
