#include "StdAfx.h"
#include "CSTab.h"
#include "TabDlg.h"
#include <zUtilO/CSProExecutables.h>
#include <zUtilF/CommonControls.h>
#include <zFormO/FormFile.h>


// The one and only CSTabApp object
CSTabApp theApp;


BEGIN_MESSAGE_MAP(CSTabApp, CWinApp)
    ON_COMMAND(ID_HELP,OnHelp)
END_MESSAGE_MAP()


CSTabApp::CSTabApp()
{
    InitializeCSProEnvironment();

    EnableHtmlHelp();
}


BOOL CSTabApp::InitInstance()
{
    InitializeCommonControls();

    AfxEnableControlContainer();

    CCommandLineInfo cmdInfo;
    ParseCommandLine(cmdInfo);

    std::string file_path = TC::ToUtf8(cmdInfo.m_strFileName);

    const bool pff_launched_from_command_line = SO::EqualsNoCase(PortableFunctions::PathGetFileExtension(file_path), FileExtensions::Pff);

    // Try to open the named event
    HANDLE ahEvent = OpenEvent(EVENT_ALL_ACCESS, FALSE, CSPRO_WNDCLASS_BATCHWND);

    if( ahEvent == nullptr )
    {
        //&&& TO DO define an event for tab //Single instance check in CSpro
        CreateEvent(NULL, TRUE, TRUE, CSPRO_WNDCLASS_BATCHWND );
    }

    m_pff = std::make_unique<CNPifFile>(UTF8_TODO::GetCString(file_path));
    m_pff->SetAppType(APPTYPE::TAB_TYPE);

    CSTabDlg dlg(m_pff, file_path);

    //m_pMainWnd = &dlg;
    m_pMainWnd = NULL;

    if( dlg.DoModal() != IDOK )
        return FALSE;

    PrepareTabRun();

    CleanFiles();

    Application& application = *m_pff->GetApplication();
    application.SetEngineAppType(EngineAppType::Batch);

    const std::shared_ptr<const CDataDict> dictionary = application.GetTabSpec()->GetSharedDictionary();
    ASSERT(dictionary != nullptr);

    auto order = std::make_shared<CDEFormFile>();
    order->CreateOrderFile(*dictionary, true);
    order->SetName(application.GetTabSpec()->GetName());
    order->SetDictionary(dictionary);
    application.AddRuntimeFormFile(order);

    order->UpdatePointers();

    DictionaryDescription* const dictionary_description = application.AddDictionaryDescription(DictionaryDescription(dictionary->GetFilePath(), order->GetFilePath(), DictionaryType::Input));
    dictionary_description->SetDictionary(dictionary.get());

    CRunTab runTab;
    runTab.InitRun(m_pff.get(), m_pff->GetApplication());

    PROCESS eCurrentProcess = m_pff->GetTabProcess();
    bool bRet = runTab.Exec(false);

    application.GetRuntimeFormFiles().clear();
    application.SetDictionaryDescriptions({ });

    if(bRet && (eCurrentProcess == CS_PREP|| eCurrentProcess == ALL_STUFF) && m_pff->GetViewResultsFlag()) {
        //Save .TBW
        CString sTabSpecFName = UTF8_TODO::GetCString(application.GetTableSpecFilePaths().front());
        CString sTbwFileName = m_pff->GetPrepOutputFName();
        if(sTbwFileName.IsEmpty()){
            ASSERT(FALSE);//Cannot be empty
            sTbwFileName =sTabSpecFName;
            PathRemoveExtension(sTbwFileName.GetBuffer(_MAX_PATH));
            sTbwFileName.ReleaseBuffer();
            sTbwFileName += UTF8_TODO::GetCString(FileExtensions::WithDot(FileExtensions::Table));
            CString sDictFName = application.GetTabSpec()->GetDictFile();
            application.GetTabSpec()->Save(sTbwFileName,sDictFName); //Save .TBW
        }

        //Here launch .tbw
        CFileStatus fStatus;
        BOOL bTBWExists = CFile::GetStatus(sTbwFileName,fStatus);
        if(bTBWExists){
            const std::optional<std::string> tblview_exe = CSProExecutables::GetExecutablePath(CSProExecutables::Program::TblView);
            if(tblview_exe.has_value()) {
                IMSASpawnApp(UTF8_TODO::GetWide(*tblview_exe), IMSA_WNDCLASS_TABLEVIEW, sTbwFileName, TRUE);
            }
        }
    }

    if( bRet && pff_launched_from_command_line )
        m_pff->ExecuteOnExitPff();

    // Since the dialog has been closed, return FALSE so that we exit the
    //  application, rather than start the application's message pump.
    return FALSE;
}


bool CSTabApp::InitNCompileApp()
{
    ASSERT(m_pff != nullptr);
    bool bRet = false;
    CString sPifFileName = m_pff->GetPifFileName();
    m_pff->SetViewListing(ONERROR);
    std::string extension = PortableFunctions::PathGetFileExtension(UTF8_TODO::GetUtf8(sPifFileName));

    CString sFileName(sPifFileName);
    PathRemoveExtension(sFileName.GetBuffer(_MAX_PATH));
    sFileName.ReleaseBuffer();

    if(extension.empty()) {
        extension = FileExtensions::TabulationApplication;
    }

    if( SO::EqualsNoCase(extension, FileExtensions::Pff) )
    {
        //Check if the PffFile is valid
        sFileName += UTF8_TODO::GetCString(FileExtensions::WithDot(FileExtensions::Pff));
        m_pff->ResetContents();
        m_pff->SetPifFileName(sFileName);
        m_pff->LoadPifFile();
    }

    else if( SO::EqualsNoCase(extension, FileExtensions::TabulationApplication) )
    {
        CString sAppFName = sFileName + UTF8_TODO::GetCString(FileExtensions::WithDot(FileExtensions::TabulationApplication));
        sPifFileName = sAppFName + UTF8_TODO::GetCString(FileExtensions::WithDot(FileExtensions::Pff));
        m_pff->SetPifFileName(sPifFileName);
        m_pff->SetAppFName(sAppFName);
    }

    else
    {
        if( !sFileName.IsEmpty() )
            AfxMessageBox(L"Invalid File Type");

        return false;
    }

    CWaitCursor wait1;
    if(m_pff->BuildAllObjects()){
        //for now &&
        bRet = true;
    }
    else {
        return bRet;
    }

    return bRet;
}


void CSTabApp::CleanFiles()
{
    //Get the lst file name
    CString sLSTFName = m_pff->GetListingFName();

    IMSASendMessage(IMSA_WNDCLASS_TEXTVIEW, WM_IMSA_FILECLOSE, sLSTFName);
    CFileStatus fStatus;
    BOOL bExists = CFile::GetStatus(sLSTFName,fStatus);
    if(bExists) {
        DeleteFile(sLSTFName);
    }

    CString sWriteFName = m_pff->GetWriteFName();
    if (!sWriteFName.IsEmpty() && m_pff->GetViewResultsFlag()) {
        IMSASendMessage(IMSA_WNDCLASS_TEXTVIEW, WM_IMSA_FILECLOSE, sWriteFName);
        bExists = CFile::GetStatus(sWriteFName,fStatus);
        if(bExists) {
            DeleteFile(sWriteFName);
        }
    }
}


void CSTabApp::PrepareTabRun()
{
    //Fill the CNPifFile Object
    std::string pff_file_path = UTF8_TODO::GetUtf8(m_pff->GetPifFileName());

    m_pff->ResetContents();
    m_pff->SetPifFileName(UTF8_TODO::GetCString(std::move(pff_file_path)));
    m_pff->LoadPifFile();

    //Delete the .lst file if it exists
    PortableFunctions::FileDelete(m_pff->GetListingFName());
}
