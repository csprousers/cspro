#include "StdAfx.h"
#include "TabDlg.h"
#include "CSTab.h"
#include <zToolsO/WinSettings.h>
#include <zUtilO/FileDlg.h>
#include <zUtilO/imsaDlg.H>
#include <zJson/JsonStream.h>
#include <ZBRIDGEO/PifDlg.h>


BEGIN_MESSAGE_MAP(CSTabDlg, CDialog)
    ON_WM_SYSCOMMAND()
    ON_WM_PAINT()
    ON_WM_QUERYDRAGICON()
    ON_BN_CLICKED(IDC_LOCATE, OnLocate)
END_MESSAGE_MAP()


CSTabDlg::CSTabDlg(std::shared_ptr<CNPifFile> pff, std::string application_file_path, CWnd* const pParent/* = nullptr*/)
    :   CDialog(IDD_CSTAB_DIALOG, pParent),
        m_pff(std::move(pff)),
        m_sFileName(UTF8_TODO::GetCString(std::move(application_file_path))),
        m_hIcon(AfxGetApp()->LoadIcon(IDR_MAINFRAME))
{
    ASSERT(m_pff != nullptr);
}


void CSTabDlg::DoDataExchange(CDataExchange* const pDX)
{
    __super::DoDataExchange(pDX);

    DDX_Text(pDX, IDC_FILENAME, m_sFileName);
}


BOOL CSTabDlg::OnInitDialog()
{
    __super::OnInitDialog();

    // Add "About..." menu item to system menu.

    // IDM_ABOUTBOX must be in the system command range.
    ASSERT(( IDM_ABOUTBOX & 0xFFF0 ) == IDM_ABOUTBOX);
    ASSERT(IDM_ABOUTBOX < 0xF000);

    CMenu* const pSysMenu = GetSystemMenu(FALSE);

    if( pSysMenu != nullptr )
    {
        CString strAboutMenu;
        strAboutMenu.LoadString(IDS_ABOUTBOX);

        if( !strAboutMenu.IsEmpty() )
        {
            pSysMenu->AppendMenu(MF_SEPARATOR);
            pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
        }
    }

    // Set the icon for this dialog.  The framework does this automatically
    //  when the application's main window is not a dialog
    SetIcon(m_hIcon, TRUE);         // Set big icon
    SetIcon(m_hIcon, FALSE);        // Set small icon

    UpdateData(FALSE);

    if( !m_sFileName.IsEmpty() && GetSafeHwnd() != nullptr )
    {
        m_pff->SetSilent(true);
        PostMessage(WM_COMMAND, IDOK);
    }

    return TRUE;  // return TRUE  unless you set the focus to a control
}


void CSTabDlg::OnSysCommand(const UINT nID, const LPARAM lParam)
{
    if( ( nID & 0xFFF0 ) == IDM_ABOUTBOX )
    {
        CIMSAAboutDlg about_dlg(L"CSTab", m_hIcon);
        about_dlg.DoModal();
    }

    else
    {
        __super::OnSysCommand(nID, lParam);
    }
}


// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CSTabDlg::OnPaint()
{
    if (IsIconic())
    {
        CPaintDC dc(this); // device context for painting

        SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

        // Center icon in client rectangle
        int cxIcon = GetSystemMetrics(SM_CXICON);
        int cyIcon = GetSystemMetrics(SM_CYICON);
        CRect rect;
        GetClientRect(&rect);
        int x = (rect.Width() - cxIcon + 1) / 2;
        int y = (rect.Height() - cyIcon + 1) / 2;

        // Draw the icon
        dc.DrawIcon(x, y, m_hIcon);
    }

    else
    {
        __super::OnPaint();
    }
}


// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CSTabDlg::OnQueryDragIcon()
{
    return static_cast<HCURSOR>(m_hIcon);
}


void CSTabDlg::OnLocate()
{
    UpdateData(TRUE);

    OpenFileDlg open_file_dlg(0, nullptr, nullptr,
                              L"Tabulation Application Files (*.xtb)|*.xtb|PFF Files (*.pff)|*.pff||", this);

    // if the file exists, start in its directory
    if( PortableFunctions::FileIsRegular(m_sFileName) )
    {
        open_file_dlg.SetInitialDirectory(PortableFunctions::PathGetDirectory(UTF8_TODO::GetUtf8(m_sFileName)));
    }

    // otherwise use the last application directory
    else
    {
        open_file_dlg.SetInitialDirectory(WinSettings::Read<std::wstring>(WinSettings::Type::LastApplicationDirectory));
    }

    if( open_file_dlg.DoModal() == IDOK )
    {
        m_sFileName = UTF8_TODO::GetCString(open_file_dlg.GetFilePath());

        WinSettings::Write(WinSettings::Type::LastApplicationDirectory, PortableFunctions::PathGetDirectory(UTF8_TODO::GetUtf8(m_sFileName)));

        UpdateData(FALSE);
    }

    Invalidate();
    UpdateWindow();
}


void CSTabDlg::OnOK()
{
    //Get the file extension and see if it is pff / .apl if it is pff we are done
    //if it is apl file select the data file
    //Check if the file is .pff / .apl
    UpdateData(TRUE);

    std::string extension = PortableFunctions::PathGetFileExtension(UTF8_TODO::GetUtf8(m_sFileName));

    CString sFileName(m_sFileName);
    PathRemoveExtension(sFileName.GetBuffer(_MAX_PATH));
    sFileName.ReleaseBuffer();

    m_pff->SetPifFileName(m_sFileName);
    m_pff->SetAppType(TAB_TYPE);

    if( extension.empty() )
    {
        extension = FileExtensions::TabulationApplication;
        m_sFileName += L"." + UTF8_TODO::GetCString(FileExtensions::TabulationApplication);
        UpdateData(FALSE);
        m_pff->SetPifFileName(m_sFileName);
    }

    if( SO::EqualsNoCase(extension, FileExtensions::Pff) )
    {
        //Check if the PffFile is valid
        sFileName += UTF8_TODO::GetCString(FileExtensions::WithDot(FileExtensions::Pff));
        m_sFileName = sFileName;
        UpdateData(FALSE);
        m_pff->SetSilent(true);
        m_pff->SetPifFileName(m_sFileName);
        if(!((CSTabApp*)AfxGetApp())->InitNCompileApp()) {
            m_pff->SetSilent(false);
            return ;
        }
        if(!CheckNCollectInputFiles()){
            m_pff->SetSilent(false);
            return;
        }
        else {//If All stuff prepare intermediate files
            if(m_pff->GetTabProcess() == ALL_STUFF){
                CString sTabOutPutFName = m_pff->GetTabOutputFName();
                CString sTempTab;
                if(sTabOutPutFName.IsEmpty()) {
                    CString sAplFName = m_pff->GetAppFName();
                    PathRemoveExtension(sAplFName.GetBuffer(_MAX_PATH));
                    sAplFName.ReleaseBuffer();
                    sTabOutPutFName = sAplFName + UTF8_TODO::GetCString(FileExtensions::WithDot(FileExtensions::BinaryTable::Tab));
                    m_pff->SetTabOutputFName(sTabOutPutFName);
                    sTempTab = sAplFName + L"_precalc" +  UTF8_TODO::GetCString(FileExtensions::WithDot(FileExtensions::BinaryTable::Tab)); // Engine is not looking at the piffile ags

                }
                if(m_pff->GetCalcInputFNamesArr().empty()) {
                    m_pff->GetCalcInputFNamesArr().emplace_back(sTempTab);
                }
                CString sCalcOFName = m_pff->GetCalcOutputFName();
                if(sCalcOFName.IsEmpty()){
                    m_pff->SetCalcOutputFName(sTabOutPutFName);
                }
            }
        }
    }

    else if( SO::EqualsNoCase(extension, FileExtensions::TabulationApplication) )
    {
        //If the file is of .apl type ShowDialog selection for the pff file generation
        //change the lpszFileName
        CFileStatus fStatus;
        m_pff->SetPifFileName(m_sFileName + UTF8_TODO::GetCString(FileExtensions::WithDot(FileExtensions::Pff)));
        if(CFile::GetStatus(m_sFileName + UTF8_TODO::GetCString(FileExtensions::WithDot(FileExtensions::Pff)), fStatus)){
           m_pff->LoadPifFile();
        }

        if(!((CSTabApp*)AfxGetApp())->InitNCompileApp()) {
            return;
        }
        if(!MakePifFile()){
        //  AfxMessageBox("Failed to generate PFF file");
            return;
        }
        else {
            m_sFileName += UTF8_TODO::GetCString(FileExtensions::WithDot(FileExtensions::Pff));
            m_pff->Save();
            UpdateData(FALSE);
        }

    }

    else
    {
        if( !sFileName.IsEmpty() )
            AfxMessageBox(L"Invalid File Type");

        return;
    }

    Invalidate();
    UpdateWindow();

    __super::OnOK();
}


bool CSTabDlg::MakePifFile()
{
    CRunTab runTab;
    PROCESS eProcess = PROCESS_INVALID;
    if(runTab.PreparePFF(m_pff.get(), eProcess, false)){
        //((CSTabApp*)AfxGetApp())->m_eProcess = eProcess;
    }
    else {
        return false;
    }
    //Then show the appropriate dialog box .

    return true;
}


bool CSTabDlg::CheckNCollectInputFiles()
{
    bool bRet = false;
    CRunTab runTab;
    if(!BuildPifInfo4Check()){
        PROCESS eCurrentProcess = m_pff->GetTabProcess();
        bRet = runTab.PreparePFF(m_pff.get(), eCurrentProcess, false);
        if(bRet && !m_pff->GetSilent())
            m_pff->Save();
    }
    else {
        bRet = true;
    }
    return bRet;
}


bool CSTabDlg::BuildPifInfo4Check()
{
    //Prepare pifinfo as if the dialog is going to be called and the check files similar to  CTRunInfoDlg::CheckFiles()
    CArray<PIFINFO,PIFINFO>  arrPifInfo;
    //Get the application file
    CFileStatus fStatus;
    PROCESS eCurrentProcess = m_pff->GetTabProcess();

    Application* pApp = m_pff->GetApplication();
    CString sXTSFile= pApp->GetTabSpec()->GetSpecFile();
    bool bHasArea = pApp->GetTabSpec()->GetConsolidate()->GetNumAreas() > 0;
    const CDataDict* pDict = pApp->GetTabSpec()->GetDict();
    //Get the dictionary file . This is the Inputdata
    if(pDict && (eCurrentProcess == ALL_STUFF || eCurrentProcess == CS_TAB))
    {
        {
            PIFINFO PifInfo;
            PifInfo.eType = PIFDICT;
            PifInfo.sUName = UTF8_TODO::GetCString(pDict->GetName());
            PifInfo.sDisplay = INPUTDATA;
            PifInfo.dictionary_file_path = pDict->GetFilePath();
            PifInfo.SetConnectionStrings(m_pff->GetInputDataConnectionStringsSerializable());
            arrPifInfo.Add(PifInfo);
        }

        //Now do each of the external dicts
        for( const std::string& dictionary_file_path : pApp->GetExternalDictionaryFilePaths() )
        {
            const DictionaryDescription* const dictionary_description = pApp->GetDictionaryDescription(dictionary_file_path);

            if( dictionary_description != nullptr && dictionary_description->GetDictionaryType() == DictionaryType::Working )
                continue;

            try
            {
                CString dictionary_name = JsonStream::GetValueFromSpecFile<CString, CDataDict>(JK::name, dictionary_file_path);

                PIFINFO PifInfo;
                PifInfo.eType = PIFDICT;
                PifInfo.sUName = dictionary_name;
                PifInfo.sDisplay = L"External File ";
                PifInfo.sDisplay += L"(" + dictionary_name + L")";
                PifInfo.dictionary_file_path = dictionary_file_path;
                PifInfo.SetConnectionString(m_pff->GetExternalDataConnectionString(dictionary_name));

                //Add pff Info for the dict
                arrPifInfo.Add(PifInfo);
            }

            catch( const CSProException& exception )
            {
                ErrorMessage::Display(exception);
                return false;
            }
        }

        //Now check for write file
        if(pApp->GetHasWriteStatements()) { //only for tab and All
            PIFINFO PifInfo;
            PifInfo.eType = FILE_NONE;
            PifInfo.sUName = WRITEFILE;
            PifInfo.sFileName = m_pff->GetWriteFName(true);
            PifInfo.sDisplay = WRITEFILE;
            arrPifInfo.Add(PifInfo);
        }

        //Now add the OutputTBD
        if( eCurrentProcess == CS_TAB) { //only for tab
            PIFINFO PifInfo;
            PifInfo.eType = FILE_NONE;
            PifInfo.sUName = OUTPUTTBD;
            PifInfo.sFileName = m_pff->GetTabOutputFName(); //Get the output TBD from the pff file
            PifInfo.sDisplay = OUTPUTTBD;
            arrPifInfo.Add(PifInfo);
        }
        if(eCurrentProcess == ALL_STUFF ) {
            if(true){//requires output tbw in CSTab
                PIFINFO PifInfo;
                PifInfo.eType = FILE_NONE;
                PifInfo.sUName = OUTPUTTBW;
                PifInfo.sFileName = m_pff->GetPrepOutputFName();
                PifInfo.sDisplay = OUTPUTTBW;
                arrPifInfo.Add(PifInfo);
            }
            if(bHasArea) { //If it has area ask for areanames file
                PIFINFO PifInfo;
                PifInfo.eType = FILE_NONE;
                PifInfo.sUName = AREANAMES;
                PifInfo.sFileName = m_pff->GetAreaFName(); //Get Area names file
                PifInfo.sDisplay = AREANAMES;
                arrPifInfo.Add(PifInfo);
            }
        }
        //Now do the listing file
        if(true){
            //Now do the listing file
            PIFINFO PifInfo;
            if(!m_pff->GetListingFName().IsEmpty()){
                PifInfo.sFileName = m_pff->GetListingFName();
            }
            //do listing
            PifInfo.eType = FILE_NONE;
            PifInfo.sUName = LISTFILE;
            //PifInfo.sFileName = m_pff->GetListingFName();
            PifInfo.sDisplay = LISTFILE;
            arrPifInfo.Add(PifInfo);
        }
    }
    else if(eCurrentProcess == CS_CON || eCurrentProcess == CS_CALC) {
        //if Con  || Calc
        PIFINFO PifInfo;
        PifInfo.eType = FILE_NONE;
        PifInfo.sUName = INPUTTBD;
        PifInfo.sFileName.Empty();
        if(eCurrentProcess == CS_CON){
            if(!m_pff->GetConInputFilenames().empty()){
                PifInfo.sFileName = m_pff->GetConInputFilenames().front();
            }
        }
        else if(eCurrentProcess == CS_CALC){
            if(!m_pff->GetCalcInputFNamesArr().empty()){
                PifInfo.sFileName = m_pff->GetCalcInputFNamesArr().front();
            }
        }
        PifInfo.sDisplay = INPUTTBD;
        arrPifInfo.Add(PifInfo);

        //output tbd
        PifInfo.eType = FILE_NONE;
        PifInfo.sUName = OUTPUTTBD;
        PifInfo.sFileName = m_pff->GetTabOutputFName();
        PifInfo.sDisplay = OUTPUTTBD;
        arrPifInfo.Add(PifInfo);

        if(!m_pff->GetListingFName().IsEmpty()){
            PifInfo.sFileName = m_pff->GetListingFName();
        }

        //Now do the listing file
        PifInfo.eType = FILE_NONE;
        PifInfo.sUName = LISTFILE;
        //PifInfo.sFileName = m_pff->GetListingFName();
        PifInfo.sDisplay = LISTFILE;
        arrPifInfo.Add(PifInfo);

    }
    else if(eCurrentProcess == CS_PREP) {
        //if Con  || Calc
        PIFINFO PifInfo;
        PifInfo.eType = FILE_NONE;
        PifInfo.sUName = INPUTTBD;
        if(!m_pff->GetPrepInputFName().IsEmpty()){
            PifInfo.sFileName = m_pff->GetPrepInputFName();
        }
        PifInfo.sDisplay = INPUTTBD;
        arrPifInfo.Add(PifInfo);

        if(bHasArea) { //Add this when the flag for areanames goes into the app file
            PifInfo.eType = FILE_NONE;
            PifInfo.sUName = AREANAMES;
            PifInfo.sFileName = m_pff->GetAreaFName(); //Get Area names file
            PifInfo.sDisplay = AREANAMES;
            arrPifInfo.Add(PifInfo);
        }
        //Output TBW
        PifInfo.eType = FILE_NONE;
        PifInfo.sUName = OUTPUTTBW;

        PifInfo.sFileName = m_pff->GetPrepOutputFName();
        PifInfo.sDisplay = OUTPUTTBW;
        arrPifInfo.Add(PifInfo);

        //Now do the listing file
        if(!m_pff->GetListingFName().IsEmpty()){
            PifInfo.sFileName = m_pff->GetListingFName();
        }
        PifInfo.eType = FILE_NONE;
        PifInfo.sUName = LISTFILE;
        //PifInfo.sFileName = m_pff->GetListingFName();
        PifInfo.sDisplay = LISTFILE;
        arrPifInfo.Add(PifInfo);

    }
    else {
        m_pff->SetTabProcess(PROCESS_INVALID);
        return false;
    }

    //Check Files
    {
        int iNumPifInfos = arrPifInfo.GetSize();
        CString sInputFile;
        CString sOutPutFile;
        for( int iPifInfo =0 ; iPifInfo < iNumPifInfos ; iPifInfo++) {

            PIFINFO& pifInfo = arrPifInfo.GetAt(iPifInfo);

            if(pifInfo.eType == FILE_NONE) {
                //The output file name can be empty
                if(pifInfo.sUName.CompareNoCase(WRITEFILE) ==0) {
                    //Write file can be empty ??
                }
                else if(pifInfo.sUName.CompareNoCase(INPUTTBD)==0) {
                    if(pifInfo.sFileName.IsEmpty()){
                        return false;
                    }
                    if(!CFile::GetStatus(pifInfo.sFileName,fStatus)){
                        //                          sMsg.FormatMessage("%1 Input file does not exist",pifInfo.sDataFileName);
                        return false;
                    }
                }
                else if(pifInfo.sUName.CompareNoCase(OUTPUTTBD) ==0) {
                    if(pifInfo.sFileName.IsEmpty()){
                        //sMsg = "Please choose output tab file name";
                        return false;
                    }
                    else {
                        if(sInputFile.CompareNoCase(pifInfo.sFileName)==0){
                            //sMsg = "Input file and output file are same. Please choose a different output or input file";
                            return false;
                        }
                        else {
                            if(CFile::GetStatus(pifInfo.sFileName,fStatus)){
                                //sMsg.Format("Output file %s already exists.\nDo you want to replace it?",pifInfo.sFileName);
                                return false;
                            }
                        }
                    }
                }
                else if(pifInfo.sUName.CompareNoCase(OUTPUTTBW) ==0) {
                    if(pifInfo.sFileName.IsEmpty()){
                        //sMsg = "Please choose output tbw file name";
                        return false;
                    }
                }
                else if(pifInfo.sUName.CompareNoCase(LISTFILE) ==0) {
                    //dont complain about listing file ?
                }
                else if(pifInfo.sUName.CompareNoCase(AREANAMES) ==0) {
                    if(pifInfo.sFileName.IsEmpty()){
                        //sMsg = "Please choose area file name";
                        return false;
                    }
                    if(!CFile::GetStatus(pifInfo.sFileName,fStatus)){
                        //sMsg.FormatMessage("%1 Area Names File does not exist",pifInfo.sFileName);
                        return false;
                    }
                }
            }
            else if(pifInfo.eType == PIFDICT){//input/external files
                if(pifInfo.connection_strings.empty()){
                    ///sMsg = "Please choose external data file name";
                    return false;
                }
                //what shld we do about external files??
            }
        }
    }

    return true;
}
