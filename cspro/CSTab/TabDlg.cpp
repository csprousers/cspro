// TabDlg.cpp : implementation file
//
#include "StdAfx.h"
#include "TabDlg.h"
#include "CSTab.h"
#include <zUtilO/Filedlg.h>
#include <zUtilO/FileUtil.h>
#include <zUtilO/imsaDlg.H>
#include <zUtilO/Interapp.h>
#include <zJson/JsonStream.h>
#include <ZBRIDGEO/PifDlg.h>
#include <zTableO/Table.h>
#include <zExTab/zExTab.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CSTabDlg dialog

CSTabDlg::CSTabDlg(CWnd* pParent /*=NULL*/)
    : CDialog(CSTabDlg::IDD, pParent)
{
    m_sFileName = _T("");
    m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CSTabDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    DDX_Text(pDX, IDC_FILENAME, m_sFileName);
}

BEGIN_MESSAGE_MAP(CSTabDlg, CDialog)
    ON_WM_SYSCOMMAND()
    ON_WM_DESTROY()
    ON_WM_PAINT()
    ON_WM_QUERYDRAGICON()
    ON_BN_CLICKED(IDC_LOCATE, OnLocate)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()


// CSTabDlg message handlers

BOOL CSTabDlg::OnInitDialog()
{
    CDialog::OnInitDialog();

    // Add "About..." menu item to system menu.

    // IDM_ABOUTBOX must be in the system command range.
    ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
    ASSERT(IDM_ABOUTBOX < 0xF000);

    CMenu* pSysMenu = GetSystemMenu(FALSE);
    if (pSysMenu != NULL)
    {
        CString strAboutMenu;
        strAboutMenu.LoadString(IDS_ABOUTBOX);
        if (!strAboutMenu.IsEmpty())
        {
            pSysMenu->AppendMenu(MF_SEPARATOR);
            pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
        }
    }

    // Set the icon for this dialog.  The framework does this automatically
    //  when the application's main window is not a dialog
    SetIcon(m_hIcon, TRUE);         // Set big icon
    SetIcon(m_hIcon, FALSE);        // Set small icon

    // TODO: Add extra initialization here
    UpdateData(FALSE);
    if(!m_sFileName.IsEmpty() && GetSafeHwnd()) {
        //this->OnOK();
        m_pPIFFile->SetSilent(true);
        this->PostMessage(WM_COMMAND,IDOK);
    }

    return TRUE;  // return TRUE  unless you set the focus to a control
}

void CSTabDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
    if ((nID & 0xFFF0) == IDM_ABOUTBOX)
    {
        CIMSAAboutDlg dlg;
        HICON hIcon = AfxGetApp()-> LoadIcon(IDR_MAINFRAME);
        dlg.m_hIcon = hIcon;
        dlg.m_csModuleName = _T("CSTab");
        dlg.DoModal();
    }
    else
    {
        CDialog::OnSysCommand(nID, lParam);
    }
}

void CSTabDlg::OnDestroy()
{
    CDialog::OnDestroy();
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
        CDialog::OnPaint();
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
    CString sFilter= _T("Tabulation Application Files (*.xtb)|*.xtb|PFF Files (*.pff)|*.pff||");
    UpdateData(TRUE);

    CString sPath(m_sFileName);
    PathRemoveFileSpec(sPath.GetBuffer(_MAX_PATH));
    sPath.ReleaseBuffer();

    CIMSAFileDialog fileDlg(TRUE,NULL,NULL,OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,sFilter,NULL,CFD_NO_DIR,FALSE);
    if(!sPath.IsEmpty()) {
      fileDlg.m_ofn.lpstrInitialDir = sPath;
    }
    else {
        CString sDir = AfxGetApp()->GetProfileString(_T("Settings"),_T("Last Folder"));
        if(m_sFileName.IsEmpty() && !sDir.IsEmpty()){
            fileDlg.m_ofn.lpstrInitialDir = sDir;
        }
    }

    if(fileDlg.DoModal() == IDOK) {
        m_sFileName= fileDlg.GetPathName();
        CString sPath(m_sFileName);
        PathRemoveFileSpec(sPath.GetBuffer(_MAX_PATH));
        sPath.ReleaseBuffer();
        AfxGetApp()->WriteProfileString(_T("Settings"),_T("Last  Folder"),sPath);
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
    CString sTemp(m_sFileName);
    CString sExt = PathFindExtension(sTemp.GetBuffer(_MAX_PATH));
    sTemp.ReleaseBuffer();

    CString sFileName(m_sFileName);
    PathRemoveExtension(sFileName.GetBuffer(_MAX_PATH));
    sFileName.ReleaseBuffer();
//    delete m_pPIFFile;
//    m_pPIFFile = new CNPifFile(m_sFileName);
    m_pPIFFile->SetPifFileName(m_sFileName);
    m_pPIFFile->SetAppType(TAB_TYPE);

    if(sExt.IsEmpty()) {
        sExt += FileExtensions::WithDot::TabulationApplication;
        m_sFileName += FileExtensions::WithDot::TabulationApplication;
        UpdateData(FALSE);
        m_pPIFFile->SetPifFileName(m_sFileName);
    }
    if(sExt.CompareNoCase(FileExtensions::WithDot::Pff) ==0) {
        //Check if the PffFile is valid
        sFileName += FileExtensions::WithDot::Pff;
        m_sFileName = sFileName;
        UpdateData(FALSE);
        m_pPIFFile->SetSilent(true);
        m_pPIFFile->SetPifFileName(m_sFileName);
        if(!((CSTabApp*)AfxGetApp())->InitNCompileApp()) {
            m_pPIFFile->SetSilent(false);
            return ;
        }
        if(!CheckNCollectInputFiles()){
            m_pPIFFile->SetSilent(false);
            return;
        }
        else {//If All stuff prepare intermediate files
            if(m_pPIFFile->GetTabProcess() == ALL_STUFF){
                CString sTabOutPutFName = m_pPIFFile->GetTabOutputFName();
                CString sTempTab;
                if(sTabOutPutFName.IsEmpty()) {
                    CString sAplFName = m_pPIFFile->GetAppFName();
                    PathRemoveExtension(sAplFName.GetBuffer(_MAX_PATH));
                    sAplFName.ReleaseBuffer();
                    sTabOutPutFName = sAplFName + FileExtensions::BinaryTable::WithDot::Tab;
                    m_pPIFFile->SetTabOutputFName(sTabOutPutFName);
                    sTempTab = sAplFName + _T("_precalc") +  FileExtensions::BinaryTable::WithDot::Tab ; // Engine is not looking at the piffile ags

                }
                if(m_pPIFFile->GetCalcInputFNamesArr().empty()) {
                    m_pPIFFile->GetCalcInputFNamesArr().emplace_back(sTempTab);
                }
                CString sCalcOFName = m_pPIFFile->GetCalcOutputFName();
                if(sCalcOFName.IsEmpty()){
                    m_pPIFFile->SetCalcOutputFName(sTabOutPutFName);
                }
            }
        }

    }
    else if (sExt.CompareNoCase(FileExtensions::WithDot::TabulationApplication) == 0) {
        //If the file is of .apl type ShowDialog selection for the pff file generation
        //change the lpszFileName
        CFileStatus fStatus;
        m_pPIFFile->SetPifFileName(m_sFileName+FileExtensions::WithDot::Pff);
        if(CFile::GetStatus(m_sFileName+FileExtensions::WithDot::Pff,fStatus)){
           m_pPIFFile->LoadPifFile();
        }

        ((CSTabApp*)AfxGetApp())->m_pPIFFile = m_pPIFFile;
        if(!((CSTabApp*)AfxGetApp())->InitNCompileApp()) {
            return ;
        }
        if(!MakePifFile()){
        //  AfxMessageBox("Failed to generate PFF file");
            return;
        }
        else {
            m_sFileName += FileExtensions::WithDot::Pff;
            m_pPIFFile->Save();
            UpdateData(FALSE);
        }

    }
    else {
        if(!sFileName.IsEmpty()){
            AfxMessageBox(_T("Invalid File Type"));
        }
        return ;
    }
    Invalidate();
    UpdateWindow();
    CDialog::OnOK();
}



/////////////////////////////////////////////////////////////////////////////////
//
//  bool CSTabDlg::MakePifFile()
//
/////////////////////////////////////////////////////////////////////////////////
bool CSTabDlg::MakePifFile()
{
    CRunTab runTab;
    PROCESS eProcess = PROCESS_INVALID;
    if(runTab.PreparePFF(m_pPIFFile,eProcess,false)){
        //((CSTabApp*)AfxGetApp())->m_eProcess = eProcess;
    }
    else {
        return false;
    }
    //Then show the appropriate dialog box .

    return true;
}

/////////////////////////////////////////////////////////////////////////////////
//
//  bool CSTabDlg::CheckNCollectInputFiles()
//
/////////////////////////////////////////////////////////////////////////////////
bool CSTabDlg::CheckNCollectInputFiles()
{
    bool bRet = false;
    CRunTab runTab;
    if(!BuildPifInfo4Check()){
        PROCESS eCurrentProcess = m_pPIFFile->GetTabProcess();
        bRet = runTab.PreparePFF(m_pPIFFile,eCurrentProcess,false);
        if(bRet && !m_pPIFFile->GetSilent())
            m_pPIFFile->Save();
    }
    else {
        bRet = true;
    }
    return bRet;
}

/////////////////////////////////////////////////////////////////////////////////
//
//  void CTRunInfoDlg::BuildPifInfo()
//
/////////////////////////////////////////////////////////////////////////////////
bool CSTabDlg::BuildPifInfo4Check()
{
    //Prepare pifinfo as if the dialog is going to be called and the check files similar to  CTRunInfoDlg::CheckFiles()
    CArray<PIFINFO,PIFINFO>  arrPifInfo;
    //Get the application file
    CFileStatus fStatus;
    PROCESS eCurrentProcess = m_pPIFFile->GetTabProcess();

    Application* pApp = m_pPIFFile->GetApplication();
    CString sXTSFile= pApp->GetTabSpec()->GetSpecFile();
    bool bHasArea = pApp->GetTabSpec()->GetConsolidate()->GetNumAreas() > 0;
    const CDataDict* pDict = pApp->GetTabSpec()->GetDict();
    //Get the dictionary file . This is the Inputdata
    if(pDict && (eCurrentProcess == ALL_STUFF || eCurrentProcess == CS_TAB)) {
        PIFINFO PifInfo;
        PifInfo.eType = PIFDICT;
        PifInfo.sUName = pDict->GetName();
        PifInfo.sDisplay = INPUTDATA;
        PifInfo.dictionary_filename = pDict->GetFullFileName();
        PifInfo.SetConnectionStrings(m_pPIFFile->GetInputDataConnectionStringsSerializable());
        arrPifInfo.Add(PifInfo);

        //Now do each of the external dicts
        for( const CString& dictionary_filename : pApp->GetExternalDictionaryFilenames() )
        {
            const DictionaryDescription* dictionary_description = pApp->GetDictionaryDescription(dictionary_filename);

            if( dictionary_description != nullptr && dictionary_description->GetDictionaryType() == DictionaryType::Working )
                continue;

            try
            {
                CString dictionary_name = JsonStream::GetValueFromSpecFile<CString, CDataDict>(JK::name, dictionary_filename);

                PIFINFO PifInfo;
                PifInfo.eType = PIFDICT;
                PifInfo.sUName = dictionary_name;
                PifInfo.sDisplay = _T("External File ");
                PifInfo.sDisplay += _T("(") + dictionary_name + _T(")");
                PifInfo.dictionary_filename = dictionary_filename;
                PifInfo.SetConnectionString(m_pPIFFile->GetExternalDataConnectionString(dictionary_name));

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
            PifInfo.sFileName =  m_pPIFFile->GetWriteFName(true);
            PifInfo.sDisplay = WRITEFILE;
            arrPifInfo.Add(PifInfo);
        }

        //Now add the OutputTBD
        if( eCurrentProcess == CS_TAB) { //only for tab
            PIFINFO PifInfo;
            PifInfo.eType = FILE_NONE;
            PifInfo.sUName = OUTPUTTBD;
            PifInfo.sFileName =  m_pPIFFile->GetTabOutputFName(); //Get the output TBD from the pff file
            PifInfo.sDisplay = OUTPUTTBD;
            arrPifInfo.Add(PifInfo);
        }
        if(eCurrentProcess == ALL_STUFF ) {
            PIFINFO PifInfo;

            PifInfo.eType = FILE_NONE;
            if(true){//requires output tbw in CSTab
                PifInfo.sUName = OUTPUTTBW;
                PifInfo.sFileName =  m_pPIFFile->GetPrepOutputFName();
                PifInfo.sDisplay = OUTPUTTBW;
                arrPifInfo.Add(PifInfo);
            }
            if(bHasArea) { //If it has area ask for areanames file
                PIFINFO PifInfo;
                PifInfo.eType = FILE_NONE;
                PifInfo.sUName = AREANAMES;
                PifInfo.sFileName  = m_pPIFFile->GetAreaFName(); //Get Area names file
                PifInfo.sDisplay = AREANAMES;
                arrPifInfo.Add(PifInfo);
            }
        }
        //Now do the listing file
        if(true){
            //Now do the listing file
            PIFINFO PifInfo;
            if(!m_pPIFFile->GetListingFName().IsEmpty()){
                PifInfo.sFileName  = m_pPIFFile->GetListingFName();
            }
            //do listing
            PifInfo.eType = FILE_NONE;
            PifInfo.sUName = LISTFILE;
            //PifInfo.sFileName  = m_pPIFFile->GetListingFName();
            PifInfo.sDisplay = LISTFILE;
            arrPifInfo.Add(PifInfo);
        }
    }
    else if(eCurrentProcess == CS_CON || eCurrentProcess == CS_CALC) {
        //if Con  || Calc
        PIFINFO PifInfo;
        PifInfo.eType = FILE_NONE;
        PifInfo.sUName = INPUTTBD;
        PifInfo.sFileName = _T("");
        if(eCurrentProcess == CS_CON){
            if(!m_pPIFFile->GetConInputFilenames().empty()){
                PifInfo.sFileName = m_pPIFFile->GetConInputFilenames().front();
            }
        }
        else if(eCurrentProcess == CS_CALC){
            if(!m_pPIFFile->GetCalcInputFNamesArr().empty()){
                PifInfo.sFileName = m_pPIFFile->GetCalcInputFNamesArr().front();
            }
        }
        PifInfo.sDisplay = INPUTTBD;
        arrPifInfo.Add(PifInfo);

        //output tbd
        PifInfo.eType = FILE_NONE;
        PifInfo.sUName = OUTPUTTBD;
        PifInfo.sFileName =  m_pPIFFile->GetTabOutputFName();
        PifInfo.sDisplay = OUTPUTTBD;
        arrPifInfo.Add(PifInfo);

        if(!m_pPIFFile->GetListingFName().IsEmpty()){
            PifInfo.sFileName  = m_pPIFFile->GetListingFName();
        }

        //Now do the listing file
        PifInfo.eType = FILE_NONE;
        PifInfo.sUName = LISTFILE;
        //PifInfo.sFileName  = m_pPIFFile->GetListingFName();
        PifInfo.sDisplay = LISTFILE;
        arrPifInfo.Add(PifInfo);

    }
    else if(eCurrentProcess == CS_PREP) {
        //if Con  || Calc
        PIFINFO PifInfo;
        PifInfo.eType = FILE_NONE;
        PifInfo.sUName = INPUTTBD;
        if(!m_pPIFFile->GetPrepInputFName().IsEmpty()){
            PifInfo.sFileName = m_pPIFFile->GetPrepInputFName();
        }
        PifInfo.sDisplay = INPUTTBD;
        arrPifInfo.Add(PifInfo);

        if(bHasArea) { //Add this when the flag for areanames goes into the app file
            PifInfo.eType = FILE_NONE;
            PifInfo.sUName = AREANAMES;
            PifInfo.sFileName  = m_pPIFFile->GetAreaFName(); //Get Area names file
            PifInfo.sDisplay = AREANAMES;
            arrPifInfo.Add(PifInfo);
        }
        //Output TBW
        PifInfo.eType = FILE_NONE;
        PifInfo.sUName = OUTPUTTBW;

        PifInfo.sFileName =  m_pPIFFile->GetPrepOutputFName();
        PifInfo.sDisplay = OUTPUTTBW;
        arrPifInfo.Add(PifInfo);

        //Now do the listing file
        if(!m_pPIFFile->GetListingFName().IsEmpty()){
            PifInfo.sFileName  = m_pPIFFile->GetListingFName();
        }
        PifInfo.eType = FILE_NONE;
        PifInfo.sUName = LISTFILE;
        //PifInfo.sFileName  = m_pPIFFile->GetListingFName();
        PifInfo.sDisplay = LISTFILE;
        arrPifInfo.Add(PifInfo);

    }
    else {
        m_pPIFFile->SetTabProcess(PROCESS_INVALID);
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
                            CFileStatus fStatus;
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
