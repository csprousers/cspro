// ExptDoc.cpp : implementation of the CExportDoc class
//

/************************************
  REVISION LOG ENTRY
  Revision By: Chirag
  Revised on 2/26/2002 4:11:15 PM
  Comments: The Document File for the export Application.
 ************************************/

#include "StdAfx.h"
#include "ExptDoc.h"
#include "CSExport.h"
#include "ExportOptionsView.h"
#include "ExptView.h"
#include "MainFrm.h"
#include <zUtilO/Filedlg.h>
#include <zUtilO/PathHelpers.h>
#include <zUtilO/Specfile.h>
#include <zJson/JsonObjectCreator.h>
#include <zJson/JsonSpecFile.h>
#include <zLogicO/ReservedWords.h>
#include <Zsrcmgro/Compiler.h>
#include <Zsrcmgro/SrcCode.h>
#include <ZBRIDGEO/PifDlg.h>
#include <zFormO/FormFile.h>
#include <zDataO/ConnectionStringProperties.h>
#include <zDataO/DataRepositoryHelpers.h>
#include <ZBRIDGEO/DataFileDlg.h>
#include <zInterfaceF/BatchLogicViewerDlg.h>
#include <zInterfaceF/LogicSettingsDlg.h>
#include <zBatchF/BatchExecutor.h>


CStringArray sArray;
CString sOldAppName;


// 20130703 moved these string literals up here as definitions
#define EXPORT_CMD_Settings                                 _T("Settings")

#define EXPORT_CMD_Yes                                      _T("Yes")
#define EXPORT_CMD_No                                       _T("No")

#define EXPORT_CMD_ExportMethod                             _T("ExportMethod")
#define EXPORT_CMD_ExportMethod_TABS                        _T("TabDelimited")
#define EXPORT_CMD_ExportMethod_COMMADEL                    _T("CommaDelimited")
#define EXPORT_CMD_ExportMethod_SEMI_COLON                  _T("SemiColon")
#define EXPORT_CMD_ExportMethod_CSPRO                       _T("CSPro")
#define EXPORT_CMD_ExportMethod_SPSS                        _T("SPSS")
#define EXPORT_CMD_ExportMethod_SAS                         _T("SAS")
#define EXPORT_CMD_ExportMethod_STATA                       _T("Stata")
#define EXPORT_CMD_ExportMethod_R                           _T("R")
#define EXPORT_CMD_ExportMethod_ALLTYPES                    _T("SPSS-SAS-Stata")

#define EXPORT_CMD_UnicodeOutput                            _T("UnicodeOutput")
#define EXPORT_CMD_DecimalComma                             _T("DecimalComma")



bool bBehaviorDone  = false;
bool bBehavior_SingleRecordsFlat    = true;


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

namespace
{
    bool CheckNDeleteFile(CString sFile)
    {
        bool bDone = true;
        if(sFile.IsEmpty()){
            return bDone;
        }
        else {
            CFileStatus fStatus;
            if(CFile::GetStatus(sFile,fStatus)) {
                CString sMsg;
                BOOL bDel = DeleteFile(sFile);
                if(!bDel) {
                    sMsg = _T("Cannot Delete ") + sFile;
                    AfxMessageBox(sMsg);
                    bDone  = false;
                    return bDone;
                }
            }
        }
        return bDone;
    }

    // with the CSPro 7.3 PFF refactoring, here is a function
    // that replicates old behavior, not added to the PFF class with the
    // thought that the export tool will soon be redesigned
    void Pff_SetFirstExportFilename(CNPifFile* pPifFile, CString filename)
    {
        std::vector<CString> saved_filenames = pPifFile->GetExportFilenames();

        pPifFile->ClearExportFilenames();
        pPifFile->AddExportFilenames(filename);

        for( size_t i = 1; i < saved_filenames.size(); i++ )
            pPifFile->AddExportFilenames(saved_filenames[i]);
    }
}

/////////////////////////////////////////////////////////////////////////////
// CExportDoc

IMPLEMENT_DYNCREATE(CExportDoc, CDocument)

BEGIN_MESSAGE_MAP(CExportDoc, CDocument)
    //{{AFX_MSG_MAP(CExportDoc)
    ON_COMMAND(ID_FILE_RUN, OnFileRun)
    ON_UPDATE_COMMAND_UI(ID_FILE_RUN, OnUpdateFileRun)
    ON_COMMAND(ID_FILE_SAVE, OnFileSave)
    ON_COMMAND(ID_FILE_SAVEAS, OnFileSaveAs)
    ON_UPDATE_COMMAND_UI(ID_FILE_SAVE, OnUpdateFileSave)
    ON_COMMAND(ID_OPTIONS_EXCLUDED, OnOptionsExcluded)
    ON_UPDATE_COMMAND_UI(ID_OPTIONS_EXCLUDED, OnUpdateOptionsExcluded)
    ON_COMMAND(ID_OPTIONS_LOGIC_SETTINGS, OnOptionsLogicSettings)
    ON_COMMAND(ID_VIEW_BATCH_LOGIC, OnViewBatchLogic)
    ON_UPDATE_COMMAND_UI(ID_VIEW_BATCH_LOGIC, OnUpdateFileRun)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CExportDoc construction/destruction

CExportDoc::CExportDoc()
    :   m_logicSettings(LogicSettings::GetUserDefaultSettings())
{
    m_pLogFile      = NULL;
    m_batchmode     = false;
    m_PifFile.SetAppType(EXPORT_TYPE);
    m_PifFile.SetViewResultsFlag(true);
    m_PifFile.SetViewListing(ONERROR);
    m_convmethod    = METHOD::TABS;
    m_iPreviousRunConvMethod = -1;
    m_bmerge     = true;
    m_bSaveExcluded = false;
    m_iLowestLevel = 0;
    m_bPostRunSave = true;

    m_bAllInOneRecord                   = true;
    m_bJoinSingleWithMultipleRecords    = false;
    m_exportRecordType                  = ExportRecordType::None;
    m_exportItemsSubitems               = ExportItemsSubitems::ItemsOnly;
    m_csExportFilesPrefix               = _T("Exported_");

    m_bForceANSI                        = true; // 20120416
    m_bCommaDecimal                     = false;


    SyncBuff_app();

    SetTreeView     ( NULL );
    SetOptionsView  ( NULL );

    m_bJOIN_SingleMultiple_UseSingleAfterMultiple   = true;

    m_sBaseFilename.Format(_T("%sCSExpRun%d"), GetTempDirectory().c_str(), GetCurrentProcessId()); // 20140312
}

CExportDoc::~CExportDoc()
{
    SAFE_DELETE(m_pLogFile);
    ((CExportApp *)AfxGetApp())->DeletePifInfos();
}

BOOL CExportDoc::OnNewDocument()
{
    if (!CDocument::OnNewDocument())
        return FALSE;

    // TODO: add reinitialization code here
    return TRUE;
}


/////////////////////////////////////////////////////////////////////////////
// CExportDoc diagnostics

#ifdef _DEBUG
void CExportDoc::AssertValid() const
{
    CDocument::AssertValid();
}

void CExportDoc::Dump(CDumpContext& dc) const
{
    CDocument::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CExportDoc commands

BOOL CExportDoc::OnOpenDocument(LPCTSTR lpszPathName)
{
    DeleteContents();
    SetModifiedFlag(FALSE);
    m_PifFile.ResetContents();
    m_embeddedDictionaryInformation.reset();
    m_csUniverse.Empty();

    // 20130703 see if a document is already open (if so, don't mess with the registry settings)
    bool bFileAlreadyOpen = IsWindow(m_pTreeView->m_dicttree) && m_pTreeView->m_dicttree.GetRootItem() != NULL;

    CString extension = PortableFunctions::PathGetFileExtension<CString>(lpszPathName);

    if (extension.CompareNoCase(FileExtensions::Pff) == 0) {
        m_bPostRunSave = false;
        m_sPFFName = lpszPathName;
        m_PifFile.SetPifFileName(lpszPathName);
        if (m_PifFile.LoadPifFile()) {
            if (OpenSpecFile(m_PifFile.GetAppFName(), true)) {
                m_batchmode = true;
                if(m_pOptionsView){
                    m_pOptionsView->FromDoc(this);
                }

                return TRUE;
            }
        }
        return FALSE;
    }

    else if (extension.CompareNoCase(FileExtensions::ExportSpec) == 0) {
        if (OpenSpecFile(lpszPathName, false)) {
            AfxGetApp()->WriteProfileString(EXPORT_CMD_Settings, _T("Last Open"), lpszPathName);
            m_PifFile.SetAppFName(lpszPathName);
            CString csFileName = lpszPathName;
            m_PifFile.SetListingFName(csFileName + FileExtensions::WithDot::Listing);
            CString csPFF = csFileName + FileExtensions::WithDot::Pff;
            CFileStatus status;
            if (CFile::GetStatus(csPFF, status)) {
                m_PifFile.SetPifFileName(csPFF);
                if (m_PifFile.LoadPifFile()) {
                    if (csFileName.CompareNoCase(lpszPathName) != 0) {
                        AfxMessageBox(FormatText(_T("Spec files in %s\ndoes not match %s"), csPFF.GetString(), lpszPathName));
                        return FALSE;
                    }
                }
            }
            if(m_pOptionsView){
                m_pOptionsView->FromDoc(this);
            }
        }
        else {
            return FALSE;
        }
    }

    else if( extension.CompareNoCase(FileExtensions::Dictionary) == 0 ||
             extension.CompareNoCase(FileExtensions::Data::CSProDB) == 0 ||
             extension.CompareNoCase(FileExtensions::Data::EncryptedCSProDB) == 0 )
    {
        if( !ProcessDictionarySource(lpszPathName) )
            return FALSE;

        m_PifFile.SetAppFName(_T(""));
        if (!OpenDictFile(m_csDictFileName, false)) {
            return FALSE;
        }

        if( !bFileAlreadyOpen ) // 20130703 load some settings in the registry: export format, unicode output, and comma as a decimal
        {
            CString sExportMethod = AfxGetApp()->GetProfileString(EXPORT_CMD_Settings,EXPORT_CMD_ExportMethod,EXPORT_CMD_ExportMethod_TABS);

            if( !sExportMethod.CompareNoCase(EXPORT_CMD_ExportMethod_TABS) )            m_convmethod = METHOD::TABS;
            else if( !sExportMethod.CompareNoCase(EXPORT_CMD_ExportMethod_COMMADEL) )   m_convmethod = METHOD::COMMADEL;
            else if( !sExportMethod.CompareNoCase(EXPORT_CMD_ExportMethod_SEMI_COLON) ) m_convmethod = METHOD::SEMI_COLON;
            else if( !sExportMethod.CompareNoCase(EXPORT_CMD_ExportMethod_CSPRO) )      m_convmethod = METHOD::CSPRO;
            else if( !sExportMethod.CompareNoCase(EXPORT_CMD_ExportMethod_SPSS) )       m_convmethod = METHOD::SPSS;
            else if( !sExportMethod.CompareNoCase(EXPORT_CMD_ExportMethod_SAS) )        m_convmethod = METHOD::SAS;
            else if( !sExportMethod.CompareNoCase(EXPORT_CMD_ExportMethod_STATA) )      m_convmethod = METHOD::STATA;
            else if( !sExportMethod.CompareNoCase(EXPORT_CMD_ExportMethod_R) )          m_convmethod = METHOD::R;
            else if( !sExportMethod.CompareNoCase(EXPORT_CMD_ExportMethod_ALLTYPES) )   m_convmethod = METHOD::ALLTYPES;

            CString sYes = EXPORT_CMD_Yes;

            m_bForceANSI = sYes.CompareNoCase(AfxGetApp()->GetProfileString(EXPORT_CMD_Settings,EXPORT_CMD_UnicodeOutput,EXPORT_CMD_No)) != 0;
            m_bCommaDecimal = !sYes.CompareNoCase(AfxGetApp()->GetProfileString(EXPORT_CMD_Settings,EXPORT_CMD_DecimalComma,EXPORT_CMD_No));
        }

        if(m_pOptionsView)
            m_pOptionsView->FromDoc(this);

        AddAllItems();
        AfxGetApp()->WriteProfileString(EXPORT_CMD_Settings,_T("Last Open"), lpszPathName);
    }

    return TRUE;
}

bool CExportDoc::OpenDictFile(const TCHAR* filename, bool silent)
{
    //  Open data dictionary
    CFileStatus fStatus;

    // csc 5/21/04 ... give a more useful message if the DCF does not exist
    if (!CFile::GetStatus(filename,fStatus)) {
        CString csMsg;
        csMsg.Format(_T("The dictionary file %s does not exist. Unable to continue export."), filename);
        AfxMessageBox(csMsg,MB_ICONSTOP);
        return false;
    }

    // Clear All memory of previous Dictionary
    m_aItems.RemoveAll();

    try
    {
        m_pDataDict = CDataDict::InstantiateAndOpen(filename, silent);

        if( m_pDataDict->GetAllowExport() )
            return true;

        AfxMessageBox(_T("The dictionary's settings prohibit its use to export data"));
    }

    catch( const CSProException& exception )
    {
#ifdef _DEBUG
        ErrorMessage::Display(exception);
#endif        
    }

    m_pDataDict = std::make_shared<CDataDict>();
    m_csDictFileName.Empty();
    return false;
}

void CExportDoc::OnFileRun()
{
    ProcessRun();
}

bool CExportDoc::IsChecked(int position) const
{
    return ( position >= 0 && m_aItems[position].selected );
}

////////////////////////////////////////////////////////////////////
//
//                    CString CExportDoc::GetNameat(int level, int record, int item, int vset)
//
////////////////////////////////////////////////////////////////////

CString CExportDoc::GetNameat(int level, int record, int item, int vset)
{
    ASSERT (level >= 0);
    ASSERT (item >= 0);
    ASSERT (vset >= 0);
    const CDictItem* pItem = m_pDataDict->GetLevel(level).GetRecord(( record == -1 ) ? COMMON : record)->GetItem(item);
    return pItem->GetName();
}

////////////////////////////////////////////////////////////////////
//
//                    CString CExportDoc::GetItemData(char *record, CDictItem*pItem)
//
////////////////////////////////////////////////////////////////////

CString CExportDoc::GetItemData(TCHAR *record, CDictItem*pItem)
{
    CString str;
    TCHAR temp[100];
    int len = pItem->GetLen();
    _tcsnccpy(temp,record+pItem->GetStart()-1 ,len);
    temp[len] = _T('\0');
    str.Format(_T("%s"),temp);
    return str;
}

////////////////////////////////////////////////////////////////////
//
//                    int CExportDoc::GetNumExpRecTypes()
//
////////////////////////////////////////////////////////////////////

int CExportDoc::GetNumExpRecTypes()
{
    m_rectypes.RemoveAll();
    m_records.RemoveAll();
    //if (m_aItems[0]->GetRecord()->GetRecTypeVal() != "")
    //  m_rectypes.Add(m_aItems[0]->GetRecord()->GetRecTypeVal());

    for (int i = 0; i < m_aItems.GetSize();i++)
    {
        if (!m_aItems[i].selected ) continue;
        int j;
        for (j = 0; j < m_rectypes.GetSize(); j++)
        {
            if (m_aItems[i].pItem->GetRecord()->GetRecTypeVal() == m_rectypes[j] || m_aItems[i].pItem->GetRecord()->GetRecTypeVal() == _T(""))
                break;
        }
        if (j == m_rectypes.GetSize())
        {
            if (m_aItems[i].pItem->GetRecord()->GetSonNumber() != COMMON)
            {
                m_rectypes.Add(m_aItems[i].pItem->GetRecord()->GetRecTypeVal());
                if (SharedSettings::ViewNamesInTree())
                    m_records.Add(m_aItems[i].pItem->GetRecord()->GetName());
                else
                    m_records.Add(m_aItems[i].pItem->GetRecord()->GetLabel());
            }
        }
    }
    return m_rectypes.GetSize();
}

////////////////////////////////////////////////////////////////////
//
//                    void CExportDoc::OnUpdateFileRun(CCmdUI* pCmdUI)
//
////////////////////////////////////////////////////////////////////

void CExportDoc::OnUpdateFileRun(CCmdUI* pCmdUI)    // BMD 24 Mar 2005
{
    bool bRelSelected = false;

    if( m_pDataDict != nullptr ) {
        for( const DictRelation& dict_relation : m_pDataDict->GetRelations() ) {
            if (m_pTreeView->IsRelationSelected(dict_relation)) {
                bRelSelected = true;
                break;
            }
        }
    }

    pCmdUI->Enable(GetNumItemsSelected() > 0 || bRelSelected);
}

////////////////////////////////////////////////////////////////////
//
//                    BOOL CExportDoc::SaveModified()
//
////////////////////////////////////////////////////////////////////

BOOL CExportDoc::SaveModified()
{
    // borrowed from Bruce::SaveModified() ; see doccore.cpp

    if( !IsModified() || m_batchmode ) {
        return TRUE;        // ok to continue
    }

    CString name = m_PifFile.GetAppFName();
    if (name.IsEmpty()) {
        VERIFY(name.LoadString(AFX_IDS_UNTITLED));
    }
    CString prompt;
    AfxFormatString1(prompt, AFX_IDP_ASK_TO_SAVE, name);
    switch (AfxMessageBox(prompt, MB_YESNOCANCEL, AFX_IDP_ASK_TO_SAVE))
    {
    case IDCANCEL:
        return FALSE;       // don't continue

    case IDYES:
        // If so, either Save or Update, as appropriate
        OnFileSave();
        return true;

    case IDNO:
        // If not saving changes, revert the document
        break;

    default:
        ASSERT(FALSE);
        break;
    }
    return TRUE;    // keep going
//  return CDocument::SaveModified();
}

////////////////////////////////////////////////////////////////////
//
//                    void CExportDoc::OnFileSave()
//
////////////////////////////////////////////////////////////////////

void CExportDoc::OnFileSave()
{
    if (m_PifFile.GetAppFName().IsEmpty()) {
        OnFileSaveAs();
        return;
    }
    SaveSpecFile();
    SetModifiedFlag(FALSE);
}

////////////////////////////////////////////////////////////////////
//
//                    void CExportDoc::OnFileSaveAs()
//
////////////////////////////////////////////////////////////////////

void CExportDoc::OnFileSaveAs()
{
    CString csPath = m_PifFile.GetAppFName();         // BMD 14 Mar 2002
    if (SO::IsBlank(csPath)) {
        CString csDictionarySourceFilename = GetDictionarySourceFilename();
        csPath = csDictionarySourceFilename.Left(csDictionarySourceFilename.ReverseFind('\\')) + _T("\\*.exf");
    }
    CString csFilter = _T("Export Specification Files (*.exf)|*.exf|All Files (*.*)|*.*||");

    CIMSAFileDialog dlgFile(FALSE, FileExtensions::ExportSpec, csPath, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, csFilter);
    dlgFile.m_ofn.lpstrTitle = _T("Save Export Specification File");
    if (dlgFile.DoModal() == IDOK) {
        m_PifFile.SetAppFName(dlgFile.GetPathName());
        m_PifFile.SetListingFName(dlgFile.GetPathName() + FileExtensions::WithDot::Listing);
        SaveSpecFile();
        AfxGetApp()->AddToRecentFileList(dlgFile.GetPathName());
        SetPathName( m_PifFile.GetAppFName(),TRUE);
        SetModifiedFlag(FALSE);
    }
}


////////////////////////////////////////////////////////////////////
//
//                    bool CExportDoc::RunBatch()
//
////////////////////////////////////////////////////////////////////

bool CExportDoc::RunBatch()
{
    m_batchmode = true;
    if ( (m_convmethod == METHOD::TABS || m_convmethod == METHOD::COMMADEL)
        || (!m_PifFile.GetExportFilenames().empty() || m_convmethod == METHOD::SPSS || m_convmethod == METHOD::SAS || m_convmethod == METHOD::STATA || m_convmethod == METHOD::ALLTYPES))
    {
        OnFileRun ();
    }
    else
        AfxMessageBox(_T("Invalid No of Output Files"));
    return true;
}

////////////////////////////////////////////////////////////////////
//
//                    void CExportDoc::OnUpdateFileSave(CCmdUI* pCmdUI)
//
////////////////////////////////////////////////////////////////////

void CExportDoc::OnUpdateFileSave(CCmdUI* pCmdUI)
{
    CString str = GetTitle();
    if (str == _T("Untitled"))
        pCmdUI->Enable(FALSE);
    else
        pCmdUI->Enable(TRUE);

}

void CExportDoc::OnCloseDocument()
{
    m_pDataDict.reset();
    SAFE_DELETE(m_pLogFile);

    CDocument::OnCloseDocument();
}

bool CExportDoc::GenerateBatchApp()
{
    Application batchApp;
    batchApp.SetEngineAppType(EngineAppType::Batch);
    batchApp.SetLogicSettings(m_logicSettings);

    CString sPath;
    // 20131220 base the export files off the pff location, if available
    sPath = m_PifFile.GetAppFName().IsEmpty() ? m_csDictFileName : m_PifFile.GetAppFName();
    PathRemoveFileSpec(sPath.GetBuffer(MAX_PATH));
    sPath.ReleaseBuffer();
    sPath.TrimRight('\\');

    //make the order spec name ;
    CString sFullFileName = sPath+_T("\\") + _T("CSExpRun.bch");
    CString sOrderFile = sPath+_T("\\")+_T("CSExpRun.ord");


    batchApp.AddFormFilename(sOrderFile);
    //Create the .ord file and save it
    CFileStatus fStatus;
    BOOL bOrderExists = CFile::GetStatus(sOrderFile,fStatus);
    if(bOrderExists){
        BOOL bDel = DeleteFile(sOrderFile);
        if(!bDel){
            CString sMsg;
            sMsg = _T("Cannot Delete ") + sOrderFile;
            AfxMessageBox(sMsg);
            return false;
        }
        else {
            bOrderExists  =false;
        }
    }
    if(!bOrderExists) {

        ASSERT(!m_csDictFileName.IsEmpty());
        CDEFormFile Order(sOrderFile,m_csDictFileName);

        //Create the .ord file and save it
        Order.CreateOrderFile(*m_pDataDict, true);
        Order.Save(sOrderFile);
    }

    batchApp.SetLabel(PortableFunctions::PathGetFilenameWithoutExtension<CString>(sFullFileName));

    if(!WriteDefaultFiles(&batchApp,sFullFileName)){
        return false;
    }

    try
    {
        batchApp.Save(sFullFileName);
    }

    catch( const CSProException& )
    {
        return false;
    }
    
    return true;
}

////////////////////////////////////////////////////////////////////
//
//                    bool CExportDoc::WriteDefaultFiles(Application *pApplication, const CString &sAppFName)
//
////////////////////////////////////////////////////////////////////

bool CExportDoc::WriteDefaultFiles(Application* pApplication, const CString &sAppFName)
{
    bool bDone = false;
    //AppFile
    CString sAppSCodeFName(sAppFName);
    PathRemoveExtension(sAppSCodeFName.GetBuffer(_MAX_PATH));
    sAppSCodeFName.ReleaseBuffer();
    sAppSCodeFName += FileExtensions::WithDot::Logic;

    CFileStatus fStatus;
    BOOL bRet = CFile::GetStatus(sAppSCodeFName,fStatus);
    if(bRet) {
        CString sMsg;
        BOOL bDel = DeleteFile(sAppSCodeFName);
        if(!bDel) {
            sMsg = _T("Cannot Delete ") + sAppSCodeFName;
            AfxMessageBox(sMsg);
            return bDone;
        }
        else {
            bRet = false;
        }
    }
    if(!bRet){
        //Create the .app file
        CSpecFile appFile(TRUE);
        appFile.Open(sAppSCodeFName,CFile::modeWrite);
        appFile.WriteString(m_logicSettings.GetDefaultFirstLineForTextSource(pApplication->GetLabel(), AppFileType::Code));
        //Now write logic into the file with the freq command
        appFile.WriteString(_T("\r\nPROC GLOBAL\r\n\n"));
        CString sExptProc =  _T("PROC ") + m_pDataDict->GetLevel(m_iLowestLevel).GetName() +_T("\r\n");

        appFile.WriteString(sExptProc);
        CString csmethod;
        switch (m_convmethod)
        {
        case METHOD::SPSS:
            csmethod = _T("set behavior() export SPSS on;\nset behavior() export SAS off;\nset behavior() export STATA off;\n");
            break;
        case METHOD::SAS:
            csmethod = _T("set behavior() export SPSS off;\nset behavior() export SAS on;\nset behavior() export STATA off;\n");
            break;
        case METHOD::STATA:
            csmethod = _T("set behavior() export SPSS off;\nset behavior() export SAS off;\nset behavior() export STATA on;\n");
            break;
        case METHOD::ALLTYPES:
            csmethod = _T("set behavior() export SPSS on;\nset behavior() export SAS on;\nset behavior() export STATA on;\n");
            break;
        default :
            break;
        }
        appFile.WriteString(csmethod);

        sExptProc =  _T("Export Flat \n");
        appFile.WriteString(sExptProc);
        sExptProc = _T(" include (");

        CString str = GetRecordItemStr();
        str = str.Left(str.ReverseFind(','));
        int pos = -10;
        while (1)
        {
            if (str.GetLength() > 100)
            {
                pos = str.Find(_T(","),pos + 100);
                if (pos < 0) {                  // BMD 19 March 2004
                    sExptProc += str + _T(");\n");
                    appFile.WriteString(sExptProc);
                    appFile.WriteString(_T("\n"));
                    appFile.Close();
                    break;
                }
                sExptProc += str.Left(pos+1) + _T("\n");
                str = str.Right(str.GetLength() - (pos+1));
                appFile.WriteString(sExptProc);
                sExptProc = _T("");
                pos = 0;
            }
            else
            {
                sExptProc += str + _T(");\n");
                appFile.WriteString(sExptProc);
                appFile.WriteString(_T("\n"));
                appFile.Close();
                break;
            }
        }
    }

    pApplication->AddCodeFile(CodeFile(CodeType::LogicMain, std::make_shared<TextSource>(CS2WS(sAppSCodeFName))));

    return true;
}

////////////////////////////////////////////////////////////////////
//
//                    bool CExportDoc::LaunchBatchApp()
//
////////////////////////////////////////////////////////////////////

bool CExportDoc::LaunchBatchApp()
{
    //Check Applications which has this form as the main one if
    //there are more than one ask the user for which application to
    //run .If there is only one proceed with it .
    if(!DeleteOutPutFiles()){//Delete all the output files before the run
        return false;
    }

    try
    {
        BatchExecutor batch_executor;
        batch_executor.Run(m_sBCHPFFName);

        AfxGetMainWnd()->PostMessage(WM_IMSA_EXPORTDONE);
    }

    catch( const CSProException& exception )
    {
        ErrorMessage::Display(exception);
    }

    return true;
}

void CExportDoc::OnOptionsExcluded()
{
    m_bSaveExcluded = !m_bSaveExcluded;
}

void CExportDoc::OnUpdateOptionsExcluded(CCmdUI* pCmdUI)
{
    pCmdUI->SetCheck(m_bSaveExcluded);
}

////////////////////////////////////////////////////////////////////
//
//                    void CExportDoc::AddAllItems()
//
////////////////////////////////////////////////////////////////////

void CExportDoc::AddAllItems()
{
    CWaitCursor wait;

    for( const DictLevel& dict_level : m_pDataDict->GetLevels() )
    {
        const CDictRecord* pIdRecord = dict_level.GetIdItemsRec(); // Common Record for the level

        for (int k = 0; k < pIdRecord->GetNumItems(); k++)
        {
            const CDictItem* pItem = pIdRecord->GetItem(k);
            ASSERT(pItem->AddToTreeFor80());
            int totocc = pItem->GetOccurs();
            if (pItem->GetParentItem() != NULL)
                totocc = pItem->GetParentItem()->GetOccurs()*totocc;

            if ( totocc > 1)
            {
                for(int occ = 0; occ < totocc; occ++)
                {
                    ITEMS a;
                    a.rel = NONE;
                    a.pItem = pItem;
                    a.occ   = occ+1;
                    a.selected = m_bSaveExcluded;
                    m_aItems.Add(a);
                }
            }
            else
            {
                ITEMS a;
                a.rel = NONE;
                a.pItem = pItem;
                a.occ   = -1;
                a.selected = m_bSaveExcluded;
                m_aItems.Add(a);
            }
        }
        for (int j = 0; j < dict_level.GetNumRecords(); j++)
        {
            const CDictRecord* pRecord = dict_level.GetRecord(j);
            for (int k = 0; k < pRecord->GetNumItems(); k++)
            {
                const CDictItem* pItem = pRecord->GetItem(k);
                if( !pItem->AddToTreeFor80() )
                    continue;
                int totocc = pItem->GetOccurs();
                if (pItem->GetParentItem() != NULL)
                    totocc = pItem->GetParentItem()->GetOccurs()*totocc;

                if ( totocc > 1)
                {
                    for(int occ = 0; occ < totocc; occ++)
                    {
                        ITEMS a;
                        a.rel = NONE;
                        a.pItem = pItem;
                        a.occ   = occ+1;
                        a.selected = m_bSaveExcluded;
                        m_aItems.Add(a);
                    }
                }
                else
                {
                    ITEMS a;
                    a.rel = NONE;
                    a.pItem = pItem;
                    a.occ   = -1;
                    a.selected = m_bSaveExcluded;
                    m_aItems.Add(a);
                }
            }
        }
    }

    int iRel = -1;
    for( const DictRelation& dict_relation : m_pDataDict->GetRelations() ) {
        ++iRel;
        int iLevel;
        int iRecord;
        int iItem;
        int iVSet;
        // Primary
        m_pDataDict->LookupName(dict_relation.GetPrimaryName(),&iLevel,&iRecord,&iItem,&iVSet);
        if (iItem == NONE) {
            // Repeating Records
            const CDictRecord* pRec = m_pDataDict->GetLevel(iLevel).GetRecord(iRecord);
            for (int k = 0; k < pRec->GetNumItems(); k++) {
                const CDictItem* pItem = pRec->GetItem(k);
                ITEMS a;
                a.rel = iRel;
                a.pItem = pItem;
                a.occ   = -1;
                a.selected = m_bSaveExcluded;
                m_aItems.Add(a);
            }
        }
        else {
            // Repeating Items or Subitems
            const CDictItem* pDictItem = m_pDataDict->GetLevel(iLevel).GetRecord(iRecord)->GetItem(iItem);
            ITEMS a;
            a.rel = NONE;
            a.pItem = pDictItem;
            a.occ   = -1;
            a.selected = m_bSaveExcluded;
            m_aItems.Add(a);
            if (pDictItem->GetItemType() == ItemType::Item) {
                for (int i = iItem + 1 ; i < m_pDataDict->GetLevel(iLevel).GetRecord(iRecord)->GetNumItems() ; i++) {
                    pDictItem = m_pDataDict->GetLevel(iLevel).GetRecord(iRecord)->GetItem(i);
                    if (pDictItem->GetItemType() == ItemType::Item) {
                        break;
                    }
                    ITEMS b;
                    b.rel = NONE;
                    b.pItem = pDictItem;
                    b.occ   = -1;
                    b.selected = m_bSaveExcluded;
                    m_aItems.Add(b);
                }
            }
        }

        for( const DictRelationPart& dict_relation_part : dict_relation.GetRelationParts() ) {
            m_pDataDict->LookupName(dict_relation_part.GetSecondaryName(),&iLevel,&iRecord,&iItem,&iVSet);
            if (iItem == NONE) {
                // Repeating Records
                const CDictRecord* pRec = m_pDataDict->GetLevel(iLevel).GetRecord(iRecord);
                for (int k = 0; k < pRec->GetNumItems(); k++) {
                    const CDictItem* pItem = pRec->GetItem(k);
                    ITEMS a;
                    a.rel = iRel;
                    a.pItem = pItem;
                    a.occ   = -1;
                    a.selected = m_bSaveExcluded;
                    m_aItems.Add(a);
                }
            }
            else {
                // Repeating Items or Subitems
                const CDictItem* pDictItem = m_pDataDict->GetLevel(iLevel).GetRecord(iRecord)->GetItem(iItem);
                ITEMS a;
                a.rel = iRel;
                a.pItem = pDictItem;
                a.occ   = -1;
                a.selected = m_bSaveExcluded;
                m_aItems.Add(a);
                if (pDictItem->GetItemType() == ItemType::Item) {
                    for (int i = iItem + 1 ; i < m_pDataDict->GetLevel(iLevel).GetRecord(iRecord)->GetNumItems() ; i++) {
                        pDictItem = m_pDataDict->GetLevel(iLevel).GetRecord(iRecord)->GetItem(i);
                        if (pDictItem->GetItemType() == ItemType::Item) {
                            break;
                        }
                        ITEMS b;
                        b.rel = iRel;
                        b.pItem = pDictItem;
                        b.occ   = -1;
                        b.selected = m_bSaveExcluded;
                        m_aItems.Add(b);
                    }
                }
            }
        }
    }
}

int CExportDoc::GetPositionInList(const wstring_view name_sv, const int occurrence) const
{
    for (int i = 0; i < m_aItems.GetSize(); i++)
    {
        if (SO::EqualsNoCase(name_sv, m_aItems[i].pItem->GetName()) && occurrence == m_aItems[i].occ)
            return i;
    }
    return -1;
}

int CExportDoc::GetPositionInList(const int relation_index, const wstring_view name_sv, const int occurrence) const
{
    for (int i = 0; i < m_aItems.GetSize(); i++)
    {
        if (relation_index == m_aItems[i].rel && SO::EqualsNoCase(name_sv, m_aItems[i].pItem->GetName()) && occurrence == m_aItems[i].occ)
            return i;
    }
    return -1;
}


int CExportDoc::GetNumItemsSelected()
{
    int count = 0;
    for (int i = 0; i < m_aItems.GetSize(); i++)
    {
        if (m_aItems[i].selected)
        {
            count++;
        }
    }
    return count;
}


CString CExportDoc::GetRecordItemStr()
{
    CString RetStr;

    for( const DictLevel& dict_level : m_pDataDict->GetLevels() )
    {
        const CDictRecord* pRecord = dict_level.GetIdItemsRec();
        {
            for (int i = 0; i < pRecord->GetNumItems(); i++)
            {
                const CDictItem* pItem = pRecord->GetItem(i);
                ASSERT(pItem->AddToTreeFor80());
                int pos = 0;
                if (pItem->GetOccurs() > 1)
                {
                    for (int o = 0; o < (int)pItem->GetOccurs(); o++)
                    {
                        pos = GetPositionInList(pItem->GetName(),o+1);
                        if (m_aItems[pos].selected) RetStr += pItem->GetName() + _T(", ");
                    }
                }
                else
                {
                    pos = GetPositionInList(pItem->GetName(),-1);
                    if (m_aItems[pos].selected) RetStr += pItem->GetName() + _T(", ");
                }
            }
        }

        for ( int r = 0; r < dict_level.GetNumRecords(); r++)
        {
            const CDictRecord* pRec = dict_level.GetRecord(r);
            bool flagrec = true;
            for (int i = 0; i < pRec->GetNumItems(); i++){
                const CDictItem* pItem = pRec->GetItem(i);
                if( !pItem->AddToTreeFor80() )
                    continue;
                int occ = pItem->GetOccurs();
                if (pItem->GetParentItem() != NULL)
                    occ = pItem->GetParentItem()->GetOccurs();
                if (occ > 1)
                {
                    for (int o = 0; o < occ; o++)
                    {
                        int pos = GetPositionInList(pItem->GetName(),o+1);
                        if (!(pos >= 0 && m_aItems[pos].selected ))
                        {
                            flagrec = false;
                            break;
                        }
                    }
                }
                else
                {
                    int pos = GetPositionInList(pItem->GetName(),-1);
                    if (!(pos >= 0 && m_aItems[pos].selected ))
                    {
                        flagrec = false;
                        break;
                    }
                }
            }
            if (flagrec )
            {
                RetStr += pRec->GetName() + _T(", ");
            }
            else
            {
                for (int i = 0; i < pRec->GetNumItems(); i++)
                {
                    const CDictItem* pItem = pRec->GetItem(i);
                    if( !pItem->AddToTreeFor80() )
                        continue;
                    int occ = pItem->GetOccurs();
                    if (pItem->GetParentItem() != NULL)
                        occ = pItem->GetParentItem()->GetOccurs()*occ;
                    if (occ > 1)
                    {
                        for (int o = 0; o < occ; o++)
                        {
                            int pos = GetPositionInList(pItem->GetName(),o+1);
                            CString str;
                            str.Format(_T("(%d)"),o+1);
                            if (m_aItems[pos].selected) RetStr += pItem->GetName() + str + _T(", ");
                        }
                    }
                    else
                    {
                        int pos = GetPositionInList(pItem->GetName(),-1);
                        if (m_aItems[pos].selected) RetStr += pItem->GetName() + _T(", ");
                    }
                }
            }
        }
    }
    return RetStr;
}

/////////////////////////////////////////////////////////////////////////////////
//
//  bool CExportDoc::GenerateBatchApp4MultiModel()
//
/////////////////////////////////////////////////////////////////////////////////
bool CExportDoc::GenerateBatchApp4MultiModel()
{
    Application batchApp;
    batchApp.SetEngineAppType(EngineAppType::Batch);
    batchApp.SetLogicSettings(m_logicSettings);

    CString sPath;
    // 20131220 base the export files off the pff location, if available
    sPath = m_PifFile.GetAppFName().IsEmpty() ? m_csDictFileName : m_PifFile.GetAppFName();
    PathRemoveFileSpec(sPath.GetBuffer(MAX_PATH));
    sPath.ReleaseBuffer();
    sPath.TrimRight('\\');

    //make the order spec name ;
    CString sFullFileName = sPath+_T("\\") + _T("CSExpRun.bch");
    CString sOrderFile = sPath+_T("\\")+_T("CSExpRun.ord");


    batchApp.AddFormFilename(sOrderFile);
    //Create the .ord file and save it
    CFileStatus fStatus;
    BOOL bOrderExists = CFile::GetStatus(sOrderFile,fStatus);
    if(bOrderExists){
        BOOL bDel = DeleteFile(sOrderFile);
        if(!bDel){
            CString sMsg;
            sMsg = _T("Cannot Delete ") + sOrderFile;
            AfxMessageBox(sMsg);
            return false;
        }
        else {
            bOrderExists  =false;
        }
    }
    if(!bOrderExists) {

        ASSERT(!m_csDictFileName.IsEmpty());
        CDEFormFile Order(sOrderFile,m_csDictFileName);

        //Create the .ord file and save it
        Order.CreateOrderFile(*m_pDataDict, true);
        Order.Save(sOrderFile);
    }

    batchApp.SetLabel(PortableFunctions::PathGetFilenameWithoutExtension<CString>(sFullFileName));

    if(!WriteDefaultFiles4MultiModel(&batchApp,sFullFileName)){
        return false;
    }

    try
    {
        batchApp.Save(sFullFileName);
    }

    catch( const CSProException& )
    {
        return false;
    }

    return true;
}
/////////////////////////////////////////////////////////////////////////////////
//
//  bool CExportDoc::WriteDefaultFiles4MultiModel(Application* pApplication, const CString &sAppFName)
//
/////////////////////////////////////////////////////////////////////////////////
bool CExportDoc::WriteDefaultFiles4MultiModel(Application* pApplication, const CString &sAppFName)
{
    bool bDone = false;
    //AppFile
    CString sAppSCodeFName(sAppFName);
    PathRemoveExtension(sAppSCodeFName.GetBuffer(_MAX_PATH));
    sAppSCodeFName.ReleaseBuffer();
    sAppSCodeFName += FileExtensions::WithDot::Logic;

    CFileStatus fStatus;
    BOOL bRet = CFile::GetStatus(sAppSCodeFName,fStatus);
    if(bRet) {
        CString sMsg;
        BOOL bDel = DeleteFile(sAppSCodeFName);
        if(!bDel) {
            sMsg = _T("Cannot Delete ") + sAppSCodeFName;
            AfxMessageBox(sMsg);
            return bDone;
        }
        else {
            bRet = false;
        }
    }
    if(!bRet){
        //Create the .app file
        CSpecFile appFile(TRUE);
        appFile.Open(sAppSCodeFName,CFile::modeWrite);
        appFile.WriteString(m_logicSettings.GetDefaultFirstLineForTextSource(pApplication->GetLabel(), AppFileType::Code));
        //Now write logic into the file with the freq command
        appFile.WriteString(_T("\r\nPROC GLOBAL\r\n"));
        appFile.WriteString(_T("numeric i;\r\n"));
        CString sExptProc =  _T("PROC ") + m_pDataDict->GetLevel(m_iLowestLevel).GetName() +_T("\r\n");

        appFile.WriteString(sExptProc);


        CString csmethod;
        switch (m_convmethod)
        {
        case METHOD::SPSS:
            csmethod = _T("set behavior() export SPSS on;\nset behavior() export SAS off;\nset behavior() export STATA off;\n");
            break;
        case METHOD::SAS:
            csmethod = _T("set behavior() export SPSS off;\nset behavior() export SAS on;\nset behavior() export STATA off;\n");
            break;
        case METHOD::STATA:
            csmethod = _T("set behavior() export SPSS off;\nset behavior() export SAS off;\nset behavior() export STATA on;\n");
            break;
        case METHOD::ALLTYPES:
            csmethod = _T("set behavior() export SPSS on;\nset behavior() export SAS on;\nset behavior() export STATA on;\n");
            break;
        default :
            break;
        }
        appFile.WriteString(csmethod);
        appFile.WriteString(_T("\r\n"));

        GenerateApplogic(appFile);

    }

    pApplication->AddCodeFile(CodeFile(CodeType::LogicMain, std::make_shared<TextSource>(CS2WS(sAppSCodeFName))));

    return true;
}


/////////////////////////////////////////////////////////////////////////////////
//
//  bool CExportDoc::GenerateApplogic(CSpecFile& appFile)
//
/////////////////////////////////////////////////////////////////////////////////
bool CExportDoc::GenerateApplogic(CSpecFile& appFile)
{
    CString RetStr;
    CString sExptProc, sCaseID;
    sExptProc = _T("Export  MultipleFiles\n");

    for( const DictLevel& dict_level : m_pDataDict->GetLevels() )
    {
        const CDictRecord* pRecord = dict_level.GetIdItemsRec();
        bool flagrec = true;
        bool bRecSelected = false;
        {//CASE IDS
            for (int i = 0; i < pRecord->GetNumItems(); i++){
                const CDictItem* pItem = pRecord->GetItem(i);
                ASSERT(pItem->AddToTreeFor80());
                int occ = pItem->GetOccurs();
                if (pItem->GetParentItem() != NULL)
                    occ = pItem->GetParentItem()->GetOccurs();
                if (occ > 1){
                    for (int o = 0; o < occ; o++){
                        int pos = GetPositionInList(pItem->GetName(),o+1);
                        if (!(pos >= 0 && m_aItems[pos].selected )){
                            flagrec = false;
                            if(bRecSelected){
                                break;
                            }
                        }
                        if (pos >= 0 && m_aItems[pos].selected) bRecSelected = true;
                    }
                }
                else{
                    int pos = GetPositionInList(pItem->GetName(),-1);
                    if (!(pos >= 0 && m_aItems[pos].selected )){
                        flagrec = false;
                        if(bRecSelected){
                            break;
                        }
                    }
                    if (pos >= 0 && m_aItems[pos].selected) bRecSelected = true;
                }
            }

            if(flagrec && bRecSelected){
                sCaseID = _T(" case_id() ");
            }
            else if(!flagrec && bRecSelected) {
                for (int i = 0; i < pRecord->GetNumItems(); i++){
                    const CDictItem* pItem = pRecord->GetItem(i);
                    if( !pItem->AddToTreeFor80() )
                        continue;
                    int pos = 0;
                    if (pItem->GetOccurs() > 1)
                    {
                        for (int o = 0; o < (int)pItem->GetOccurs(); o++)
                        {
                            pos = GetPositionInList(pItem->GetName(),o+1);
                            if (m_aItems[pos].selected) RetStr += pItem->GetName() + _T(", ");
                        }
                    }
                    else
                    {
                        pos = GetPositionInList(pItem->GetName(),-1);
                        if (m_aItems[pos].selected) RetStr += pItem->GetName() + _T(", ");
                    }
                }
                RetStr.TrimRight(_T(", "));
                if(!RetStr.IsEmpty()){
                    sCaseID = _T(" case_id(") + RetStr+ _T(" ) ");
                }
            }
        }//END CASE IDS
        RetStr.Empty();
        for ( int r = 0 ; r < dict_level.GetNumRecords(); r++ ){
            bool bRecordSelected = false;
            RetStr.Empty();
            CString sForLoop;
            const CDictRecord* pRec = dict_level.GetRecord(r);
            /*int pos = GetPositionInList(pItem->GetName(),-1);
            if (!(pos >= 0 && m_aItems[pos].selected )){
                flagrec = false;
                break;
            }*/

            if(pRec->GetMaxRecs() > 1) {
                CString sEdtName = pRec->GetName() + _T("_EDT");  // BMD 25 Jul 2006
            //  sForLoop = "for i in " + sEdtName+ " do\r\n";
            }
            bool flagrecord = true;
            for (int i = 0; i < pRec->GetNumItems(); i++){
                const CDictItem* pItem = pRec->GetItem(i);
                if( !pItem->AddToTreeFor80() )
                    continue;
                int occ = pItem->GetOccurs();
                if (pItem->GetParentItem() != NULL)
                    occ = pItem->GetParentItem()->GetOccurs();
                if (occ > 1){
                    for (int o = 0; o < occ; o++){
                        int pos = GetPositionInList(pItem->GetName(),o+1);
                        if (!(pos >= 0 && m_aItems[pos].selected )){
                            flagrecord = false;
                            if(bRecordSelected) {
                                break;
                            }
                        }
                        if (pos >= 0 && m_aItems[pos].selected) bRecordSelected = true;
                    }
                }
                else{
                    int pos = GetPositionInList(pItem->GetName(),-1);
                    if (!(pos >= 0 && m_aItems[pos].selected )){
                        flagrecord = false;
                        if(bRecordSelected) {
                            break;
                        }
                    }
                    if (pos >= 0 && m_aItems[pos].selected) bRecordSelected = true;
                }
            }
            if(bRecordSelected) {
                CString sFName = GetFileName4Rec(pRec);
                if(!sFName.IsEmpty()){
                    sExptProc = _T("Export to (") + sFName+ _T(") MultipleFiles\n");
                    sExptProc += sCaseID;
                }
                else {
                    sExptProc += sCaseID;
                }
            }
            if (flagrecord && bRecordSelected) {
                if(sForLoop.IsEmpty()){
                    RetStr = _T("Record (") + pRec->GetName() + _T(");\r\n");
                    sExptProc += RetStr;
                    appFile.WriteString(sExptProc);
                    sExptProc =_T("");
                }
                else{
                    RetStr = _T("Record (") + pRec->GetName() + _T(");\r\n");
                //  RetStr += "enddo; \r\n";
                //  appFile.WriteString(sForLoop);
                    sExptProc += RetStr;
                    appFile.WriteString(sExptProc);
                    sExptProc=_T("");

                }
            }
            else if(bRecordSelected){

                for (int i = 0; i < pRec->GetNumItems(); i++){
                    const CDictItem* pItem = pRec->GetItem(i);
                    if( !pItem->AddToTreeFor80() )
                        continue;
                    int occ = pItem->GetOccurs();
                    if (pItem->GetParentItem() != NULL)
                        occ = pItem->GetParentItem()->GetOccurs()*occ;
                    if (occ > 1){
                        for (int o = 0; o < occ; o++)
                        {
                            int pos = GetPositionInList(pItem->GetName(),o+1);
                            CString str;
                            str.Format(_T("(%d)"),o+1);
                            if (m_aItems[pos].selected) RetStr += pItem->GetName() + str + _T(", ");
                        }
                    }
                    else{
                        int pos = GetPositionInList(pItem->GetName(),-1);
                        if (m_aItems[pos].selected) RetStr += pItem->GetName() + _T(", ");
                    }
                }
                CString sInclude = GenerateInclude(RetStr);
                sInclude.Trim();
                if(sForLoop.IsEmpty() && !sInclude.IsEmpty()){
                    RetStr = _T("Record (") + pRec->GetName()+ _T(" ") + sInclude + _T(" ") +_T(");\r\n");
                    sExptProc += RetStr;
                    appFile.WriteString(sExptProc);
                    sExptProc=_T("");
                }
                else if(!sInclude.IsEmpty()) {
                    RetStr = _T("Record (") + pRec->GetName() + _T(" ") + sInclude + _T(" ") + _T(");\r\n");
                //  RetStr += "enddo; \r\n";
                //  appFile.WriteString(sForLoop);
                    sExptProc += RetStr;
                    appFile.WriteString(sExptProc);
                    sExptProc=_T("");

                }


            }
        }
    }
    return true;
}


/////////////////////////////////////////////////////////////////////////////////
//
//  CString CExportDoc::GenerateInclude(CString& sRecString)
//
/////////////////////////////////////////////////////////////////////////////////
CString CExportDoc::GenerateInclude(CString& sRecString)
{
    CString sRetInclude = _T(" include (");

    sRecString = sRecString.Left(sRecString.ReverseFind(','));
    int pos = -10;
    bool bDone = false;
    while (!bDone)
    {
        if (sRecString.GetLength() > 100)
        {
            pos = sRecString.Find(_T(","),pos + 100);
            if (pos < 0) {                  // BMD 19 March 2004
                sRetInclude += sRecString + _T(");\n");
                break;
            }
            sRetInclude += sRecString.Left(pos+1) + _T("\n");
            sRecString = sRecString.Right(sRecString.GetLength() - (pos+1));
            //sExptProc = "";
            pos = 0;
        }
        else
        {
            sRetInclude += sRecString + _T(")\n");
            bDone = true;
        }
    }
    return sRetInclude;
}

/////////////////////////////////////////////////////////////////////////////////
//
//  CString  CExportDoc::GetFileName4Rec(CDictRecord* pRecord)
//
/////////////////////////////////////////////////////////////////////////////////
CString CExportDoc::GetFileName4Rec(const CDictRecord* pRecord)
{
    CString sRet;
    int rectypes = GetNumExpRecTypes();

    for (int ifile = 0; ifile < rectypes; ifile++){
        if(pRecord->GetRecTypeVal().CompareNoCase(m_rectypes[ifile]) ==0 ){
            if (rectypes <= (int)m_PifFile.GetExportFilenames().size()){
                sRet =  m_PifFile.GetExportFilenames()[ifile] ;
                sRet = _T("\"") + sRet + _T("\"");
                return sRet;
            }
            else{
                return sRet;
            }
        }
        else {
            continue;
        }
    }
    return sRet;
}


/////////////////////////////////////////////////////////////////////////////////
//
//  void CExportDoc::DoPostRunSave();
//
/////////////////////////////////////////////////////////////////////////////////
void CExportDoc::DoPostRunSave()
{
    DoPostRunCleanUp();

    if(m_bPostRunSave && !m_PifFile.GetAppFName().IsEmpty()){
        int iSize = sArray.GetSize();
        if(m_sPFFName.IsEmpty() ){
            m_sPFFName = m_PifFile.GetAppFName() + FileExtensions::WithDot::Pff;
        }
        if(iSize > 0) {
            for(int iIndex =0; iIndex < iSize;iIndex++) {
                m_PifFile.AddExportFilenames(sArray[iIndex]);
            }

            m_PifFile.SetPifFileName(m_sPFFName);
            m_PifFile.Save();
            sArray.RemoveAll();
        }
    }
}

void CExportDoc::DoPostRunCleanUp()
{
    //delete the .pff and other files
    if( !m_sBCHPFFName.IsEmpty() ) {
#ifndef _DEBUG
        DeleteFile(m_sBCHPFFName);
        DeleteFile(m_sBaseFilename + FileExtensions::WithDot::BatchApplication);
        DeleteFile(m_sBaseFilename + FileExtensions::WithDot::Order);
        DeleteFile(m_sBaseFilename + FileExtensions::WithDot::Logic);
#endif
    }
}


#define NUMERIC_VARS_DECLARATION_MARK   _T("NUMERIC_VARS_DECLARATION_MARK")
#define SETFILES_MARK                   _T("SETFILES_MARK")
#define FILES_DECLARATION_MARK          _T("FILES_DECLARATION_MARK")

bool IsIdItem( const CDictItem* pDictItem, CMap<const CDictItem*,const CDictItem*,const CDictItem*,const CDictItem*>& rMapDictIdItems ){

    if(!pDictItem)
        return false;

    const CDictItem* pSameDictItem;
    if(!rMapDictIdItems.Lookup( pDictItem, pSameDictItem ))
        return false;

    return true;
}

int CExportDoc::GetSelectedItems(   CArray<const CDictItem*,const CDictItem*>*                                                                  paSelItems,
                                    CMap<const CDictItem*,const CDictItem*,CArray<int,int>*,CArray<int,int>*>*                                  pMapSelOccsByItem,
                                    CMap<const CDictItem*,const CDictItem*,int,int>*                                                            pMapItems,
                                    CMap<int,int,CArray<const CDictItem*,const CDictItem*>*,CArray<const CDictItem*,const CDictItem*>*>*                    paMap_SelectedIdItems_by_LevelIdx,
                                    CMap<int,int,CArray<const CDictItem*,const CDictItem*>*,CArray<const CDictItem*,const CDictItem*>*>*                    paMap_SelectedNonIdItems_by_LevelIdx,
                                    CMap<const CDictItem*,const CDictItem*,const CDictItem*,const CDictItem*>*                                              pMapDictIdItems,
                                    CMap<const CDictRecord*,const CDictRecord*,int,int>*                                                        pMapLevelIdxByRecord,
                                    CMap<const CDictRecord*,const CDictRecord*,bool,bool>*                                                      pMapIsIdBySelRec,
                                    CMap<const CDictRecord*,const CDictRecord*,CArray<const CDictItem*,const CDictItem*>*,CArray<const CDictItem*,const CDictItem*>*>*  pMapSelItemsByRecord,
                                    CArray<const CDictRecord*,const CDictRecord*>*                                                              paSelIdRecords,
                                    CArray<const CDictRecord*,const CDictRecord*>*                                                              paSelNonIdRecords,
                                    CArray<const CDictRecord*,const CDictRecord*>*                                                              paSelRecords,
                                    CArray<const DictRelation*, const DictRelation*>*                                               paSelRelations){

    if(!m_pDataDict)
        return -1;

    if( paSelItems )
        paSelItems->RemoveAll();

    if( pMapItems )
        pMapItems->RemoveAll();



    //needed to fast recognize of selected items
    CMap<const CDictItem*,const CDictItem*,int,int> aMapSelItems;

    //needed to fast recognize of id items
    CMap<const CDictItem*,const CDictItem*,const CDictItem*,const CDictItem*>       aMapIdItems;

    //needed to know the level of each selected item (excluding ids)
    CMap<const CDictItem*,const CDictItem*,int,int> aMapLevelByItem;

    
    std::vector<int> aAcumItemsByLevelIdx;
    std::vector<const CDictRecord*> aDictIdRecs;
    std::vector<const CDictItem*> aIdItems = m_pDataDict->GetIdItems(&aAcumItemsByLevelIdx, &aDictIdRecs);
    int iNumLevels = (int)aAcumItemsByLevelIdx.size();
    int i=0;
    int iLevelIdx;
    for(iLevelIdx =0; iLevelIdx<iNumLevels; iLevelIdx++ ){

        //ID Items
        int iNumIdItemsByLevel = iLevelIdx>0 ? aAcumItemsByLevelIdx[iLevelIdx] - aAcumItemsByLevelIdx[iLevelIdx-1] : aAcumItemsByLevelIdx[iLevelIdx];
        for(int iItemIdx=0; iItemIdx<iNumIdItemsByLevel; iItemIdx++){
            const CDictItem* pIdItem = aIdItems[i];
            aMapIdItems.SetAt( pIdItem, pIdItem );
            aMapLevelByItem.SetAt( pIdItem, iLevelIdx );
            if( pMapLevelIdxByRecord ){
                const CDictRecord* pRec = pIdItem->GetRecord();
                pMapLevelIdxByRecord->SetAt( pRec, iLevelIdx );
            }
            i++;
        }

        //NON ID Items
        int iNumRecs = m_pDataDict->GetLevel( iLevelIdx ).GetNumRecords();
        for(int iRecIdx=0; iRecIdx<iNumRecs; iRecIdx++){
            int iNumItems = m_pDataDict->GetLevel( iLevelIdx ).GetRecord( iRecIdx )->GetNumItems();
            for(int iItemIdx=0; iItemIdx<iNumItems; iItemIdx++){
                const CDictItem* pDictItem = m_pDataDict->GetLevel( iLevelIdx ).GetRecord( iRecIdx )->GetItem( iItemIdx );
                if( !pDictItem->AddToTreeFor80() )
                    continue;
                aMapLevelByItem.SetAt( pDictItem, iLevelIdx );
                if( pMapLevelIdxByRecord ){
                    const CDictRecord* pRec = pDictItem->GetRecord();
                    pMapLevelIdxByRecord->SetAt( pRec, iLevelIdx );
                }
            }
        }

    }

    //Fill aMapDictIdRecs to fast recognize of selected Id records
    CMap<const CDictRecord*,const CDictRecord*,const CDictRecord*,const CDictRecord*> aMapDictIdRecs;
    int iNumIdRecords = (int)aDictIdRecs.size();
    for(int iDictIdRecordIdx=0; iDictIdRecordIdx<iNumIdRecords; iDictIdRecordIdx++)
        aMapDictIdRecs.SetAt(aDictIdRecs[iDictIdRecordIdx], aDictIdRecs[iDictIdRecordIdx]);

    if( pMapDictIdItems ){
        const CDictItem* pDictItem;
        const CDictItem* pDictItemAux;
        POSITION pos = aMapIdItems.GetStartPosition();
        while(pos){
            aMapIdItems.GetNextAssoc(pos,pDictItem,pDictItemAux);

            pMapDictIdItems->SetAt( pDictItem, pDictItem );
        }
    }

    CArray<const CDictItem*,const CDictItem*> aSelItems;
    CMap<const CDictItem*,const CDictItem*,CArray<int,int>*,CArray<int,int>*>   aMapSelOccsByItem;
    int n = m_pTreeView->GetSelItems( false, aSelItems, aMapSelOccsByItem );

    for ( i = 0; i<n ;i++){

        const CDictItem* pDictItem = aSelItems.GetAt(i);
        CArray<int,int>* pSelOccs = NULL;
        aMapSelOccsByItem.Lookup( pDictItem, pSelOccs );


        if( pMapSelOccsByItem ){

            if( pSelOccs && pSelOccs->GetSize()>0 ){

                CArray<int,int>* pOccs = NULL;
                pMapSelOccsByItem->Lookup( pDictItem, pOccs );
                if(!pOccs){
                    pOccs = new CArray<int,int>;
                    pMapSelOccsByItem->SetAt( pDictItem, pOccs );
                }
                if( pSelOccs )
                    pOccs->Append( *pSelOccs );
            }
        }

        const CDictRecord* pSelDictRecord = pDictItem->GetRecord();

        //remember each selected record, and remember if is an ID rec or not.
        #ifdef _DEBUG
        if( paSelIdRecords || paSelNonIdRecords || paSelRecords){
            ASSERT( pMapIsIdBySelRec!=NULL );
        }
        #endif
        if( pMapIsIdBySelRec ){
            bool bIsIdRec;
            if( !pMapIsIdBySelRec->Lookup( pSelDictRecord, bIsIdRec ) ){

                bIsIdRec = false;
                if( aMapDictIdRecs.Lookup( pSelDictRecord, pSelDictRecord ) )
                    bIsIdRec = true;

                if( bIsIdRec ){
                    if( paSelIdRecords )
                        paSelIdRecords->Add( pSelDictRecord );
                } else {
                    if( paSelNonIdRecords )
                        paSelNonIdRecords->Add( pSelDictRecord );
                }
                if( paSelRecords )
                    paSelRecords->Add( pSelDictRecord );
                pMapIsIdBySelRec->SetAt( pSelDictRecord, bIsIdRec );
            }
        }

        //remember selected items, sorted by selected record.
        if(pMapSelItemsByRecord){
            CArray<const CDictItem*,const CDictItem*>* pSelItems;
            if( !pMapSelItemsByRecord->Lookup( pSelDictRecord, pSelItems ) ){
                pSelItems = new CArray<const CDictItem*,const CDictItem*>;
                pMapSelItemsByRecord->SetAt( pSelDictRecord, pSelItems );
            }
            pSelItems->Add(pDictItem);
        }



        if( pMapItems )
            pMapItems->SetAt( pDictItem, i );

        //aSelItems.Add( pDictItem );
        aMapSelItems.SetAt( pDictItem, i );

    }
    if( paSelItems )
        paSelItems->Append( aSelItems );

    //Fill aMap_SelectedIdItems_by_LevelIdx
    int iNotUsed;
    int iNumIdItemsByLevel;
    if( paMap_SelectedIdItems_by_LevelIdx ){
        i=0;
        for(int iLevelIndex=0; iLevelIndex<iNumLevels; iLevelIndex++ ){

            //ID ITEMS
            iNumIdItemsByLevel = iLevelIndex>0 ? aAcumItemsByLevelIdx[iLevelIndex] - aAcumItemsByLevelIdx[iLevelIndex-1] : aAcumItemsByLevelIdx[iLevelIndex];
            for(int iItemIdx=0; iItemIdx<iNumIdItemsByLevel; iItemIdx++){
                const CDictItem* pIdItem = aIdItems[i];

                if( aMapSelItems.Lookup( pIdItem, iNotUsed ) ){

                    CArray<const CDictItem*,const CDictItem*>* pSelectedIdItemsByLevelIdx = NULL;
                    paMap_SelectedIdItems_by_LevelIdx->Lookup( iLevelIndex, pSelectedIdItemsByLevelIdx );

                    if( !pSelectedIdItemsByLevelIdx ){
                        pSelectedIdItemsByLevelIdx = new CArray<const CDictItem*,const CDictItem*>;
                        paMap_SelectedIdItems_by_LevelIdx->SetAt( iLevelIndex, pSelectedIdItemsByLevelIdx );
                    }

                    pSelectedIdItemsByLevelIdx->Add( pIdItem );
                }

                i++;
            }

        }
    }

    int iNumSelectedItems = aMapSelItems.GetCount();
    //Fill paMap_SelectedNonIdItems_by_LevelIdx
    if( paMap_SelectedNonIdItems_by_LevelIdx ){
        for(int j=0; j<iNumSelectedItems; j++){
            const CDictItem* pDictItem = aSelItems.GetAt(j);

            if( IsIdItem( pDictItem, aMapIdItems ) )
                continue;


            aMapLevelByItem.Lookup( pDictItem, iLevelIdx );

            CArray<const CDictItem*,const CDictItem*>* pSelNonIdItemsByLevelIdx = NULL;
            paMap_SelectedNonIdItems_by_LevelIdx->Lookup( iLevelIdx, pSelNonIdItemsByLevelIdx );

            if(!pSelNonIdItemsByLevelIdx){
                pSelNonIdItemsByLevelIdx = new CArray<const CDictItem*,const CDictItem*>;
                paMap_SelectedNonIdItems_by_LevelIdx->SetAt( iLevelIdx, pSelNonIdItemsByLevelIdx );
            }

            pSelNonIdItemsByLevelIdx->Add( pDictItem );
        }
    }

    //retrieve selected relations
    if( paSelRelations && m_pDataDict ){
        for( const DictRelation& dict_relation : m_pDataDict->GetRelations() ) {
            if( m_pTreeView->IsRelationSelected(dict_relation) ) {
                paSelRelations->Add(&dict_relation);
            }
        }
    }


    const CDictItem* pItem;
    CArray<int,int>* pOccs;
    POSITION pos = aMapSelOccsByItem.GetStartPosition();
    while(pos){
        aMapSelOccsByItem.GetNextAssoc(pos,pItem,pOccs);
        if( pOccs )
            delete( pOccs );
    }


    return m_aItems.GetSize();

}

CString GetTabs(int iNumTabs){
    CString csTabs;
    for(int i=0; i<iNumTabs; i++){
        csTabs += _T("\t");
    }
    return csTabs;
}



int CExportDoc::GetHighestLevel( CMap<int,int,CArray<const CDictItem*,const CDictItem*>*,CArray<const CDictItem*,const CDictItem*>*>& aMap_Items_by_LevelIdx ){
    int                             iMaxLevel = -1;
    int                             iLevelIdx;
    CArray<const CDictItem*,const CDictItem*>*  pItems;
    POSITION pos = aMap_Items_by_LevelIdx.GetStartPosition();
    while(pos){
        aMap_Items_by_LevelIdx.GetNextAssoc(pos,iLevelIdx,pItems );
        if( pItems && pItems->GetSize()>0){
            if( iLevelIdx>iMaxLevel )
                iMaxLevel = iLevelIdx;
        }
    }
    return iMaxLevel;
}


int CExportDoc::GetHighestLevel(    CMap<int,int,CArray<const CDictItem*,const CDictItem*>*,CArray<const CDictItem*,const CDictItem*>*>& aMap_SelectedIdItems_by_LevelIdx,
                        CMap<int,int,CArray<const CDictItem*,const CDictItem*>*,CArray<const CDictItem*,const CDictItem*>*>& aMap_SelectedNonIdItems_by_LevelIdx ){

    int a = GetHighestLevel( aMap_SelectedIdItems_by_LevelIdx );
    int b = GetHighestLevel( aMap_SelectedNonIdItems_by_LevelIdx );

    if( a>b )
        return a;

    return b;
}

int CExportDoc::GetHighestLevel(CArray<const CDictItem*,const CDictItem*>& raItems, CMap<const CDictRecord*,const CDictRecord*,int,int>& rMapLevelIdxByRecord ){

    int iMaxLevel = -1;
    int n = raItems.GetSize();
    for(int i=0; i<n; i++){
        const CDictItem* pDictItem = raItems.GetAt(i);
        const CDictRecord* pDictRecord = pDictItem->GetRecord();
        int iLevel      = -1;
        rMapLevelIdxByRecord.Lookup( pDictRecord, iLevel );
        if( iLevel>iMaxLevel )
            iMaxLevel = iLevel;
    }
    return iMaxLevel;
}

void DeleteMapOccsByItem(CMap<const CDictItem*,const CDictItem*,CArray<int,int>*,CArray<int,int>*>& aMapSelOccsByItem){

    const CDictItem* pDictItem;
    CArray<int,int>* pOccs;
    POSITION pos = aMapSelOccsByItem.GetStartPosition();
    while(pos){
        aMapSelOccsByItem.GetNextAssoc(pos,pDictItem, pOccs );
        if( pOccs )
            delete( pOccs );
    }
}

int CExportDoc::GetHighestSelectedLevel(const DictRelation& dict_relation, CMap<const CDictRecord*,const CDictRecord*,int,int>& rMapLevelIdxByRecord ){

    int iMaxLevel = -1;
    if(m_pTreeView){
        CArray<const CDictItem*,const CDictItem*> aSelItems;
        CMap<const CDictItem*,const CDictItem*,CArray<int,int>*,CArray<int,int>*> aMapSelOccsByItem;
        int iNumSelItems = m_pTreeView->GetSelItemsByRelation(dict_relation, aSelItems, aMapSelOccsByItem );

        for(int iItemIdx=0; iItemIdx<iNumSelItems; iItemIdx++){
            const CDictItem* pDictItem = aSelItems.GetAt( iItemIdx );

            int iItemLevel = -1;
            rMapLevelIdxByRecord.Lookup( pDictItem->GetRecord(), iItemLevel );
            if( iItemLevel>iMaxLevel)
                iMaxLevel = iItemLevel;
        }

        DeleteMapOccsByItem(aMapSelOccsByItem);

    }

    return iMaxLevel;
}


CString GetDefaultExportFileExt( METHOD method, CString* pcsExportFileType, CArray<CString,CString>* paExt){

    CString                 csExportFileType;
    CArray<CString,CString> aExt;

    switch( method ){

        case METHOD::TABS:{
            csExportFileType =_T("Tab delimited");
            aExt.Add(_T(".txt"));
        } break;

        case METHOD::COMMADEL:{
            csExportFileType = _T("Comma delimited");
            aExt.Add(FileExtensions::WithDot::CSV);
        } break;

        case METHOD::SEMI_COLON:{
            csExportFileType = _T("Semicolon delimited");
            aExt.Add(FileExtensions::WithDot::CSV);
        } break;

        case METHOD::CSPRO:{
            csExportFileType = _T("CSPro");
            aExt.Add(FileExtensions::Data::WithDot::TextDataDefault);
            aExt.Add(FileExtensions::WithDot::Dictionary);
        } break;

        case METHOD::SPSS:{
            csExportFileType = _T("SPSS");
            aExt.Add(FileExtensions::Data::WithDot::TextDataDefault);
            aExt.Add(FileExtensions::WithDot::SpssSyntax);
        } break;

        case METHOD::SAS:{
            csExportFileType = _T("SAS");
            aExt.Add(FileExtensions::Data::WithDot::TextDataDefault);
            aExt.Add(FileExtensions::WithDot::SasSyntax);
        } break;

        case METHOD::STATA:{
            csExportFileType = _T("Stata");
            aExt.Add(FileExtensions::Data::WithDot::TextDataDefault);
            aExt.Add(FileExtensions::WithDot::StataDictionary);
            aExt.Add(FileExtensions::WithDot::StataDo);
        }break;

        case METHOD::R:{
            csExportFileType = _T("R");
            aExt.Add(FileExtensions::Data::WithDot::TextDataDefault);
            aExt.Add(FileExtensions::WithDot::RSyntax);
        }break;

        case METHOD::ALLTYPES:{
            csExportFileType = _T("SPSS, SAS, Stata, and R");
            aExt.Add(FileExtensions::Data::WithDot::TextDataDefault);
            aExt.Add(FileExtensions::WithDot::SpssSyntax);
            aExt.Add(FileExtensions::WithDot::SasSyntax);
            aExt.Add(FileExtensions::WithDot::StataDictionary);
            aExt.Add(FileExtensions::WithDot::StataDo);
            aExt.Add(FileExtensions::WithDot::RSyntax);
        }break;

        default :{
            ASSERT(0);
            csExportFileType = _T("Tab delimited");
            aExt.Add(_T(".txt"));
        } break;
    }

    CString csExt;
    if( aExt.GetSize()>0 )
        csExt = aExt.ElementAt(0);

    if(pcsExportFileType)
        *pcsExportFileType = csExportFileType;

    if(paExt)
        (*paExt).Append( aExt );


    return csExt;
}

void CExportDoc::CheckFileExtension() {
    if( m_csExportFileName.IsEmpty() )
        return;

    CString csExt = _T(".") + PortableFunctions::PathGetFileExtension<CString>(m_csExportFileName);

    CArray<CString,CString> aValidExts;
    CString csDefExt = GetDefaultExportFileExt( m_convmethod, NULL, &aValidExts );

    for(int i=0; i<aValidExts.GetSize(); i++) {
        if( csExt.CompareNoCase(aValidExts.GetAt(i))==0 ){
            return;
        }
    }

    // invalid extension
    CString csOldExportFileName = m_csExportFileName;
    m_csExportFileName = PortableFunctions::PathRemoveFileExtension<CString>(m_csExportFileName) + csDefExt;

    if( csOldExportFileName.Compare( m_csExportFileName ) !=0 ) {
        m_csExportApp.Replace( csOldExportFileName, m_csExportFileName );
    }
}

void CExportDoc::CheckUniqueName(CString& rcsName ){

    CString csOldName(rcsName);
    while( !m_aMapUsedUniqueNames[rcsName].IsEmpty() || !m_pDataDict->IsNameUnique(rcsName) || Logic::ReservedWords::IsReservedWord(rcsName) )
    {
        int iLen = rcsName.GetLength();
        TCHAR c = rcsName[iLen - 1];
        c++;
        if (c < 'A') {
            // only try to get a unique name with upper-case letters
            c = _T('A');
        }
        if (c > 'Z') {
            // need to add a letter to the end of the name, then keep trying
            csOldName += _T('A');
            rcsName = csOldName;
            iLen = rcsName.GetLength();
            c = rcsName[iLen - 1];
        }
        rcsName.SetAt(iLen - 1, c);
    }
    m_aMapUsedUniqueNames[rcsName] = rcsName;
}

int CExportDoc::FillExportFiles(/*input*/ const CDataDict* pDataDict, CMap<const CDictRecord*,const CDictRecord*,bool,bool>& raMapIsIdBySelRec, CArray<const CDictItem*,const CDictItem*>& aSelItems, CArray<const CDictRecord*,const CDictRecord*>& aSelRecords, CArray<const DictRelation*,const DictRelation*>& aSelRelations, bool bSingleFile, CString* pcsExportFileName, CString* pcsExportFilesFolder, CString* pcsExportFilesPrefix, METHOD convmethod,
                    /*output*/
                    CArray<CString,CString>& aExportFiles, CArray<CString,CString>& raExportFileVars,
                    CMap<const CDictRecord*,const CDictRecord*,CString,LPCTSTR>& raMapExportFileVarByRecord,
                    CMap<const DictRelation*,const DictRelation*,CString,LPCTSTR>& raMapExportFileVarByRelation ){
    UNREFERENCED_PARAMETER(aSelItems);
    UNREFERENCED_PARAMETER(raMapIsIdBySelRec);
    if(!pDataDict)
        return 0;



    CString csExt = GetDefaultExportFileExt( convmethod, NULL, NULL );




    CString csCurFolder;
    GetCurrentDirectory(_MAX_PATH , csCurFolder.GetBuffer(_MAX_PATH));
    csCurFolder.ReleaseBuffer();


        CString csPrefix;
        if( pcsExportFilesPrefix )
            csPrefix = *pcsExportFilesPrefix;



    //bSingleFile <=> m_bmerge
    if( bSingleFile ){

        CString csExportFileName;
        if(pcsExportFileName){
            csExportFileName = *pcsExportFileName;

        } else {
            csExportFileName = csCurFolder + _T("\\") + csPrefix + pDataDict->GetName() + csExt;
        }
        aExportFiles.Add( csExportFileName );

    } else {



        CString csExportFilesFolder;
        if( pcsExportFilesFolder )
            csExportFilesFolder = *pcsExportFilesFolder;
        csExportFilesFolder.Replace(_T('/'),'\\');
        if( csExportFilesFolder.GetLength()>0 && csExportFilesFolder[csExportFilesFolder.GetLength()-1]=='\\' )
            csExportFilesFolder = csExportFilesFolder.Left( csExportFilesFolder.GetLength()-1 );

        if( csExportFilesFolder.IsEmpty() ){
            csExportFilesFolder = csCurFolder;
        }


        /*Please note that these file names will be considered only as a proposed values : they can be changed using the pif dlg */

        int iNumSelectedRecords = aSelRecords.GetSize();
        for(int iSelRecordIdx=0; iSelRecordIdx<iNumSelectedRecords; iSelRecordIdx++){
            const CDictRecord* pDictRecord = aSelRecords.GetAt(iSelRecordIdx);

            CString csExportFileName = csExportFilesFolder + _T("\\") + csPrefix + pDictRecord->GetName() + csExt;
            aExportFiles.Add( csExportFileName );
        }

        //we are assuming that each relation is "like" a new record
        for(int iSelRelationIdx=0; iSelRelationIdx<aSelRelations.GetSize(); iSelRelationIdx++)
            aExportFiles.Add( csExportFilesFolder + _T("\\") + csPrefix + aSelRelations.GetAt( iSelRelationIdx )->GetName() + csExt );

    }

    int n = aSelRecords.GetSize();
    if( bSingleFile ){



        //CString csFileVar = "File";
        CString csFileVar = _T("cspro_export_file_var_f");
        CheckUniqueName(csFileVar);

        ASSERT( aExportFiles.GetSize()==1 );

        raExportFileVars.Add(csFileVar);

        for(int i=0; i<n; i++){
            raMapExportFileVarByRecord.SetAt( aSelRecords.GetAt(i), csFileVar );
        }

        //RELATIONS
        for( int i=0; i<aSelRelations.GetSize(); i++)
            raMapExportFileVarByRelation.SetAt( aSelRelations.GetAt(i), csFileVar );


    } else {
        ASSERT( aExportFiles.GetSize() == (aSelRecords.GetSize() + aSelRelations.GetSize()) );
        for(int i=0; i<n; i++){
            CString csFileVar = _T("file_") + aSelRecords.GetAt(i)->GetName();
            CheckUniqueName(csFileVar);

            raExportFileVars.Add( csFileVar );
            raMapExportFileVarByRecord.SetAt( aSelRecords.GetAt(i), csFileVar );
        }

        //RELATIONS
        for( int i=0; i<aSelRelations.GetSize(); i++){
            CString csFileVar = _T("file_") + aSelRelations.GetAt(i)->GetName();
            CheckUniqueName(csFileVar);

            raExportFileVars.Add( csFileVar );
            raMapExportFileVarByRelation.SetAt( aSelRelations.GetAt(i), csFileVar );
        }


    }


    return aExportFiles.GetSize();
}

CString ToStrExportItemsSubItems(ExportItemsSubitems eExportItems)
{
    CString csStr;

    switch( eExportItems ){
        case ExportItemsSubitems::ItemsOnly    : csStr = _T("ItemOnly")    ;   break;
        case ExportItemsSubitems::SubitemsOnly : csStr = _T("SubitemOnly") ;   break;
        case ExportItemsSubitems::Both         : csStr = _T("ItemSubitem") ;   break;
        default : ASSERT(0); break;
    }

    return csStr;
}

CString ToStrExportFormat(METHOD method)
{
    CString csStrExportFormat;
    switch( method ){
        case METHOD::TABS       :   csStrExportFormat = _T("TabDelim");         break;
        case METHOD::COMMADEL   :   csStrExportFormat = _T("CommaDelim");       break;
        case METHOD::SPSS       :   csStrExportFormat = _T("SPSS");             break;
        case METHOD::SAS        :   csStrExportFormat = _T("SAS");              break;
        case METHOD::STATA      :   csStrExportFormat = _T("Stata");            break;
        case METHOD::R          :   csStrExportFormat = _T("R");                break;
        case METHOD::ALLTYPES   :   csStrExportFormat = _T("All");              break;
        case METHOD::SEMI_COLON :   csStrExportFormat = _T("SemicolonDelim");   break;
        case METHOD::CSPRO      :   csStrExportFormat = _T("CSPro");            break;
        default : ASSERT(0); break;
    }
    return csStrExportFormat;
}


CString ToStrExportAdditionalOptions(bool bForceANSI,bool bCommaDecimal) // 20120416
{
    CString str;

    if( bForceANSI )    str = _T("ANSI");
    else                str = _T("Unicode");

    if( bCommaDecimal )
        str += _T(", CommaDecimal");

    return str;
}


void FillCaseIds(   //input
                    CArray<const CDictRecord*,const CDictRecord*>& raSelIdRecords,
                    CMap<const CDictRecord*,const CDictRecord*,CArray<const CDictItem*,const CDictItem*>*,CArray<const CDictItem*,const CDictItem*>*>& raMapSelItemsByRecord,
                    bool bSingleFile,
                    CMap<const CDictRecord*,const CDictRecord*,int,int>& raMapLevelIdxByRecord,

                    //output
                    CArray<CString,CString>*    paCaseIdsByLevelIdx,
                    CString*                    pcsCaseIds ){

    UNREFERENCED_PARAMETER(bSingleFile);
    int     iLevelIdx;
    CMap<int,int,CString,LPCTSTR> aMapCaseIdsByLevel;

    CArray<const CDictItem*,const CDictItem*>* pSelItems;
    int iNumSelectedIdRecs = raSelIdRecords.GetSize();
    for(int iSelIdRecIdx=0; iSelIdRecIdx<iNumSelectedIdRecs; iSelIdRecIdx++){
        const CDictRecord* pSelIdRec = raSelIdRecords.GetAt( iSelIdRecIdx );

        if( !raMapLevelIdxByRecord.Lookup( pSelIdRec, iLevelIdx ) ){
            ASSERT( 0 );
        }

        if( !raMapSelItemsByRecord.Lookup( pSelIdRec, pSelItems ) ){
            ASSERT( 0 );
        }

        CString csCaseIds;
        int iNumSelItems = pSelItems ? pSelItems->GetSize() : 0;
        for(int iSelItemIdx=0; iSelItemIdx<iNumSelItems; iSelItemIdx++){
            if( !csCaseIds.IsEmpty() )
                csCaseIds += _T(", ");
            csCaseIds += pSelItems->GetAt(iSelItemIdx)->GetName();
        }

        aMapCaseIdsByLevel.SetAt( iLevelIdx, csCaseIds );
    }
    // 0 -> x,y
    // 2 -> z

    const CDictRecord* pSelRecord;
    int iMaxLevelIdx = -1;
    POSITION pos = raMapSelItemsByRecord.GetStartPosition();
    while(pos){
        raMapSelItemsByRecord.GetNextAssoc(pos,pSelRecord,pSelItems);
        if( !raMapLevelIdxByRecord.Lookup( pSelRecord, iLevelIdx ) ){
            ASSERT( 0 );
        }
        if( iLevelIdx>iMaxLevelIdx )
            iMaxLevelIdx = iLevelIdx;
    }
    CString csCaseIds, csLevelCaseIds;
    for(iLevelIdx=0; iLevelIdx<=iMaxLevelIdx; iLevelIdx++){
        if( aMapCaseIdsByLevel.Lookup( iLevelIdx, csLevelCaseIds ) ){
            if(!csCaseIds.IsEmpty())
                csCaseIds += _T(", ");
            csCaseIds += csLevelCaseIds;
        }
        aMapCaseIdsByLevel.SetAt( iLevelIdx, csCaseIds );

        if(paCaseIdsByLevelIdx)
            paCaseIdsByLevelIdx->Add( csCaseIds );

    }
    if(pcsCaseIds)
        *pcsCaseIds = csCaseIds;

    //pcsCaseIds : "x,y,z"

    // paCaseIdsByLevelIdx :
    // 0 -> "x,y"
    // 1 -> "x,y"
    // 2 -> "x,y,z"  ----> match pcsCaseIds
}


static inline int CompareInt( const void* elem1, const void* elem2 ){
    int* pInt1 = (int*) elem1;
    int* pInt2 = (int*) elem2;
    return *pInt1 > *pInt2 ? 1 : *pInt1==*pInt2 ? 0 : -1;
}


void CExportDoc::FillProcs( /*input*/
                            const CDataDict* pDataDict,
                            bool bSingleFile,
                            CMap<int,int,CArray<const CDictItem*,const CDictItem*>*,CArray<const CDictItem*,const CDictItem*>*>& aMap_SelectedIdItems_by_LevelIdx,
                            CMap<int,int,CArray<const CDictItem*,const CDictItem*>*,CArray<const CDictItem*,const CDictItem*>*>& aMap_SelectedNonIdItems_by_LevelIdx,
                            CArray<const DictRelation*,const DictRelation*>&                             raSelectedRelations,
                            CMap<const CDictRecord*,const CDictRecord*,int,int>&                                     rMapLevelIdxByRecord,
                            CArray<const CDictItem*,const CDictItem*>&                                               raSingleItemsPrefix,

                            /*output*/
                            CArray<int,int>& aLevelsWithExportProc ){
    UNREFERENCED_PARAMETER(raSingleItemsPrefix);
    UNREFERENCED_PARAMETER(pDataDict);

    //int iPrefixLevel = GetHighestLevel( raSingleItemsPrefix, rMapLevelIdxByRecord );


    CMap<int,int,int,int> aMapAddedProcs;


    if( bSingleFile && m_bAllInOneRecord ){

        //the only level with export proc will be the last selected level <- valid in the non-relations tree
        int iMaxSelLevel = GetHighestLevel( aMap_SelectedIdItems_by_LevelIdx, aMap_SelectedNonIdItems_by_LevelIdx );
        if( iMaxSelLevel != -1 ){
            aLevelsWithExportProc.Add( iMaxSelLevel );
            aMapAddedProcs.SetAt( iMaxSelLevel, iMaxSelLevel );
        }

    } else {

        //getexportproc will perform the filter of wich levels has some proc
        int iNumLevels = m_pDataDict ? (int)m_pDataDict->GetNumLevels() : -1;
        for(int l=0; l<iNumLevels; l++){
            aLevelsWithExportProc.Add( l );
            aMapAddedProcs.SetAt( l, l);
        }
    }

    //each selected relation can be in a different proc
    int iNumSelRelations = raSelectedRelations.GetSize();
    for(int iRelIdx=0; iRelIdx<iNumSelRelations; iRelIdx++){
        const DictRelation*  pDictRelation = raSelectedRelations.GetAt( iRelIdx );
        int iMaxSelLevel = GetHighestSelectedLevel(*pDictRelation, rMapLevelIdxByRecord);
        if( iMaxSelLevel>0 && !aMapAddedProcs.Lookup( iMaxSelLevel, iMaxSelLevel ) )
            aLevelsWithExportProc.Add( iMaxSelLevel );
    }

    qsort( aLevelsWithExportProc.GetData(), aLevelsWithExportProc.GetSize(), sizeof(int), CompareInt );
}



void CExportDoc::Append( const CString& rcsLines, CString* pcsAppBuff ){

    if( rcsLines.IsEmpty() )
        return;

    if( !pcsAppBuff )
        pcsAppBuff = &m_csExportApp;

    if(!pcsAppBuff->IsEmpty())
        *pcsAppBuff += _T("\n");
    *pcsAppBuff += rcsLines;

}


void CExportDoc::PROC_LEVEL(const DictLevel& dict_level)
{
    Append( _T("PROC ") + dict_level.GetName() );
}



void CExportDoc::PreProc(CString* pcsProcLevel){
    Append(_T("PreProc"), pcsProcLevel ? pcsProcLevel : &m_csExportApp);
}
void CExportDoc::PostProc(CString* pcsProcLevel){
    Append(_T("PostProc"), pcsProcLevel ? pcsProcLevel : &m_csExportApp);
}

void CExportDoc::SetBehavior(CString* pcsBuff){

    if( bBehaviorDone )
        return;

    Append( GetTabs(m_tabs) + _T("set behavior() export (")
        + ToStrExportFormat( m_convmethod )
        + _T(", ")
        + ToStrExportItemsSubItems(m_exportItemsSubitems)
        + _T(", ")
        + ToStrExportAdditionalOptions(m_bForceANSI,m_bCommaDecimal)
        + _T(");"), pcsBuff );
    Append( _T("\n"), pcsBuff );

    bBehaviorDone = true;

}

void CExportDoc::DeclareNumericVars(CMapStringToString& aMapUsedNumericVars){

    CString csCmdNumericVars;
    CString csNumVar;
    POSITION pos = aMapUsedNumericVars.GetStartPosition();
    while(pos){
        aMapUsedNumericVars.GetNextAssoc(pos,csNumVar,csNumVar);
        if( !csCmdNumericVars.IsEmpty() )
            csCmdNumericVars += _T("\n");
        csCmdNumericVars += _T("NUMERIC ") + csNumVar + _T(";");
    }
    if( !csCmdNumericVars.IsEmpty() ){
        m_csExportApp.Replace( NUMERIC_VARS_DECLARATION_MARK, _T("\n") + csCmdNumericVars + _T("\n") );

    } else {
        m_csExportApp.Replace( NUMERIC_VARS_DECLARATION_MARK, _T("") );
    }
}


void CExportDoc::DeclareFileVars( CArray<CString,CString>& aExportFileVars, CMapStringToString& rMapUsedFiles ){

    CString csFilesDeclaration;
    m_tabs=0;
    int iNumExportFiles = aExportFileVars.GetSize();
    for(int iExportFileVarIdx=0; iExportFileVarIdx<iNumExportFiles; iExportFileVarIdx++){

        const CString& rcsFileVar = aExportFileVars.ElementAt(iExportFileVarIdx);
        if( rMapUsedFiles[rcsFileVar].IsEmpty() )
            continue;

        Append( GetTabs(m_tabs) + _T("FILE ") + rcsFileVar + _T(";"), &csFilesDeclaration );
    }

    if( !csFilesDeclaration.IsEmpty() ){
        m_csExportApp.Replace( FILES_DECLARATION_MARK, _T("\n") + csFilesDeclaration + _T("\n"));

    } else {
        m_csExportApp.Replace( FILES_DECLARATION_MARK, _T(""));

    }
}


void InitFor( CString& rcsExportApp, const CString& rcsIdx, const CString& rcsVarType,
              const CString& rcsVarName, int& tabs, CMapStringToString& rUsedVars ){

    ASSERT( rcsVarType.IsEmpty() || rcsVarType.CompareNoCase(_T("SUBITEM")) ||rcsVarType.CompareNoCase(_T("ITEM")) || rcsVarType.CompareNoCase(_T("RECORD")) || rcsVarType.CompareNoCase(_T("RELATION")) );

    CString csVarDescription = rcsVarType.IsEmpty() ? rcsVarName : rcsVarType + _T(" ") + rcsVarName;
    CString csInitFor = _T("For ") + rcsIdx + _T(" in ") +  csVarDescription + _T(" do");
    if( !rcsExportApp.IsEmpty() )
        rcsExportApp += _T("\n");
    rcsExportApp += GetTabs(tabs) + csInitFor;
    tabs++;

    rUsedVars.SetAt( rcsIdx, rcsIdx );

}

void InitIf( CString& rcsExportApp, const CString& rcsIdx, CArray<int,int>* pOccs, int& tabs){

    ASSERT( !rcsIdx.IsEmpty() );
    if(!pOccs || pOccs->GetSize()==0 )
        return;

    CString csOccs;
    int n = pOccs->GetSize();
    for(int i=0; i<n; i++){

        if( !csOccs.IsEmpty() )
            csOccs += _T(", ");

        csOccs += IntToString(pOccs->GetAt(i));
        if (i != n - 1) {
            if (pOccs->GetAt(i) + 1 == pOccs->GetAt(i+1)) {
                for(int j=i+1; j<n; j++){
                    if (pOccs->GetAt(j-1) + 1 != pOccs->GetAt(j)) {
                        csOccs += _T(":") + IntToString(pOccs->GetAt(j-1));
                        i = j - 1;
                        break;
                   }
                    else if (j == n-1) {
                        csOccs += _T(":") + IntToString(pOccs->GetAt(j));
                        i = n - 1;
                        break;
                    }
                }
            }
        }
    }

    CString csIf = _T("If ") + rcsIdx + _T(" in ") + csOccs + _T(" then");

    if( !rcsExportApp.IsEmpty() )
        rcsExportApp += _T("\n");
    rcsExportApp += GetTabs(tabs) + csIf;
    tabs++;
}

void EndIf(CString& rcsExportApp, CArray<int,int>* pOccs, int& tabs){

    if(!pOccs || pOccs->GetSize()==0)
        return;

    CString csEndIf = _T("EndIf;");
    tabs--;
    if( !rcsExportApp.IsEmpty() )
        rcsExportApp += _T("\n");
    rcsExportApp += GetTabs(tabs) + csEndIf;
}

bool IsMultiple( const CDictItem* pDictItem ){

    bool    bMultiple = pDictItem->GetOccurs()>1;

    bMultiple = bMultiple || (pDictItem->GetItemType() == ItemType::Subitem && pDictItem->GetParentItem()->GetOccurs()>1);

    return bMultiple;
}

void InitFor( CString& rcsExportApp, const CString& rcsIdx, const CDictItem* pDictItem,
              int& tabs, CMapStringToString& rUsedVars ){

    bool bMultiple = IsMultiple( pDictItem );
    if(  bMultiple ){

        CString csMask;
        CString csName;
        if( pDictItem->GetOccurs()>1 ){
            csMask = pDictItem->GetItemType() == ItemType::Subitem ? _T("SUBITEM") : _T("ITEM");
            csName = pDictItem->GetName();

        } else {

            if( pDictItem->GetItemType() == ItemType::Subitem && pDictItem->GetParentItem()->GetOccurs()>1){
                csMask = _T("ITEM");
                csName = pDictItem->GetParentItem()->GetName();

            } else {

                csMask = _T("ITEM");
                csName = pDictItem->GetName();
            }
        }

        InitFor( rcsExportApp, rcsIdx, csMask, csName, tabs, rUsedVars );
    }
}



void InitFor( CString& rcsExportApp, const CString& rcsIdx, const CDictRecord* pDictRecord,
              int& tabs, CMapStringToString& rUsedVars ){
    if( pDictRecord->GetMaxRecs()>1 )
        InitFor( rcsExportApp, rcsIdx, _T("RECORD"), pDictRecord->GetName(), tabs, rUsedVars );
}

void EndFor( CString& rcsExportApp, int& tabs ){
    CString csEndFor = _T("Enddo;");
    tabs--;
    if( !rcsExportApp.IsEmpty() )
        rcsExportApp += _T("\n");
    rcsExportApp += GetTabs(tabs) + csEndFor;
}

void EndFor( CString& rcsExportApp, const CDictItem* pDictItem, int& tabs ){
    bool bMultiple = IsMultiple( pDictItem );
    if( bMultiple )
        EndFor( rcsExportApp, tabs );

}

void EndFor( CString& rcsExportApp, const CDictRecord* pDictRecord, int& tabs ){
    if( pDictRecord->GetMaxRecs()>1 )
        EndFor( rcsExportApp, tabs );
}


void InitUniverse( CString& rcsExportApp, const CString& rcsUniverse, int& tabs ){

    CString csUniverse = rcsUniverse;
    csUniverse.TrimLeft();
    csUniverse.TrimRight();
    if( csUniverse.IsEmpty() )
        return;

    if( !rcsExportApp.IsEmpty() )
        rcsExportApp += _T("\n");
    rcsExportApp += GetTabs(tabs) + _T("If ") + csUniverse + _T(" then");
    tabs++;
}
void EndUniverse( CString& rcsExportApp, const CString& rcsUniverse, int& tabs ){

    CString csUniverse = rcsUniverse;
    csUniverse.TrimLeft();
    csUniverse.TrimRight();
    if( csUniverse.IsEmpty() )
        return;

    tabs--;
    if( !rcsExportApp.IsEmpty() )
        rcsExportApp += _T("\n");
    rcsExportApp += GetTabs(tabs) + _T("Endif;");
}

void CASE_ID(CString& rcsExportApp, const CString& rcsCaseId, int tabs){

    if( rcsCaseId.IsEmpty() )
        return;

    CString csCmdCaseId = _T("CASE_ID(") + rcsCaseId + _T(")");
    if( !rcsExportApp.IsEmpty() )
        rcsExportApp += _T("\n");
    rcsExportApp += GetTabs(tabs) + csCmdCaseId;
}

void EXPORT_TO( CString& rcsExportApp, const CString& rcsFileVar, int tabs ){

    CString csExportTo = GetTabs(tabs) + ( !rcsFileVar.IsEmpty() ? _T("EXPORT TO ") + rcsFileVar : _T("EXPORT"));
    if( !rcsExportApp.IsEmpty() )
        rcsExportApp += _T("\n");
    rcsExportApp += csExportTo;
}


void EXPORT_LIST( CString& rcsExportApp, const CString& rcsRecItemNames, int tabs){

    if( rcsRecItemNames.IsEmpty() )
        return;

    if(!rcsExportApp.IsEmpty() )
        rcsExportApp += _T("\n");
    rcsExportApp += GetTabs(tabs) + rcsRecItemNames + _T(";");
}


void CExportDoc::SetTreeView( CExportView* pTreeView ){
    m_pTreeView = pTreeView;
}

void CExportDoc::SetOptionsView( CExportOptionsView* pOptionsView ){
    m_pOptionsView = pOptionsView;
}



bool NeedExplicitOccs(CArray<int,int>* pOccs, const CDictItem* pDictItem){

    //Need to be rewriten for 3D


    if(!pOccs)
        return false;

    ASSERT( pDictItem );
    if(!pDictItem)
        return false;


    bool bMultiple = false;
    int  iMaxOccs  = 1;

    const CDictItem* pParent = pDictItem->GetItemType() == ItemType::Subitem ? pDictItem->GetParentItem() : NULL;

    //subitem inside a multiple item Item
    if( pDictItem->GetItemType() == ItemType::Subitem && pParent && pParent->GetOccurs()>1 ){
        bMultiple   = true;
        iMaxOccs    = pParent->GetOccurs();
        //2D

    //multiple item
    } else if( pDictItem->GetOccurs()>1 ){

        bMultiple = true;
        iMaxOccs  = pDictItem->GetOccurs();


    //any thing inside a multiple record
    } else if( pDictItem->GetRecord()->GetMaxRecs()>1 ){
        bMultiple = true;

        //item single inside a record multiple -> 2D index
        if( pDictItem->GetOccurs()==1 ){
            iMaxOccs = pDictItem->GetRecord()->GetMaxRecs();

        } else {

            //this needs 2 index -> 3D
            ASSERT( 0 );
            /*
            iMaxOccs_item = pDictItem->GetOccurs();
            iMaxOccs_rec  = pDictItem->GetRecord()->GetMaxRecs();
            */
        }


    }

    if( !bMultiple ){
        ASSERT( 0 ); //pOccs must be NULL for single items ???

        return false;
    }


    //bMultiple
    if( pOccs->GetSize()<iMaxOccs )
        return true;

    ASSERT( pOccs->GetSize()==iMaxOccs );
    if( pOccs->GetSize()==iMaxOccs )
        return false;

    ASSERT(0);
    return true;
}

bool HasAnyMultipleItem(const CDictRecord* pDictRecord){
    bool bHasAnyMultipleItem = false;
    int n = pDictRecord->GetNumItems();
    for(int i=0; i<n; i++){
        if( pDictRecord->GetItem(i)->GetOccurs()>1 ){
            bHasAnyMultipleItem = true;
            break;
        }
    }
    return bHasAnyMultipleItem;
}


//static
bool CExportDoc::IsFlatExport( const CDictRecord* pDictRecord, bool* pbHasAnyMultipleItem ){

    //take care : enabled doesn't mean checked...
    if(m_pOptionsView && !m_pOptionsView->IsJoinEnabled())
        return false;

    if( !m_bJoinSingleWithMultipleRecords )
        return false;


    if(!pDictRecord)
        return false;

    bool bHasAnyMultipleItem = HasAnyMultipleItem( pDictRecord );
    if( pbHasAnyMultipleItem )
        *pbHasAnyMultipleItem = bHasAnyMultipleItem;

    bool    bIsFlatExport = false;
    if( bBehavior_SingleRecordsFlat ){

        //check the record is single
        if( pDictRecord->GetMaxRecs()==1 ){

            //check it has any multiple item
            if( bHasAnyMultipleItem ){
                bIsFlatExport = true;
            }
        }
    }

    return bIsFlatExport;
}

void CExportDoc::SetJoin( bool bJoin ){
    m_bJoinSingleWithMultipleRecords = bJoin;

}

void CExportDoc::FillExportList(//input
                    CArray<const CDictItem*,const CDictItem*>& aSelItems,
                    CMap<const CDictItem*,const CDictItem*,CArray<int,int>*,CArray<int,int>*>& rMapSelOccsByItem,
                    CMap<const CDictItem*,const CDictItem*,const CDictItem*,const CDictItem*>& rMapDictIdItems,
                    bool bReplaceItemNamesByRecordName,
                    bool bIncludeMultipleItems,
                    bool bIncludeSingleItemsWithMultipleParents,
                    const CDictRecord* pExcludedRec,
                    CString*     pcsRecordLoopIdx,
                    CString*     pm_csMultItemLoopIdx,
                    bool         bFlatExport,


                    //output
                    CString& rcsExportList){

    ASSERT( !( pcsRecordLoopIdx!=NULL && pm_csMultItemLoopIdx!=NULL ) );


    if( bFlatExport ){
        bIncludeMultipleItems                   = true;
        bIncludeSingleItemsWithMultipleParents  = true;
    }

    if( pm_csMultItemLoopIdx!=NULL )
        bReplaceItemNamesByRecordName = false;
    CMap<const CDictRecord*,const CDictRecord*,bool,bool> aMapFlatExport;



    CArray<const DictNamedBase*,const DictNamedBase*> aSelObjects;

    int iNumSelItemsByRecord;
    CMap<const CDictRecord*,const CDictRecord*,int,int> aMap_SelectedItems_by_Record;
    CMap<const CDictItem*,const CDictItem*,bool,bool>   aMapAddedItems;
    int iNumSelItems = aSelItems.GetSize();
    for(int iSelItemIdx=0; iSelItemIdx<iNumSelItems; iSelItemIdx++){

        const CDictItem* pSelItem = aSelItems.GetAt(iSelItemIdx);
        const CDictRecord* pSelRec = pSelItem->GetRecord();

        bool bFlatExportRecord;
        if( !aMapFlatExport.Lookup(pSelRec,bFlatExportRecord) ){
            bFlatExportRecord = IsFlatExport( pSelRec, NULL );
            aMapFlatExport.SetAt( pSelRec, bFlatExportRecord );
        }
        if( bFlatExportRecord ){
            bIncludeMultipleItems                   = true;
            bIncludeSingleItemsWithMultipleParents  = true;
        }

        //avoid duplicate items
        bool bAdded = false;
        if(aMapAddedItems.Lookup( pSelItem, bAdded )){
            continue;
        }

        //avoid id items
        if(IsIdItem( pSelItem, rMapDictIdItems ))
            continue;

        //avoid multiple items (behavior deppends on given flag)
        if(!bIncludeMultipleItems && pSelItem->GetOccurs()>1)
            continue;

        if(!bIncludeMultipleItems && pSelItem->GetItemType() == ItemType::Subitem && pSelItem->GetParentItem()->GetOccurs()>1)
            continue;

        if(!bIncludeSingleItemsWithMultipleParents && pSelItem->GetOccurs()==1 && pSelItem->GetRecord()->GetMaxRecs()>1)
            continue;

        if(!bIncludeSingleItemsWithMultipleParents && pSelItem->GetItemType() == ItemType::Subitem && pSelItem->GetParentItem()->GetOccurs()>1 )
            continue;

        //avoid items that belong to any excluded rec
        if( pExcludedRec!=NULL && pSelRec==pExcludedRec )
            continue;


        aSelObjects.Add( pSelItem );
        aMapAddedItems.SetAt( pSelItem, true );

        if( !aMap_SelectedItems_by_Record.Lookup( pSelRec, iNumSelItemsByRecord) ){
            iNumSelItemsByRecord = 1;
        } else {
            iNumSelItemsByRecord++;
        }
        aMap_SelectedItems_by_Record.SetAt( pSelRec, iNumSelItemsByRecord );


        if( bReplaceItemNamesByRecordName ){
            if( iNumSelItemsByRecord == pSelRec->GetNumItems() ){

                //first : remove each item that belong to the same record :
                int iNumAddedObjects = aSelObjects.GetSize();
                for(int j=iNumAddedObjects-1; j>=0; j--){
                    const DictNamedBase* pObj = aSelObjects.GetAt(j);
                    if( pObj->GetElementType() == DictElementType::Item && assert_cast<const CDictItem*>(pObj)->GetRecord()==pSelRec ){
                        aSelObjects.RemoveAt(j);
                    }
                }

                //then, add the record
                aSelObjects.Add( pSelRec );
            }
        }
    }

    CString csIdx;
    /*
    if(pcsRecordLoopIdx)
        csIdx += *pcsRecordLoopIdx;
    if(pm_csMultItemLoopIdx){
        ASSERT( csIdx.IsEmpty() );
        if(!csIdx.IsEmpty())
            csIdx += ",";
        csIdx += *pm_csMultItemLoopIdx;
    }
    */

    int n = aSelObjects.GetSize();
    int nLineLength = 0;
    for(int i=0; i<n; i++){

        if(!rcsExportList.IsEmpty())
            rcsExportList += _T(", ");
        if (nLineLength > 79) {
            rcsExportList += _T("\n");
            nLineLength = 0;
        }

        const DictNamedBase* pObj = aSelObjects.GetAt(i);
        if( pObj->GetElementType() == DictElementType::Item ){

            const CDictItem* pDictItem = (CDictItem*)pObj;

            CArray<int,int>* pOccs = NULL;
            rMapSelOccsByItem.Lookup( pDictItem, pOccs );
            if( pOccs ){

                if( NeedExplicitOccs(pOccs,pDictItem) ){

                    int iNumSelOccs = pOccs->GetSize();
                    for(int iSelOccIdx=0; iSelOccIdx<iNumSelOccs; iSelOccIdx++){

                        if( iSelOccIdx>0 ){
                            if(!rcsExportList.IsEmpty())
                                rcsExportList += _T(", ");
                            if (nLineLength > 79) {
                                rcsExportList += _T("\n");
                                nLineLength = 0;
                            }
                        }
                        rcsExportList += pDictItem->GetName();
                        nLineLength += pDictItem->GetName().GetLength();

                        //specify the occurrence
                        int iOcc = pOccs->GetAt(iSelOccIdx);
                        if( iOcc!=-1 ){
                            CString csOcc = IntToString(iOcc);
                            rcsExportList += _T("(") + csOcc + _T(")");
                            nLineLength += 2 + csOcc.GetLength();
                        }
                    }

                } else {

                    //all occurs selected => include the item
                    rcsExportList += pDictItem->GetName();
                    nLineLength += pDictItem->GetName().GetLength();
                }
            } else {

                //there are no occurrences for the given item
                rcsExportList += pDictItem->GetName();
                nLineLength += pDictItem->GetName().GetLength();
            }

        } else if( pObj->GetElementType() == DictElementType::Record ){
            rcsExportList += ((const CDictRecord*)pObj)->GetName();
            nLineLength += ((const CDictRecord*)pObj)->GetName().GetLength();

        } else {
            ASSERT(0);
        }

        if( !csIdx.IsEmpty() ) {
            rcsExportList += _T("(") + csIdx + _T(")");
            nLineLength += 2 + csIdx.GetLength();
        }
    }
}

CString CExportDoc::GetExportList( CArray<const CDictItem*,const CDictItem*>& aSelItems, CMap<const CDictItem*,const CDictItem*,CArray<int,int>*,CArray<int,int>*>& rMapSelOccsByItem, CMap<const CDictItem*,const CDictItem*,const CDictItem*,const CDictItem*>& rMapDictIdItems, bool bReplaceItemNamesByRecordName, bool bIncludeMultipleItems, bool bIncludeSingleItemsWithMultipleParent, const CDictRecord* pExcludedRec, CString* pcsRecordLoopIdx, CString* pm_csMultItemLoopIdx, bool bFlatExport ){
    CString csExportList;
    CExportDoc::FillExportList(aSelItems,rMapSelOccsByItem,rMapDictIdItems,bReplaceItemNamesByRecordName,bIncludeMultipleItems, bIncludeSingleItemsWithMultipleParent, pExcludedRec,pcsRecordLoopIdx, pm_csMultItemLoopIdx, bFlatExport,csExportList);
    return csExportList;
}

void CExportDoc::FillExportList(//input
                    CArray<const CDictRecord*,const CDictRecord*>* paSelRecs,
                    CMap<const CDictRecord*,const CDictRecord*,CArray<const CDictItem*,const CDictItem*>*,CArray<const CDictItem*,const CDictItem*>*>&  rMapSelItemsByRecord,
                    CMap<const CDictItem*,const CDictItem*,CArray<int,int>*,CArray<int,int>*>& rMapSelOccsByItem,
                    CMap<const CDictItem*,const CDictItem*,const CDictItem*,const CDictItem*>& rMapDictIdItems,
                    bool bReplaceItemNamesByRecordName,
                    bool bIncludeMultipleItems,
                    bool bIncludeSingleItemsWithMultipleParent,
                    const CDictRecord* pExcludedRec,
                    CString*     pcsRecordLoopIdx,
                    CString*     pm_csMultItemLoopIdx,
                    bool         bFlatExport,

                    //output
                    CString& rcsExportList){

    CArray<const CDictItem*,const CDictItem*> aSelItems;
    int iNumSelRecs = paSelRecs->GetSize();
    for(int iSelRecIdx=0; iSelRecIdx<iNumSelRecs; iSelRecIdx++){
        const CDictRecord* pSelRec = paSelRecs->GetAt(iSelRecIdx);
        CArray<const CDictItem*,const CDictItem*>* pSelItemsByRec = NULL;
        if(!rMapSelItemsByRecord.Lookup( pSelRec, pSelItemsByRec ) || !pSelItemsByRec){
            ASSERT(0);
        }
        aSelItems.Append( *pSelItemsByRec );
    }

    FillExportList( aSelItems, rMapSelOccsByItem, rMapDictIdItems, bReplaceItemNamesByRecordName, bIncludeMultipleItems, bIncludeSingleItemsWithMultipleParent, pExcludedRec, pcsRecordLoopIdx, pm_csMultItemLoopIdx, bFlatExport,
                    rcsExportList );
}

void CExportDoc::FillExportList(//input
                    const CDictRecord* pSelRec,
                    CMap<const CDictRecord*,const CDictRecord*,CArray<const CDictItem*,const CDictItem*>*,CArray<const CDictItem*,const CDictItem*>*>&  rMapSelItemsByRecord,
                    CMap<const CDictItem*,const CDictItem*,CArray<int,int>*,CArray<int,int>*>&  rMapSelOccsByItem,
                    CMap<const CDictItem*,const CDictItem*,const CDictItem*,const CDictItem*>& rMapDictIdItems,
                    bool bReplaceItemNamesByRecordName,
                    bool bIncludeMultipleItems,
                    bool bIncludeSingleItemsWithMultipleParent,
                    const CDictRecord* pExcludedRec,
                    CString*     pcsRecordLoopIdx,
                    CString*     pm_csMultItemLoopIdx,
                    bool         bFlatExport,

                    //output
                    CString& rcsExportList){

    CArray<const CDictRecord*,const CDictRecord*> aSelRecs;
    aSelRecs.Add(pSelRec);

    FillExportList( &aSelRecs, rMapSelItemsByRecord, rMapSelOccsByItem, rMapDictIdItems, bReplaceItemNamesByRecordName, bIncludeMultipleItems, bIncludeSingleItemsWithMultipleParent, pExcludedRec, pcsRecordLoopIdx, pm_csMultItemLoopIdx, bFlatExport,
                    rcsExportList );
}


void DumpSelRecords( int iLevelIdx,
                    CMap<int,int,CArray<const CDictItem*,const CDictItem*>*,CArray<const CDictItem*,const CDictItem*>*>& aMap_SelectedItems_by_LevelIdx,
                    CArray<const CDictRecord*,const CDictRecord*>& aSelRecords){

    CArray<const CDictItem*,const CDictItem*>* pSelItems;
    if(!aMap_SelectedItems_by_LevelIdx.Lookup( iLevelIdx, pSelItems ) || !pSelItems )
        return;

    CMap<const CDictRecord*,const CDictRecord*,bool,bool> aMapAddedRecords;
    int n = pSelItems->GetSize();
    for(int i=0; i<n; i++){
        const CDictItem* pDictItem = pSelItems->GetAt(i);
        const CDictRecord* pDictRecord = pDictItem->GetRecord();

        bool bAddedRec = false;
        aMapAddedRecords.Lookup( pDictRecord, bAddedRec );
        if( !bAddedRec ){
            aSelRecords.Add( pDictRecord );
            aMapAddedRecords.SetAt( pDictRecord, true );
        }
    }
}

CString GetFileVar( const CDictRecord* pDictRecord, CMap<const CDictRecord*,const CDictRecord*,CString,LPCTSTR>& aMapExportFileVarByRecord ){

    CString csFileVar;
    aMapExportFileVarByRecord.Lookup( pDictRecord, csFileVar );
    return csFileVar;
}

CString GetFileVar( const DictRelation* pDictRelation, CMap<const DictRelation*,const DictRelation*,CString,LPCTSTR>& rMapExportFileVarByRelation ){

    CString csFileVar;
    rMapExportFileVarByRelation.Lookup( pDictRelation, csFileVar );
    return csFileVar;
}


void CExportDoc::FillMapExportFilesByProc(//input
                              bool                                                                          bSingleFile,
                              CArray<int,int>&                                                              aLevelsWithExportProc,
                              CArray<CString,CString>&                                                      aExportFileVars,
                              CMap<int,int,CArray<const CDictItem*,const CDictItem*>*,CArray<const CDictItem*,const CDictItem*>*>&  aMap_SelectedIdItems_by_LevelIdx,
                              CMap<int,int,CArray<const CDictItem*,const CDictItem*>*,CArray<const CDictItem*,const CDictItem*>*>&  aMap_SelectedNonIdItems_by_LevelIdx,
                              CMap<const CDictRecord*,const CDictRecord*,CString,LPCTSTR>&                              aMapExportFileVarByRecord,

                              //RELATIONS
                              CMap<const DictRelation*,const DictRelation*,CString,LPCTSTR>&                aMapExportFileVarByRelation,
                              CArray<const DictRelation*,const DictRelation*>&                              aSelRelations,
                              CMap<const CDictRecord*,const CDictRecord*,int,int>&                                      rMapLevelIdxByRecord,

                              //output
                              CMap<int,int,CStringArray*,CStringArray*>&                                    aMapExportFileVarsByProc ){


    //RELATIONS
    CMap<int,int,CArray<const DictRelation*,const DictRelation*>*,CArray<const DictRelation*,const DictRelation*>*> aMapSelRelationsByLevel;
    int n = aSelRelations.GetSize();
    for(int i=0; i<n; i++){
        const DictRelation*  pDictRel = aSelRelations.GetAt(i);
        int iLevelIdx   = GetHighestSelectedLevel(*pDictRel, rMapLevelIdxByRecord);
        CArray<const DictRelation*,const DictRelation*>* pRels = NULL;
        if(!aMapSelRelationsByLevel.Lookup( iLevelIdx, pRels )){
            pRels = new CArray<const DictRelation*,const DictRelation*>;
            aMapSelRelationsByLevel.SetAt( iLevelIdx, pRels );
        }
        pRels->Add( pDictRel );
    }
    //RELATIONS


    //for each level that can have a proc :
    int iNumExportProcs = aLevelsWithExportProc.GetSize();
    for( int i=0; i<iNumExportProcs; i++){
        int iLevelIdx = aLevelsWithExportProc.GetAt(i);

        CStringArray* pExportFilesByLevel = NULL;
        aMapExportFileVarsByProc.Lookup( iLevelIdx, pExportFilesByLevel );
        if( !pExportFilesByLevel ){
            pExportFilesByLevel = new CStringArray();
            aMapExportFileVarsByProc.SetAt( iLevelIdx, pExportFilesByLevel );
        }

        if( bSingleFile && m_bAllInOneRecord){

            //just one export proc, in the latest selected level

            //ASSERT( iNumExportProcs==1 ); valid only when Num selected relations == 0
            ASSERT( aExportFileVars.GetSize()==1 );
            //iLevelIdx must be equal to the last selected level


            pExportFilesByLevel->Add( aExportFileVars.GetAt(0) );

        } else {

            //multiple files =>   number of export procs == number of selected levels
            //                    and each proc will have only the selected records
            //                    plus selected relations for the associated level

            //retrieve selected records, for the level iLevelIdx :
            CArray<const CDictRecord*,const CDictRecord*> aSelRecords;
            DumpSelRecords( iLevelIdx, aMap_SelectedIdItems_by_LevelIdx,    aSelRecords );
            DumpSelRecords( iLevelIdx, aMap_SelectedNonIdItems_by_LevelIdx, aSelRecords );

            int iNumSelRecs = aSelRecords.GetSize();
            for(int iSelRecIdx=0; iSelRecIdx<iNumSelRecs; iSelRecIdx++)
                pExportFilesByLevel->Add( GetFileVar( aSelRecords.GetAt(iSelRecIdx), aMapExportFileVarByRecord ) );

            //RELATIONS
            CArray<const DictRelation*,const DictRelation*>* pSelRelations = NULL;
            aMapSelRelationsByLevel.Lookup( iLevelIdx, pSelRelations );
            int iNumSelRels = pSelRelations ? pSelRelations->GetSize() : 0;
            for(int iSelRelIdx=0; iSelRelIdx<iNumSelRels; iSelRelIdx++)
                pExportFilesByLevel->Add( GetFileVar( pSelRelations->GetAt(iSelRelIdx), aMapExportFileVarByRelation ) );
            //RELATIONS


        }


        //avoid any duplicated file var
        CMapStringToString  map;
        CStringArray        arr;
        int iNumExportFileVars = pExportFilesByLevel->GetSize();
        for(int iExportFileVar = 0;  iExportFileVar<iNumExportFileVars; iExportFileVar++){
            const CString& rcsFileVar = pExportFilesByLevel->ElementAt(iExportFileVar);
            if( map[rcsFileVar].IsEmpty() ){
                arr.Add( rcsFileVar );
                map[rcsFileVar] = rcsFileVar;
            }
        }
        pExportFilesByLevel->RemoveAll();
        pExportFilesByLevel->Append( arr );



    }


    //RELATIONS
    int iLevel;
    CArray<const DictRelation*,const DictRelation*>* pRels;
    POSITION pos = aMapSelRelationsByLevel.GetStartPosition();
    while(pos){
        aMapSelRelationsByLevel.GetNextAssoc(pos,iLevel,pRels);
        delete( pRels );
    }
    //RELATIONS

}

CStringArray* GetExportFileVars( int iProcLevelIdx, CMap<int,int,CStringArray*,CStringArray*>& aMapExportFileVarsByProc ){
    CStringArray* paExportFileVars = NULL;
    aMapExportFileVarsByProc.Lookup( iProcLevelIdx, paExportFileVars );
    return paExportFileVars;
}



void InvertMap(CArray<const CDictRecord*,const CDictRecord*>& raSelRecords, CMap<const CDictRecord*,const CDictRecord*,CString,LPCTSTR>& aMapExportFileVarByRecord,
               CMap<CString,LPCTSTR,CArray<const CDictRecord*,const CDictRecord*>*,CArray<const CDictRecord*,const CDictRecord*>*>& aMapSelRecsByFileVar){

    int n = raSelRecords.GetSize();
    for(int i=0; i<n; i++){
        const CDictRecord* pDictRecord = raSelRecords.GetAt(i);

        //each dict record can belong to just one file
        CString csFileVar = GetFileVar( pDictRecord, aMapExportFileVarByRecord );

        CArray<const CDictRecord*,const CDictRecord*>* paSelRecs = NULL;
        if(!aMapSelRecsByFileVar.Lookup( csFileVar, paSelRecs )){
            paSelRecs = new CArray<const CDictRecord*,const CDictRecord*>;
            aMapSelRecsByFileVar.SetAt( csFileVar, paSelRecs );
        }
        paSelRecs->Add(pDictRecord);

    }
}
void InvertMap(CArray<const DictRelation*,const DictRelation*>& raSelRelations,
               CMap<const DictRelation*,const DictRelation*,CString,LPCTSTR>& aMapExportFileVarByRelation,
               CMap<CString,LPCTSTR,CArray<const DictRelation*,const DictRelation*>*,CArray<const DictRelation*,const DictRelation*>*>& aMapSelRelsByFileVar){

    int n = raSelRelations.GetSize();
    for(int i=0; i<n; i++){
        const DictRelation* pDictRelation = raSelRelations.GetAt(i);

        CString csFileVar = GetFileVar( pDictRelation, aMapExportFileVarByRelation );

        CArray<const DictRelation*,const DictRelation*>* paSelRels = NULL;
        if(!aMapSelRelsByFileVar.Lookup( csFileVar, paSelRels )){
            paSelRels = new CArray<const DictRelation*,const DictRelation*>;
            aMapSelRelsByFileVar.SetAt( csFileVar, paSelRels );
        }
        paSelRels->Add(pDictRelation);

    }

}




void REC_TYPE(CString& rcsBuff, const CString* pcsRecType, int tabs){

    if(!pcsRecType)
        return;

    if( pcsRecType->IsEmpty() )
        return;

    if( !rcsBuff.IsEmpty() )
        rcsBuff += _T("\n");
    rcsBuff += GetTabs(tabs) + _T("REC_TYPE(\"") + *pcsRecType + _T("\")");


}

CString CExportDoc::ExportCmd(  CString&            rcsExportProc,
                    const CString&      rcsUniverse,
                    const CString&      rcsExportFileVar,
                    const CString&      rcsCaseIdItems,
                    const CString&      rcsExportList,
                    int&                tabs,
                    CMapStringToString& rMapUsedFiles,
                    ExportRecordType    exportRecordType,
                    const CString*      pcsRecType){

    if( rcsExportList.IsEmpty() )
        return _T("");

    CString csExportCmd;
    InitUniverse(csExportCmd, rcsUniverse, tabs);
    {
        EXPORT_TO   ( csExportCmd, rcsExportFileVar, tabs );

        if( exportRecordType==ExportRecordType::BeforeIds )
            REC_TYPE(csExportCmd, pcsRecType, tabs );

        CASE_ID( csExportCmd, rcsCaseIdItems,   tabs );

        if( exportRecordType==ExportRecordType::AfterIds )
            REC_TYPE(csExportCmd, pcsRecType, tabs);

        EXPORT_LIST ( csExportCmd, rcsExportList,   tabs );
    }
    EndUniverse(csExportCmd, rcsUniverse, tabs);

    if(!csExportCmd.IsEmpty()){
        rMapUsedFiles[ rcsExportFileVar ] = rcsExportFileVar;
        m_arrFileVars4Pff.Add(rcsExportFileVar);
        if( !rcsExportProc.IsEmpty() )
            rcsExportProc += _T("\n");
        rcsExportProc += csExportCmd;
    }

    return csExportCmd;
}


void CExportDoc::Relation(  const DictRelation& dict_relation,
                            const CString& rcsExportFileVar,
                            const CString& rcsCaseIdItems,
                            const CString& rcsRecType,
                            CMap<const CDictItem*,const CDictItem*,const CDictItem*,const CDictItem*>& rMapDictIdItems,
                            CMapStringToString& rMapUsedVars,
                            CMapStringToString& rMapUsedFiles,
                            CString& rcsExportProc,
                            int tabs,
                            CString* pcsSingleItemsPrefix )
{
    CArray<const CDictItem*,const CDictItem*> aSelItems;
    CMap<const CDictItem*,const CDictItem*,CArray<int,int>*,CArray<int,int>*> aMapSelOccsByItem;
    m_pTreeView->GetSelItemsByRelation(dict_relation, aSelItems, aMapSelOccsByItem);

    rcsExportProc += _T("\n");

    CString csExportList;
    if( pcsSingleItemsPrefix )
        csExportList = *pcsSingleItemsPrefix;
    bool bFlatExport = false;

    FillExportList( aSelItems, aMapSelOccsByItem, rMapDictIdItems, true, true, true, NULL, NULL, NULL, bFlatExport, csExportList );

    InitFor( rcsExportProc, _T("cspro_export_loop_var_i"), _T("RELATION"), dict_relation.GetName(), tabs, rMapUsedVars );
    {
        ExportCmd( rcsExportProc, m_csUniverse, rcsExportFileVar, rcsCaseIdItems, csExportList, tabs, rMapUsedFiles, m_exportRecordType, &rcsRecType );
    }
    EndFor( rcsExportProc, tabs );

    DeleteMapOccsByItem( aMapSelOccsByItem );
}

CString CExportDoc::GetExportProc(  int                                                                                             iLevelIdx,
                                    CStringArray*                                                                                   paExportFileVars,
                                    CMap<CString,LPCTSTR,CArray<const CDictRecord*,const CDictRecord*>*,CArray<const CDictRecord*,const CDictRecord*>*>&    rMapSelRecsByFile,
                                    CMap<const CDictRecord*,const CDictRecord*,CArray<const CDictItem*,const CDictItem*>*,CArray<const CDictItem*,const CDictItem*>*>&  rMapSelItemsByRecord,
                                                CMap<const CDictItem*,const CDictItem*,CArray<int,int>*,CArray<int,int>*>&                      rMapSelOccsByItem,
                                    CArray<CString,CString>&                                                                        raCaseIdsByLevelIdx,
                                    CMap<const CDictItem*,const CDictItem*,const CDictItem*,const CDictItem*>&                                              rMapDictIdItems,
                                    bool                                                                                            bSingleFile,
                                    bool                                                                                            bAllInOneRecord,
                                    ExportRecordType                                                                                exportRecordType,
                                    bool                                                                                            bJoinSingleMultiple,
                                    const CString&                                                                                  rcsUniverse,
                                    int                                                                                             tabs,
                                    CMapStringToString&                                                                             rMapUsedVars,
                                    CMapStringToString&                                                                             rMapUsedFiles,
                                    CArray<const CDictItem*,const CDictItem*>&                                                                  aSingleItemsPrefix,
                                    CMap<CString,LPCTSTR,CArray<const CDictRecord*,const CDictRecord*>*,CArray<const CDictRecord*,const CDictRecord*>*>&    aMapRecordsToUpLevel,
                                    int                                                                                             iPrefixLevel,

                                    //RELATIONS
                                    CMap<CString,LPCTSTR,CArray<const DictRelation*,const DictRelation*>*,CArray<const DictRelation*,const DictRelation*>*>& rMapSelRelsByFileVar,
                                    CMap<const DictRelation*,const DictRelation*,CString,LPCTSTR>&                                                           rMapRecTypeByRelation,
                                    CMap<const CDictRecord*,const CDictRecord*,int,int>&                                                                                 rMapLevelIdxByRecord,
                                    //RELATIONS

                                    CMap<const CDictRecord*,const CDictRecord*,const CDictRecord*,const CDictRecord*>& rMapJoinHelper)
{
    CString csExportProc;

    CString csCaseIdItems;
    if(raCaseIdsByLevelIdx.GetSize()>0){
        if( bSingleFile ){
            csCaseIdItems = raCaseIdsByLevelIdx.ElementAt( raCaseIdsByLevelIdx.GetSize()-1 );
        } else {

            for(int iLevel=iLevelIdx; iLevel>=0; iLevel--){

                if( iLevel < raCaseIdsByLevelIdx.GetSize() ){
                    csCaseIdItems = raCaseIdsByLevelIdx[iLevel];
                    break;
                }
            }
        }
    }

    int iNumExportFiles = paExportFileVars->GetSize();
    #ifdef _DEBUG
    for(int i=0; i<iNumExportFiles; i++)
        TRACE(_T("%s\n"), paExportFileVars->GetAt(i).GetString());
    #endif

    bool bExistMultiple = bJoinSingleMultiple;








    for( int iExportFileIdx=0; iExportFileIdx<iNumExportFiles; iExportFileIdx++){
        const CString& rcsExportFileVar = paExportFileVars->ElementAt(iExportFileIdx);

        //For the given file, retrieve all selected records : the map doesn't include any record selected in any relation
        CArray<const CDictRecord*,const CDictRecord*>* paSelRecs = NULL;
        rMapSelRecsByFile.Lookup( rcsExportFileVar, paSelRecs );



        CArray<const CDictRecord*,const CDictRecord*> aSelRecs;
        if( bAllInOneRecord && bSingleFile ){

            // allow each record from each level
            if(paSelRecs)
                aSelRecs.Append( *paSelRecs );
        } else {

            //allow only records that belong to the level
            int n = paSelRecs ? paSelRecs->GetSize() : 0;
            for(int i=0; i<n; i++){
                const CDictRecord* pSelRec = paSelRecs->GetAt(i);
                int iRecLevelIdx = -1;
                rMapLevelIdxByRecord.Lookup( pSelRec, iRecLevelIdx );

                if( iRecLevelIdx==iLevelIdx ){

                    if( iLevelIdx < iPrefixLevel ){
                        //aRecordsToUpLevel.Add( pSelRec );

                        CArray<const CDictRecord*,const CDictRecord*>* paRecordsToUpLevel = NULL;
                        aMapRecordsToUpLevel.Lookup( _T("")/*rcsExportFileVar*/, paRecordsToUpLevel );
                        if( !paRecordsToUpLevel ){
                            paRecordsToUpLevel = new CArray<const CDictRecord*,const CDictRecord*>;
                            aMapRecordsToUpLevel.SetAt( _T("") /*rcsExportFileVar*/, paRecordsToUpLevel );
                        }
                        paRecordsToUpLevel->Add( pSelRec );


                    } else {
                        aSelRecs.Add( pSelRec );
                    }
                }
            }
        }


        CArray<const CDictRecord*,const CDictRecord*>   aSelRecsAux;

        CArray<const CDictRecord*,const CDictRecord*>*  paRecordsToUpLevel = NULL;
        aMapRecordsToUpLevel.Lookup( _T("")/*rcsExportFileVar*/,  paRecordsToUpLevel );

        CArray<int,int> aUpRecsToRemove;

        //now, the loop will see the records inside aSelRecs plus the ones that are inside aRecordsToUpLevel
        int iNumRecordsToUpLevel = paRecordsToUpLevel ? paRecordsToUpLevel->GetSize() : 0;
        for(int i=0; i<iNumRecordsToUpLevel; i++){
            const CDictRecord* pRec = paRecordsToUpLevel->GetAt( i );
            int iRecLevelIdx = -1;
            if( rMapLevelIdxByRecord.Lookup( pRec, iRecLevelIdx ) ){

                if( iPrefixLevel!=-1 && iRecLevelIdx<iPrefixLevel && iPrefixLevel==iLevelIdx ){

                    ASSERT( iRecLevelIdx <= iLevelIdx );

                    aSelRecsAux.Add( pRec );
                    aUpRecsToRemove.Add( i );
                }
            }
        }
        for(int i=aUpRecsToRemove.GetSize()-1; i>=0; i--)
            paRecordsToUpLevel->RemoveAt( aUpRecsToRemove.GetAt(i) );


        aSelRecsAux.Append( aSelRecs );

        paSelRecs = &aSelRecsAux;


        if( bAllInOneRecord ){

            CString csExportList;
            FillExportList( paSelRecs, rMapSelItemsByRecord, rMapSelOccsByItem, rMapDictIdItems, true, true, true, NULL, NULL, NULL, false,
                            csExportList);

            CString csExportCmd;
            ExportCmd( csExportCmd, rcsUniverse, rcsExportFileVar, csCaseIdItems, csExportList, tabs, rMapUsedFiles, ExportRecordType::None, NULL );
            if( !csExportCmd.IsEmpty() ){
                if(!csExportProc.IsEmpty())
                    csExportProc += _T("\n\n");
                csExportProc += csExportCmd;
            }

        } else {

            //multiple elements will generate a For loop

            int iNumSelRecs = paSelRecs ? paSelRecs->GetSize() : 0;
            for(int iSelRecIdx=0; iSelRecIdx<iNumSelRecs; iSelRecIdx++){

                const CDictRecord* pSelRec = paSelRecs->GetAt(iSelRecIdx);
                CString csRecType = pSelRec->GetRecTypeVal(); //pSelRec->GetName();

                bool            bHasAnyMultipleItem = false;
                bool            bIsFlatExport       = IsFlatExport( pSelRec, &bHasAnyMultipleItem );

                ASSERT( !bIsFlatExport || bHasAnyMultipleItem );


                if( !bJoinSingleMultiple || !bHasAnyMultipleItem || bIsFlatExport ){

                    //export with all single items
                    CString csExportList;
                    if( pSelRec->GetMaxRecs()>1 )
                        csExportList = GetExportList( aSingleItemsPrefix,rMapSelOccsByItem,rMapDictIdItems,true, false, false, pSelRec, pSelRec->GetMaxRecs()>1 ? &m_csRecLoopIdx : NULL, NULL, bIsFlatExport );

                    //bIgnoreMultipleItems will be ignored : here all items are single (but the record can be multiple)
                    //items that are single, but the parent is multiple will be included


                    FillExportList( /*input*/pSelRec, rMapSelItemsByRecord, rMapSelOccsByItem, rMapDictIdItems, true, false, true, NULL, pSelRec->GetMaxRecs()>1 ? &m_csRecLoopIdx : NULL, NULL, bIsFlatExport,
                                    /*output*/csExportList );


                    if( pSelRec->GetMaxRecs()==1 && bJoinSingleMultiple  && !m_bJOIN_SingleMultiple_UseSingleAfterMultiple && !rMapJoinHelper.Lookup(pSelRec,pSelRec)   ){
                        //do nothing : single items will be included as a prefix for multiple elements : there are multiple elements after

                    } else if( pSelRec->GetMaxRecs()==1 && bJoinSingleMultiple  && m_bJOIN_SingleMultiple_UseSingleAfterMultiple && bExistMultiple ){
                        //it will be included by any multiple element

                    } else {

                    CString csExportCmd;
                    InitFor( csExportCmd, m_csRecLoopIdx, pSelRec, tabs, rMapUsedVars );
                    {
                        ExportCmd( csExportCmd, rcsUniverse, rcsExportFileVar, csCaseIdItems, csExportList, tabs, rMapUsedFiles, exportRecordType, &csRecType );
                    }
                    EndFor( csExportCmd, pSelRec, tabs );
                    if( !csExportCmd.IsEmpty() ){
                        //m_arrFileVars4Pff.Add(rcsExportFileVar);
                        if(!csExportProc.IsEmpty())
                            csExportProc += _T("\n\n");
                        csExportProc += csExportCmd;
                    }
                    }

                } else {
                    //join and exist some multiple item => do nothing : single items will be included in the export of each multiple item
                }



                //an EXPORT for each multiple item
                CArray<const CDictItem*,const CDictItem*>* pSelItems = NULL;
                rMapSelItemsByRecord.Lookup( pSelRec, pSelItems );

                int iNumSelItems = pSelItems ? pSelItems->GetSize() : 0;


                CString csExportList;
                for(int iSelItemIdx=0; iSelItemIdx<iNumSelItems; iSelItemIdx++){
                    const CDictItem* pSelItem = pSelItems->GetAt(iSelItemIdx);
                    bool bMultiple = pSelItem->GetOccurs()>1;

                    bMultiple = bMultiple || (pSelItem->GetItemType() == ItemType::Subitem && pSelItem->GetParentItem()->GetOccurs()>1 );

                    if(  !bMultiple  || bIsFlatExport){


                        if( !m_bJOIN_SingleMultiple_UseSingleAfterMultiple ){

                            if( bJoinSingleMultiple ){

                                if(!bMultiple){
                            aSingleItemsPrefix.Add( pSelItem );
                                }
                            }

                        } else {

                            //allready added by the pre scaning of single items
                        }

                        continue;
                    }

                    CArray<int,int>* pOccs = NULL;
                    rMapSelOccsByItem.Lookup( pSelItem, pOccs );

                    csExportList.Empty();
                    if( bJoinSingleMultiple )
                        csExportList = GetExportList( aSingleItemsPrefix,rMapSelOccsByItem,rMapDictIdItems,true,false,false, NULL, pSelRec->GetMaxRecs()>1 ? &m_csRecLoopIdx : NULL, pSelItem->GetOccurs()>1 ? &m_csMultItemLoopIdx : NULL, bIsFlatExport );


                    if(!csExportList.IsEmpty())
                        csExportList +=_T(", ");
                    csExportList += pSelItem->GetName();

                    CString csItemIdx = pSelItem->GetItemType() == ItemType::Subitem && pSelItem->GetOccurs()>1 ? m_csMultSubItemLoopIdx : m_csMultItemLoopIdx;

                    CString csExportCmd;
                    InitFor( csExportCmd, m_csRecLoopIdx, pSelRec, tabs, rMapUsedVars );
                    {
                        InitFor( csExportCmd, csItemIdx, pSelItem, tabs, rMapUsedVars );
                        {
                            InitIf( csExportCmd, csItemIdx, pOccs, tabs );
                            {
                                ExportCmd( csExportCmd, rcsUniverse, rcsExportFileVar, csCaseIdItems, csExportList, tabs, rMapUsedFiles, exportRecordType, &csRecType );
                            }
                            EndIf(csExportCmd, pOccs, tabs);
                        }
                        EndFor( csExportCmd, pSelItem, tabs );
                    }
                    EndFor(csExportCmd, pSelRec, tabs );
                    if( !csExportCmd.IsEmpty() ){
                        //m_arrFileVars4Pff.Add(rcsExportFileVar);
                        if(!csExportProc.IsEmpty())
                            csExportProc += _T("\n\n");
                        csExportProc += csExportCmd;
                    }
                }
            }
        }

        //RELATIONS INIT
        //For the given file, retrieve all selected relations
        CArray<const DictRelation*,const DictRelation*>* paSelRels = NULL;
        rMapSelRelsByFileVar.Lookup( rcsExportFileVar, paSelRels );
        int iNumSelRels = paSelRels ? paSelRels->GetSize() : 0;
        for(int iRelIdx=0; iRelIdx<iNumSelRels; iRelIdx++){

            const DictRelation* pDictRelation = paSelRels->GetAt(iRelIdx);

            if( GetHighestSelectedLevel(*pDictRelation, rMapLevelIdxByRecord ) !=iLevelIdx )
                continue;
            CString csSingleItemsPrefix;
            FillExportList( aSingleItemsPrefix, rMapSelOccsByItem, rMapDictIdItems, true, false, false, NULL, NULL, NULL, false,
                            csSingleItemsPrefix );

            CString csRecType;
            rMapRecTypeByRelation.Lookup( pDictRelation, csRecType );

            Relation(*pDictRelation, rcsExportFileVar, csCaseIdItems, csRecType, rMapDictIdItems, rMapUsedVars, rMapUsedFiles, csExportProc, tabs, &csSingleItemsPrefix );
        }
        //RELATIONS END

    }

    return csExportProc;
}

void FormatExportApp(CString& rcsExportApp){
    CString csNewApp;

    int iNumEmptyLines = 0;
    std::vector<std::wstring> aLines = SO::SplitString(rcsExportApp, '\n');

    for( size_t i = 0; i < aLines.size(); ++i ) {
        CString rcsLine = WS2CS(aLines[i]);

        if( !rcsLine.IsEmpty() ){
            iNumEmptyLines = 0;

            if( !csNewApp.IsEmpty() )
                csNewApp += _T("\n");
            csNewApp += rcsLine;
        } else {
            iNumEmptyLines++;
            if( iNumEmptyLines==1 ){
                csNewApp += _T("\n");
            }
        }
    }

    rcsExportApp = csNewApp;
}


CString NextDictRelationRecType(const CString& rcsPrevRecType, int iMaxRecTypeLen, bool& bContinue, int iMaxNum = -1){

    /*
    max rec type len    DictRelation Record Type
        0
        1               {r,R}
        2               {r0,...,r9,R0,...,R9}
        3               {r00,...,r99,R00,...,R99}
    */


    CString s;
    if( iMaxRecTypeLen>0 ){
        if( rcsPrevRecType.IsEmpty() ){
            s = _T("r");

        } else {

            if( iMaxNum == -1 ){
                CString csMaxNum;
                for(int i=0; i<iMaxRecTypeLen-1; i++)
                    csMaxNum += _T(" ");
                csMaxNum.Replace(_T(' '),'9');
                iMaxNum = _ttoi(csMaxNum);
            }

            TCHAR   r       = rcsPrevRecType[0]; //it can be 'r' or 'R'
            CString csNum   = rcsPrevRecType.Right( rcsPrevRecType.GetLength()-1 );
            int     i       = CIMSAString::IsNumeric(csNum) ? _ttoi(csNum) : -1;

            int     i_new = 0;
            TCHAR   r_new = 0;

            //try to advance number
            if( i<iMaxNum ){

                i_new   = i+1;
                r_new   = r;

            //try to advance letter
            } else if( r=='r' ){

                i_new   = 0;
                r_new   = _T('R');

            //can't continue
            } else {
                bContinue = false;
            }


            if( bContinue ){
                CString csFmt;
                csFmt.Format(_T("%s0%d%s"),_T("\\%s\\%"),iMaxRecTypeLen-1,_T("d"));

                s.Format(csFmt, CString(r_new).GetString(), i_new);

                TRACE(_T("%s\n"), s.GetString());
            }
        }

    } else {

        //can't continue
        bContinue = false;
    }

    return s;
}

bool CExportDoc::FillRecTypeByRelation( /*input*/CMapStringToString& rMapUsedRecTypes,
                                       /*output*/CMap<const DictRelation*, const DictRelation*,CString,LPCTSTR>& rMapRecTypeByRelation )
{
    //for each DictRelation we will generate a new record type.

    /*
    max rec type len    DictRelation Record Type
        0
        1               {r,R}
        2               {r0,...,r9,R0,...,R9}
        3               {r00,...,r99,R00,...,R99}
    */


    bool bOk = true;

    if( m_exportRecordType != ExportRecordType::None ){

        int iRecTypeLen     =  m_pDataDict ? m_pDataDict->GetRecTypeLen() : 0;

        CString csMaxNum;
        for(int i=0; i<iRecTypeLen-1; i++)
            csMaxNum += _T(" ");
        csMaxNum.Replace(_T(' '),'9');
        int iMaxNum = _ttoi(csMaxNum);

        CString         csRecType;
        bool            bCanContinue = true;

        CArray<CString,CString> aDuplicatedRecTypes;
        CArray<const DictRelation*,const DictRelation*> aDictRelationsWithDupRecTypes;

        if (m_pDataDict) {
            for( const DictRelation& dict_relation : m_pDataDict->GetRelations() ) {
                csRecType   = NextDictRelationRecType( csRecType, iRecTypeLen, bCanContinue, iMaxNum );

                //generated rec type for the given dict relation can't be inside rMapUsedRecTypes
                if( rMapUsedRecTypes.Lookup( csRecType, csRecType ) ){
                    aDictRelationsWithDupRecTypes.Add(&dict_relation);
                    aDuplicatedRecTypes.Add( csRecType );
                }

                // pDictRel => csRecType
                rMapRecTypeByRelation.SetAt(&dict_relation, csRecType );

                if (!bCanContinue) {
                    break;
                }
            }
        }

        int n = aDuplicatedRecTypes.GetSize();
        if( n>0 ){
            CString csRecTypes;
            CString csDictRels;
            for(int i=0; i<n; i++){

                if(!csRecTypes.IsEmpty())
                    csRecTypes += _T(", ");
                csRecTypes += aDuplicatedRecTypes.GetAt(i);

                if(!csDictRels.IsEmpty())
                    csDictRels += _T(", ");
                csDictRels += aDictRelationsWithDupRecTypes.GetAt(i)->GetName() + _T(" (") + aDuplicatedRecTypes.GetAt(i) + _T(")");
            }

            CString csMsg;

            csMsg +=  _T("Can't export relation");
            if(n>1)
                csMsg += _T("s");
            csMsg += _T(" ") + csDictRels + _T(" because of record types conflicts");

            AfxMessageBox(csMsg);

            bOk = false;

        }
    }




    return bOk;
}

int CExportDoc::PreScanSingleItemsPrefix(   CMap<const CDictItem*,const CDictItem*,const CDictItem*,const CDictItem*>&  rMapDictIdItems,
                                            CMap<const CDictRecord*,const CDictRecord*,int,int>&            rMapLevelIdxByRecord,
                                            CArray<const CDictItem*,const CDictItem*>&                      aSingleItemsPrefix )
{


    if(!m_pTreeView)
        return -1;

    CMap<const CDictItem*,const CDictItem*,const CDictItem*,const CDictItem*> aMapDictItems;

    CMap<const CDictRecord*,const CDictRecord*,bool,bool> aMapFlatExport;


    CArray<const CDictItem*,const CDictItem*> aSelItems;
    CMap<const CDictItem*,const CDictItem*,CArray<int,int>*,CArray<int,int>*>   aMapSelOccsByItem;
    int iNumSelItems = m_pTreeView->GetSelItems( false, aSelItems, aMapSelOccsByItem );
    for(int iSelItemIdx=0; iSelItemIdx<iNumSelItems; iSelItemIdx++){
        const CDictItem* pSelItem = aSelItems.GetAt(iSelItemIdx);
        const CDictRecord* pSelRec = pSelItem->GetRecord();

        //each child of a multiple record is a multiple element : must be excluded
        if( pSelRec->GetMaxRecs()>1 )
            continue;

        bool bFlatExport;
        if( !aMapFlatExport.Lookup( pSelRec, bFlatExport ) ){
            bFlatExport = IsFlatExport( pSelRec, NULL );
            aMapFlatExport.SetAt( pSelRec, bFlatExport);
        }

        //if the record match the flat export criteria,
        //each of its child items can be considerer a join
        //prefix, even when they are multiple.

        //remember that the flat export can be applied only
        //when !m_bAllInOneRecord

        if( m_bAllInOneRecord || !bFlatExport ){

        //exclude each multiple item
        if( pSelItem->GetOccurs()>1 )
            continue;

        }


        //exclude each item that is an id
        if( IsIdItem( pSelItem, rMapDictIdItems ) )
            continue;

        if( !aMapDictItems.Lookup( pSelItem, pSelItem ) ){
            aSingleItemsPrefix.Add( pSelItem );
            aMapDictItems.SetAt( pSelItem, pSelItem );
        }
    }


    const CDictItem* pItem;
    CArray<int,int>* pOccs;
    POSITION pos = aMapSelOccsByItem.GetStartPosition();
    while(pos){
        aMapSelOccsByItem.GetNextAssoc(pos,pItem,pOccs);
        if( pOccs )
            delete( pOccs );
    }

    int     iPrefixLevel = GetHighestLevel( aSingleItemsPrefix, rMapLevelIdxByRecord );
    return  iPrefixLevel;



}

void FillUsedRecTypes(CArray<const CDictRecord*,const CDictRecord*>& rSelRecords, CMapStringToString& rMapUsedRecTypes ){

    int iNumRecords = rSelRecords.GetSize();
    for(int iRecIdx=0; iRecIdx<iNumRecords; iRecIdx++){
        const CDictRecord* pDictRecord = rSelRecords.GetAt( iRecIdx );
        //rMapUsedRecTypes.SetAt( pDictRecord->GetName(), pDictRecord->GetName() );
        rMapUsedRecTypes.SetAt( pDictRecord->GetRecTypeVal(), pDictRecord->GetRecTypeVal() );
    }
}



void CExportDoc::SyncBuff_app(){

    m_arrFileVars4Pff.RemoveAll();
    m_aMapUsedUniqueNames.RemoveAll();
    if( m_pDataDict ){

        m_csRecLoopIdx          = _T("rec_occ");
        m_csMultItemLoopIdx     = _T("item_occ");
        m_csMultSubItemLoopIdx  = _T("subitem_occ");

        CheckUniqueName( m_csRecLoopIdx         );
        CheckUniqueName( m_csMultItemLoopIdx    );
        CheckUniqueName( m_csMultSubItemLoopIdx );

    } else {
        //there is no problem, because of the buffer will be empty
    }



    ExportRecordType exportRecordTypeAux = m_exportRecordType;
    if( m_bAllInOneRecord ){
        exportRecordTypeAux = ExportRecordType::None;
    }




    bBehaviorDone   = false;

    CString* pcsExportFileName      = m_bmerge ? (!m_csExportFileName.IsEmpty()    ? &m_csExportFileName     : NULL ) : NULL;
    CString* pcsExportFilesFolder   = !m_bmerge ? (!m_csExportFilesFolder.IsEmpty() ? &m_csExportFilesFolder : NULL ) : NULL;
    CString* pcsExportFilesPrefix   = !m_bmerge ? (!m_csExportFilesPrefix.IsEmpty() ? &m_csExportFilesPrefix : NULL ) : NULL;


    CString csExportApp;

    //Ask for all needed info in just one call
    CArray<const CDictItem*,const CDictItem*>                                                                   aSelItems;
    CMap<const CDictItem*,const CDictItem*,CArray<int,int>*,CArray<int,int>*>                                   aMapSelOccsByItem;
    CMap<int,int,CArray<const CDictItem*,const CDictItem*>*,CArray<const CDictItem*,const CDictItem*>*>                     aMap_SelectedIdItems_by_LevelIdx;
    CMap<int,int,CArray<const CDictItem*,const CDictItem*>*,CArray<const CDictItem*,const CDictItem*>*>                     aMap_SelectedNonIdItems_by_LevelIdx;
    CMap<const CDictItem*,const CDictItem*,const CDictItem*,const CDictItem*>                                               aMapDictIdItems;
    CMap<const CDictItem*,const CDictItem*,int,int>                                                             aMapItemIdxByItem;
    CMap<const CDictRecord*,const CDictRecord*,int,int>                                                         aMapLevelIdxByRecord;
    CMap<const CDictRecord*,const CDictRecord*,bool,bool>                                                       aMapIsIdBySelRec;
    CMap<const CDictRecord*,const CDictRecord*,CArray<const CDictItem*,const CDictItem*>*,CArray<const CDictItem*,const CDictItem*>*>   aMapSelItemsByRecord;
    CArray<const CDictRecord*,const CDictRecord*>                                                               aSelIdRecords;
    CArray<const CDictRecord*,const CDictRecord*>                                                               aSelNonIdRecords;
    CArray<const CDictRecord*,const CDictRecord*>                                                               aSelRecords;
    CArray<const DictRelation*,const DictRelation*>                                                 aSelRelations;
    GetSelectedItems( &aSelItems, &aMapSelOccsByItem, &aMapItemIdxByItem, &aMap_SelectedIdItems_by_LevelIdx, &aMap_SelectedNonIdItems_by_LevelIdx, &aMapDictIdItems, &aMapLevelIdxByRecord, &aMapIsIdBySelRec, &aMapSelItemsByRecord, &aSelIdRecords, &aSelNonIdRecords, &aSelRecords, &aSelRelations );

    CMapStringToString aMapUsedRecTypes;
    FillUsedRecTypes( aSelRecords, aMapUsedRecTypes);




    //Prepare CASE_ID params -> accumulated by levels
    CArray<CString,CString> aCaseIdsByLevelIdx;
    CString                 csCaseIds;
    FillCaseIds( /*input*/  aSelIdRecords, aMapSelItemsByRecord, m_bmerge, aMapLevelIdxByRecord,
                 /*output*/ &aCaseIdsByLevelIdx, &csCaseIds );

    CArray<CString,CString>                         aExportFiles;
    CArray<CString,CString>                         aExportFileVars;
    CMap<const CDictRecord*,const CDictRecord*,CString,LPCTSTR> aMapExportFileVarByRecord;

    //RELATIONS
    CMap<const DictRelation*,const DictRelation*,CString,LPCTSTR> aMapExportFileVarByRelation;

    FillExportFiles( /*input*/  m_pDataDict.get(), aMapIsIdBySelRec, aSelItems, aSelRecords, aSelRelations, m_bmerge, pcsExportFileName, pcsExportFilesFolder, pcsExportFilesPrefix, m_convmethod,
                     /*output*/ aExportFiles, aExportFileVars, aMapExportFileVarByRecord, aMapExportFileVarByRelation );

    CMap<const DictRelation*, const DictRelation*, CString, LPCTSTR> aMapRecTypeByRelation;
    if(!FillRecTypeByRelation( /*input*/aMapUsedRecTypes, /*output*/aMapRecTypeByRelation))
        return;

    m_aExportFiles.RemoveAll();
    m_aExportFiles.Append( aExportFiles );
    m_aExportFileVars.RemoveAll();
    m_aExportFileVars.Append( aExportFileVars );
    if(m_bmerge && aExportFiles.GetSize()>0)
        this->m_csExportFileName = aExportFiles[0];

    m_csExportApp.Empty();

    //Pre scan all single items
    CArray<const CDictItem*,const CDictItem*> aSingleItemsPrefix;
    int iPrefixLevel = -1;
    if( m_bJoinSingleWithMultipleRecords && m_bJOIN_SingleMultiple_UseSingleAfterMultiple){
        iPrefixLevel = PreScanSingleItemsPrefix(aMapDictIdItems, aMapLevelIdxByRecord, aSingleItemsPrefix);
    }


    //Pre scan to know wich levels can have an export proc
    CArray<int,int> aLevelsWithExportProc;

    FillProcs( m_pDataDict.get(), m_bmerge, aMap_SelectedIdItems_by_LevelIdx, aMap_SelectedNonIdItems_by_LevelIdx, aSelRelations,aMapLevelIdxByRecord, aSingleItemsPrefix,
               aLevelsWithExportProc );




    if( aLevelsWithExportProc.GetSize()>0 || aSelRelations.GetSize()>0 ){

        m_tabs=0;
        Append( _T("PROC GLOBAL") );

        Append( NUMERIC_VARS_DECLARATION_MARK );
        Append( FILES_DECLARATION_MARK   );


        CMap<int,int,CStringArray*,CStringArray*> aMapExportFileVarsByProc;
        FillMapExportFilesByProc(   /*input*/m_bmerge, aLevelsWithExportProc, aExportFileVars, aMap_SelectedIdItems_by_LevelIdx,aMap_SelectedNonIdItems_by_LevelIdx,aMapExportFileVarByRecord,  /*RELAIONS*/ aMapExportFileVarByRelation, aSelRelations, aMapLevelIdxByRecord/*RELATIONS*/,
                                    /*output*/aMapExportFileVarsByProc);


        //InvertMap is needed because of GetExportProc needs to know wich are the selected records for each file that is used in the export proc
        CMap<CString,LPCTSTR,CArray<const CDictRecord*,const CDictRecord*>*,CArray<const CDictRecord*,const CDictRecord*>*> aMapSelRecsByFileVar;
        InvertMap( /*input*/aSelRecords, aMapExportFileVarByRecord, /*output*/aMapSelRecsByFileVar);


        CMap<CString,LPCTSTR,CArray<const DictRelation*,const DictRelation*>*,CArray<const DictRelation*,const DictRelation*>*> aMapSelRelsByFileVar;
        InvertMap( /*input*/aSelRelations, aMapExportFileVarByRelation, /*output*/aMapSelRelsByFileVar );


        //if aMapJoinHelper.Lookup(p,p) then after p we will not found any other record single without multiple items =>
        //it will not be included by any other multiple element when join is active and !m_bJOIN_SingleMultiple_UseSingleAfterMultiple.
        CMap<const CDictRecord*,const CDictRecord*,const CDictRecord*,const CDictRecord*> aMapJoinHelper;
        int n = aSelRecords.GetSize();
        for(int i=n-1; i>=0; i--){
            const CDictRecord* pSelRec = aSelRecords.GetAt(i);
            if( pSelRec->GetMaxRecs()==1 && !HasAnyMultipleItem(pSelRec) ){
                aMapJoinHelper.SetAt( pSelRec, pSelRec );
            } else {
                break;
            }
        }



        int iNumWritedProcs = 0;

        //Main export application loop

        CMap<CString,LPCTSTR,CArray<const CDictRecord*,const CDictRecord*>*,CArray<const CDictRecord*,const CDictRecord*>*> aMapRecordsToUpLevel;

        m_tabs=0;
        CMapStringToString  aMapUsedNumericVars;
        m_aMapUsedFiles.RemoveAll();
        int iNumExportProcs = aLevelsWithExportProc.GetSize();
        for( int i=0; i<iNumExportProcs; i++){
            int         iProcLevelIdx   = aLevelsWithExportProc.GetAt(i);

            bool bNeedPreProc = true;
            CString csProcLevel;
            if(!bBehaviorDone){
                PreProc(&csProcLevel );
                bNeedPreProc = false;
                m_tabs=1;
                {
                    SetBehavior(&csProcLevel);
                    //Append( SETFILES_MARK, &csProcLevel );
                }
                m_tabs=0;
            }

            CString csExportProc = GetExportProc(   iProcLevelIdx,
                                                    GetExportFileVars( iProcLevelIdx, aMapExportFileVarsByProc ),
                                                    aMapSelRecsByFileVar,
                                                    aMapSelItemsByRecord,
                                                    aMapSelOccsByItem,
                                                    aCaseIdsByLevelIdx,
                                                    aMapDictIdItems,
                                                    m_bmerge,
                                                    m_bAllInOneRecord,
                                                    m_exportRecordType,
                                                    m_bJoinSingleWithMultipleRecords,
                                                    m_csUniverse,
                                                    1,
                                                    aMapUsedNumericVars,
                                                    m_aMapUsedFiles,
                                                    aSingleItemsPrefix,
                                                    aMapRecordsToUpLevel,
                                                    iPrefixLevel,

                                                    //RELATIONS
                                                    aMapSelRelsByFileVar,
                                                    aMapRecTypeByRelation,
                                                    aMapLevelIdxByRecord,
                                                    //RELATIONS

                                                    aMapJoinHelper );
            if( !csExportProc.IsEmpty() ){
                //PostProc(&csProcLevel);
                if(bNeedPreProc)
                    PreProc(&csProcLevel);
                Append(csExportProc, &csProcLevel );
            }

            if(!csProcLevel.IsEmpty() ){
                m_csExportApp += _T("\n");
                PROC_LEVEL( m_pDataDict->GetLevel( iProcLevelIdx ));
                Append( csProcLevel );
                iNumWritedProcs++;
            }

        }

        CString f;
        CArray<const CDictRecord*,const CDictRecord*>* pArr;
        POSITION pos = aMapRecordsToUpLevel.GetStartPosition();
        while(pos){
            aMapRecordsToUpLevel.GetNextAssoc(pos, f, pArr );
            if( pArr )
                delete( pArr );
        }


        //replace NUMERIC_VARS_DECLARATION_MARK with used variables
        DeclareNumericVars( aMapUsedNumericVars );

        //replace FILES_DECLARATION_MARK with used files declaration
        DeclareFileVars(aExportFileVars, m_aMapUsedFiles );


        //free mem
        CString csFileVar;
        CArray<const CDictRecord*,const CDictRecord*>* pSelRecs;
         pos = aMapSelRecsByFileVar.GetStartPosition();
        while(pos){
            aMapSelRecsByFileVar.GetNextAssoc(pos,csFileVar,pSelRecs);
            if(pSelRecs)
                delete( pSelRecs );
        }

        //RELATIONS_INIT
        CArray<const DictRelation*,const DictRelation*>* pSelRels;
        pos = aMapSelRelsByFileVar.GetStartPosition();
        while(pos){
            aMapSelRelsByFileVar.GetNextAssoc(pos,csFileVar,pSelRels);
            if(pSelRels)
                delete(pSelRels);
        }
        //RELATIONS_END

        int iProcLevel;
        CStringArray* pFileVars;
        pos = aMapExportFileVarsByProc.GetStartPosition();
        while(pos){
            aMapExportFileVarsByProc.GetNextAssoc(pos,iProcLevel,pFileVars);
            if( pFileVars )
                delete( pFileVars );
        }



    }

    FormatExportApp( m_csExportApp );

    //Just free memory
    const CDictRecord* pDictRecord;
    CArray<const CDictItem*,const CDictItem*>* pSelItems;
    POSITION pos = aMapSelItemsByRecord.GetStartPosition();
    while(pos){
        aMapSelItemsByRecord.GetNextAssoc(pos,pDictRecord,pSelItems);
        if(pSelItems)
            delete(pSelItems);
    }
    int iLevelIdx;
    pos = aMap_SelectedIdItems_by_LevelIdx.GetStartPosition();
    while(pos){
        aMap_SelectedIdItems_by_LevelIdx.GetNextAssoc(pos,iLevelIdx,pSelItems );
        if( pSelItems )
            delete(pSelItems);
    }
    pos = aMap_SelectedNonIdItems_by_LevelIdx.GetStartPosition();
    while(pos){
        aMap_SelectedNonIdItems_by_LevelIdx.GetNextAssoc(pos,iLevelIdx,pSelItems );
        if( pSelItems )
            delete(pSelItems);
    }
    pos = aMapSelOccsByItem.GetStartPosition();
    CArray<int,int>* pOccs;
    const CDictItem* pDictItem;
    while(pos){
        aMapSelOccsByItem.GetNextAssoc(pos,pDictItem,pOccs);
        if( pOccs )
            delete( pOccs );
    }

    m_exportRecordType = exportRecordTypeAux;
}

bool CExportDoc::IsSelectedAnyMultiple(bool bIncludeRelations ){

    bool bIgnoreChildsOfSingleRecord  = false;
    if( bBehavior_SingleRecordsFlat )
        bIgnoreChildsOfSingleRecord = true;

    return m_pTreeView ? m_pTreeView->IsSelectedAnyMultiple( bIncludeRelations, bIgnoreChildsOfSingleRecord ) : false;
}

bool CExportDoc::IsSelectedAnySingle(bool bIncludeRelations ){
    return m_pTreeView ? m_pTreeView->IsSelectedAnySingle( bIncludeRelations ) : false;
}


//it is 00 because of in the future could appears more particular cases
bool CExportDoc::CheckSpecialFilter00(){

    bool bCheckOk = true;

    CArray<const CDictItem*,const CDictItem*> aSelItems;
    CMap<const CDictItem*,const CDictItem*,CArray<int,int>*,CArray<int,int>*> aMapSelOccsByItem;
    if( m_pTreeView ){
        m_pTreeView->GetSelItems( true, aSelItems, aMapSelOccsByItem );

    }

    //the idea is : if every selected item belong to the same record, and the record is single, then can't enable join.
    const CDictRecord* pDictRecord            = NULL;
    bool         bAllItemsAreBrothers   = true;
    int n = aSelItems.GetSize();
    for(int i=0; i<n; i++){
        if( !pDictRecord ){
            pDictRecord = aSelItems.GetAt(i)->GetRecord();

        } else {
            if( pDictRecord != aSelItems.GetAt(i)->GetRecord() ){
                bAllItemsAreBrothers = false;
                break;
            }
        }
    }
    if( n>0 && bAllItemsAreBrothers && pDictRecord && pDictRecord->GetMaxRecs()==1 )
        bCheckOk = false;

    return bCheckOk;
}

bool CExportDoc::CanEnableJoin( bool bAllInOneRecord ){

    bool bCanEnableJoin = true;

    //join only in "As Separate Records"
    if(bAllInOneRecord)
        return false;

    //at least one single selected
    if(!IsSelectedAnySingle(false))
        return false;

    //at least one multiple selected
    if(!IsSelectedAnyMultiple(true))
        return false;

    //take care about the exceptions to the rules...
    if(!CheckSpecialFilter00())
        return false;

    return bCanEnableJoin;
}
void CExportDoc::Checks(){

    //Check #1 : Items may be selected from relations only when "As separate records" is selected
    ASSERT( m_pTreeView );
    if( m_pTreeView )
        m_pTreeView->CheckRelationsOnlyInSepRecs();

    //Check #2 : As separate records + CsPro export + more than one selected record => record type may have to be coded
    if(!m_bAllInOneRecord &&  m_bmerge && m_convmethod==METHOD::CSPRO && m_exportRecordType==ExportRecordType::None){

        CArray<const CDictItem*,const CDictItem*>               aSelItems;
        CMap<const CDictRecord*,const CDictRecord*,bool,bool>   aMapIsIdBySelRec;
        CArray<const CDictRecord*,const CDictRecord*>           aSelRecords;
        GetSelectedItems( &aSelItems, NULL, NULL, NULL, NULL, NULL, NULL, &aMapIsIdBySelRec, NULL, NULL, NULL, &aSelRecords );

        // Check if rec type needed      BMD 18 Mar 2005
        bool bNeedRecType = false;
        int i = 0;
        int n = 0;
        for (i = 0 ; i < aSelRecords.GetSize() ; i++) {
            const CDictRecord* pRec = aSelRecords.GetAt(i);
            if (m_bJoinSingleWithMultipleRecords) {
                if (pRec->GetName().GetAt(0) != _T('_') && pRec->GetMaxRecs() > 1) {
                    n++;
                }
            }
            else {
                if (pRec->GetName().GetAt(0) != '_') {
                    n++;
                }
            }
            if (n > 1) {
                bNeedRecType = true;
            }
        }
        if (bNeedRecType) {
            AfxMessageBox(_T("You must include a record type, either before or after case ids"));

            //propose "before" to correct the problem
            m_exportRecordType = ExportRecordType::BeforeIds;

            if(m_pOptionsView)
                m_pOptionsView->FromDoc(this);

            SyncBuff_app();
        }
    }

    //Check #3 : join single-multiple only enabled if at least one single and one multiple are selected
    if( m_pOptionsView ){

        m_pOptionsView->EnableJoinSingleMultiple( CanEnableJoin(m_bAllInOneRecord) );
    }

    //Check #4 : Delimited Export + One File + As Separate Record => only on record type output         BMD 18 Mar 2005
    if ((m_convmethod==METHOD::TABS || m_convmethod==METHOD::COMMADEL || m_convmethod==METHOD::SEMI_COLON) &&
                    m_bmerge && !m_bAllInOneRecord) {

        CArray<const CDictItem*,const CDictItem*>               aSelItems;
        CMap<const CDictRecord*,const CDictRecord*,bool,bool>   aMapIsIdBySelRec;
        CArray<const CDictRecord*,const CDictRecord*>           aSelRecords;
        GetSelectedItems( &aSelItems, NULL, NULL, NULL, NULL, NULL, NULL, &aMapIsIdBySelRec, NULL, NULL, NULL, &aSelRecords );

        // Check if OK
        bool bOK = true;
        int i = 0;
        int n = 0;
        for (i = 0 ; i < aSelRecords.GetSize() ; i++) {
            const CDictRecord* pRec = aSelRecords.GetAt(i);
            if (m_bJoinSingleWithMultipleRecords) {
                if (pRec->GetName().GetAt(0) != _T('_') && pRec->GetMaxRecs() > 1) {
                    n++;
                }
            }
            else {
                if (pRec->GetName().GetAt(0) != '_') {
                    n++;
                }
            }
            if (n > 1) {
                bOK = false;
            }
        }
        if (!bOK) {
            if (m_bJoinSingleWithMultipleRecords) {
                AfxMessageBox(_T("Only the items from the first multiple record will be exported."));
            }
            else {
                AfxMessageBox(_T("Only the items from the first record will be exported."));
            }
        }
    }

    //Check #5 : CsPro export => must have ids specified            // BMD 18 Mar 2005
    if(m_convmethod==METHOD::CSPRO) {

        // 20131220 if an .exf file was opened in CSPro export mode, this got called and would
        // return an error before the tree had even been set up
        if( IsWindow(m_pTreeView->m_dicttree) && m_pTreeView->m_dicttree.GetRootItem() != NULL )
        {
            CArray<const CDictItem*,const CDictItem*>               aSelItems;
            CMap<const CDictRecord*,const CDictRecord*,bool,bool>   aMapIsIdBySelRec;
            CArray<const CDictRecord*,const CDictRecord*>           aSelRecords;
            GetSelectedItems( &aSelItems, NULL, NULL, NULL, NULL, NULL, NULL, &aMapIsIdBySelRec, NULL, NULL, NULL, &aSelRecords );

            // Check if case ids chosen
            bool bCaseIds = false;
            int i = 0;
            for (i = 0 ; i < aSelRecords.GetSize() ; i++) {
                const CDictRecord* pRec = aSelRecords.GetAt(i);
                if (pRec->GetName().GetAt(0) == '_') {
                    bCaseIds = true;
                    break;
                }
            }
            if (!bCaseIds) {
                AfxMessageBox(_T("CSPro export requires at least one case id to be chosen."));
            }
        }
    }
}


/////////////////////////////////////////////////////////////////////////////////
//
//  bool CExportDoc::ProcessRun()
//
/////////////////////////////////////////////////////////////////////////////////
void CExportDoc::ProcessRun()
{
    //FABN
    CArray<const DictRelation*, const DictRelation*> aSelRelations;

    SyncBuff_app();
    GetSelectedItems(NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,&aSelRelations);

    if ((GetNumExpRecTypes() + /*FABN*/aSelRelations.GetSize()/*FABN*/) <= 0){
        AfxMessageBox(_T("Please select any non ID-Field to Continue"), MB_ICONEXCLAMATION);
        return;
    }
    /*if(!DeleteOutPutFiles()){
        return;
    }*/

    if(!CompileApp()){
        if(!m_csUniverse.IsEmpty()){
            AfxMessageBox(_T("Invalid Universe"));
        }
        else {
            AfxMessageBox(_T("Internal Error"));
        }
        return;
    }

    SaveSettingsToRegistry(); // 20130703

    if (!m_batchmode) {

        int m_iCurrentRunConvMethod = (int)m_convmethod;

        // if the run method was changed, clear out the output filenames
        if( ( m_iPreviousRunConvMethod >= 0 ) && ( m_iCurrentRunConvMethod != m_iPreviousRunConvMethod ) )
            m_PifFile.ClearExportFilenames();

        m_iPreviousRunConvMethod = m_iCurrentRunConvMethod;
    }


    bool bCSproAsMulti= (m_convmethod== METHOD::CSPRO   && !m_bmerge);
    bool bSPSSorSASorStata = (m_convmethod == METHOD::SPSS || m_convmethod == METHOD::SAS || m_convmethod == METHOD::STATA
                || m_convmethod == METHOD::ALLTYPES || m_convmethod== METHOD::CSPRO || m_convmethod == METHOD::R);

    if (m_convmethod == METHOD::TABS || m_convmethod == METHOD::COMMADEL || m_convmethod == METHOD::SEMI_COLON || bCSproAsMulti ){
        bSPSSorSASorStata =false;
        // log file creation
        SAFE_DELETE(m_pLogFile);
        if (m_PifFile.GetListingFName().IsEmpty()){
            CString sPath;
            sPath = m_csDictFileName;
            PathRemoveFileSpec(sPath.GetBuffer(MAX_PATH));
            sPath.ReleaseBuffer();
            sPath.TrimRight('\\');

            m_PifFile.SetListingFName(sPath + _T("\\CSExpRun.lst"));
        }

        m_csLogFile = m_PifFile.GetListingFName();

        m_pLogFile = new CStdioFileUnicode(); // 20111228 for unicode
        if (!m_pLogFile->Open(m_csLogFile,CFile::modeCreate|CFile::modeWrite))
        {
            m_csErrorMessage.Format(_T("Cannot open: %s\n"), m_csLogFile.GetString());
            AfxMessageBox(m_csErrorMessage, MB_ICONEXCLAMATION);
            return;
        }
        //  Initialize log file
        switch (m_convmethod){
        case METHOD::TABS:
            m_pLogFile->WriteString(_T("Export Format: Tab Delimited\n"));
            break;
        case METHOD::COMMADEL:
            m_pLogFile->WriteString(_T("Export Format: Comma Delimited\n"));
            break;
        }
        if (m_bmerge){
            CWaitCursor cursor;
            m_pLogFile->WriteString(_T("Files Created: One file\n\n"));
            if(m_pLogFile){
                m_pLogFile->Close();
                SAFE_DELETE(m_pLogFile);
            }

            SingleFileModel4NoDataDef();
        }
        else{
            m_pLogFile->WriteString(_T("Files Created: One file per record type\n\n"));
            //ExecuteTab();
            MFilesModel2();
        }
        //m_pLogFile->Close();

    }
    else if(bSPSSorSASorStata){
        if (m_bmerge){
            if(m_pLogFile){
                m_pLogFile->Close();
                SAFE_DELETE(m_pLogFile);
            }
            SingleFileModel();
        }
        else
        {
            MFilesModel2();
        }
    }

}

void CExportDoc::MFilesModel2()
{
    //Start Preparing the data files
    CString sPath;
    sPath = m_csDictFileName;
    PathRemoveFileSpec(sPath.GetBuffer(MAX_PATH));
    sPath.ReleaseBuffer();
    sPath.TrimRight('\\');

    if(!ExecuteFileInfo2()){
        if(m_pLogFile) {
            m_pLogFile->Close();
            SAFE_DELETE(m_pLogFile);
        }
        m_PifFile.SetAppFName(sOldAppName);
        sOldAppName =_T("");
        return;
    }
    //End Preparing the data files

    if(!GenerateBatchApp4MultiModel2()){
        AfxMessageBox(_T("Failed to generate Export app"));
    }
    else {//launch the stuff after piff file gen

        //make the order spec name ;
        // m_sBCHPFFName =sPath+_T("\\") + _T("CSExpRun.pff");
        m_sBCHPFFName =m_sBaseFilename + FileExtensions::WithDot::Pff;
        sArray.RemoveAll();
        for( auto filename : m_PifFile.GetExportFilenames() )
            sArray.Add(filename);
        m_PifFile.ClearExportFilenames();
        m_csSPSSOutFile=_T("");
        m_csSPSSDescFile = _T("");
        m_csCSProDCFFile =_T("");
        m_PifFile.SetCSPROSyntaxFName(_T(""));
        SaveBatchPffAdjustingOnExit(&m_PifFile);
        LaunchBatchApp();
    }
    m_PifFile.SetAppFName(sOldAppName);
    sOldAppName =_T("");
}

bool CExportDoc::ExecuteFileInfo2()
{
    bool bRet = true;

    SyncBuff_app();

    //Start Preparing the data files
    CString sPath;
    // 20131220 base the export files off the pff location, if available
    sPath = m_PifFile.GetAppFName().IsEmpty() ? m_csDictFileName : m_PifFile.GetAppFName();
    PathRemoveFileSpec(sPath.GetBuffer(MAX_PATH));
    sPath.ReleaseBuffer();
    sPath.TrimRight('\\');
    //CString sAplFile = sPath+_T("\\")+_T("CSExpRun.bch");
    CString sAplFile = m_sBaseFilename +FileExtensions::WithDot::BatchApplication;

    CString sListFile;
    if (m_PifFile.GetListingFName().IsEmpty()){
        sListFile = sPath+_T("\\")+_T("CSExpRun.lst");
    }
    else{
        sListFile = m_PifFile.GetListingFName();
    }

    sOldAppName = m_PifFile.GetAppFName();
    m_PifFile.SetAppFName(sAplFile); //Now set the bch file name
    m_PifFile.SetListingFName(sListFile);
    m_csLogFile = m_PifFile.GetListingFName();

    ((CExportApp *)AfxGetApp())->DeletePifInfos();

    if( !GetInputDataFilenames() )
        return false;

    CString pzTempdir = GetDirectoryForOutputs();

    CArray<PIFINFO*,PIFINFO*>& arrPifInfo = ((CExportApp *)AfxGetApp())->m_arrPifInfo;
    GetNumExpRecTypes();

    CMapStringToString mapUniqNames;
    for (int ifile = 0; ifile < m_arrFileVars4Pff.GetSize(); ifile++)
    {
        CString sExt = GetDefaultExportFileExt( m_convmethod, NULL, NULL );

        CString sFileVarName = m_arrFileVars4Pff[ifile];

        CString filename = m_PifFile.LookUpUsrDatFile(sFileVarName);

        CString sOriginalPrefix;
        CString sPrefix = _T("file_");
        int iPrefix = sPrefix.GetLength();
        sOriginalPrefix =sFileVarName.Left(iPrefix);

        if(!(sOriginalPrefix.CompareNoCase(sPrefix) ==0) ){
            continue;
        }
        sFileVarName = sFileVarName.Right(sFileVarName.GetLength() - iPrefix);

        if( filename.IsEmpty() )
            filename = pzTempdir + sFileVarName+ sExt;

        CString sVal;
        if(mapUniqNames.Lookup(sFileVarName,sVal)){
            continue;//name already added
        }
        else {
            mapUniqNames.SetAt(sFileVarName,sFileVarName);
        }
    //  dlgbrowse.AddFile(filename);
        PIFINFO * pinfo = new PIFINFO;
        pinfo->eType = FILE_NONE;
        pinfo->sFileName = filename;
        pinfo->sDisplay = pinfo->sUName = sFileVarName;
        pinfo->uOptions = PIF_ALLOW_BLANK;
        arrPifInfo.Add(pinfo);

        // JH 11/17/06 - track which index in pff this file var goes to, need this later
        // to update associations
        mapFileVarToPifIndex[m_arrFileVars4Pff[ifile]] = arrPifInfo.GetSize() - 1;

    }

    if (!m_batchmode){
        CExportApp* pApp = (CExportApp*)AfxGetApp();
        CPifDlg pifDlg(pApp->m_arrPifInfo, _T("Specify Names of Exported Files"));
        pifDlg.m_pPifFile = &m_PifFile;
        if(pifDlg.DoModal() != IDOK) {
            ((CExportApp *)AfxGetApp())->DeletePifInfos();
            return false ;
        }

        // 20100625 bug trevor reported on 6/18; PFF file shouldn't have this specified if using
        // multiple files, but it would be specified if they previously did a single file export
        m_PifFile.SetSPSSSyntaxFName(_T(""));
        m_PifFile.SetSTATADOFName(_T(""));
        m_PifFile.SetSTATASyntaxFName(_T(""));
        m_PifFile.SetRSyntaxFName(_T(""));

        m_PifFile.ClearExportFilenames();
        m_PifFile.ClearUserFilesMap();
        for (int ifile = 0; ifile < arrPifInfo.GetSize(); ifile++){
            m_PifFile.AddExportFilenames(((CExportApp *)AfxGetApp())->m_arrPifInfo[ifile]->sFileName);
        }
    }

    if( m_pLogFile )
        m_pLogFile->Close();

    SAFE_DELETE(m_pLogFile);

    m_pLogFile = new CStdioFileUnicode(); // 20111228 for unicode
    if (!m_csLogFile.IsEmpty() && !m_pLogFile->Open(m_csLogFile,CFile::modeCreate|CFile::modeWrite)){
        m_csErrorMessage.Format(_T("Cannot open: %s\n"), m_csLogFile.GetString());
        AfxMessageBox(m_csErrorMessage, MB_ICONEXCLAMATION);
        return false;
    }

    for( const ConnectionString& connection_string : m_PifFile.GetInputDataConnectionStrings() )
        m_pLogFile->WriteString(FormatText(_T("   Input File: %s\n"), connection_string.GetFilename().c_str()));

    m_pLogFile->Close();
    SAFE_DELETE(m_pLogFile);

    //End File Collection

    //Begin Serpo code update
    //Fill the items properly from  the output file names array

    // JH 11/17/06 Fixed multi-file export crash, use mapFileVarToPifIndex instead of
    // m_arrFileVars4Pff which didn't have one-to-one mapping with pif
    for (int iIndex = 0; iIndex < m_aExportFileVars.GetSize(); ++iIndex) {
        int iPifIndex;
        if (mapFileVarToPifIndex.Lookup(m_aExportFileVars.GetAt(iIndex), iPifIndex)) {
            CString sOriginal = m_aExportFiles.GetAt(iIndex);
            if( iPifIndex < (int)m_PifFile.GetExportFilenames().size() )
            {
                CString sReplace = m_PifFile.GetExportFilenames()[iPifIndex];
                m_csExportApp.Replace(sOriginal , sReplace);
                m_PifFile.SetUsrDatAssoc(m_aExportFileVars.ElementAt(iIndex),sReplace);
            }
        }
    }

    if( !CheckInInputOutputFilenamesAreDifferent() )
        return false;

    //End  Serpro code update
    return bRet;
}


/////////////////////////////////////////////////////////////////////////////////
//
//  bool CExportDoc::GenerateBatchApp4MultiModel2()
//
/////////////////////////////////////////////////////////////////////////////////
bool CExportDoc::GenerateBatchApp4MultiModel2()
{
    Application batchApp;
    batchApp.SetEngineAppType(EngineAppType::Batch);
    batchApp.SetLogicSettings(m_logicSettings);

    CString sPath;
    // 20131220 base the export files off the pff location, if available
    sPath = m_PifFile.GetAppFName().IsEmpty() ? m_csDictFileName : m_PifFile.GetAppFName();
    PathRemoveFileSpec(sPath.GetBuffer(MAX_PATH));
    sPath.ReleaseBuffer();
    sPath.TrimRight('\\');

    //make the order spec name ;
    /*CString sFullFileName = sPath+_T("\\") + _T("CSExpRun.bch");
    CString sOrderFile = sPath+_T("\\")+_T("CSExpRun.ord");*/
    CString sFullFileName = m_sBaseFilename + FileExtensions::WithDot::BatchApplication;
    CString sOrderFile = m_sBaseFilename + FileExtensions::WithDot::Order;


    batchApp.AddFormFilename(sOrderFile);
    //Create the .ord file and save it
    CFileStatus fStatus;
    BOOL bOrderExists = CFile::GetStatus(sOrderFile,fStatus);
    if(bOrderExists){
        BOOL bDel = DeleteFile(sOrderFile);
        if(!bDel){
            CString sMsg;
            sMsg = _T("Cannot Delete ") + sOrderFile;
            AfxMessageBox(sMsg);
            return false;
        }
        else {
            bOrderExists  =false;
        }
    }
    if(!bOrderExists) {

        ASSERT(!m_csDictFileName.IsEmpty());
        CDEFormFile Order(sOrderFile,m_csDictFileName);

        //Create the .ord file and save it
        Order.CreateOrderFile(*m_pDataDict, true);
        Order.Save(sOrderFile);
    }

    batchApp.SetLabel(PortableFunctions::PathGetFilenameWithoutExtension<CString>(sFullFileName));

//Update Serpo App Code

    if(!WriteDefaultFiles4MultiModel2(&batchApp,sFullFileName)){
        return false;
    }

    try
    {
        batchApp.Save(sFullFileName);
    }

    catch( const CSProException& )
    {
        return false;
    }

    return true;
}
/////////////////////////////////////////////////////////////////////////////////
//
//  bool CExportDoc::WriteDefaultFiles4MultiModel(Application* pApplication, const CString &sAppFName)
//
/////////////////////////////////////////////////////////////////////////////////
bool CExportDoc::WriteDefaultFiles4MultiModel2(Application* pApplication, const CString &sAppFName)
{
    bool bDone = false;
    //AppFile
    CString sAppSCodeFName(sAppFName);
    PathRemoveExtension(sAppSCodeFName.GetBuffer(_MAX_PATH));
    sAppSCodeFName.ReleaseBuffer();
    sAppSCodeFName += FileExtensions::WithDot::Logic;

    CFileStatus fStatus;
    BOOL bRet = CFile::GetStatus(sAppSCodeFName,fStatus);
    if(bRet) {
        CString sMsg;
        BOOL bDel = DeleteFile(sAppSCodeFName);
        if(!bDel) {
            sMsg = _T("Cannot Delete ") + sAppSCodeFName;
            AfxMessageBox(sMsg);
            return bDone;
        }
        else {
            bRet = false;
        }
    }
    if(!bRet){
        //Create the .app file
        CSpecFile appFile(TRUE);
        appFile.Open(sAppSCodeFName,CFile::modeWrite);
        appFile.WriteString(m_logicSettings.GetDefaultFirstLineForTextSource(pApplication->GetLabel(), AppFileType::Code));

        //Start Serpro App code generation
        SyncBuff_app();

        for( const wstring_view line_sv : SO::SplitString<wstring_view>(m_csExportApp, '\n') ) {
            appFile.WriteLine(SO::Trim(line_sv));
        }

        appFile.Close();

        //End Serpro App Code generation

    }

    pApplication->AddCodeFile(CodeFile(CodeType::LogicMain, std::make_shared<TextSource>(CS2WS(sAppSCodeFName))));

    //Help File
    if(pApplication->GetEngineAppType() != EngineAppType::Batch) {
        CString sHelpFName(sAppFName);
        PathRemoveExtension(sHelpFName.GetBuffer(_MAX_PATH));
        sHelpFName.ReleaseBuffer();
        sHelpFName += FileExtensions::WithDot::QuestionText;

        bRet = CFile::GetStatus(sHelpFName,fStatus);
        if(!bRet){
            //Create the .qsf file
            CSpecFile appFile(TRUE);
            appFile.Open(sHelpFName,CFile::modeWrite);
            appFile.WriteString(m_logicSettings.GetDefaultFirstLineForTextSource(pApplication->GetLabel(), AppFileType::Code));
            appFile.Close();
        }
        pApplication->SetQuestionTextFilename(sHelpFName);
    }
    return true;
}

////////////////////////////////////////////////////////////////////
//
//                  void CExportDoc::SingleFileModel()
//
////////////////////////////////////////////////////////////////////

void CExportDoc::SingleFileModel()
{
    m_bmerge = true;

// To Be done..?
    // 1 Create Batch Application
    ////////////////////////////////   GenerateBatchApp
    // 2 Run Batch Application
    // 3 open the listing file
    // 4 Do Not run Export Viewer
    m_csSPSSDescFile = _T("");
    m_csSTATADescFile = _T("");
    m_csSTATALabelFile = _T("");
    m_csSASDescFile = _T("");
    m_csCSProDCFFile =_T("");
    m_csRDescFile =_T("");

    // the output description filenames are saved and retrieved from one run to the next, so do the same with the output
    // data filename (by loading NPFF::m_exportFiles with the filename, which will be picked up later)
    if( m_PifFile.GetExportFilenames().empty() && !m_csSPSSOutFile.IsEmpty() )
        m_PifFile.AddExportFilenames(m_csSPSSOutFile);

    CString sPath;
    // 20131220 base the export files off the pff location, if available
    sPath = m_PifFile.GetAppFName().IsEmpty() ? GetDirectoryForOutputs() : m_PifFile.GetAppFName();
    PathRemoveFileSpec(sPath.GetBuffer(MAX_PATH));
    sPath.ReleaseBuffer();
    sPath.TrimRight('\\');
    //CString sAplFile = sPath+_T("\\")+_T("CSExpRun.bch");
    CString sAplFile = m_sBaseFilename +FileExtensions::WithDot::BatchApplication;

    CString sListFile;
    if (m_PifFile.GetListingFName().IsEmpty())
        sListFile = sPath+_T("\\")+_T("CSExpRun.lst");
    else
        sListFile = m_PifFile.GetListingFName();

    CString csDataFile;
    m_csSTATALabelFile = _T("");
    switch (m_convmethod)
    {
        case METHOD::TABS:
        case METHOD::SEMI_COLON:
            m_csSPSSOutFile = sPath + _T("\\") + _T("Exported.txt");
            break;
        case METHOD::COMMADEL:
            m_csSPSSOutFile = sPath + _T("\\") + _T("Exported.csv");
            break;
        case METHOD::SPSS :
            m_csSPSSOutFile = sPath + _T("\\") + _T("Exported.dat");
            m_csSPSSDescFile = sPath + _T("\\") + _T("Exported.sps");
            break;
        case METHOD::SAS:
            m_csSPSSOutFile = sPath + _T("\\") + _T("Exported.dat");
            m_csSASDescFile = sPath + _T("\\") + _T("Exported.sas");
            break;
        case METHOD::CSPRO:
            m_csSPSSOutFile = sPath + _T("\\") + _T("Exported.dat");
            m_csCSProDCFFile = sPath + _T("\\") + _T("Exported.dcf");
            break;
        case METHOD::STATA:
            m_csSPSSOutFile = sPath + _T("\\") + _T("Exported.dat");
            m_csSTATADescFile = sPath + _T("\\") + _T("Exported.dct");
            m_csSTATALabelFile = sPath + _T("\\") + _T("Exported.do");
            break;
        case METHOD::R:
            m_csSPSSOutFile = sPath + _T("\\") + _T("Exported.dat");
            m_csRDescFile = sPath + _T("\\") + _T("Exported.R");
            break;
        case METHOD::ALLTYPES:
            m_csSPSSOutFile = sPath + _T("\\") + _T("Exported.dat");
            m_csSPSSDescFile = sPath + _T("\\") + _T("Exported.sps");
            m_csSASDescFile = sPath + _T("\\") + _T("Exported.sas");
            m_csSTATADescFile = sPath + _T("\\") + _T("Exported.dct");
            m_csSTATALabelFile = sPath + _T("\\") + _T("Exported.do");
            m_csRDescFile = sPath + _T("\\") + _T("Exported.R");
            break;
    }

    if (!m_PifFile.GetExportFilenames().empty())
        m_csSPSSOutFile = m_PifFile.GetExportFilenames().front();

    m_PifFile.ClearExportFilenames();

    switch (m_convmethod)
    {
    case METHOD::SPSS :
        if (!m_PifFile.GetSPSSSyntaxFName().IsEmpty())
            m_csSPSSDescFile = m_PifFile.GetSPSSSyntaxFName();
        break;
    case METHOD::SAS:
        if (!m_PifFile.GetSASSyntaxFName().IsEmpty())
            m_csSASDescFile = m_PifFile.GetSASSyntaxFName();
        break;
    case METHOD::CSPRO:
        if (!m_PifFile.GetCSPROSyntaxFName().IsEmpty())
            m_csCSProDCFFile = m_PifFile.GetCSPROSyntaxFName();
        break;
    case METHOD::STATA:
        if (!m_PifFile.GetSTATASyntaxFName().IsEmpty())
            m_csSTATADescFile = m_PifFile.GetSTATASyntaxFName();
        if (!m_PifFile.GetSTATADOFName().IsEmpty())
            m_csSTATALabelFile = m_PifFile.GetSTATADOFName();
        break;
    case METHOD::R:
        if (!m_PifFile.GetRSyntaxFName().IsEmpty())
            m_csRDescFile = m_PifFile.GetRSyntaxFName();
        break;
    case METHOD::ALLTYPES:
        if (!m_PifFile.GetSPSSSyntaxFName().IsEmpty())
            m_csSPSSDescFile = m_PifFile.GetSPSSSyntaxFName();
        if (!m_PifFile.GetSASSyntaxFName().IsEmpty())
            m_csSASDescFile = m_PifFile.GetSASSyntaxFName();
        if (!m_PifFile.GetSTATASyntaxFName().IsEmpty())
            m_csSTATADescFile = m_PifFile.GetSTATASyntaxFName();
        if (!m_PifFile.GetSTATADOFName().IsEmpty())
            m_csSTATALabelFile = m_PifFile.GetSTATADOFName();
        if (!m_PifFile.GetRSyntaxFName().IsEmpty())
            m_csRDescFile = m_PifFile.GetRSyntaxFName();
        break;
    }

    if( !GetInputDataFilenames() )
        return;

    ((CExportApp *)AfxGetApp())->DeletePifInfos();

    CArray<PIFINFO*,PIFINFO*>& arrPifInfo = ((CExportApp *)AfxGetApp())->m_arrPifInfo;

    PIFINFO * pinfo = new PIFINFO;
    pinfo->eType = FILE_NONE;
    pinfo->sFileName = m_csSPSSOutFile;
    pinfo->sDisplay = pinfo->sUName = _T("Output data file name");
    pinfo->uOptions = PIF_ALLOW_BLANK;
    arrPifInfo.Add(pinfo);

    if (!m_csSPSSDescFile.IsEmpty())
    {
        pinfo = new PIFINFO;
        pinfo->eType = FILE_NONE;
        pinfo->sFileName = m_csSPSSDescFile;
        pinfo->sDisplay = pinfo->sUName = _T("SPSS Description file name") ;
        pinfo->uOptions = PIF_ALLOW_BLANK;
        arrPifInfo.Add(pinfo);
    }
    if (!m_csCSProDCFFile.IsEmpty())
    {
        pinfo = new PIFINFO;
        pinfo->eType = FILE_NONE;
        pinfo->sFileName = m_csCSProDCFFile;
        pinfo->sDisplay = pinfo->sUName = _T("CSpro data dictionary file name") ;
        pinfo->uOptions = PIF_ALLOW_BLANK;
        arrPifInfo.Add(pinfo);
    }
    if (!m_csSASDescFile.IsEmpty())
    {
        pinfo = new PIFINFO;
        pinfo->eType = FILE_NONE;
        pinfo->sFileName = m_csSASDescFile;
        pinfo->sDisplay = pinfo->sUName = _T("SAS Description file name") ;
        pinfo->uOptions = PIF_ALLOW_BLANK;
        arrPifInfo.Add(pinfo);
    }
    if (!m_csSTATADescFile.IsEmpty())
    {
        pinfo = new PIFINFO;
        pinfo->eType = FILE_NONE;
        pinfo->sFileName = m_csSTATADescFile;
        pinfo->sDisplay = pinfo->sUName = _T("Stata Description file name") ;
        pinfo->uOptions = PIF_ALLOW_BLANK;
        arrPifInfo.Add(pinfo);
    }
    if (!m_csSTATALabelFile.IsEmpty())
    {
        pinfo = new PIFINFO;
        pinfo->eType = FILE_NONE;
        pinfo->sFileName = m_csSTATALabelFile;
        pinfo->sDisplay = pinfo->sUName = _T("Stata Labels file name") ;
        pinfo->uOptions = PIF_ALLOW_BLANK;
        arrPifInfo.Add(pinfo);
    }
    if (!m_csRDescFile.IsEmpty())
    {
        pinfo = new PIFINFO;
        pinfo->eType = FILE_NONE;
        pinfo->sFileName = m_csRDescFile;
        pinfo->sDisplay = pinfo->sUName = _T("R Description file name") ;
        pinfo->uOptions = PIF_ALLOW_BLANK;
        arrPifInfo.Add(pinfo);
    }

    if (!m_batchmode)
    {
        CExportApp* pApp = (CExportApp*)AfxGetApp();
        CPifDlg pifDlg(pApp->m_arrPifInfo, _T("Specify Names of Exported Files"));
        pifDlg.m_pPifFile = &m_PifFile;
        if(pifDlg.DoModal() != IDOK) {
            //((CExportApp *)AfxGetApp())->DeletePifInfos();
            return;
        }
        m_csSPSSOutFile = arrPifInfo[0]->sFileName;
        Pff_SetFirstExportFilename(&m_PifFile, m_csSPSSOutFile);
        switch (m_convmethod)
        {
        case METHOD::SPSS:
            m_csSPSSDescFile = arrPifInfo[1]->sFileName;
            m_PifFile.SetSPSSSyntaxFName(m_csSPSSDescFile);
            break;
        case METHOD::SAS:
            m_csSASDescFile = arrPifInfo[1]->sFileName;
            m_PifFile.SetSASSyntaxFName(m_csSASDescFile);
            break;
        case METHOD::CSPRO:
            m_csCSProDCFFile = arrPifInfo[1]->sFileName;
            m_PifFile.SetCSPROSyntaxFName(m_csCSProDCFFile);
            break;
        case METHOD::STATA:{
                m_csSTATADescFile = arrPifInfo[1]->sFileName;
                m_csSTATALabelFile = arrPifInfo[2]->sFileName;
                CString sExt = PortableFunctions::PathGetFileExtension<CString>(m_csSTATADescFile);
                if(sExt.CompareNoCase(FileExtensions::StataDictionary) != 0){
                    m_csSTATADescFile += FileExtensions::WithDot::StataDictionary;
                }
                m_PifFile.SetSTATASyntaxFName(m_csSTATADescFile);
                m_PifFile.SetSTATADOFName(m_csSTATALabelFile);
            }
            break;
        case METHOD::R:
            m_csRDescFile = arrPifInfo[1]->sFileName;
            m_PifFile.SetRSyntaxFName(m_csRDescFile);
            break;
        case METHOD::ALLTYPES:
            {
            m_csSPSSDescFile = arrPifInfo[1]->sFileName;
            m_csSASDescFile = arrPifInfo[2]->sFileName;
            m_csSTATADescFile = arrPifInfo[3]->sFileName;
            CString sExt = PortableFunctions::PathGetFileExtension<CString>(m_csSTATADescFile);
            if(sExt.CompareNoCase(FileExtensions::StataDictionary) != 0){
                m_csSTATADescFile += FileExtensions::WithDot::StataDictionary;
            }
            m_PifFile.SetSPSSSyntaxFName(m_csSPSSDescFile);
            m_PifFile.SetSASSyntaxFName(m_csSASDescFile);
            m_PifFile.SetSTATASyntaxFName(m_csSTATADescFile);
            m_csSTATALabelFile = arrPifInfo[4]->sFileName;
            m_PifFile.SetSTATADOFName(m_csSTATALabelFile);
            m_csRDescFile = arrPifInfo[5]->sFileName;
            m_PifFile.SetRSyntaxFName(m_csRDescFile);
            }
            break;
        }
    }

    ((CExportApp *)AfxGetApp())->DeletePifInfos();

    //Fix the association
    int iExportedFileIdx =0;
    CString rcsExportedFile = m_aExportFiles.ElementAt(iExportedFileIdx);
    CString rcsExportFileVar = m_aExportFileVars.ElementAt(iExportedFileIdx);
    {
        bool bFound = true;
        while( m_aMapUsedFiles[rcsExportFileVar].IsEmpty()){//We are using this to eliminate the id record .'cos file is not used for ID record
            iExportedFileIdx++;
            if(iExportedFileIdx >= m_aExportFileVars.GetSize()){
                //We reached the end and still did not find
                bFound = false;
                break;
            }
            rcsExportFileVar    = m_aExportFileVars.ElementAt(iExportedFileIdx);

        }
        if(bFound) {
            ASSERT(iExportedFileIdx <= m_aExportFiles.GetSize());
            CString sOriginal = m_aExportFiles.GetAt(iExportedFileIdx);
            m_csExportApp.Replace(sOriginal , m_csSPSSOutFile);

            iExportedFileIdx++;
            if(iExportedFileIdx < m_aExportFileVars.GetSize()){
                rcsExportFileVar = m_aExportFileVars.ElementAt(iExportedFileIdx);
            }
        }
    }

    //Update data
    iExportedFileIdx =0;
    rcsExportedFile = m_aExportFiles.ElementAt(iExportedFileIdx);
    rcsExportFileVar    = m_aExportFileVars.ElementAt(iExportedFileIdx);

    {
        bool bFound = true;
        while( m_aMapUsedFiles[rcsExportFileVar].IsEmpty()){//We are using this to eliminate the id record .'cos file is not used for ID record
            iExportedFileIdx++;
            if(iExportedFileIdx >= m_aExportFileVars.GetSize()){
                //We reached the end and still did not find
                bFound = false;
                break;
            }
            rcsExportFileVar    = m_aExportFileVars.ElementAt(iExportedFileIdx);

        }
        ASSERT(iExportedFileIdx <= m_aExportFiles.GetSize());
        if(bFound){
            m_aExportFiles[iExportedFileIdx] = m_csSPSSOutFile;
            iExportedFileIdx++;
            if(iExportedFileIdx < m_aExportFileVars.GetSize()){
                rcsExportFileVar = m_aExportFileVars.ElementAt(iExportedFileIdx);
            }
        }
    }


    //End of the data file to "FILE" association

    if(!GenerateBatchApp4MultiModel2()){
        AfxMessageBox(_T("Failed to generate Export app"));
    }
    else {//launch the stuff after piff file gen

        //make the order spec name ;
        //m_sBCHPFFName =sPath+_T("\\") + _T("CSExpRun.pff");
        m_sBCHPFFName =m_sBaseFilename + FileExtensions::WithDot::Pff;

        DeleteFile(sListFile);
    //    CNPifFile pifFile(m_sBCHPFFName);
        CNPifFile& pifFile = m_PifFile;
        sOldAppName = pifFile.GetAppFName();
        pifFile.SetAppFName(sAplFile);
        pifFile.SetListingFName(sListFile);
        Pff_SetFirstExportFilename(&pifFile, m_csSPSSOutFile);
        pifFile.SetSPSSSyntaxFName(m_csSPSSDescFile);
        pifFile.SetSASSyntaxFName(m_csSASDescFile);
        if(!m_csSTATADescFile.IsEmpty()){
            CString sExt = PortableFunctions::PathGetFileExtension<CString>(m_csSTATADescFile);
            if(sExt.CompareNoCase(FileExtensions::StataDictionary) != 0){
                m_csSTATADescFile += FileExtensions::WithDot::StataDictionary;
            }
        }
        pifFile.SetSTATASyntaxFName(m_csSTATADescFile);
        pifFile.SetSTATADOFName(m_csSTATALabelFile);
        pifFile.SetCSPROSyntaxFName(m_csCSProDCFFile); // 20140520 added, as it should have been here before
        pifFile.SetRSyntaxFName(m_csRDescFile);
        SaveBatchPffAdjustingOnExit(&pifFile);
        LaunchBatchApp();
        pifFile.SetAppFName(sOldAppName);
    }
    if (!m_PifFile.GetAppFName().IsEmpty()) {
        m_sPFFName = m_PifFile.GetAppFName() + FileExtensions::WithDot::Pff;
        if (m_bPostRunSave)
        {
            m_PifFile.SetPifFileName(m_sPFFName);
            m_PifFile.Save();
        }
    }
}
////////////////////////////////////////////////////////////////////
//
//                  void CExportDoc::SingleFileModel4NoDataDef()
//  FOR : TXT : SEMICOLON : CSV
//
////////////////////////////////////////////////////////////////////

void CExportDoc::SingleFileModel4NoDataDef()
{
    m_bmerge = true;

// To Be done..?
    // 1 Create Batch Application
    ////////////////////////////////   GenerateBatchApp
    // 2 Run Batch Application
    // 3 open the listing file
    // 4 Do Not run Export Viewer
    m_csSPSSDescFile = _T("");
    m_csSTATADescFile = _T("");
    m_csSTATALabelFile = _T("");
    m_csSASDescFile = _T("");
    m_csCSProDCFFile =_T("");

    if( !GetInputDataFilenames() )
        return;

    CString csExt;
    CString csFilter;
    if (m_convmethod == METHOD::COMMADEL)
    {
        csFilter = _T("Output Data File (*.csv)|*.*|");
        csExt = FileExtensions::CSV;
    }
    else
    {
        csFilter = _T("Output Data File (*.txt)|*.*|");
        csExt = _T("txt");
    }

    CString filename = GetDirectoryForOutputs() + _T("Exported.") + csExt;
    if (!m_PifFile.GetExportFilenames().empty()) {
        filename = m_PifFile.GetExportFilenames().front();
    }

    if (!m_batchmode) {
        SetCurrentDirectory(AfxGetApp()->GetProfileString(EXPORT_CMD_Settings, _T("Last Data Folder")));
        CIMSAFileDialog dlg1 (FALSE, NULL, filename, OFN_HIDEREADONLY, csFilter, AfxGetApp()->GetMainWnd(), CFD_NO_DIR);
        dlg1.m_ofn.lpstrTitle = _T("Specify Name of Exported File");

        if( dlg1.DoModal() != IDOK )
            return;

        filename = dlg1.GetPathName();

        Pff_SetFirstExportFilename(&m_PifFile, filename);
    }

    if( !CheckInInputOutputFilenamesAreDifferent() )
        return;

    CString sPath;
    // 20131220 base the export files off the pff location, if available
    sPath = m_PifFile.GetAppFName().IsEmpty() ? m_csDictFileName : m_PifFile.GetAppFName();
    PathRemoveFileSpec(sPath.GetBuffer(MAX_PATH));
    sPath.ReleaseBuffer();
    sPath.TrimRight('\\');
    //CString sAplFile = sPath+_T("\\")+_T("CSExpRun.bch");
    CString sAplFile = m_sBaseFilename + FileExtensions::WithDot::BatchApplication;
    CString sListFile ;
    if (m_PifFile.GetListingFName().IsEmpty())
        sListFile = sPath+_T("\\")+_T("CSExpRun.lst");
    else
        sListFile = m_PifFile.GetListingFName();


    CString csDataFile;

    //Fix the association
    int iExportedFileIdx =0;
    CString rcsExportedFile = m_aExportFiles.ElementAt(iExportedFileIdx);
    CString rcsExportFileVar = m_aExportFileVars.ElementAt(iExportedFileIdx);
    {
        bool bFound = true;
        while( m_aMapUsedFiles[rcsExportFileVar].IsEmpty()){//We are using this to eliminate the id record .'cos file is not used for ID record
            iExportedFileIdx++;
            if(iExportedFileIdx >= m_aExportFileVars.GetSize()){
                //We reached the end and still did not find
                bFound = false;
                break;
            }
            rcsExportFileVar    = m_aExportFileVars.ElementAt(iExportedFileIdx);


        }
        if(bFound) {
            ASSERT(iExportedFileIdx <= m_aExportFiles.GetSize());
            CString sOriginal = m_aExportFiles.GetAt(iExportedFileIdx);
            m_csExportApp.Replace(sOriginal , filename);

            iExportedFileIdx++;
            if(iExportedFileIdx < m_aExportFileVars.GetSize()){
                rcsExportFileVar = m_aExportFileVars.ElementAt(iExportedFileIdx);
            }
        }
    }

    //Update data
    iExportedFileIdx =0;
    rcsExportedFile = m_aExportFiles.ElementAt(iExportedFileIdx);
    rcsExportFileVar= m_aExportFileVars.ElementAt(iExportedFileIdx);

    {
        bool bFound = true;
        while( m_aMapUsedFiles[rcsExportFileVar].IsEmpty()){//We are using this to eliminate the id record .'cos file is not used for ID record
            iExportedFileIdx++;
            if(iExportedFileIdx >= m_aExportFileVars.GetSize()){
                //We reached the end and still did not find
                bFound = false;
                break;
            }
            rcsExportFileVar    = m_aExportFileVars.ElementAt(iExportedFileIdx);

        }
        ASSERT(iExportedFileIdx <= m_aExportFiles.GetSize());
        if(bFound){
            m_aExportFiles[iExportedFileIdx] = filename;
            iExportedFileIdx++;
            if(iExportedFileIdx < m_aExportFileVars.GetSize()){
                rcsExportFileVar = m_aExportFileVars.ElementAt(iExportedFileIdx);
            }

        }
    }


    //End of the data file to "FILE" association

    if(!GenerateBatchApp4MultiModel2()){
        AfxMessageBox(_T("Failed to generate Export app"));
    }
    else {//launch the stuff after piff file gen

        //make the order spec name ;
        //m_sBCHPFFName =sPath+_T("\\") + _T("CSExpRun.pff");
        m_sBCHPFFName = m_sBaseFilename + FileExtensions::WithDot::Pff;
        DeleteFile(sListFile);
    //    CNPifFile pifFile(m_sBCHPFFName);
        CNPifFile& pifFile = m_PifFile;
        sOldAppName = pifFile.GetAppFName();
        pifFile.SetAppFName(sAplFile);
        pifFile.SetListingFName(sListFile);
        m_csSPSSOutFile = filename;
        SaveBatchPffAdjustingOnExit(&pifFile);
        LaunchBatchApp();
        pifFile.SetAppFName(sOldAppName);
    }
    if( !m_PifFile.GetAppFName().IsEmpty() ) {
        m_sPFFName = m_PifFile.GetAppFName() + FileExtensions::WithDot::Pff;
        if (m_bPostRunSave) {
            m_PifFile.SetPifFileName(m_sPFFName);
            m_PifFile.Save();
        }
    }
}

////////////////////////////////////////////////////////////////////
//
//                   bool  CExportDoc::DeleteOutPutFiles()
//
////////////////////////////////////////////////////////////////////
bool  CExportDoc::DeleteOutPutFiles()
{
    bool bRet = false;

    if(!m_csSPSSOutFile.IsEmpty()){
        if(!CheckNDeleteFile(m_csSPSSOutFile)){
            return bRet;
        }

        if(m_convmethod == METHOD::CSPRO){
            CString sDCFFile = m_csSPSSOutFile;
            PathRemoveExtension(sDCFFile.GetBuffer(_MAX_PATH));
            sDCFFile.ReleaseBuffer();
            sDCFFile += FileExtensions::WithDot::Dictionary;
            if(!CheckNDeleteFile(sDCFFile)){
                return bRet;
            }
        }
//  IMSASendMessage(IMSA_WNDCLASS_TEXTVIEW, WM_IMSA_FILEOPEN, m_csSPSSOutFile);
        if(!CheckNDeleteFile(m_csSPSSOutFile)){
            return bRet;
        }
        if(!CheckNDeleteFile(m_csSPSSDescFile)){
            return bRet;
        }
        if(!CheckNDeleteFile(m_csSASDescFile)){
            return bRet;
        }
        if(!CheckNDeleteFile(m_csSTATADescFile)){
            return bRet;
        }
        if(!CheckNDeleteFile(m_csSTATALabelFile)){
            return bRet;
        }
    }
    else {
        GetNumExpRecTypes();
        int iSize = ((CExportApp *)AfxGetApp())->m_arrPifInfo.GetSize();
        for (int i=0; i < iSize; i++) {
           PIFINFO* pPif = ((CExportApp *)AfxGetApp())->m_arrPifInfo[i];
           CString csDataFile = pPif->sFileName ;

            if(!CheckNDeleteFile(csDataFile)){
                return bRet;
            }

            switch (m_convmethod)
            {
                case METHOD::SPSS:
                {
                    PathRemoveExtension(csDataFile.GetBuffer(_MAX_PATH));
                    csDataFile.ReleaseBuffer();
                    csDataFile += FileExtensions::WithDot::SpssSyntax;

                    if(!CheckNDeleteFile(csDataFile)){
                        return bRet;
                    }
                }
                break;

                case METHOD::SAS:
                {
                    PathRemoveExtension(csDataFile.GetBuffer(_MAX_PATH));
                    csDataFile.ReleaseBuffer();
                    csDataFile += FileExtensions::WithDot::SasSyntax;
                    if(!CheckNDeleteFile(csDataFile)){
                        return bRet;
                    }
                }
                break;

                case METHOD::STATA:
                {
                    PathRemoveExtension(csDataFile.GetBuffer(_MAX_PATH));
                    csDataFile.ReleaseBuffer();
                    csDataFile += FileExtensions::WithDot::StataDictionary;
                    if(!CheckNDeleteFile(csDataFile)){
                        return bRet;
                    }

                    PathRemoveExtension(csDataFile.GetBuffer(_MAX_PATH));
                    csDataFile.ReleaseBuffer();
                    csDataFile += FileExtensions::WithDot::StataDo;
                    if(!CheckNDeleteFile(csDataFile)){
                        return bRet;
                    }
                }
                break;

                case METHOD::CSPRO:
                {
                    PathRemoveExtension(csDataFile.GetBuffer(_MAX_PATH));
                    csDataFile.ReleaseBuffer();
                    csDataFile += FileExtensions::WithDot::Dictionary;
                    if(!CheckNDeleteFile(csDataFile)){
                        return bRet;
                    }

                }
                break;

                case METHOD::ALLTYPES:
                {
                    PathRemoveExtension(csDataFile.GetBuffer(_MAX_PATH));
                    csDataFile.ReleaseBuffer();
                    csDataFile += FileExtensions::WithDot::SpssSyntax;
                    if(!CheckNDeleteFile(csDataFile)){
                        return bRet;
                    }

                    PathRemoveExtension(csDataFile.GetBuffer(_MAX_PATH));
                    csDataFile.ReleaseBuffer();
                    csDataFile += FileExtensions::WithDot::SasSyntax;
                    if(!CheckNDeleteFile(csDataFile)){
                        return bRet;
                    }

                    PathRemoveExtension(csDataFile.GetBuffer(_MAX_PATH));
                    csDataFile.ReleaseBuffer();
                    csDataFile += FileExtensions::WithDot::StataDictionary;
                    if(!CheckNDeleteFile(csDataFile)){
                        return bRet;
                    }

                    PathRemoveExtension(csDataFile.GetBuffer(_MAX_PATH));
                    csDataFile.ReleaseBuffer();
                    csDataFile += FileExtensions::WithDot::StataDo;
                    if(!CheckNDeleteFile(csDataFile)){
                        return bRet;
                    }
                }
                break;
            }
        }
    }

    return true;
}


/////////////////////////////////////////////////////////////////////////////////
//
//  bool  CExportDoc::CompileApp()
//
/////////////////////////////////////////////////////////////////////////////////
bool CExportDoc::CompileApp()
{
    if(!GenerateBatchApp4MultiModel2()){
        AfxMessageBox(_T("Failed to generate Export app"));
        return false;
    }

    CString sPath;
    // 20131220 base the export files off the pff location, if available
    sPath = m_PifFile.GetAppFName().IsEmpty() ? m_csDictFileName : m_PifFile.GetAppFName();
    PathRemoveFileSpec(sPath.GetBuffer(MAX_PATH));
    sPath.ReleaseBuffer();
    sPath.TrimRight('\\');

    //m_sBCHPFFName =sPath+_T("\\") + _T("CSExpRun.pff");
    m_sBCHPFFName = m_sBaseFilename + FileExtensions::WithDot::Pff;
    CNPifFile pifFile(m_sBCHPFFName);
    pifFile.SetAppType(BATCH_TYPE);

    //CString   csExport_bch = sPath + _T("\\CSExpRun.bch");
    CString csExport_bch = m_sBaseFilename + FileExtensions::WithDot::BatchApplication;

    // pifFile.SetCSPROSyntaxFName(_T("")); (20140520 it's not clear why this code was here ... it prevented the .dcf from being created when running the .pff)

    pifFile.SetAppFName(csExport_bch);
    pifFile.Save();

    CFileStatus fStatus;
    if(CFile::GetStatus(m_sBCHPFFName,fStatus)){
        pifFile.LoadPifFile();
        pifFile.BuildAllObjects();
    }
    ASSERT(pifFile.GetApplication());

    try
    {
        CWaitCursor wait;

        if( pifFile.GetApplication()->GetCodeFiles().empty() )
            return false;

        CSourceCode srcCode(*pifFile.GetApplication());
        pifFile.GetApplication()->SetAppSrcCode(&srcCode);
        srcCode.Load();

        std::vector<CString> proc_names = pifFile.GetApplication()->GetRuntimeFormFiles().front()->GetOrder();
        srcCode.SetOrder(proc_names);

        CCompiler compiler(pifFile.GetApplication());
        CCompiler::Result err = compiler.FullCompile(pifFile.GetApplication()->GetAppSrcCode());

        if(err == CCompiler::Result::CantInit || err == CCompiler::Result::NoInit)
            throw CSProException("Cannot init the compiler");

        if (err != CCompiler::Result::NoErrors )
            return false;

        return true;
    }

    catch( const CSProException& exception )
    {
        ErrorMessage::Display(exception);
            return false;
    }
}


bool CExportDoc::IsRecordSingle(const CString& sRecTypeVal)
{
    bool bRet =false;
    for(int iIndex =0; iIndex < m_aItems.GetSize(); iIndex++){
        if(m_aItems[iIndex].pItem->GetRecord()->GetRecTypeVal().CompareNoCase(sRecTypeVal) == 0 ) {
            m_aItems[iIndex].pItem->GetRecord()->GetMaxRecs()>1 ? bRet = false : bRet = true;
            break;
        }
    }

    return bRet;

}

bool CExportDoc::WantFlatExport(){
    return bBehavior_SingleRecordsFlat;
}

void CExportDoc::UpdateOptionsPane(){
    if( m_pOptionsView ){
        bool bNewJoinValue = false;
        m_pOptionsView->UpdateEnabledDisabledCtrlsStatus( &bNewJoinValue );
        m_bJoinSingleWithMultipleRecords = bNewJoinValue;
    }
}


void CExportDoc::OnOptionsLogicSettings()
{
    LogicSettingsDlg dlg(m_logicSettings);

    if( dlg.DoModal() != IDOK || m_logicSettings == dlg.GetLogicSettings() )
        return;

    m_logicSettings = dlg.GetLogicSettings();
    SetModifiedFlag();

    // refresh the Scintilla lexer
    m_pOptionsView->RefreshLexer();
}


void CExportDoc::OnViewBatchLogic()
{
    // 20110805 Cambodia requested the ability to copy the code to the clipboard,
    // rather than having to click run and use the temporary files created
    SyncBuff_app();

    BatchLogicViewerDlg dlg(*m_pDataDict, m_logicSettings, CS2WS(m_csExportApp));
    dlg.DoModal();
}



void CExportDoc::SaveSettingsToRegistry() // 20130703
{
    CString sConvMethod;

    switch( m_convmethod )
    {
        case METHOD::TABS:
            sConvMethod = EXPORT_CMD_ExportMethod_TABS;
            break;

        case METHOD::COMMADEL:
            sConvMethod = EXPORT_CMD_ExportMethod_COMMADEL;
            break;

        case METHOD::SEMI_COLON:
            sConvMethod = EXPORT_CMD_ExportMethod_SEMI_COLON;
            break;

        case METHOD::CSPRO:
            sConvMethod = EXPORT_CMD_ExportMethod_CSPRO;
            break;

        case METHOD::SPSS:
            sConvMethod = EXPORT_CMD_ExportMethod_SPSS;
            break;

        case METHOD::SAS:
            sConvMethod = EXPORT_CMD_ExportMethod_SAS;
            break;

        case METHOD::STATA:
            sConvMethod = EXPORT_CMD_ExportMethod_STATA;
            break;

        case METHOD::R:
            sConvMethod = EXPORT_CMD_ExportMethod_R;
            break;

        case METHOD::ALLTYPES:
            sConvMethod = EXPORT_CMD_ExportMethod_ALLTYPES;
            break;
    }

    AfxGetApp()->WriteProfileString(EXPORT_CMD_Settings,EXPORT_CMD_ExportMethod,sConvMethod);
    AfxGetApp()->WriteProfileString(EXPORT_CMD_Settings,EXPORT_CMD_UnicodeOutput,m_bForceANSI ? EXPORT_CMD_No : EXPORT_CMD_Yes);
    AfxGetApp()->WriteProfileString(EXPORT_CMD_Settings,EXPORT_CMD_DecimalComma,m_bCommaDecimal ? EXPORT_CMD_Yes : EXPORT_CMD_No);
}


void CExportDoc::SaveBatchPffAdjustingOnExit(CNPifFile* pPifFile)
{
    CString original_pff_filename = pPifFile->GetPifFileName();
    CString csSavedOnExitFilename = pPifFile->GetOnExitFilename();

    pPifFile->SetPifFileName(m_sBCHPFFName);

    // OnExit should only be executed if CSExport was run with a PFF as a command line argument
    if( !m_batchmode )
        pPifFile->SetOnExitFilename(_T(""));

    if (m_batchmode && !m_PifFile.GetStartLanguageString().IsEmpty())
        pPifFile->SetStartLanguageString(m_PifFile.GetStartLanguageString());
    else
        pPifFile->SetStartLanguageString(WS2CS(m_pDataDict->GetCurrentLanguage().GetName()));

    pPifFile->Save();

    pPifFile->SetPifFileName(original_pff_filename);
    pPifFile->SetOnExitFilename(csSavedOnExitFilename);
}


bool CExportDoc::ProcessDictionarySource(const wstring_view filename_sv)
{
    m_embeddedDictionaryInformation = std::make_unique<std::tuple<std::unique_ptr<TemporaryFile>, ConnectionString>>(nullptr, filename_sv);

    std::unique_ptr<CDataDict> embedded_dictionary = DataRepositoryHelpers::GetEmbeddedDictionary(std::get<1>(*m_embeddedDictionaryInformation));

    if( embedded_dictionary != nullptr )
    {
        // for now, save the embedded dictionary; ideally this would not need to be saved to the disk
        try
        {
            std::get<0>(*m_embeddedDictionaryInformation) = std::make_unique<TemporaryFile>();
            embedded_dictionary->Save(std::get<0>(*m_embeddedDictionaryInformation)->GetPath());
        }

        catch( const CSProException& exception )
        {
            ErrorMessage::Display(exception);
            return false;
        }

        m_csDictFileName = WS2CS(std::get<0>(*m_embeddedDictionaryInformation)->GetPath());
    }

    else
    {
        m_embeddedDictionaryInformation.reset();
        m_csDictFileName = filename_sv;
    }

    return true;
}


CString CExportDoc::GetDictionarySourceFilename() const
{
    return ( m_embeddedDictionaryInformation != nullptr ) ? WS2CS(std::get<1>(*m_embeddedDictionaryInformation).GetFilename()) :
                                                            m_csDictFileName;
}

CString CExportDoc::GetDocumentWindowTitle() const
{
    CString csMainFilename = m_PifFile.GetAppFName();

    if( csMainFilename.IsEmpty() )
        csMainFilename = GetDictionarySourceFilename();

    CString csDocumentTitle = GetFileName(csMainFilename);

    // add the dictionary name
    if( !csDocumentTitle.IsEmpty() && ( m_pDataDict != nullptr ) )
        csDocumentTitle.AppendFormat(_T(" (%s)"), m_pDataDict->GetName().GetString());

    return csDocumentTitle;
}

CString CExportDoc::GetDirectoryForOutputs() const
{
    CString filename =
        ( m_embeddedDictionaryInformation != nullptr )                     ? WS2CS(std::get<1>(*m_embeddedDictionaryInformation).GetFilename()) :
        m_PifFile.GetSingleInputDataConnectionString().IsFilenamePresent() ? WS2CS(m_PifFile.GetSingleInputDataConnectionString().GetFilename()) :
        !m_PifFile.GetAppFName().IsEmpty()                                 ? m_PifFile.GetAppFName() :
                                                                             m_csDictFileName;

    return PortableFunctions::PathGetDirectory<CString>(filename);
}


bool CExportDoc::GetInputDataFilenames()
{
    if( m_embeddedDictionaryInformation != nullptr )
        m_PifFile.SetSingleInputDataConnectionString(std::get<1>(*m_embeddedDictionaryInformation));

    else if( !m_batchmode )
    {
        DataFileDlg data_file_dlg(DataFileDlg::Type::OpenExisting, true, m_PifFile.GetInputDataConnectionStringsSerializable());
        data_file_dlg.SetDictionaryFilename(m_csDictFileName)
                     .AllowMultipleSelections();

        if ( data_file_dlg.DoModal() != IDOK )
            return false;

        m_PifFile.ClearAndAddInputDataConnectionStrings(data_file_dlg.GetConnectionStrings());
    }

    return true;
}

bool CExportDoc::CheckInInputOutputFilenamesAreDifferent() const
{
    // check to make sure that input and output data files are different
    for( const auto& exportFilename : m_PifFile.GetExportFilenames() )
    {
        for( const ConnectionString& connection_string : m_PifFile.GetInputDataConnectionStrings() )
        {
            if( connection_string.FilenameMatches(exportFilename) )
            {
                AfxMessageBox(FormatText(_T("Input file: %s is the same as output file. Please use different names for input and ouput files and try again."), exportFilename.GetString()));
                return false;
            }
        }
    }

    return true;
}



/////////////////////////////////////////////////////////////////////////////////
//
// Spec file serialization
//
/////////////////////////////////////////////////////////////////////////////////

CREATE_JSON_VALUE_TEXT_OVERRIDE(export_, export)

CREATE_ENUM_JSON_SERIALIZER(METHOD,
    { METHOD::TABS,       _T("tab") },
    { METHOD::COMMADEL,   _T("CSV") },
    { METHOD::SPSS,       _T("SPSS") },
    { METHOD::SAS,        _T("SAS") },
    { METHOD::STATA,      _T("Stata") },
    { METHOD::ALLTYPES,   _T("all") },
    { METHOD::SEMI_COLON, _T("semicolon") },
    { METHOD::CSPRO,      _T("CSPro") },
    { METHOD::R,          _T("R") })

CREATE_ENUM_JSON_SERIALIZER(ExportRecordType,
    { ExportRecordType::None,      _T("none") },
    { ExportRecordType::BeforeIds, _T("beforeIds") },
    { ExportRecordType::AfterIds,  _T("afterIds") })

CREATE_ENUM_JSON_SERIALIZER(ExportItemsSubitems,
    { ExportItemsSubitems::ItemsOnly,    _T("itemsOnly") },
    { ExportItemsSubitems::SubitemsOnly, _T("subitemsOnly") },
    { ExportItemsSubitems::Both,         _T("both") })

// ideally these enums would be used by the class instead of bools, but
// that refactoring can be done at a later point
enum class DecimalMark { Period, Comma };
enum class ExportEncoding { Ansi, Utf8Bom };
enum class ExportStructure { Flat, Rectangular };
enum class ItemDisplay { Labels, Names };
enum class ItemSerialization { Included, Excluded };

CREATE_ENUM_JSON_SERIALIZER(DecimalMark,
    { DecimalMark::Period, CSValue::period },
    { DecimalMark::Comma,  CSValue::comma })

CREATE_ENUM_JSON_SERIALIZER(ExportEncoding,
    { ExportEncoding::Ansi,    CSValue::ANSI },
    { ExportEncoding::Utf8Bom, CSValue::UTF_8_BOM })

CREATE_ENUM_JSON_SERIALIZER(ExportStructure,
    { ExportStructure::Flat,        _T("flat") },
    { ExportStructure::Rectangular, _T("rectangular") })

CREATE_ENUM_JSON_SERIALIZER(ItemDisplay,
    { ItemDisplay::Labels, _T("labels") },
    { ItemDisplay::Names,  _T("names") })

CREATE_ENUM_JSON_SERIALIZER(ItemSerialization,
    { ItemSerialization::Included, _T("included") },
    { ItemSerialization::Excluded, _T("excluded") })


bool CExportDoc::OpenSpecFile(const TCHAR* filename, bool silent)
{
    try
    {
        auto json_reader = JsonSpecFile::CreateReader(filename, nullptr, [&]() { return ConvertPre80SpecFile(filename); });

        try
        {
            json_reader->CheckVersion();
            json_reader->CheckFileType(JV::export_);

            // open the dictionary
            std::wstring dictionary_filename = json_reader->GetAbsolutePath(JK::dictionary);

            if( !ProcessDictionarySource(dictionary_filename) || !OpenDictFile(m_csDictFileName, silent) )
                throw CSProException(_T("The dictionary could not be read: %s"), dictionary_filename.c_str());

            // reestablish the dictionary language
            const std::optional<wstring_view> language_name_sv = json_reader->GetOptional<wstring_view>(JK::language);

            if( language_name_sv.has_value() )
            {
                const std::optional<size_t> language_index = m_pDataDict->IsLanguageDefined(*language_name_sv);

                if( language_index.has_value() )
                {
                    m_pDataDict->SetCurrentLanguage(*language_index);
                }

                else
                {
                    json_reader->LogWarning(_T("The dictionary language '%s' is not in the dictionary '%s'"),
                                            std::wstring(*language_name_sv).c_str(), m_pDataDict->GetName().GetString());
                }
            }

            const auto& output_node = json_reader->GetOrEmpty(JK::output);
            m_convmethod = output_node.GetOrDefault<METHOD>(JK::format, METHOD::TABS);
            m_bForceANSI = ( output_node.GetOrDefault(JK::encoding, ExportEncoding::Ansi) == ExportEncoding::Ansi );
            m_bCommaDecimal = ( output_node.GetOrDefault(JK::decimalMark, DecimalMark::Period) == DecimalMark::Comma );

            const auto& model_node = json_reader->GetOrEmpty(JK::model);
            m_bmerge = !model_node.GetOrDefault(JK::separateRecords, false);
            m_bAllInOneRecord = ( model_node.GetOrDefault(JK::structure, ExportStructure::Flat) == ExportStructure::Flat );
            m_bJoinSingleWithMultipleRecords = ( !m_bAllInOneRecord && model_node.GetOrDefault(JK::joinSingleRecord, false) );
            m_exportRecordType = model_node.GetOrDefault(JK::recordType, ExportRecordType::None);
            m_exportItemsSubitems = model_node.GetOrDefault(JK::itemMode, ExportItemsSubitems::ItemsOnly);

            m_logicSettings = json_reader->GetOrDefault(JK::logicSettings, m_logicSettings);

            m_csUniverse = json_reader->GetOrDefault(JK::universe, SO::EmptyCString);

            const std::optional<ItemDisplay> item_display = json_reader->GetOptional<ItemDisplay>(JK::itemDisplay);

            if( item_display.has_value() )
                SharedSettings::ToggleViewNamesInTree(( *item_display == ItemDisplay::Names ));

            m_bSaveExcluded = ( json_reader->GetOrDefault<ItemSerialization>(JK::itemSerialization, ItemSerialization::Included) == ItemSerialization::Excluded );

            // create the list of all the items from the dictionary
            AddAllItems();

            // read the items
            for( const auto& item_node : json_reader->GetArrayOrEmpty(JK::items) )
            {
                const std::optional<wstring_view> item_name_sv = item_node.GetOptional<wstring_view>(JK::name);

                if( !item_name_sv.has_value() )
                    continue;

                const std::optional<wstring_view> relation_name_sv = item_node.GetOptional<wstring_view>(JK::relation);
                const int occurrence = item_node.GetOrDefault(JK::occurrence, -1);
                int position_in_list;

                if( !relation_name_sv.has_value() )
                {
                    position_in_list = GetPositionInList(*item_name_sv, occurrence);
                }

                else
                {
                    const std::vector<DictRelation>& dict_relations = m_pDataDict->GetRelations();
                    const auto& dict_relations_lookup = std::find_if(dict_relations.cbegin(), dict_relations.cend(),
                        [&](const DictRelation& dict_relation) { return SO::EqualsNoCase(*relation_name_sv, dict_relation.GetName()); });

                    if( dict_relations_lookup == dict_relations.cend() )
                    {
                        json_reader->LogWarning(_T("'%s' is not a valid relation in the dictionary '%s'"),
                                                std::wstring(*relation_name_sv).c_str(), m_pDataDict->GetName().GetString());
                        continue;
                    }

                    position_in_list = GetPositionInList(std::distance(dict_relations.cbegin(), dict_relations_lookup), *item_name_sv, occurrence);
                }

                if( position_in_list < 0 )
                {
                    json_reader->LogWarning(_T("'%s%s' is not a valid item in the dictionary '%s'"),
                                            std::wstring(*item_name_sv).c_str(),
                                            ( occurrence != -1 ) ? FormatText(_T("(%d)"), occurrence).GetString() : _T(""),
                                            m_pDataDict->GetName().GetString());
                }

                else
                {
                    SetItemCheck(position_in_list, !m_bSaveExcluded);
                }
            }
        }

        catch( const CSProException& exception )
        {
            json_reader->GetMessageLogger().RethrowException(filename, exception);
        }

        // report any warnings
        json_reader->GetMessageLogger().DisplayWarnings(silent);

        return true;
    }

    catch( const CSProException& exception )
    {
        ErrorMessage::Display(exception);
        return false;
    }
}


void CExportDoc::SaveSpecFile() const
{
    try
    {
        auto json_writer = JsonSpecFile::CreateWriter(m_PifFile.GetAppFName(), JV::export_);

        json_writer->WriteRelativePath(JK::dictionary, CS2WS(GetDictionarySourceFilename()));

        json_writer->Write(JK::language, m_pDataDict->GetCurrentLanguage().GetName());

        json_writer->Key(JK::output).WriteObject(
            [&]()
            {
                json_writer->Write(JK::format, m_convmethod)
                            .Write(JK::encoding, m_bForceANSI ? ExportEncoding::Ansi : ExportEncoding::Utf8Bom)
                            .Write(JK::decimalMark, m_bCommaDecimal ? DecimalMark::Comma : DecimalMark::Period);
            });

        json_writer->Key(JK::model).WriteObject(
            [&]()
            {
                json_writer->Write(JK::separateRecords, !m_bmerge)
                            .Write(JK::structure, m_bAllInOneRecord ? ExportStructure::Flat : ExportStructure::Rectangular);

                if( !m_bAllInOneRecord )
                    json_writer->Write(JK::joinSingleRecord, m_bJoinSingleWithMultipleRecords);

                json_writer->Write(JK::recordType, m_exportRecordType)
                            .Write(JK::itemMode, m_exportItemsSubitems);
            });

        json_writer->Write(JK::logicSettings, m_logicSettings);

        json_writer->WriteIfNotBlank(JK::universe, m_csUniverse);

        json_writer->Write(JK::itemDisplay, SharedSettings::ViewNamesInTree() ? ItemDisplay::Names : ItemDisplay::Labels)
                    .Write(JK::itemSerialization, m_bSaveExcluded ? ItemSerialization::Excluded : ItemSerialization::Included);

        json_writer->BeginArray(JK::items);

        for( int i = 0; i < m_aItems.GetSize(); ++i )
        {
            if( IsChecked(i) == m_bSaveExcluded )
                continue;

            const auto& export_item = m_aItems[i];

            json_writer->WriteObject(
                [&]()
                {
                    if( export_item.rel >= 0 )
                        json_writer->Write(JK::relation, m_pDataDict->GetRelation(export_item.rel).GetName());

                    json_writer->Write(JK::name, export_item.pItem->GetName());

                    if( export_item.occ > 0 )
                        json_writer->Write(JK::occurrence, export_item.occ);
                });
        }

        json_writer->EndArray();

        json_writer->EndObject();
    }

    catch( const CSProException& exception )
    {
        ErrorMessage::Display(exception);
    }
}


std::wstring CExportDoc::ConvertPre80SpecFile(NullTerminatedString filename)
{
    CSpecFile specfile;

    if( !specfile.Open(filename.c_str(), CFile::modeRead) )
        throw CSProException(_T("Failed to open the Export Data specification file: %s"), filename.c_str());

    auto json_writer = Json::CreateStringWriter();

    json_writer->BeginObject();

    json_writer->Write(JK::version, 7.7);
    json_writer->Write(JK::fileType, JV::export_);

    json_writer->Write(JK::logicSettings, LogicSettings::GetOriginalSettings());

    try
    {
        // Is correct spec file?
        if( !specfile.IsHeaderOK(_T("[CSExport]")) )
            throw CSProException(_T("Spec File does not begin with\n\n    [CSExport]"));

        // Ignore version errors
        specfile.IsVersionOK(CSPRO_VERSION);

        CString command;
        CString argument;

        Json::ObjectCreator output_node;
        Json::ObjectCreator model_node;
        CString universe;
        std::vector<std::tuple<std::optional<std::wstring>, std::wstring, std::optional<int>>> relations_names_and_occurrences;

        while( specfile.GetLine(command, argument) == SF_OK )
        {
            if( command.CompareNoCase(_T("File")) == 0 )
            {
                json_writer->Write(JK::dictionary, specfile.EvaluateRelativeFilename(argument));
            }

            else if( command.CompareNoCase(_T("DoMerge")) == 0 )
            {
                model_node.Set(JK::separateRecords, ( argument.CompareNoCase(EXPORT_CMD_No) == 0 ));
            }

            else if( command.CompareNoCase(_T("FilesCreated")) == 0 )
            {
                model_node.Set(JK::separateRecords, ( argument.CompareNoCase(_T("Multiple")) == 0 ));
            }

            else if( command.CompareNoCase(_T("ReportLabels")) == 0 || command.CompareNoCase(_T("ReportLables")) == 0 )
            {
                json_writer->Write(JK::itemDisplay, ( argument.CompareNoCase(_T("Names")) == 0 ) ? ItemDisplay::Names : ItemDisplay::Labels);
            }

            else if( command.CompareNoCase(_T("ItemsAre")) == 0 )
            {
                json_writer->Write(JK::itemSerialization, ( argument.CompareNoCase(_T("Excluded")) == 0 ) ? ItemSerialization::Excluded : ItemSerialization::Included);
            }

            else if( command.CompareNoCase(_T("Language")) == 0 )
            {
                json_writer->Write(JK::language, argument);
            }

            else if( command.CompareNoCase(EXPORT_CMD_ExportMethod) == 0 )
            {
                output_node.Set(JK::format, ( argument.CompareNoCase(_T("TAB")) == 0 )                          ? METHOD::TABS :
                                            ( argument.CompareNoCase(EXPORT_CMD_ExportMethod_TABS) == 0 )       ? METHOD::TABS :
                                            ( argument.CompareNoCase(_T("COMMADEL")) == 0 )                     ? METHOD::COMMADEL :
                                            ( argument.CompareNoCase(EXPORT_CMD_ExportMethod_COMMADEL) == 0 )   ? METHOD::COMMADEL :
                                            ( argument.CompareNoCase(EXPORT_CMD_ExportMethod_SEMI_COLON) == 0 ) ? METHOD::SEMI_COLON :
                                            ( argument.CompareNoCase(EXPORT_CMD_ExportMethod_SPSS) == 0 )       ? METHOD::SPSS :
                                            ( argument.CompareNoCase(EXPORT_CMD_ExportMethod_CSPRO) == 0 )      ? METHOD::CSPRO :
                                            ( argument.CompareNoCase(EXPORT_CMD_ExportMethod_SAS) == 0 )        ? METHOD::SAS :
                                            ( argument.CompareNoCase(EXPORT_CMD_ExportMethod_STATA) == 0 )      ? METHOD::STATA :
                                            ( argument.CompareNoCase(_T("ALLTYPES")) == 0 )                     ? METHOD::ALLTYPES :
                                            ( argument.CompareNoCase(EXPORT_CMD_ExportMethod_ALLTYPES) == 0 )   ? METHOD::ALLTYPES :
                                                                                                                  METHOD::TABS);
            }

            else if( command.CompareNoCase(_T("MultipleOccs")) == 0 )
            {
                model_node.Set(JK::structure, ( argument.CompareNoCase(_T("OneRecord")) == 0 ) ? ExportStructure::Flat : ExportStructure::Rectangular);
            }

            else if( command.CompareNoCase(_T("JoinSingleWithMultiple")) == 0 )
            {
                model_node.Set(JK::joinSingleRecord, ( argument.CompareNoCase(EXPORT_CMD_Yes) == 0 ));
            }

            else if( command.CompareNoCase(_T("ExportRecordType")) == 0 )
            {
                model_node.Set(JK::recordType, ( argument.CompareNoCase(_T("Before")) == 0 ) ? ExportRecordType::BeforeIds :
                                               ( argument.CompareNoCase(_T("After")) == 0 )  ? ExportRecordType::AfterIds :
                                                                                               ExportRecordType::None);
            }

            else if( command.CompareNoCase(_T("ExportItemsSubItems")) == 0 )
            {
                model_node.Set(JK::itemMode, ( argument.CompareNoCase(_T("Items")) == 0 )    ? ExportItemsSubitems::ItemsOnly :
                                             ( argument.CompareNoCase(_T("SubItems")) == 0 ) ? ExportItemsSubitems::SubitemsOnly :
                                                                                               ExportItemsSubitems::Both);
            }

            else if( command.CompareNoCase(EXPORT_CMD_UnicodeOutput) == 0 )
            {
                output_node.Set(JK::encoding, ( argument.CompareNoCase(EXPORT_CMD_Yes) == 0 ) ? ExportEncoding::Utf8Bom : ExportEncoding::Ansi);
            }

            else if( command.CompareNoCase(EXPORT_CMD_DecimalComma) == 0 )
            {
                output_node.Set(JK::decimalMark, ( argument.CompareNoCase(EXPORT_CMD_Yes) == 0 ) ? DecimalMark::Comma : DecimalMark::Period);
            }

            else if( command.CompareNoCase(_T("Universe")) == 0 )
            {
                if( !universe.IsEmpty() )
                    universe.Append(_T("\r\n"));

                universe.Append(argument);
            }

            else if( command.CompareNoCase(_T("Item")) == 0 )
            {
                auto& relation_name_and_occurrence = relations_names_and_occurrences.emplace_back();
                wstring_view item_name_sv = argument;

                const size_t dot_pos = item_name_sv.find('.');

                if( dot_pos != std::wstring::npos )
                {
                    std::get<0>(relation_name_and_occurrence) = item_name_sv.substr(0, dot_pos);
                    item_name_sv = item_name_sv.substr(dot_pos + 1);
                }

                const size_t comma_pos = item_name_sv.find(',');

                if( comma_pos != wstring_view::npos )
                {
                    std::get<2>(relation_name_and_occurrence) = _ttoi(std::wstring(item_name_sv.substr(comma_pos + 1)).c_str());
                    item_name_sv = item_name_sv.substr(0, comma_pos);
                }

                std::get<1>(relation_name_and_occurrence) = item_name_sv;
            }

            else if( command.CompareNoCase(_T("[Dictionaries]")) != 0 &&
                     command.CompareNoCase(_T("[ExportType]")) != 0 &&
                     command.CompareNoCase(_T("[ExportXMLMetadataDDI2]")) != 0 &&
                     command.CompareNoCase(_T("[ExportXMLMetadataDDI3]")) != 0 &&
                     command.CompareNoCase(_T("[ExportXMLMetadataCSPro]")) != 0 &&
                     command.CompareNoCase(_T("[ExportXMLMetadataWeight]")) != 0 &&
                     command.CompareNoCase(_T("[ExportXMLMetadataIncludeFrequencies]")) != 0 &&
                     command.CompareNoCase(_T("[Items]")) != 0 )
            {
                throw CSProException(_T("Spec File: Invalid command: %s"), command.GetString());
            }
        }

        json_writer->Write(JK::output, output_node.GetJsonNode());
        json_writer->Write(JK::model, model_node.GetJsonNode());

        if( !universe.IsEmpty() )
            json_writer->Write(JK::universe, universe);

        json_writer->WriteObjects(JK::items, relations_names_and_occurrences,
            [&](const auto& relation_name_and_occurrence)
            {
                if( std::get<0>(relation_name_and_occurrence).has_value() )
                    json_writer->Write(JK::relation, *std::get<0>(relation_name_and_occurrence));

                json_writer->Write(JK::name, std::get<1>(relation_name_and_occurrence));

                if( std::get<2>(relation_name_and_occurrence).has_value() )
                    json_writer->Write(JK::occurrence, *std::get<2>(relation_name_and_occurrence));
            });

        specfile.Close();
    }

    catch( const CSProException& exception )
    {
        specfile.Close();

        throw CSProException(_T("There was an error reading the Export Data specification file %s:\n\n%s"),
                             PortableFunctions::PathGetFilename(filename), exception.GetErrorMessage().c_str());
    }

    json_writer->EndObject();

    return json_writer->GetString();
}
