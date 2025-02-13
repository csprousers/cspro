#include "StdAfx.h"
#include "TabDoc.h"
#include "TabView.h"
#include "TabChWnd.h"
#include "FlashMsg.h"
#include <zToolsO/Tools.h>
#include <zJson/JsonStream.h>


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CLIPFORMAT NEAR CTabulateDoc::m_cfPrivate = NULL;   // savi

/////////////////////////////////////////////////////////////////////////////
// CTabulateDoc
IMPLEMENT_DYNCREATE(CTabulateDoc, ApplicationDoc)


CTabulateDoc::CTabulateDoc()
    :   m_TableSpec(std::make_shared<CTabSet>())
{
    m_cfPrivate = (CLIPFORMAT)::RegisterClipboardFormat(_T("Tab")); // savi

    TCHAR acPath[_MAX_PATH];
    GetTempPath(_MAX_PATH, acPath);
    m_csClipFile = acPath;
    m_csClipFile += IMSA_TABLE_CLIPFILE;

    m_auFormat[TD_TABLE_FORMAT] = RegisterClipboardFormat(_T("CSPro CrossTab Table"));
}


CTabulateDoc::~CTabulateDoc()
{
}


void CTabulateDoc::FileToClip(UINT uFormat)
{
    CString csClipFile = GetClipFile();
    if (!PortableFunctions::FileExists(csClipFile)) {
        // Clip file doesn't exist
        return;
    }

    CFile file(csClipFile, CFile::modeRead);
    int iSize = static_cast<int>(file.GetLength() + 1);

    if (iSize > 0)  {
        HGLOBAL hGlobalMemory;
        VERIFY ((hGlobalMemory = GlobalAlloc (GHND, iSize)) != nullptr);
        char FAR *lpGlobalMemory = (char FAR *) GlobalLock (hGlobalMemory);
        file.Read(lpGlobalMemory, (UINT) iSize);
        GlobalUnlock (hGlobalMemory);
        POSITION pos = GetFirstViewPosition();
        ASSERT(pos != nullptr);
        CTabView* pView = (CTabView*)GetNextView(pos);

        OpenClipboard(pView->m_hWnd);
        EmptyClipboard ();
        SetClipboardData (uFormat, hGlobalMemory);
        CloseClipboard ();
    }
    file.Close();

    TRY  {
        CFile::Remove(csClipFile);
    }
    CATCH(CFileException, e) {
        CString cs;
        cs.Format(_T("Warning:  Clipboard scratch file %s cannot be removed."), (LPCTSTR)csClipFile);
        AfxMessageBox(cs,MB_OK | MB_ICONINFORMATION);
    }
    END_CATCH
}
/////////////////////////////////////////////////////////////////////////////
//
//                            CTabulateDoc::ClipToFile
//
/////////////////////////////////////////////////////////////////////////////

bool CTabulateDoc::ClipToFile (UINT uFormat)
{
    HGLOBAL hClipData, hText;
    ASSERT(IsClipboardFormatAvailable(uFormat));     // OnUpdateUI should prevent us from arriving here if this isn't the case

    POSITION pos = GetFirstViewPosition();
    ASSERT(pos != nullptr);
    CTabView* pView = (CTabView*)GetNextView(pos);


    OpenClipboard(pView->m_hWnd);
    if ((hClipData = GetClipboardData (uFormat)) == nullptr) {
        // Nothing there
        CloseClipboard();
        return FALSE;
    }
    if ((hText = GlobalAlloc (GHND, GlobalSize(hClipData)+1)) == nullptr) {
        AfxMessageBox(_T("Clipboard error"));
        CloseClipboard();
        return FALSE;
    }
    char FAR* lpClipData = (char FAR*) GlobalLock(hClipData);
    char FAR* lpszText = (char FAR*) GlobalLock(hText);
    lstrcpyA (lpszText, lpClipData);
    GlobalUnlock (hClipData);
    GlobalUnlock (hText);
    CloseClipboard();

    CString csClipFile = GetClipFile();
    //CStdioFile file(csClipFile, CFile::modeCreate|CFile::typeBinary|CFile::modeWrite);
    //file.WriteString (lpszText);

    // 20120625 changed for unicode
    CFile file(csClipFile, CFile::modeCreate|CFile::typeBinary|CFile::modeWrite);
    file.Write(lpszText,strlen(lpszText));

    file.Close();
    return TRUE;
}

BOOL CTabulateDoc::OnNewDocument()
{
    if (!CDocument::OnNewDocument())
        return FALSE;
    return TRUE;
}



/////////////////////////////////////////////////////////////////////////////
// CTabulateDoc commands
/////////////////////////////////////////////////////////////////////////////
//
//                        CTabulateDoc::LoadSpecFile
//
/////////////////////////////////////////////////////////////////////////////

bool CTabulateDoc::LoadSpecFile(const CString& csFileName, std::shared_ptr<const CDataDict> pWorkDict/* = nullptr*/)
{
    POSITION pos = GetFirstViewPosition();
    ASSERT(pos != nullptr);
    CTabView* pView = (CTabView*)GetNextView(pos);

//  get dictionary name
    m_TableSpec->SetWorkDict(pWorkDict);
    CString csDictPathName = m_TableSpec->GetDictFile();

//  Open the dictionary, if not already open
    bool bViewer = ((CTableChildWnd*)pView->GetParentFrame())->IsViewer();
    DictionaryDictTreeNode* dictionary_dict_tree_node = nullptr;

    if(!bViewer) { //if it is not Viewer then do the dict stuff
        bool bDict = true;
        CDDTreeCtrl* pDictTree = GetTabTreeCtrl()->GetDDTreeCtrl();
        dictionary_dict_tree_node = pDictTree->GetDictionaryTreeNode(csDictPathName);
        if(dictionary_dict_tree_node->GetDDDoc() == nullptr) {
            bDict = pDictTree->OpenDictionary(csDictPathName, FALSE);
        }
        if (!bDict) {
            return false;
        }
    }

//  Open and load the spec file
    bool bOK = m_TableSpec->Open(csFileName);
    if (!bOK) {
        return false;
    }
    if(!bViewer && dictionary_dict_tree_node != nullptr) {
        m_TableSpec->SetDict(dictionary_dict_tree_node->GetDDDoc()->GetSharedDictionary());
    }
    else {//TabSpec has no Dict in viewer module
        m_TableSpec->SetDict(nullptr);
    }

    // RECONCILE
    /*CString sErr;
    AfxGetMainWnd()->SendMessage(WM_IMSA_UPDATE_SYMBOLTBL,(WPARAM)this,0);
    if (Reconcile(sErr)) {
        if(!sErr.IsEmpty()){
            AfxMessageBox(sErr);
        }
        SetModifiedFlag(TRUE);
    }

    // format registry notifications
    if (!m_TableSpec->GetFmtReg().GetErrorMsg().IsEmpty()) {
        CFlashMsgDlg dlg;
        dlg.m_iSeconds=3;
        dlg.m_sFlashMsg=m_TableSpec->GetFmtReg().GetErrorMsg();
        dlg.DoModal();
        m_TableSpec->GetFmtRegPtr()->ResetErrorMsg();
    }
    */
    return true;
}

void CTabulateDoc::DisplayFmtErrorMsg()
{
    if (!m_TableSpec->GetFmtReg().GetErrorMsg().IsEmpty()) {
        // SetModifiedFlag(); 20100316 this will no longer create a modification status
        CFlashMsgDlg dlg;
        dlg.m_iSeconds=1;//3;
        dlg.m_sFlashMsg=m_TableSpec->GetFmtReg().GetErrorMsg();
        dlg.DoModal();
        m_TableSpec->GetFmtRegPtr()->ResetErrorMsg();
    }
}


HTREEITEM CTabulateDoc::BuildAllTrees()
{
    HTREEITEM hItem = nullptr;

//  Get the handle to tree controls
    CTabTreeCtrl* pTabTree = GetTabTreeCtrl();
    CDDTreeCtrl* pDictTree = pTabTree->GetDDTreeCtrl();

//  add node to dict tree, or add reference

    CString csDictFile = m_TableSpec->GetDictFile();
    CString csDictLabel = m_sDictLabel;

    DictionaryDictTreeNode* dictionary_dict_tree_node = pDictTree->GetDictionaryTreeNode(csDictFile);
    if(dictionary_dict_tree_node != nullptr) {
        dictionary_dict_tree_node->AddRef();
    }
    else {
        hItem = pDictTree->InsertDictionary(csDictLabel,csDictFile,nullptr);

        TVITEM pItem;
        pItem.hItem = hItem;
        pItem.mask  = TVIF_CHILDREN ;
        pItem.cChildren = 1;
        pDictTree->SetItem(&pItem);
    }

//  add node to table tree, or add reference

    CString csTabPath = GetPathName();

    TableSpecTabTreeNode* table_spec_tab_tree_node = pTabTree->GetTableSpecTabTreeNode(csTabPath);
    if(table_spec_tab_tree_node != nullptr) {
        table_spec_tab_tree_node->AddRef();
        hItem = table_spec_tab_tree_node->GetHItem();
    }
    else {
        hItem = pTabTree->InsertTableSpec(csTabPath, this);

        TVITEM pItem;
        pItem.hItem = hItem;
        pItem.mask  = TVIF_CHILDREN ;
        pItem.cChildren = 1;
        pTabTree->SetItem(&pItem);
    }

    return hItem;

}


CTreeCtrl* CTabulateDoc::GetTreeCtrl()
{
    return GetTabTreeCtrl();
}


std::shared_ptr<const CDataDict> CTabulateDoc::GetSharedDictionary() const
{
    return m_TableSpec->GetSharedDictionary();
}


/////////////////////////////////////////////////////////////////////////////////
//
//  void CTabulateDoc::ReleaseDicts()
//
/////////////////////////////////////////////////////////////////////////////////
void CTabulateDoc::ReleaseDicts()
{
    return;
}

bool CTabulateDoc::IsTabModified() const
{
    return false;
}

bool CTabulateDoc::InitTreeCtrl()
{
    return false;
}


/////////////////////////////////////////////////////////////////////////////////
//
//  BOOL CTabulateDoc::OnOpenDocument(LPCTSTR lpszPathName)
//
/////////////////////////////////////////////////////////////////////////////////
BOOL CTabulateDoc::OnOpenDocument(LPCTSTR lpszPathName)
{
    BOOL bOK = TRUE;

    POSITION pos = GetFirstViewPosition();
    ASSERT(pos != nullptr);
    CTabView* pView = (CTabView*)GetNextView(pos);
    bool bViewer = ((CTableChildWnd*)pView->GetParentFrame())->IsViewer();

//  read table spec file to get label
    CSpecFile  specFile(TRUE);
    bOK = specFile.Open(lpszPathName, CFile::modeRead);
    if(bOK) {
        CString csLabel = ValFromHeader(specFile,CSPRO_CMD_LABEL);
        m_TableSpec->SetLabel (csLabel);
        specFile.Close();
    }
    else {
        m_TableSpec->SetLabel (_T(""));
        CString sString ;
        sString.FormatMessage(IDS_OPENDDFLD, lpszPathName);
        AfxMessageBox(sString);
        return FALSE;
    }

//  read table spec file to get dictionary file name

    bOK = specFile.Open(lpszPathName,CFile::modeRead);
    if (!bOK) {
        return FALSE;
    }
    CString sCmd, sArg;                // BMD 11 Jul 2006
    specFile.GetLine(sCmd, sArg);
    if (specFile.GetState() == SF_EOF) {
        AfxMessageBox (_T("Empty tabulation spec file"));
        return FALSE;
    }
    else {
        CString sVersion = CSPRO_VERSION;
        if (!specFile.IsVersionOK(sVersion)) {
            if (!IsValidCSProVersion(sVersion, 3.0)) {
                CString sFile = lpszPathName;
                CString sExt = sFile.Right(3);
                if (sExt.CompareNoCase(FileExtensions::Table) == 0) {
                    AfxMessageBox (_T("Incorrect TBW file version"));
                }
                else {
                    AfxMessageBox (_T("Incorrect XTB file version"));
                }
                return FALSE;
            }
        }
    }


    if(bViewer) {
        specFile.Close();
        bOK = this->LoadSpecFile(lpszPathName);
        if(bOK) {
            AfxGetMainWnd()->SendMessage(UWM::Table::UpdateTree, (WPARAM)this);
        }
    }

    else {
        std::vector<std::wstring> dictionary_filenames = GetFileNameArrayFromSpecFile(specFile, CSPRO_DICTS);
        specFile.Close();

        if (dictionary_filenames.empty()) {
            // &&& no dictionary name in spec file; ask for it?
            AfxMessageBox (_T("No data dictionary specified in spec file"));
            return FALSE;
        }

        CString csDictPathName = WS2CS(dictionary_filenames.front());
        SetDictFileName(csDictPathName);

        m_TableSpec->SetDictFile(csDictPathName);

        //  open dictionary file to get label
        try
        {
            LabelSet dictionary_label_set = JsonStream::GetValueFromSpecFile<LabelSet, CDataDict>(JK::labels, m_TableSpec->GetDictFile());
            m_sDictLabel = dictionary_label_set.GetLabel();
        }

        catch( const CSProException& exception )
        {
		    ErrorMessage::Display(exception);
        }

        if (bOK) {
            SetPathName(lpszPathName);
        }
    }

    return bOK ;
}
/////////////////////////////////////////////////////////////////////////////////
//
//  bool CTabulateDoc::GetVarList(CTabVar* pTabVar , CString& sVarList)
//
/////////////////////////////////////////////////////////////////////////////////
bool CTabulateDoc::GetVarList(CTabVar* pTabVar , CString& sVarList)
{
    bool bRet = true;
    int iSize = pTabVar->GetNumChildren();
    for (int iIndex = 0; iIndex < iSize; iIndex++) {
        const DictLevel* pDictLevel = nullptr;
        const CDictRecord* pDictRecord = nullptr;
        const CDictItem* pDictItem = nullptr;
        const DictValueSet* pDictVSet = nullptr;

        CString sTotal , sName ;
        CTabVar* pChild = pTabVar->GetChild(iIndex);

    /*  if(m_varNameMap.Lookup(pChild,sName)){
        }
        else {
            sName = pChild->GetName();
        }*/
        sName = pChild->GetName();
        const CDataDict* pDict= m_TableSpec->LookupName(pChild->GetName(),&pDictLevel,&pDictRecord,&pDictItem,&pDictVSet);

        if(!pDictVSet && pDict ) {
            sName = MakeVarList4DummyVSet(pDictItem);
        }
        if(pChild->GetName().CompareNoCase(_T("TT_BOTH")) ==0 /*&& pChild->IsCustom() && !pChild->IsDisplayed()*/)
            continue;
        //Either &&& do a look up /get from the tabvar name
        //For now we are including ("TOTAL" at the end once the
        //interface comes in then determine whether to add total /yes/no/before/after
//        sVarList +=  "(" + sName + "+Total)";
         sVarList +=   sName ;
        if(pChild->GetNumChildren() > 0 ) {
            bool bBoth  = pChild->GetChild(0)->GetName().CompareNoCase(_T("TT_BOTH")) ==0;
            bool bCustom =  pChild->GetChild(0)->GetType() == VT_CUSTOM ;
            if(bBoth && bCustom ){
                //do nothing
            }
            else {
                sVarList += _T(" * ");
                sVarList += _T(" ( ") ;
                GetVarList(pChild , sVarList);
                sVarList += _T(" ) ") ;
            }

        }
        if(iIndex < iSize-1) {
            sVarList +=  _T(" + ");
        }
    }
    return bRet;
}

/////////////////////////////////////////////////////////////////////////////
//
//                        CTabulateDoc::OnSaveDocument
//
/////////////////////////////////////////////////////////////////////////////

BOOL CTabulateDoc::OnSaveDocument(LPCTSTR lpszPathName)
{

    CString csName = lpszPathName;
    POSITION pos = GetFirstViewPosition();
    ASSERT(pos != nullptr);
    CTabView* pView = (CTabView*)GetNextView(pos);
    bool bViewer = ((CTableChildWnd*)pView->GetParentFrame())->IsViewer();
    if(!bViewer){
        SaveAllDictionaries();
    }
    m_TableSpec->Save(csName,GetDictFileName());
    SetModifiedFlag(FALSE);

    return TRUE;
}


/////////////////////////////////////////////////////////////////////////////////
//
//  void CTabulateDoc::SaveAllDictionaries()
//
/////////////////////////////////////////////////////////////////////////////////
void CTabulateDoc::SaveAllDictionaries()
{
    CTabTreeCtrl*   pTabTree  = GetTabTreeCtrl();

    if (pTabTree == nullptr)
        return;

    CDDTreeCtrl* pDictTree = pTabTree->GetDDTreeCtrl();

    DictionaryDictTreeNode* dictionary_dict_tree_node = pDictTree->GetDictionaryTreeNode(this->GetDictFileName());
    if( dictionary_dict_tree_node != nullptr && dictionary_dict_tree_node->GetDDDoc() != nullptr ) {
        if(dictionary_dict_tree_node->GetDDDoc()->IsModified()){
            dictionary_dict_tree_node->GetDDDoc()->OnSaveDocument(dictionary_dict_tree_node->GetDDDoc()->GetPathName());
        }
    }
}



/////////////////////////////////////////////////////////////////////////////////
//
//  void CTabulateDoc::OnCloseDocument()
//
/////////////////////////////////////////////////////////////////////////////////
void CTabulateDoc::OnCloseDocument()
{
    POSITION pos = GetFirstViewPosition();
    if (pos != nullptr) {                      // BMD 11 Jul 2006
        CTabView* pView = (CTabView*)GetNextView(pos);

    //  Open the dictionary, if not already open
        bool bViewer = ((CTableChildWnd*)pView->GetParentFrame())->IsViewer();

        if(bViewer && m_pTabTreeCtrl) {
            TableSpecTabTreeNode* table_spec_tab_tree_node = m_pTabTreeCtrl->GetTableSpecTabTreeNode(*this);
            ASSERT(table_spec_tab_tree_node != nullptr);
            m_pTabTreeCtrl->ReleaseDoc(*table_spec_tab_tree_node);
        }
    }
    CDocument::OnCloseDocument();
}



/////////////////////////////////////////////////////////////////////////////////
//
//  void CTabulateDoc::MakeCrossTabStatement(CTable* pTable , CString& sCrossTabStatement)
//
/////////////////////////////////////////////////////////////////////////////////
void CTabulateDoc::MakeCrossTabStatement(CTable* pRefTable, CString& sCrossTabStatement)
{
    m_TableSpec->MakeCrossTabStatement(pRefTable, sCrossTabStatement);
}



/////////////////////////////////////////////////////////////////////////////////
//
//  bool CTabulateDoc::ReconcileLinkTables(CArray<CLinkTable*,CLinkTable*>& arrLinkTable ,CString& sErr);//
/////////////////////////////////////////////////////////////////////////////////
bool CTabulateDoc::ReconcileLinkTables(CArray<CLinkTable*,CLinkTable*>& arrLinkTable ,CString& sErr )
{
    //on view logic designer---> logic generate the app code and put it in ;
    //on view logic logic---> designer check the link tables
    //if a link table exists and has a correspoding CTable reconcile
    //if a link table exists and has nocorrespoding CTable reconcile then try and get it into TabSet ?
    //if a CTable exists and there is no link table put the code in to the logic

    //for each table in TabSet
    int iNumLinkTables = arrLinkTable.GetSize();
    for (int iIndex = 0; iIndex < iNumLinkTables ; iIndex++){
        CLinkTable* pLinkTable = arrLinkTable.GetAt(iIndex);
        if(!pLinkTable)
            continue;
        //Given a plinkTable get the correspoding CTable
        CTable* pTable = GetTableFromLink(pLinkTable);
        if(pTable) {
            ReconcileTblNLinkObj(pTable,pLinkTable ,sErr );
        }
        else { //Generate one and put it in the table spec
            AfxMessageBox(_T("Table defined in logic has no spec"));
        }


    }


    //Now check the tables  against the linking tables for links that do not exist generate the code
    for (int iIndex =0; iIndex < this->GetTableSpec()->GetNumTables(); iIndex++ ) {
        CTable* pTable = nullptr;
        pTable = GetTableSpec()->GetTable(iIndex);
        CLinkTable* pLinkTable = GetLinkFromTable(arrLinkTable,pTable);
        if(!pLinkTable) {// put table proc in to the logic
            CTabVar* pRowRoot = pTable->GetRowRoot();
            CTabVar* pColRoot = pTable->GetColRoot();
            if(pRowRoot->GetNumChildren() ==  0 || pColRoot->GetNumChildren() ==0){
                continue;
            }

            CString sMsg;
            sMsg.Format(_T("App logic missing for spec table %s .Will generate the code for the spec") , (LPCTSTR)pTable->GetName());
            AfxMessageBox(sMsg);

            POSITION pos = GetFirstViewPosition();
            ASSERT(pos != nullptr);
            CTabView* pView = (CTabView*)GetNextView(pos);
            CTblGrid* pGrid = pView->GetGrid();

            CString sCrossTabStmt;
            MakeCrossTabStatement(pGrid->GetTable(),sCrossTabStmt);
            //Send a message to CSPro and putsource code for this
            TBL_PROC_INFO tblProcInfo;
            tblProcInfo.pTabDoc = this;
            tblProcInfo.pTable = pGrid->GetTable();
            tblProcInfo.sTblLogic = sCrossTabStmt;
            tblProcInfo.eEventType = CSourceCode_Tally;
            tblProcInfo.pLinkTable = nullptr;
            tblProcInfo.bGetLinkTables = false;

            if(AfxGetMainWnd()->SendMessage(UWM::Table::PutTallyProc, 0, (LPARAM)&tblProcInfo) == -1){
                AfxMessageBox(_T("Failed to put logic in App"));
                return 0;
            }
        }


    }
    return false;
}


/////////////////////////////////////////////////////////////////////////////////
//
//  CTable* CTabulateDoc::GetTableFromLink(CLinkTable* pLinkTable)
//
/////////////////////////////////////////////////////////////////////////////////
CTable* CTabulateDoc::GetTableFromLink(CLinkTable* pLinkTable)
{
    CTable* pRetTable = nullptr;

    for (int iIndex =0; iIndex < this->GetTableSpec()->GetNumTables(); iIndex++ ) {
        CTable* pTable = nullptr;
        pTable = GetTableSpec()->GetTable(iIndex);
        CString sTableName = pTable->GetName();
        CString sLinkTableName = pLinkTable->GetName().m_csText;
        if( sTableName.CompareNoCase(sLinkTableName) == 0 ) {
            pRetTable = pTable;
        }
    }

    return pRetTable;
}


/////////////////////////////////////////////////////////////////////////////////
//
//  CLinkTable* CTabulateDoc::GetLinkFromTable(CArray<CLinkTable*,CLinkTable*>& arrLinkTable, CTable* pTable)
//
/////////////////////////////////////////////////////////////////////////////////
CLinkTable* CTabulateDoc::GetLinkFromTable(CArray<CLinkTable*,CLinkTable*>& arrLinkTable, CTable* pTable)
{
    CLinkTable* pRetTable = nullptr;

    for (int iIndex =0; iIndex < arrLinkTable.GetSize(); iIndex++ ) {
        CLinkTable* pLinkTable = nullptr;
        pLinkTable = arrLinkTable.GetAt(iIndex);
        if(!pLinkTable)
            continue;
        CString sLinkTableName = pLinkTable->GetName().m_csText;
        CString sTableName = pTable->GetName();
        if( sTableName.CompareNoCase(sLinkTableName) == 0 ) {
            pRetTable = pLinkTable;
        }
    }

    return pRetTable;
}


/////////////////////////////////////////////////////////////////////////////////
//
//  bool CTabulateDoc:: ReconcileTblNLinkObj(CTable* pTable, CLinkTable* pLinkTable , bool bSilent /*= false*/, CString sErr)
//
/////////////////////////////////////////////////////////////////////////////////
bool CTabulateDoc::ReconcileTblNLinkObj(CTable* pTable, CLinkTable* pLinkTable ,CString& /*sErr*/, bool /*bSilent = false */)
{
    bool bRet = false;

    //Compare the row var names .//SAVY do check for duplicate names in the dictionary
    //Compare the col var names .//SAVY do check for duplicate names in the dictionary

    CString sLinkRowExpr = pLinkTable->GetRowExpr().m_csText;
    CString sLinkColExpr = pLinkTable->GetColExpr().m_csText;


    CString sTblRowExpr;
    GetVarList(pTable->GetRowRoot(),sTblRowExpr);

    bool bMatch = MatchExpr(sTblRowExpr,sLinkRowExpr);
    if(!bMatch){
       CString sMsg;
       sMsg += _T("Row Expression in application logic does not match with the spec \r\n");
       sMsg += _T("Link row expression:") + sLinkRowExpr  +_T("\r\n");
       sMsg += _T("Tbl row expression:") + sTblRowExpr  +_T("\r\n");
       bRet = false;
       AfxMessageBox(sMsg);

    }

    CString sTblColExpr;
    GetVarList(pTable->GetColRoot(),sTblColExpr);

    bMatch = MatchExpr(sTblColExpr,sLinkColExpr);
    if(!bMatch){
        CString sMsg;
        sMsg += _T("Col Expression in application logic does not match with the spec \r\n");
        sMsg += _T("Link col expression:") + sLinkColExpr  +_T("\r\n");
        sMsg += _T("Tbl col expression:") + sTblColExpr  +_T("\r\n");
        bRet = false;
        AfxMessageBox(sMsg);
        CStringPos sPos = pLinkTable->GetFullCommand();
        CArray<CStringPos*,CStringPos*>aComponents;
        aComponents.Add(&sPos);
        CString sTarget,sSource;
        MakeCrossTabStatement(pTable , sSource);

        TBL_PROC_INFO tblProcInfo;
        tblProcInfo.pTabDoc = this;
        tblProcInfo.pTable = pTable;
        tblProcInfo.sTblLogic = sSource;
        tblProcInfo.eEventType = CSourceCode_Tally;
        tblProcInfo.pLinkTable = pLinkTable;
        tblProcInfo.bGetLinkTables = false;

        if(AfxGetMainWnd()->SendMessage(UWM::Table::PutTallyProc, 0, (LPARAM)&tblProcInfo) == -1){
            AfxMessageBox(_T("Failed to put logic in App"));
        }
    }

    //Tab spec in zTableO should have stuff for include/exclude/weight/universe/logic
     //Try and get row expression stuff and try to match it with the spec

    CString sTabLogic = pLinkTable->GetTablogic().m_csText;
    //AfxMessageBox(sTabLogic);
    return bRet;
}


/////////////////////////////////////////////////////////////////////////////////
//
//  bool CTabulateDoc::MatchExpr(CString sTblString, sLinkString)
//
/////////////////////////////////////////////////////////////////////////////////
bool CTabulateDoc::MatchExpr(CIMSAString sTblString, CIMSAString sLinkString)
{
    bool bMatch = true;

    sTblString.Trim();
    sTblString.TrimLeft(_T(","));
    sTblString.TrimRight(_T(","));

    sLinkString.Trim();
    sLinkString.TrimLeft(_T(","));
    sLinkString.TrimRight(_T(","));
    CString sTblToken = sTblString;
    CString sLinkToken = sLinkString;
    if(sTblString.CompareNoCase(sLinkString) == 0 ) {
        return bMatch;
    }

    while(!sTblToken.IsEmpty())  {
        TCHAR cTFound = _T('\0');
        TCHAR cLFound = _T('\0');
        sTblToken = sTblString.GetToken(_T(",*+()"),&cTFound);
        sLinkToken = sLinkString.GetToken(_T(",*+()"),&cLFound);
        if(cLFound != cTFound){
            bMatch = false;
            break;
        }
        sTblToken.Trim();
        sLinkToken.Trim();
        if(sTblToken.CompareNoCase(sLinkToken) != 0){
            bMatch = false;
            break;
        }
        sTblString.Trim();
        sLinkString.Trim();
        if(sLinkString.IsEmpty() && sTblString.IsEmpty())
            return true;
    }

    return bMatch;
}

/////////////////////////////////////////////////////////////////////////////////
//
//  CFmtBase*  CTabulateDoc::GetFormat(CTblOb* pTblOb)
//
/////////////////////////////////////////////////////////////////////////////////
/*const CFmtBase*  CTabulateDoc::GetFormat(CTblOb* pTblOb)
{
    const CFmtBase* pFmt = pTblOb->GetFmt();
    if(!pFmt) {
        if(pTblOb->IsKindOf(RUNTIME_CLASS(CTable))){
            pFmt = this->GetTableSpec()->GetFmtReg().GetFmt(FMT_ID_TABLE);
        }
        else if(pTblOb->IsKindOf(RUNTIME_CLASS(CTabVar))){
            pFmt = this->GetTableSpec()->GetFmtReg().GetFmt(FMT_ID_SPANNER);//What about ROW/COL stuff
            //for now I am sending in COL .
        }
        else if(pTblOb->IsKindOf(RUNTIME_CLASS(CDataCell))){
            pFmt = this->GetTableSpec()->GetFmtReg().GetFmt(FMT_ID_DATACELL);
        }
    }
    return pFmt;
}
*/
bool CTabulateDoc::GetUnitStatement(CTable* pTable,CString& sUnitStatement)
{
    bool bRet = false;
    CArray<CUnitSpec,CUnitSpec&>& arrUnitSpec = pTable->GetUnitSpecArr();
    for(int iIndex = 0 ; iIndex <arrUnitSpec.GetSize();iIndex++){
        CUnitSpec& unitSpec = arrUnitSpec[iIndex];
        if(unitSpec.IsUnitPresent()){
            bRet = true;
            sUnitStatement += _T("\r\n UNIT (") + unitSpec.GetSubTableString()+_T(")");
            if(!unitSpec.GetLoopingVarName().IsEmpty()){
                sUnitStatement += _T(" ( ") + unitSpec.GetLoopingVarName()+ _T(" ") ;
            }
            //Do Value stuff here ??
            if(!unitSpec.GetWeightExpr().IsEmpty() || !unitSpec.GetValue().IsEmpty()){
                CString sValue = unitSpec.GetValue();
                sValue.Trim();
                CString sWeight = unitSpec.GetWeightExpr();
                sWeight.Trim();
                if(!sValue.IsEmpty() && !sWeight.IsEmpty()){
                    sValue = _T("( ") + sValue + _T(" ) *");
                    sWeight = _T("( ") + sWeight + _T(" )");
                    sUnitStatement += _T(" WEIGHTED BY ") + sValue + sWeight;
                }
                else {
                    sUnitStatement += _T(" WEIGHTED BY ") + sValue + sWeight ;
                }
            }
            if(!unitSpec.GetUniverse().IsEmpty()){
                sUnitStatement += _T(" SELECT ") + unitSpec.GetUniverse() ;
            }
            sUnitStatement += _T(")\r\n");
        }
    }
    const CUnitSpec& unitSpec = pTable->GetTableUnit();
    if(!unitSpec.GetLoopingVarName().IsEmpty()){
        bRet = true;
        sUnitStatement += _T("UNIT ( ") + unitSpec.GetLoopingVarName()+ _T(" )\r\n") ;
    }
    //Do Value stuff here ??
    if(!unitSpec.GetWeightExpr().IsEmpty() || !unitSpec.GetValue().IsEmpty()){
        bRet  = true;
        CString sValue = unitSpec.GetValue();
        sValue.Trim();
        CString sWeight = unitSpec.GetWeightExpr();
        sWeight.Trim();
        if(!sValue.IsEmpty() && !sWeight.IsEmpty()){
            sValue = _T("( ") + sValue + _T(" ) *");
            sWeight = _T("( ") + sWeight + _T(" )");
            sUnitStatement += _T(" WEIGHTED BY ") + sValue + sWeight +_T("\r\n");
        }
        else {
            sUnitStatement += _T(" WEIGHTED BY ") + sValue + sWeight +_T("\r\n");
        }
    }
    if(!unitSpec.GetUniverse().IsEmpty()){
        bRet = true;
        sUnitStatement += _T(" SELECT ") + unitSpec.GetUniverse()+_T("\r\n");
    }
    return bRet;
}

/////////////////////////////////////////////////////////////////////////////////
//
//  CString CTabulateDoc::MakeVarList4DummyVSet (const CDictItem* pDictItem)
//
/////////////////////////////////////////////////////////////////////////////////
CString CTabulateDoc::MakeVarList4DummyVSet(const CDictItem* pDictItem)
{
    CString sVarList;
    sVarList = pDictItem->GetName() ;
    // It's an item with no value sets
    int len = pDictItem->GetLen();
    int iVals = (len > 1 ? 11 : 10);
    CString sValue;
    for(int iIndex =0;iIndex < iVals ; iIndex++ ){
        if (len > 1) {
            if (iIndex == 0) {
                sVarList += _T("[");
//              sValue = "< 0";
                //serpro's syntax does not take negative values
                continue;
            }
            else {
                TCHAR pszTemp[20];
                GetPrivateProfileString(_T("intl"), _T("sDecimal"), _T("."), pszTemp, 10, _T("WIN.INI"));
                TCHAR cDec = pszTemp[0];

                CString csFrom(_T('0'), len);
                CString csTo(_T('9'), len);
                csFrom.SetAt(0, (TCHAR)(_T('0') + iIndex - 1));
                csTo.SetAt(0, (TCHAR)(_T('0') + iIndex - 1));

                UINT uDec = pDictItem->GetDecimal();
                if (uDec > 0 && pDictItem->GetDecChar()) {
                    csFrom.SetAt(len - uDec - 1, pszTemp[0]);
                    csTo.SetAt(len - uDec - 1, pszTemp[0]);
                }
                double dTemp = atod(csFrom, uDec);
                csFrom = dtoa(dTemp, pszTemp, uDec, cDec, false);
                dTemp = atod(csTo, uDec);
                csTo = dtoa(dTemp, pszTemp, uDec, cDec, false);
                sValue += _T("(");
                sValue += csFrom;
                sValue += _T(":");
                sValue += csTo;
                sValue += _T("),");
                sVarList += sValue;
            }
        }
        else {
            TCHAR pszTemp[20];
            GetPrivateProfileString(_T("intl"), _T("sDecimal"), _T("."), pszTemp, 10, _T("WIN.INI"));
            TCHAR cDec = pszTemp[0];

            CString csFrom(_T('0'), len);
            csFrom.SetAt(0, (TCHAR)(_T('0') + iIndex));

            UINT uDec = pDictItem->GetDecimal();
            if (uDec > 0 && pDictItem->GetDecChar()) {
                csFrom.SetAt(len - uDec - 1, pszTemp[0]);
            }
            double dTemp = atod(csFrom, uDec);
            csFrom = dtoa(dTemp, pszTemp, uDec, cDec, false);
            sValue = csFrom;
            sVarList += sValue +_T(",");
        }
        if(!sValue.IsEmpty()){
            sValue=_T("");
        }
    }
    sVarList.TrimRight(_T(","));
    sVarList+=_T("]");
    return sVarList;
}

/////////////////////////////////////////////////////////////////////////////////
//
//  void CTabulateDoc::MakeNameMap(CTable* pTable)
//
/////////////////////////////////////////////////////////////////////////////////
void CTabulateDoc::MakeNameMap(CTable* pTable)
{
    m_varNameMap.RemoveAll();
    CTabVar* pRowRoot = pTable->GetRowRoot();
    CMapStringToString  arrNames;
    MakeNameMap(pRowRoot , arrNames);
    CTabVar* pColRoot = pTable->GetColRoot();
    MakeNameMap(pColRoot , arrNames);
    return;
}
/////////////////////////////////////////////////////////////////////////////////
//
//  void CTabulateDoc::MakeNameMap(CTabVar* pTabVar, CMapStringToString& arrNames)
//
/////////////////////////////////////////////////////////////////////////////////
void CTabulateDoc::MakeNameMap(CTabVar* pTabVar, CMapStringToString& arrNames)
{
    CString sName = pTabVar->GetName();
    sName.Trim();
    if(!sName.IsEmpty()){
        sName.MakeUpper();
        CIMSAString sKeyVal;
        int iVal = -1;
        if(arrNames.Lookup(sName,sKeyVal)){
            iVal = (int)sKeyVal.Val();
            CString sVal = IntToString(iVal+1);
            CString sNewName;
            sNewName = sName+_T("(")+sVal+_T(")");
            m_varNameMap[pTabVar]=sNewName;
            arrNames[sName] =sVal;
            //look for the var which has this name and make it name(0)
            POSITION pos = m_varNameMap.GetStartPosition();
            while(pos != nullptr) {
                CTabVar* pKeyVar = nullptr;
                CString sAssocName;
                m_varNameMap.GetNextAssoc(pos,pKeyVar,sAssocName);
                if(sAssocName.CompareNoCase(sName) ==0){
                    m_varNameMap.SetAt(pKeyVar,sName+_T("(1)"));
                    break;
                }
            }
        }
        else {
            arrNames[sName] = _T("1");
            m_varNameMap[pTabVar]=sName;
        }
    }
    for(int iIndex =0; iIndex < pTabVar->GetNumChildren(); iIndex++){
        CTabVar* pChild  =pTabVar->GetChild(iIndex);
        MakeNameMap(pChild, arrNames);
    }
    return ;
}
void CTabulateDoc::CloseAreaNameFile()
{
    return m_TableSpec->CloseAreaNameFile();
}

/////////////////////////////////////////////////////////////////////////////////
//
//  bool CTabulateDoc::OpenAreaNameFile()
//
/////////////////////////////////////////////////////////////////////////////////
bool CTabulateDoc::OpenAreaNameFile()
{
    //SAVY moved the code to zTableO
    return m_TableSpec->OpenAreaNameFile(m_sAreaFileName);
}

/////////////////////////////////////////////////////////////////////////////////
//
//  bool CTabulateDoc::BuildAreaLookupMap(void)
//
/////////////////////////////////////////////////////////////////////////////////
bool CTabulateDoc::BuildAreaLookupMap(void)
{
    return m_TableSpec->BuildAreaLookupMap();
}

bool CTabulateDoc::Reconcile(CString& sError, bool bSilent /* = false */, bool bAutoFix /* = false */)
{
    // reconcile tab spec
    bool bResult =  false; //Indicates if something has changed
    for (int iTbl=0 ; iTbl<m_TableSpec->GetNumTables() ; iTbl++)  {
        m_TableSpec->GetTable(iTbl)->SetReconcileErrMsg(_T(""));
    }
    bResult = m_TableSpec->Reconcile(sError, bSilent, bAutoFix);

    // need to also check syntax of weights and universe - can't do that from inside table spec
    // since syntax check API needs to be at UI level.
    if(m_TableSpec->GetDict()) { //If the data dict is available  then it is reconcile for the designer
        //Do Reconcile Tables
        for (int iTbl=0 ; iTbl<m_TableSpec->GetNumTables() ; iTbl++)  {
            CTable* pTbl = m_TableSpec->GetTable(iTbl);
            if(pTbl->GetRowRoot()->GetNumChildren() ==0 ||pTbl->GetColRoot()->GetNumChildren() ==0 ){
                continue; //empty table nothing to reconcile
            }
            CString sTblMsg ;
            pTbl->GetReconcileErrMsg().IsEmpty() ? sTblMsg=pTbl->GetName()+_T(" :\r\n") : sTblMsg=pTbl->GetReconcileErrMsg();
            if (GetTableSpec()->GetGenLogic() && !ReconcileUniverseAndWeights(pTbl, sTblMsg)) {
                sError += sTblMsg;
                pTbl->SetReconcileErrMsg(sTblMsg);
                bResult = true;
            }
            else if(!pTbl->GetReconcileErrMsg().IsEmpty()){
                sError += pTbl->GetReconcileErrMsg()+_T("\r\n");
            }
        }

        if(!bSilent){
            if(!sError.IsEmpty()){
                AfxMessageBox(sError);
            }
        }
    }
    for (int iTbl=0 ; iTbl<m_TableSpec->GetNumTables() ; iTbl++)  {
        m_TableSpec->GetTable(iTbl)->SetReconcileErrMsg(_T(""));
    }
    return (bResult);
}


/////////////////////////////////////////////////////////////////////////////////
//
//  bool CTabulateDoc::ReconcileUniverseAndWeights(CTable* pTbl, CString& sError)
//
/////////////////////////////////////////////////////////////////////////////////
bool CTabulateDoc::ReconcileUniverseAndWeights(CTable* pTable, CString& sError)
{
    bool bRet = true;
    CArray<CStringArray,CStringArray&> arrValidSubtableUnitNames;
    CStringArray arrValidTableUnitNames;

    GetTableSpec()->UpdateSubtableList(pTable, arrValidSubtableUnitNames, arrValidTableUnitNames,
                false,nullptr,nullptr,nullptr);

    if(pTable->GetTableUnit().GetLoopingVarName().IsEmpty() && pTable->GetTableUnit().GetUseDefaultLoopingVar() && arrValidTableUnitNames.GetSize() > 0){
        pTable->GetTableUnit().SetLoopingVarName(arrValidTableUnitNames.GetAt(0));
    }
    // Table
    if (!pTable->GetTableUnit().GetUniverse().IsEmpty()) {
        if (!CheckUnitSyntax(pTable, pTable->GetTableUnit(), XTABSTMENT_UNIV_ONLY, true)) {
            sError += _T("Universe for table ") + pTable->GetName() + _T(" no longer valid.  Removing universe statement.\n");
            pTable->GetTableUnit().SetUniverse(_T(""));
            bRet = false;
        }
    }

    if (!pTable->GetTableUnit().GetWeightExpr().IsEmpty() ||
        !pTable->GetTableUnit().GetValue().IsEmpty() ) {
        if (!CheckUnitSyntax(pTable, pTable->GetTableUnit(), XTABSTMENT_WGHT_ONLY, true)) {
            sError += _T("Weight for table ") + pTable->GetName() + _T(" no longer valid.  Removing weight statement.\n");
            pTable->GetTableUnit().SetWeightExpr(_T(""));
            pTable->GetTableUnit().SetValue(_T(""));
            bRet = false;
        }
    }

    // subtables
    for (int i = 0; i < pTable->GetUnitSpecArr().GetCount(); ++i) {

        CUnitSpec subtableUnit = pTable->GetUnitSpecArr()[i];
        if(subtableUnit.GetLoopingVarName().IsEmpty() && subtableUnit.GetUseDefaultLoopingVar()){
            subtableUnit.SetLoopingVarName(arrValidSubtableUnitNames[i].GetAt(0));
        }
        // Table
        if (!subtableUnit.GetUniverse().IsEmpty()) {
            if (!CheckUnitSyntax(pTable, subtableUnit, XTABSTMENT_UNIV_ONLY, false)) {
                sError += _T("Universe for table ") + pTable->GetName() + _T(" subtable ") + subtableUnit.GetSubTableString() + _T(" no longer valid.  Removing universe statement.\n");
                pTable->GetUnitSpecArr()[i].SetUniverse(_T(""));
                bRet = false;
            }
        }

        if (!subtableUnit.GetWeightExpr().IsEmpty() || !subtableUnit.GetValue().IsEmpty()) {
            if (!CheckUnitSyntax(pTable, subtableUnit, XTABSTMENT_WGHT_ONLY, false)) {
                sError += _T("Weight for table ") + pTable->GetName() + _T(" subtable ") + subtableUnit.GetSubTableString() + _T(" no longer valid.  Removing weight statement.\n");
                pTable->GetUnitSpecArr()[i].SetWeightExpr(_T(""));
                pTable->GetUnitSpecArr()[i].SetValue(_T(""));
                bRet = false;
            }
        }
    }

    return bRet;
}

/////////////////////////////////////////////////////////////////////////////////
//
//  bool CTabulateDoc::CheckUnitSyntax
//
// Check syntax for unit, universe, weight statement in table.  Returns true if
// syntax is legit.
//
/////////////////////////////////////////////////////////////////////////////////
bool CTabulateDoc::CheckUnitSyntax(CTable* pTable, CUnitSpec& unit, XTABSTMENT_TYPE eStatementType, bool bTable) // tbl or subtable unit
{
    // save off old units
    CArray<CUnitSpec, CUnitSpec&>  arrOldUnitSpec;
    arrOldUnitSpec.Append(pTable->GetUnitSpecArr());
    CUnitSpec oldTblUnitSpec = pTable->GetTableUnit();

    // clear out all units
    pTable->GetUnitSpecArr().RemoveAll();

    // add in unit to check
    if (bTable) {
        pTable->GetTableUnit() = unit;
    }
    else {
        pTable->GetTableUnit() = CUnitSpec();
        if(pTable->GetTableUnit().GetLoopingVarName().IsEmpty()){
            pTable->GetTableUnit().SetLoopingVarName(oldTblUnitSpec.GetLoopingVarName());
        }
        pTable->GetUnitSpecArr().Add(unit);
    }

    //compile the table with new unit stuff
    //Make the crosstab statement with just the universe .
    //Send for compile .If the compile fails . Bring the dialog back ?
    //Follow the flow in OnTblCompile;
    TableElementTreeNode table_element_tree_node(this, pTable);

    // do the check
    bool bRet = true;
    if(AfxGetMainWnd()->SendMessage(UWM::Table::CheckSyntax, static_cast<WPARAM>(eStatementType), reinterpret_cast<LPARAM>(&table_element_tree_node)) == -1) {
        bRet = false;
    }

    // restore old units
    pTable->GetTableUnit() = oldTblUnitSpec;
    pTable->GetUnitSpecArr().RemoveAll();
    pTable->GetUnitSpecArr().Append(arrOldUnitSpec);

    return bRet;
}

bool CTabulateDoc::SyntaxCheckPasteTbl(CTable* pTable, CString& sError)
{
    //Call update subtable list to get the unit stuff poopulated correctly
    CArray<CStringArray,CStringArray&> arrValidSubtableUnitNames;
    CStringArray arrValidTableUnitNames;
    GetTableSpec()->UpdateSubtableList(pTable, arrValidSubtableUnitNames, arrValidTableUnitNames,
                false,nullptr,nullptr,nullptr);
    m_arrUnitSpecForPasteTblChk.RemoveAll();
    CArray<CUnitSpec,CUnitSpec&>&arrUnits = pTable->GetUnitSpecArr();

            //Update units
    if(arrValidSubtableUnitNames.GetSize() > 0  || arrValidTableUnitNames.GetSize() > 0){
        CUnitSpec copyOfUnitspec = pTable->GetTableUnit();
        m_arrUnitSpecForPasteTblChk.Add(copyOfUnitspec);
        for(int iIndex =0;iIndex< arrUnits.GetSize();iIndex++){
            CUnitSpec copyOfUnitspec2 = arrUnits[iIndex];
            m_arrUnitSpecForPasteTblChk.Add(copyOfUnitspec2);
        }
    }
    return CheckSyntaxAll(pTable,sError);
}

bool CTabulateDoc::CheckSyntaxAll(CTable* pTable, CString& sError)
{
    CTabulateDoc* pDoc = this;
    // make sure that if tbl has units, subtables don't and vice versa
    if(!CheckSyntax(pTable,0, XTABSTMENT_POSTCALC_ONLY,sError)){
        CString sMsg = _T("*** PostCalc ***\r\n");
        sMsg +=  pDoc->GetErrorString();
       //AfxMessageBox(sMsg);
        sError += sMsg;

        return false;
    }
    if(!CheckSyntax(pTable,0, XTABSTMENT_TABLOGIC_ONLY,sError)){
        CString sMsg = _T("*** Tablogic ***\r\n");
        sMsg +=  pDoc->GetErrorString();
        //AfxMessageBox(sMsg);
        sError += sMsg;
        return false;
    }
    bool bSubTblHasUnits = false;
    if (true) {
        // table is selected, check all subtables
        for (int i = 1; i < m_arrUnitSpecForPasteTblChk.GetCount(); ++i) {
            CUnitSpec* pSubTblSpec = &m_arrUnitSpecForPasteTblChk[i];
            if (pSubTblSpec->IsUnitPresent()) {
                bSubTblHasUnits = true;
                break;
            }
        }
    }
    if(!CheckSyntaxSubtable(pTable,0,sError)){
        return false;
    }

    return true;
}

/////////////////////////////////////////////////////////////////////////////////
//
//  bool CTabulateDoc::CheckSyntaxSubtable(int iSubtable)
//
/////////////////////////////////////////////////////////////////////////////////
bool CTabulateDoc::CheckSyntaxSubtable(CTable *pTable,int iSubtable,CString& sError)
{
    if(!CheckSyntax(pTable,iSubtable, XTABSTMENT_UNITS_ONLY,sError)){
        CString sMsg = _T("*** Unit ***\r\n");
        sMsg +=  this->GetErrorString();
        //AfxMessageBox(sMsg);
        sError += sMsg;
        return false;
    }
    if(!CheckSyntax(pTable,iSubtable, XTABSTMENT_WGHT_ONLY,sError)){
        CString sMsg = _T("*** Value or Weight ***\r\n");
        sMsg +=  this->GetErrorString();
        sError += sMsg;

        //AfxMessageBox(sMsg);
        return false;
    }
    if(!CheckSyntax(pTable,iSubtable, XTABSTMENT_UNIV_ONLY,sError)){
        CString sMsg = _T("*** Universe ***\r\n");
        sMsg +=  this->GetErrorString();
        sError += sMsg;
        //AfxMessageBox(sMsg);
        return false;
    }
    if(!CheckSyntax(pTable,iSubtable, XTABSTMENT_TABLOGIC_ONLY,sError)){
        CString sMsg = _T("*** Tablogic ***\r\n");
        sMsg +=  this->GetErrorString();
        //AfxMessageBox(sMsg);
        sError += sMsg;
        return false;
    }
    return true;
}

bool CTabulateDoc::CheckSyntax(CTable* pTable,int iSubtable, XTABSTMENT_TYPE eStatementType,CString& /*sError*/)
{
    bool bRet = false;
    CTabulateDoc* pDoc = this;
    pDoc->SetErrorString();
    //Store current table units in temp
    CArray<CUnitSpec, CUnitSpec&>  arrTempUnitSpec;
    arrTempUnitSpec.Append(pTable->GetUnitSpecArr());
    CUnitSpec tempTblUnitSpec = pTable->GetTableUnit();

    CStringArray arrTempPostCalc;
    arrTempPostCalc.Append(pTable->GetPostCalcLogic());
    CUnitSpec unitSpec = m_arrUnitSpecForPasteTblChk[iSubtable];
    //CUnitSpec* pTableUnitSpec = &((CUnitSpec)m_arrUnitSpecForPasteTblChk[0]);



    if(eStatementType == XTABSTMENT_UNIV_ONLY){
        unitSpec.SetWeightExpr(_T(""));
        unitSpec.SetValue(_T(""));

        if(unitSpec.GetUniverse().IsEmpty()){
            return true;
        }
    }
    else if(eStatementType == XTABSTMENT_WGHT_ONLY){
        unitSpec.SetUniverse(_T(""));

        if(unitSpec.GetWeightExpr().IsEmpty() && unitSpec.GetValue().IsEmpty()){
            return true;
        }
    }
    else if(eStatementType == XTABSTMENT_TABLOGIC_ONLY){
        unitSpec.SetUniverse(_T(""));
        unitSpec.SetWeightExpr(_T(""));
        unitSpec.SetValue(_T(""));


        CStringArray& arrTabLogic  = unitSpec.GetTabLogicArray();

        if(arrTabLogic.IsEmpty()){
            return true;
        }
    }
    else if(eStatementType == XTABSTMENT_POSTCALC_ONLY){
        unitSpec.SetUniverse(_T(""));
        unitSpec.SetWeightExpr(_T(""));
        unitSpec.SetValue(_T(""));

        if(pTable->GetPostCalcLogic().IsEmpty()){
            return true;
        }
    }
   else if(eStatementType == XTABSTMENT_UNITS_ONLY){
        unitSpec.SetUniverse(_T(""));
        unitSpec.SetWeightExpr(_T(""));
        unitSpec.SetValue(_T(""));
    }
    else {
        ASSERT(FALSE);
    }

    //use the dialog's unit specs for compilation
    pTable->GetUnitSpecArr().RemoveAll();

    if (iSubtable == 0) {
       // entire table
       pTable->GetTableUnit() = unitSpec;
    }
    else {
        // current subtable
        pTable->GetUnitSpecArr().Add(unitSpec);
    }

    //compile the table with new unit stuff
    //Make the crosstab statement with just the universe .
    //Send for compile .If the compile fails . Bring the dialog back ?
    //Follow the flow in OnTblCompile;
    TableElementTreeNode table_element_tree_node(pDoc, pTable);

    if(AfxGetMainWnd()->SendMessage(UWM::Table::CheckSyntax, static_cast<WPARAM>(eStatementType), reinterpret_cast<LPARAM>(&table_element_tree_node)) == -1){
//        AfxMessageBox("Invalid Universe Syntax");
        bRet = false;
    }
    else {
        bRet = true;
    }
    //reset the unit stuff of table back to its original unit specs
    pTable->GetUnitSpecArr().RemoveAll();
    pTable->GetUnitSpecArr().Append(arrTempUnitSpec);
    arrTempUnitSpec.RemoveAll();
    pTable->GetTableUnit() = tempTblUnitSpec;
    pTable->GetPostCalcLogic().RemoveAll();
    pTable->GetPostCalcLogic().Append(arrTempPostCalc);
    return bRet;
}
