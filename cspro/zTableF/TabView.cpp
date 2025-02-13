// TabView.cpp : implementation file
//

#include "StdAfx.h"
#include "TabView.h"
#include "AppFmtD.h"
#include "CmpFmtD.h"
#include "TabChWnd.h"
#include "TabDoc.h"
#include "TblFmtD.h"
#include "TblPFmtD.h"
#include "TlyVrDlg.h"
#include "TTallyFD.h"
#include "Tvdlg.h"
#include <zDictF/UWM.h>
#include <zTableO/TllyStat.h>
#include <tblview/TblView.h>
#include <zUtilO/BCMenu.h>
#include <zUtilO/Filedlg.h>
#include <strstream>
#include <sstream>
#include <fstream>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

const int INVALID_ROW_COL = -9999;
/////////////////////////////////////////////////////////////////////////////
// CTabView
#define TABTOOLBAR     997         //this is defined in the CSPro CMainFrame class create
#define MAPNAME_CONCATSTR       _T("; ")

IMPLEMENT_DYNCREATE(CTabView, CView)

CTabView::CTabView()
{
   m_pGrid = new CTblGrid();
   m_iCurrSelArea = 0;
   m_iVertScrollPos =-1;
   m_iHorzScrollPos=-1;
}

CTabView::~CTabView()
{
    if (m_pGrid) {
        m_pGrid->DestroyWindow();
        delete  m_pGrid;
    }
}


BEGIN_MESSAGE_MAP(CTabView, CView)
    //{{AFX_MSG_MAP(CTabView)
    ON_WM_SIZE()
    ON_COMMAND(ID_ADD_TABLE, OnAddTable)
    ON_UPDATE_COMMAND_UI(ID_ADD_TABLE, OnUpdateAddTable)
    ON_COMMAND(ID_DELETE_TABLE, OnDeleteTable)
    ON_UPDATE_COMMAND_UI(ID_DELETE_TABLE, OnUpdateDeleteTable)
    ON_COMMAND(ID_INSERT_TABLE, OnInsertTable)
    ON_UPDATE_COMMAND_UI(ID_INSERT_TABLE, OnUpdateInsertTable)
    ON_COMMAND(ID_TOGGLE_TREE, OnToggleTree)
    ON_UPDATE_COMMAND_UI(ID_TOGGLE_TREE, OnUpdateToggleTree)
    ON_COMMAND(ID_TBL_RUN, OnTblRun)
    ON_UPDATE_COMMAND_UI(ID_TBL_RUN, OnUpdateTblRun)
    ON_COMMAND(ID_RUN_PCS, OnRunPcs)
    ON_UPDATE_COMMAND_UI(ID_RUN_PCS, OnUpdateRunPcs)
    ON_COMMAND(ID_FILE_SAVE_TABLE, OnFileSaveTable)
    ON_UPDATE_COMMAND_UI(ID_FILE_SAVE_TABLE, OnUpdateFileSaveTable)
    ON_WM_RBUTTONUP()

    ON_COMMAND_EX(ID_EDIT_VAR_TALLYATTRIB, OnEditVarTallyAttributes)
    ON_COMMAND_EX(ID_EDIT_VAR_TALLYATTRIB1,OnEditVarTallyAttributes)
    ON_COMMAND(ID_EDIT_TBL_TALLYATTRIB, OnEditTableTallyAttributes)
    ON_COMMAND(ID_EDIT_SUBTBL_TALLYATTRIB, OnEditSubTableTallyAttributes)

    ON_COMMAND(ID_EDIT_COMPONENT_FMT, OnEditTblCompFmt)
    ON_COMMAND(ID_EDIT_TBL_FMT, OnEditTableFmt)
    ON_COMMAND(ID_EDIT_APP_FMT, OnEditAppFmt)

    ON_COMMAND(ID_EDIT_SPLIT_SPANNERS, OnEditTblSplitSpanners)
    ON_COMMAND(ID_EDIT_JOIN_SPANNERS, OnEditTblJoinSpanners)

    ON_COMMAND(ID_EDIT_TBL_PRINTFMT, OnEditTablePrintFmt)

    ON_COMMAND(ID_DESELECTALL, OnDeselectAll)
    ON_COMMAND(ID_SELECTALL, OnSelectAll)
    ON_COMMAND(ID_SHIFT_F10, OnShiftF10)
    ON_WM_MOUSEMOVE()
    //}}AFX_MSG_MAP
      ON_MESSAGE(WM_IMSA_DROPITEM,OnDropItem)
      ON_MESSAGE(UWM::Dictionary::DraggedOffTable, OnDictDroppedOn)
      ON_COMMAND(ID_AREA, OnArea)
      ON_UPDATE_COMMAND_UI(ID_AREA, OnUpdateArea)
      ON_COMMAND(ID_EDIT_COPY, OnEditCopy)
      ON_UPDATE_COMMAND_UI(ID_EDIT_COPY, OnUpdateEditCopy)
      ON_COMMAND(ID_EDIT_COPYCELLSONLY, OnEditCopycellsonly)
      ON_UPDATE_COMMAND_UI(ID_EDIT_COPYCELLSONLY, OnUpdateEditCopy)
      ON_CBN_SELCHANGE(ID_AREA_COMBO, OnSelChangeAreaComboBox)
      ON_COMMAND(ID_FILE_PRINT, OnFilePrint)
      ON_COMMAND(ID_EDIT_COPYTABLESTRUCTURE, &CTabView::OnEditCopytablestructure)
      ON_UPDATE_COMMAND_UI(ID_EDIT_COPYTABLESTRUCTURE, &CTabView::OnUpdateEditCopytablestructure)
      ON_COMMAND(ID_EDIT_PASTETABLE, &CTabView::OnEditPastetable)
      ON_UPDATE_COMMAND_UI(ID_EDIT_PASTETABLE, &CTabView::OnUpdateEditPastetable)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTabView drawing

void CTabView::OnDraw(CDC* /*pDC*/)
{
    //CDocument* pDoc = GetDocument();
    // TODO: add draw code here
}

/////////////////////////////////////////////////////////////////////////////
// CTabView diagnostics

#ifdef _DEBUG
void CTabView::AssertValid() const
{
    CView::AssertValid();
}

void CTabView::Dump(CDumpContext& dc) const
{
    CView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CTabView message handlers


LRESULT CTabView::OnDropItem(WPARAM wParam,LPARAM lParam)
{
    if (!IsDropValid(wParam, lParam)) {
        return 0;
    }

    const DictLevel* pDictLevel = nullptr;
    const CDictRecord* pDictRecord = nullptr;
    const CDictItem* pDictItem = nullptr;
    const DictValueSet* pDictVSet = nullptr;
    CTabVar* pSrcItem = nullptr;
    CIMSAString csName, csLabel;
    CRect rect;
    CTabulateDoc* pDoc = GetDocument();
    CTabSet* pSpec = pDoc->GetTableSpec();

    CTable* pTable = m_pGrid->GetTable();
    bool bGridVarDrop = m_pGrid->IsGridVarDrop();
    /////////////////////////////////////////////////////////////////////////////
    // get dictionary item information from lParam
    DictTreeNode* dict_tree_node = reinterpret_cast<DictTreeNode*>(lParam);

    if(dict_tree_node == nullptr || !bGridVarDrop && !IsDropValidDictItem(*dict_tree_node))
        return 0;

    DICT_LOOKUP_INFO lookupInfo;
    if(!bGridVarDrop && !GetDictInfo(*dict_tree_node, lookupInfo)){
        AfxMessageBox(_T("Failed to retrieve dictionary item information"));
        ASSERT(FALSE); //This shld never happen .
        return 0;
    }
    if(bGridVarDrop){
        pSrcItem = m_pGrid->GetDragSourceItem();
    }
    else {
        m_pGrid->SetDragSourceItem(nullptr);
        pDictItem = lookupInfo.pItem;
    }

    if(pDictItem){
        if(pDictItem->GetName().CompareNoCase(WORKVAR_TOTAL_NAME) ==0){
            AfxMessageBox(FormatText(_T("Cannot Drop %s"), (LPCTSTR)WORKVAR_TOTAL_NAME));
            return 0;
        }
        if(pDictItem->GetContentType() != ContentType::Numeric){
            AfxMessageBox(_T("Can only drop numeric variables"));
            return 0;
        }
    }
    pDictVSet = lookupInfo.pVSet;

    // get point at which dropped from wParam
    CPoint point;
    point.x = LOWORD(wParam);
    point.y = HIWORD(wParam);

    /////////////////////////////////////////////////////////////////////////////
    // make sure point is inside the view
    GetWindowRect(&rect);
    if (!rect.PtInRect(point)) {
        return 0;
    }


    int iGridCol;
    long iGridRow;

    iGridCol = iGridRow =0;
    m_pGrid->ScreenToClient(&point);
    point.x = point.x - m_pGrid->m_GI->m_sideHdgWidth;
    point.y = point.y - m_pGrid->m_GI->m_topHdgHeight;


    TB_DROP_TYPE eDropType  = GetDropType(point);
    TB_DROP_OPER eDropOper  = GetDropOperation(point);
    CTabVar* pTargetTabVar = GetTargetItem(point);
    if(!pTargetTabVar)
        return 0;
    else if(bGridVarDrop){
        if(pTargetTabVar == pSrcItem || pTargetTabVar->GetParent() == pSrcItem){
            return 0;
        }
    }
    bool bAfter  = true;
    if(IsStubDrop(point)){
        bAfter = false;
    }
    if( !m_pGrid->IsGridVarDrop()){
        if(!DoMultiRecordChk(lookupInfo.pRecord,pTargetTabVar,eDropType,eDropOper)){
            CString sMultErr;
            sMultErr.Format(IDS_MULT_ERR, (LPCTSTR)lookupInfo.csName);
            AfxMessageBox(sMultErr);
            return 0;
        }
        if(!DoMultiItemChk(lookupInfo.pItem,pTargetTabVar,eDropType,eDropOper)){
            CString sMultOccErr;
            sMultOccErr.Format(IDS_OCCS_ERR, (LPCTSTR)lookupInfo.csName);
            AfxMessageBox(sMultOccErr);
            return 0;
        }
    }
    else {
        const CDataDict* pDict = pSpec->LookupName(m_pGrid->GetDragSourceItem()->GetName(), &pDictLevel, &pDictRecord, &pDictItem, &pDictVSet);
        ASSERT(pDict);
        if(!DoMultiRecordChk(pDictRecord,pTargetTabVar,eDropType,eDropOper)){
            CString sMultErr;
            sMultErr.Format(IDS_MULT_ERR, (LPCTSTR)m_pGrid->GetDragSourceItem()->GetName());
            AfxMessageBox(sMultErr);
            return 0;
        }
        if(!DoMultiItemChk(pDictItem,pTargetTabVar,eDropType,eDropOper)){
            CString sMultOccErr;
            sMultOccErr.Format(IDS_OCCS_ERR, (LPCTSTR)m_pGrid->GetDragSourceItem()->GetName());
            AfxMessageBox(sMultOccErr);
            return 0;
        }
    }
    //Check not required any more .
    /*if(!bGridVarDrop && !pSpec->DoMultiLevelChk4Item(m_pGrid->GetTable(),dict_tree_node->GetLevel())){
        AfxMessageBox("Cannot drop items from different levels");
        return 0;
    }*/
    bool bModify = false;
    CIMSAString sUnitName4WkStgSubTable;
    CArray<CStringArray,CStringArray&> arrValidSubtableUnitNames;
    CStringArray arrValidTableUnitNames;
    //Do update Subtable list for getting the unitname for wkstg
    bool bCopyTableUnit2Subtable =false;
    GetDocument()->GetTableSpec()->UpdateSubtableList(pTable, arrValidSubtableUnitNames, arrValidTableUnitNames);
    if(pTable->GetUnitSpecArr().GetCount() <=1){
        bCopyTableUnit2Subtable =true;
    }
    if(!GetDocument()->GetTableSpec()->DoAllSubTablesHaveSameUnit(pTable,sUnitName4WkStgSubTable) || sUnitName4WkStgSubTable.IsEmpty()){
        int iLevel = pSpec->GetTableLevelFromUnits(pTable);
        sUnitName4WkStgSubTable = pSpec->GetDict()->GetLevel(iLevel).GetName();
    }
    CTabVar* pNewVarAdded = nullptr;
    if (eDropType == TB_DROP_ROW) {
        /*if(m_pGrid->GetTable()->GetRowRoot()->GetNumChildren()== 10){
            AfxMessageBox(IDS_MAXLIMIT);
            return 0;
        }*/

        switch( eDropOper) {
            case TB_DROP_PLUS:
            case TB_DROP_STARPLUS:
                pTargetTabVar->RemoveAllPrtViewInfo();
                pNewVarAdded = AddRowVarPlus(lookupInfo,pTargetTabVar,bAfter);
                pTargetTabVar->RemoveAllPrtViewInfoRecursive();
                bModify = true;
                break;
            case TB_DROP_STAR:
                m_pGrid->m_bReconcileSpecialCells= true;
                pTargetTabVar->RemoveAllPrtViewInfo();
                pNewVarAdded = AddRowVarStar(lookupInfo,pTargetTabVar,bAfter);
                pTargetTabVar->RemoveAllPrtViewInfoRecursive();
                bModify = true;
                break;
            case TB_DROP:
                pNewVarAdded = AddRowVarPlus(lookupInfo,pTargetTabVar,bAfter);
                bModify = true;
                break;
        }

    }
    else if (eDropType == TB_DROP_COL) {
        /*if(m_pGrid->GetTable()->GetColRoot()->GetNumChildren()== 10){
            AfxMessageBox(IDS_MAXLIMIT);
            return 0;
        }*/
        switch( eDropOper) {
            case TB_DROP_PLUS:
            case TB_DROP_STARPLUS:
                m_pGrid->m_bReconcileSpecialCells= true;
                pTargetTabVar->RemoveAllPrtViewInfo();
                pNewVarAdded = AddColVarPlus(lookupInfo,pTargetTabVar,bAfter);
                pTargetTabVar->RemoveAllPrtViewInfoRecursive();
                bModify = true;
                break;
            case TB_DROP_STAR:
                m_pGrid->m_bReconcileSpecialCells= true;
                pTargetTabVar->RemoveAllPrtViewInfo();
                pNewVarAdded = AddColVarStar(lookupInfo,pTargetTabVar,bAfter);
                pTargetTabVar->RemoveAllPrtViewInfoRecursive();
                bModify = true;
                break;
            case TB_DROP:
                m_pGrid->m_bReconcileSpecialCells= true;
                pNewVarAdded = AddColVarPlus(lookupInfo,pTargetTabVar,bAfter);
                bModify = true;
                break;
        }

    }
    else {
        return 0;
    }
    RemoveSystemTotalVar();
    AddSystemTotalVar();
    RefreshViewAfterDrop();
    if(bModify) {
      // reset units, weight and universe to defaults
        if(m_pGrid->GetTable()->GetRowRoot()->GetNumChildren() == 0 && m_pGrid->GetTable()->GetColRoot()->GetNumChildren() == 0){
            //Reset Table units
            //pDoc->GetTableSpec()->ResetTableUnits(m_pGrid->GetTable());
            CUnitSpec& unit = m_pGrid->GetTable()->GetTableUnit();
            //GSF made the decision use set the table back to default
            unit.SetLoopingVarName(_T(""));
            unit.SetUseDefaultLoopingVar(true);
        }
      pDoc->GetTableSpec()->ReconcileLevels4Tbl(m_pGrid->GetTable());
      RefreshLogicAfterDrop();
      CArray<CUnitSpec,CUnitSpec&>& arrUnitSpec = pTable->GetUnitSpecArr();
      //see if the subtable went from 1 to 2 and use the copytableunit...rule
      GetDocument()->GetTableSpec()->UpdateSubtableList(pTable, arrValidSubtableUnitNames, arrValidTableUnitNames);
      if(bCopyTableUnit2Subtable && pTable->GetUnitSpecArr().GetCount() > 1){
          if(!pTable->GetTableUnit().GetUseDefaultLoopingVar()){
              pTable->GetUnitSpecArr().GetAt(0).SetLoopingVarName(pTable->GetTableUnit().GetLoopingVarName());
              pTable->GetUnitSpecArr().GetAt(0).SetUseDefaultLoopingVar(pTable->GetTableUnit().GetUseDefaultLoopingVar());
          }
      }
      if (eDropType == TB_DROP_COL) {
          pSpec->UpdateSubtableListNMarkNewSubTables(pTable,nullptr,pNewVarAdded);//Get New subtables
      }
      else  if (eDropType == TB_DROP_ROW) {
          pSpec->UpdateSubtableListNMarkNewSubTables(pTable,pNewVarAdded);//Get New subtables
      }
      //Process working storage new subtables and reset the marked new to false

      int iNumUnits = arrUnitSpec.GetCount();
      for(int iIndex =0;iIndex < iNumUnits;iIndex++){
          CUnitSpec& unitSpec = arrUnitSpec[iIndex];
          if(unitSpec.IsMarkedNew()){
              unitSpec.SetNewMark(false);
              if(pSpec->IsSubTableFromWrkStg(pTable,unitSpec)){
                  unitSpec.SetUseDefaultLoopingVar(false);
                  unitSpec.SetLoopingVarName(sUnitName4WkStgSubTable);
              }

          }
      }
    }
    return 0;
}

void CTabView::OnInitialUpdate()
{
    CView::OnInitialUpdate();

    SetCurVar(nullptr);

    CTabulateDoc* pDoc = GetDocument();

    if (m_pGrid) {
        m_pGrid->DestroyWindow();
        delete m_pGrid;
    }
    m_pGrid = new CTblGrid();
//  m_pGrid->m_piCurrTable = &(pDoc->GetCurTblNum());

    CTabSet* pSpec = pDoc->GetTableSpec();
    m_pGrid->SetSpec (pSpec);

    CTable* pTable = nullptr;
    if(pSpec->GetNumTables() > 0){
        pTable = pDoc->GetTableSpec()->GetTable(0);
    }
    else {
        //SAVY &&& this crashes fix @ ztableo
        /*pTable = new CTable("0");
        pDoc->GetTableSpec()->AddTable(pTable);*/
    }
    m_pGrid->SetTable(pTable);

    RECT rect = {0,0,100,100};
    GetClientRect (&rect);
    m_pGrid->CreateGrid(WS_CHILD | WS_VISIBLE,rect,this,123);
    m_pGrid->SetParent(this);
    m_pGrid->MoveWindow(&rect);
    m_pGrid->Update();
    m_pGrid->SendMessage(WM_SIZE);
//  m_pGrid->SetNumberRows(5);
//  m_pGrid->SetNumberCols(5);

}

void CTabView::OnSize(UINT nType, int cx, int cy)
{
    CView::OnSize(nType, cx, cy);

    RECT rect;              // this + next 2 are UGrid suggested
    GetClientRect(&rect);
    if(m_pGrid->GetSafeHwnd()){
        m_pGrid->MoveWindow(&rect);
    }
}

bool CTabView::IsDropPointValid(CPoint point)
{
    CRect rect;

    /*CIMSAString sMsg;
    sMsg.Format("iX = %d , iY =%d\n" ,point.x,point.y);
    TRACE(sMsg);*/
    /////////////////////////////////////////////////////////////////////////////
    // if universe dialog box is showing, must be anywhere on it


    /////////////////////////////////////////////////////////////////////////////
    // if parameters dialog box is showing, must be on value or weight edit box

    /////////////////////////////////////////////////////////////////////////////
    // otherwise, point must be inside the view

    //Check how many variable are there in the table
    //if they are none then the point in rect scheme is valid
    CTable* pTable = m_pGrid->GetTable();
    bool bRowVarPresent = pTable->GetRowRoot()->GetNumChildren() >0 ;
    bool bColVarPresent = pTable->GetColRoot()->GetNumChildren() >0 ;

    GetWindowRect(&rect);
    bool bPointInRect = rect.PtInRect(point)?true:false;
    if (!bRowVarPresent || !bColVarPresent){
        /*int iVal =-1;
        !bPointInRect ? iVal = 0 :iVal = 1;
        TRACE("Pt in Rect is %d \n",iVal);*/
        return bPointInRect;
    }
    else if(bPointInRect){
        //check if the point is in the row or column stuff
        //Get col cell
        RECT rect;
        int iHeaderRows = m_pGrid->GetNumHeaderRows();
        if(m_pGrid->GetNumGridRows() > iHeaderRows) {
               m_pGrid->GetCellRect(0,iHeaderRows,&rect);
        }
        else {
            m_pGrid->GetCellRect(0,0,&rect);
        }
        CPoint localPoint = point;
        m_pGrid->ScreenToClient(&localPoint);
        //SAVY&&& Check if you have to transform for valid point
        /*localPoint.x = localPoint.x - m_pGrid->m_GI->m_sideHdgWidth;
        localPoint.y = localPoint.y - m_pGrid->m_GI->m_topHdgHeight;*/

        if( localPoint.x < rect.right ){
            return true;
        }
        else {
            if(iHeaderRows <1 ) {
                iHeaderRows = 1;
            }

            m_pGrid->GetCellRect(0,iHeaderRows-1,&rect);
            CPoint localPoint = point;
            m_pGrid->ScreenToClient(&localPoint);
            int iHeight = m_pGrid->GetTH_Height();

            if( localPoint.y < rect.bottom+iHeight){
                return true;
            }

        }
    }

    return false;

}
/////////////////////////////////////////////////////////////////////////////
//
//                            CTabView::IsDropValid
//
//  This is called on dict tree -> grid drag/drops (adding an item)
//
/////////////////////////////////////////////////////////////////////////////

bool CTabView::IsDropValid(WPARAM wParam,LPARAM lParam)
{
    const DictLevel* pDictLevel;
    const CDictRecord* pDictRecord;
    const CDictItem* pDictItem;
    CIMSAString csName, csLabel;
    CRect rect;

    CTabulateDoc* pDoc = GetDocument();
    CTabSet* pSpec = pDoc->GetTableSpec();

    const CDataDict* pDataDict = pSpec->GetDict();

    /////////////////////////////////////////////////////////////////////////////
    // get point at which dropped from wParam
    CPoint point;
    point.x = LOWORD(wParam);
    point.y = HIWORD(wParam);

    if (!IsDropPointValid(point)) {
        return false;
    }
    if(m_pGrid->IsGridVarDrop()){
        return true;
    }
    /////////////////////////////////////////////////////////////////////////////
    // get dictionary item information from lParam
    DictTreeNode* dict_tree_node = reinterpret_cast<DictTreeNode*>(lParam);
    DictElementType dict_element_type = dict_tree_node->GetDictElementType();

    int iLevel = dict_tree_node->GetLevelIndex();
    int iRec   = dict_tree_node->GetRecordIndex();
    int iItem  = dict_tree_node->GetItemIndex();
    int iVSet  = dict_tree_node->GetValueSetIndex();

    bool bValidDict = pDataDict == dict_tree_node->GetDDDoc()->GetDict() || pSpec->GetWorkDict() == dict_tree_node->GetDDDoc()->GetDict();
    if(!bValidDict) {
        AfxMessageBox(_T("Dictionary item does not belong to the dictionary used in tab spec"));
        return false;
    }
    else {
       pDataDict =  dict_tree_node->GetDDDoc()->GetDict();
    }
    switch (dict_element_type) {
        case DictElementType::Item :
        {
            pDictItem = pDataDict->GetLevel(iLevel).GetRecord(iRec)->GetItem(iItem);
            if (!pDictItem->HasValueSets()) {
                csName =    pDictItem->GetName();
                csLabel =   pDictItem->GetLabel();
            }
            else {
                iVSet = 0;
                const DictValueSet& dict_value_set = pDataDict->GetLevel(iLevel).GetRecord(iRec)->GetItem(iItem)->GetValueSet(iVSet);
                csName = dict_value_set.GetName();
                csLabel = dict_value_set.GetLabel();
            }
            break;
        }
        case DictElementType::ValueSet :
        {
            const DictValueSet& dict_value_set = pDataDict->GetLevel(iLevel).GetRecord(iRec)->GetItem(iItem)->GetValueSet(iVSet);
            csName = dict_value_set.GetName();
            csLabel = dict_value_set.GetLabel();
            break;
        }
        default:
        {
            return false;
        }
    }

    const DictValueSet* pDictVSet;
    const CDataDict* pDict = pSpec->LookupName(csName, &pDictLevel, &pDictRecord, &pDictItem, &pDictVSet);
    if (!pDict) {
        CIMSAString csMsg = csName;
        csMsg += _T(" not found in dictionary");
        AfxMessageBox (csMsg);
        return false;
    }
    int iDictLevel, iDictRecord, iDictItem, iDictVSet;
    pSpec->LookupName(csName, &iDictLevel, &iDictRecord, &iDictItem, &iDictVSet);

    /////////////////////////////////////////////////////////////////////////////
    // is the item we're trying to drop from a multiple record or occurring item?

    CString csMultErr;
    csMultErr.Format(IDS_MULT_ERR, (LPCTSTR)csLabel);

    CString csOccsErr;
    csOccsErr.Format(IDS_OCCS_ERR, (LPCTSTR)csLabel);

    // see if this item belongs to a multiple record
    const CDictRecord* pMultRecord = nullptr;
    BOOL bMult = (pDictRecord->GetMaxRecs() > 1);
    if (bMult) {
        pMultRecord = pDictRecord;
    }

    // see if occurring item, or sub or occurring item
    CString csTempOcc = _T("");
    BOOL bOcc = false;
    UINT uOccs = GetDictOccs(pDictRecord, pDictItem, iDictItem);
    if (uOccs > 1 && dict_tree_node->GetItemOccurs() == NONE) {
        bOcc = true;
        const CDictItem* pDictOccItem = GetDictOccItem(pDictRecord, pDictItem, iDictRecord, iDictItem);
        csTempOcc = pDictOccItem->GetName();
    }

    if (!bMult && !bOcc) {
        return true;
    }
   /* if(bOcc){
        if(!DoMultiItemChk(pDictItem)){
            AfxMessageBox (csOccsErr);
            return false;
        }
    }*/

    // check row items
    /*for (i=0; i<pSpecTbl->GetNonBlankRowItems(); i++) {
        pXtabItem = pSpecTbl->GetRowItem(i);
        pXtabItem->GetDictInfo(&pDictLevel, &pDictRecord, &pDictItem, &pDictVSet);
        if (bMult) {
            if (pDictRecord->GetMaxRecs() > 1) {
                if (pDictRecord != pMultRecord) {
                    AfxMessageBox (csMultErr);
                    return false;
                }
                else {
                    pMultRecord = pDictRecord;
                }
            }
        }
        if (bOcc) {
            if (pXtabItem->GetOcc() == 0) {
                // BMD  23 Oct 2001
                if (pDictItem->GetItemType() == ItemType::Item || pDictItem->GetOccurs() > 1) {
                    csName = pXtabItem->GetName();
                }
                else {
                    csName = pDictItem->GetParentItem()->GetName();
                }
                if (csName.CompareNoCase(csTempOcc)) {
                    AfxMessageBox (csOccsErr);
                    return false;
                }
                else {
                    csTempOcc = csName;
                }
            }
        }
    }*/
    // check col items
    /*for (i=0; i<pSpecTbl->GetNonBlankColItems(); i++) {
        pXtabItem = pSpecTbl->GetColItem(i);
        pXtabItem->GetDictInfo(&pDictLevel, &pDictRecord, &pDictItem, &pDictVSet);
        if (bMult) {
            if (pDictRecord->GetMaxRecs() > 1) {
                if (pDictRecord != pMultRecord) {
                    AfxMessageBox (csMultErr);
                    return false;
                }
                else {
                    pMultRecord = pDictRecord;
                }
            }
        }
        if (bOcc) {
            if (pXtabItem->GetOcc() == 0) {
                if (pDictItem->GetItemType() == ItemType::Item || pDictItem->GetOccurs() > 1) {
                    csName = pXtabItem->GetName();
                }
                else {
                    csName = pDictItem->GetParentItem()->GetName();
                }
                if (csName.CompareNoCase(csTempOcc)) {
                    AfxMessageBox (csOccsErr);
                    return false;
                }
                else {
                    csTempOcc = csName;
                }
            }
        }
    }*/
    // check weight item &&&

    // check value item &&&
    // check universe    &&&

    return true;
}

/////////////////////////////////////////////////////////////////////////////
//
//                            CTabView::OnDictDroppedOn
//
//  This is called on grid -> dict tree drag/drops (removing an item)
//
/////////////////////////////////////////////////////////////////////////////

LRESULT CTabView::OnDictDroppedOn(WPARAM /*wParam*/,LPARAM lParam) {

    CTabulateDoc* pDoc = (CTabulateDoc*)GetDocument();

    /////////////////////////////////////////////////////////////////////////////
    // get string from lParam
    CString sString = (LPCTSTR)lParam;
    int iSourceRow = 0;
    int iSourceCol= 0;
    int iFound = sString.Find(_T("-"));
    CTabVar* pSItem = nullptr;

    if (iFound != -1) {
        CString sRow = sString.Left(iFound);
        CString sCol = sString.Right(sString.GetLength()-iFound-1);
        iSourceRow = _ttoi(sRow);
        iSourceCol= _ttoi(sCol);

        CUGCell cellGrid;
        m_pGrid->GetCell(iSourceCol,iSourceRow,&cellGrid);

        CTblOb** pTblOb = (CTblOb**)cellGrid.GetExtraMemPtr();
        if(pTblOb && (*pTblOb) && (*pTblOb)->IsKindOf(RUNTIME_CLASS(CGTblCol))){
            pSItem = ((CGTblCol*)(*pTblOb))->GetTabVar();
            m_pGrid->m_bReconcileSpecialCells = true;
        }
    }
    else {
        //Get the variable and do a flip
        iSourceRow = _ttoi(sString);
        pSItem  = m_pGrid->GetRowVar(iSourceRow);
    }
    if(!pSItem)
        return 0;
    m_pGrid->m_bReconcileSpecialCells = true;

    pSItem->Remove();
    delete pSItem;

    RemoveSystemTotalVar();
    AddSystemTotalVar();
    /////////////////////////////////////////////////////////////////////////////
    // title
    m_pGrid->GetTable()->GenerateTitle();

    /////////////////////////////////////////////////////////////////////////////
    // update the tree
    CTabTreeCtrl* pTreeCtrl = pDoc->GetTabTreeCtrl();
    pTreeCtrl->ReBuildTree();

    /////////////////////////////////////////////////////////////////////////////
    // update the grid
    m_pGrid->GetTable()->RemoveAllData();
    // force design view variable is added/removed JH 8/05
    ((CTableChildWnd*) GetParentFrame())->SetDesignView(TRUE);
    m_pGrid->Update();
    Invalidate();
    pDoc->SetModifiedFlag(TRUE);

    // reset units, weight and universe to defaults
    if(m_pGrid->GetTable()->GetRowRoot()->GetNumChildren() == 0 && m_pGrid->GetTable()->GetColRoot()->GetNumChildren() == 0){
    //Reset Table units
        //pDoc->GetTableSpec()->ResetTableUnits(m_pGrid->GetTable());
        CUnitSpec& unit = m_pGrid->GetTable()->GetTableUnit();
        //GSF made the decision use set the table back to default
        unit.SetLoopingVarName(_T(""));
        unit.SetUseDefaultLoopingVar(true);
    }

    CIMSAString sCrossTabStmt;
    pDoc->MakeCrossTabStatement(m_pGrid->GetTable(),sCrossTabStmt);
    //Send a message to CSPro and putsource code for this
    TBL_PROC_INFO tblProcInfo;
    tblProcInfo.pTabDoc = pDoc;
    tblProcInfo.pTable = m_pGrid->GetTable();
    tblProcInfo.sTblLogic = sCrossTabStmt;
    tblProcInfo.eEventType = CSourceCode_Tally;
    tblProcInfo.pLinkTable = nullptr;
    tblProcInfo.bGetLinkTables = true;

    if(AfxGetMainWnd()->SendMessage(UWM::Table::PutTallyProc, 0, (LPARAM)&tblProcInfo) == -1){
        AfxMessageBox(_T("Failed to put logic in App"));
        return 0;
    }
    pDoc->GetTableSpec()->ReconcileLevels4Tbl(m_pGrid->GetTable());
    return 0;
}


/////////////////////////////////////////////////////////////////////////////////
//
//  void CTabView::OnAddTable()
//
/////////////////////////////////////////////////////////////////////////////////
void CTabView::OnAddTable()
{
    CWaitCursor dummy;      // gsf 01/30/01

    CTabulateDoc* pDoc = GetDocument();
    CTabSet* pSpec = pDoc->GetTableSpec();
    CTabTreeCtrl* pTreeCtrl = pDoc->GetTabTreeCtrl();

    int iSize = pSpec->GetNumTables() - 1;
    CIMSAString sNum;
    if(iSize >= 0){
        sNum    = pSpec->GetTable(iSize)->GetNum();
    }
    else {
        sNum =_T("0");
    }
    sNum = pDoc->GetTableSpec()->GetNextTabNum(sNum);
    CTable* pTable = new CTable(sNum);

    pTable->GenerateTitle();

    if(pTreeCtrl->IsPrintTree()){
        pSpec->AddTable(pTable);
    }
    else {
        int iLevel = pTreeCtrl->GetCurLevel();
        pSpec->AddTable(pTable,iLevel);
    }

    pTable->SetFmtRegPtr(pSpec->GetFmtRegPtr());
    m_pGrid->SetTable(pTable);

    // force design view when new table is created JH 8/05
    ((CTableChildWnd*) GetParentFrame())->SetDesignView(TRUE);

    m_pGrid->Update();

    pTreeCtrl->ReBuildTree();
    pDoc->SetModifiedFlag(TRUE);
}
/////////////////////////////////////////////////////////////////////////////////
//
//  void CTabView::OnUpdateAddTable(CCmdUI* pCmdUI)
//
/////////////////////////////////////////////////////////////////////////////////
void CTabView::OnUpdateAddTable(CCmdUI* pCmdUI)
{
    CTabulateDoc* pDoc = GetDocument();
    CTabSet* pSpec = pDoc->GetTableSpec();

    int iNumTables = pSpec->GetNumTables();
    bool bEnable = false;
    if(iNumTables > 0 ) {
        //CTable* pTable =pSpec->GetTable(iNumTables-1);
        //bEnable = (pTable->GetRowRoot()->GetNumChildren() > 0 || pTable->GetColRoot()->GetNumChildren() > 0);
        bEnable = true;
    }

    pCmdUI->Enable(bEnable);
}


/////////////////////////////////////////////////////////////////////////////////
//
//  void CTabView::OnDeleteTable()
//
/////////////////////////////////////////////////////////////////////////////////
void CTabView::OnDeleteTable()
{
    if (AfxMessageBox(_T("Are you sure you want to delete this table?"), MB_YESNOCANCEL | MB_DEFBUTTON2) != IDYES) {
        return;
    }
    CTabulateDoc* pDoc = GetDocument();
    CTabSet* pSpec = pDoc->GetTableSpec();

    TBL_PROC_INFO tblProcInfo;
    tblProcInfo.pTabDoc = pDoc;
    tblProcInfo.pTable = m_pGrid->GetTable();


    AfxGetMainWnd()->SendMessage(UWM::Table::DeleteLogic, (WPARAM)0, (LPARAM)&tblProcInfo);

    CTable* pTable = m_pGrid->GetTable();
    pSpec->DeleteTable(pTable);

    if(pSpec->GetNumTables() == 0 ) {
        OnAddTable();
    }

    m_pGrid->SetTable(pSpec->GetTable(0)); //where should we position the user ??
    m_pGrid->Update();

    CTabTreeCtrl* pTreeCtrl = pDoc->GetTabTreeCtrl();
    pTreeCtrl->ReBuildTree();
    pDoc->SetModifiedFlag(TRUE);
    delete pTable; // JH 8/05 fix memory leak


}
/////////////////////////////////////////////////////////////////////////////////
//
//  void CTabView::OnUpdateDeleteTable(CCmdUI* pCmdUI)
//
/////////////////////////////////////////////////////////////////////////////////
void CTabView::OnUpdateDeleteTable(CCmdUI* pCmdUI)
{
    CTabulateDoc* pDoc = GetDocument();
    CTabSet* pSpec = pDoc->GetTableSpec();
    int iNumTables = pSpec->GetNumTables();
    bool bEnable = (iNumTables> 0);
    pCmdUI->Enable(bEnable);
}
/////////////////////////////////////////////////////////////////////////////////
//
//  void CTabView::OnInsertTable()
//
/////////////////////////////////////////////////////////////////////////////////
void CTabView::OnInsertTable()
{
    CWaitCursor dummy;      // gsf 01/30/01

    CTabulateDoc* pDoc = GetDocument();
    CTabSet* pSpec = pDoc->GetTableSpec();
    CTabTreeCtrl* pTreeCtrl = pDoc->GetTabTreeCtrl();

    CTable* pCurTable = m_pGrid->GetTable();
    int iPrev = -1;
    for (int i = 0 ; i < pSpec->GetNumTables() ; i++) {
        if (pSpec->GetTable(i) == pCurTable) {
            iPrev = i - 1;
            break;
        }
    }
    CIMSAString sNum = _T("0");
    if (iPrev >= 0) {
        sNum = pSpec->GetTable(iPrev)->GetNum();
    }
    sNum = pDoc->GetTableSpec()->GetNextTabNum(sNum);
    CTable* pTable = new CTable(sNum);
    pTable->GenerateTitle();

    if(pTreeCtrl->IsPrintTree()){
        pSpec->InsertTable(pTable,m_pGrid->GetTable());
    }
    else {
        int iLevel = pTreeCtrl->GetCurLevel();
        pSpec->InsertTable(pTable,m_pGrid->GetTable(),iLevel);
    }
    m_pGrid->SetTable(pTable);
    pTable->SetFmtRegPtr(pSpec->GetFmtRegPtr());
    // force design view when new table is created JH 8/05
    ((CTableChildWnd*) GetParentFrame())->SetDesignView(TRUE);
    m_pGrid->Update();
    pTreeCtrl->ReBuildTree();
}
/////////////////////////////////////////////////////////////////////////////////
//
//  void CTabView::OnUpdateInsertTable(CCmdUI* pCmdUI)
//
/////////////////////////////////////////////////////////////////////////////////
void CTabView::OnUpdateInsertTable(CCmdUI* pCmdUI)
{
    CTabulateDoc* pDoc = GetDocument();
    CTabSet* pSpec = pDoc->GetTableSpec();
    int iNumTables = pSpec->GetNumTables();
    bool bEnable = (iNumTables> 0);
    pCmdUI->Enable(bEnable);
}
/////////////////////////////////////////////////////////////////////////////////
//
//  void CTabView::OnUpdateToggleTree(CCmdUI* pCmdUI)
//
/////////////////////////////////////////////////////////////////////////////////
void CTabView::OnUpdateToggleTree(CCmdUI* pCmdUI)
{
    CTabulateDoc* pDoc = (CTabulateDoc*)GetDocument();
    CTabTreeCtrl* pTreeCtrl = pDoc->GetTabTreeCtrl();
    HTREEITEM hItem = pTreeCtrl->GetRootItem();
    if(hItem) {
        pCmdUI->Enable(TRUE);
    }
    else {
        pCmdUI->Enable(FALSE);
    }
}

/////////////////////////////////////////////////////////////////////////////////
//
//  void CTabView::OnToggleTree()
//
/////////////////////////////////////////////////////////////////////////////////
void CTabView::OnToggleTree()
{
    CTabulateDoc* pDoc = (CTabulateDoc*)GetDocument();
    CTabTreeCtrl* pTreeCtrl = pDoc->GetTabTreeCtrl();
    pTreeCtrl->OnToggleTree();
    return;
}

/////////////////////////////////////////////////////////////////////////////////
//
//  void CTabView::OnTblRun()
//
/////////////////////////////////////////////////////////////////////////////////
void CTabView::OnTblRun()
{
    CTabulateDoc* pDoc = (CTabulateDoc*)GetDocument();
    //    AfxMessageBox("Do Reconcile");
    pDoc->OpenAreaNameFile();
    pDoc->BuildAreaLookupMap();
    pDoc->CloseAreaNameFile();
    AfxGetMainWnd()->SendMessage(UWM::Table::RunActiveApplication, (WPARAM)1, (LPARAM) pDoc);
}

void CTabView::OnUpdateTblRun(CCmdUI* pCmdUI)
{
    CTabulateDoc* pDoc = (CTabulateDoc*)GetDocument();
    if(m_pGrid->GetTable() && pDoc->GetTableSpec()->GetNumTables() > 0 ) {
        if(m_pGrid->GetTable()->GetColRoot()->GetNumChildren()>0){
            pCmdUI->Enable(TRUE);
        }
        else if(m_pGrid->GetTable()->GetRowRoot()->GetNumChildren()>0) {
            pCmdUI->Enable(TRUE);
        }
        else {
            pCmdUI->Enable(FALSE);
        }
    }
    else {
        pCmdUI->Enable(FALSE);
    }
}


/////////////////////////////////////////////////////////////////////////////////
//
//  void CTabView::OnRunPcs()
//
/////////////////////////////////////////////////////////////////////////////////
void CTabView::OnRunPcs()
{
    CTabulateDoc* pDoc = (CTabulateDoc*)GetDocument();
    m_pGrid->SetCurSelArea(0);
//    AfxMessageBox("Do Reconcile");
    AfxGetMainWnd()->SendMessage(UWM::Table::RunActiveApplication, (WPARAM)2, (LPARAM) pDoc);
}


/////////////////////////////////////////////////////////////////////////////////
//
//  void CTabView::OnUpdateRunPcs(CCmdUI* pCmdUI)
//
/////////////////////////////////////////////////////////////////////////////////
void CTabView::OnUpdateRunPcs(CCmdUI* pCmdUI)
{
    CTabulateDoc* pDoc = (CTabulateDoc*)GetDocument();
    if(m_pGrid->GetTable() && pDoc->GetTableSpec()->GetNumTables() > 0 ) {
        if(m_pGrid->GetTable()->GetColRoot()->GetNumChildren()>0){
            pCmdUI->Enable(TRUE);
        }
        else if(m_pGrid->GetTable()->GetRowRoot()->GetNumChildren()>0) {
            pCmdUI->Enable(TRUE);
        }
        else {
            pCmdUI->Enable(FALSE);
        }
    }
    else {
        pCmdUI->Enable(FALSE);
    }
}

/////////////////////////////////////////////////////////////////////////////////
//
//  void CTabView::OnFileSaveTable()
//
/////////////////////////////////////////////////////////////////////////////////
void CTabView::OnFileSaveTable()
{
    CIMSAString sFilename;
    SaveTables(sFilename);
}


void CTabView::OnUpdateFileSaveTable(CCmdUI* pCmdUI)
{
    CTabulateDoc* pDoc = (CTabulateDoc*)GetDocument();
    if(m_pGrid->GetTable() && pDoc->GetTableSpec()->GetNumTables() > 0 ) {
        if(m_pGrid->GetTable()->GetColRoot()->GetNumChildren()>0){
            pCmdUI->Enable(TRUE);
        }
        else if(m_pGrid->GetTable()->GetRowRoot()->GetNumChildren()>0) {
            pCmdUI->Enable(TRUE);
        }
        else {
            pCmdUI->Enable(FALSE);
        }
    }
    else {
        pCmdUI->Enable(FALSE);
    }
}

/////////////////////////////////////////////////////////////////////////////////
//
//  bool CTabView::SaveTables()
//
// Save tables as HTML, RTF, TBW,...
// Return false if user cancels.
/////////////////////////////////////////////////////////////////////////////////
bool CTabView::SaveTables(CString& csSaveAsFile) // filename returned
{
    CTabulateDoc* pDoc = (CTabulateDoc*)GetDocument();
    CIMSAString sTBWFName  = pDoc->GetPathName();
    PathRemoveExtension(sTBWFName.GetBuffer(_MAX_PATH));
    sTBWFName.ReleaseBuffer();
    CIMSAString sHtmlFile = sTBWFName;
    CIMSAString sRTFFile = sTBWFName;
    CIMSAString sTxtFile = sTBWFName;
    sTBWFName += FileExtensions::WithDot::Table;
    sHtmlFile += FileExtensions::WithDot::HTML;
    sRTFFile += _T(".rtf");
    sTxtFile += _T(".txt");
    //Save Tables dialog
#ifdef _TABLE_BLOCKED_IMPL //implement block stuff from CSPro 2.5 later
    int i;
    BOOL bBlocked = FALSE;
    CTable *pTable = GetTable(m_iCurrTable);
    if (pTable->GetBlocked()) {

        CSelectionDialog seldlg(_T("Save As"), _T("Save"));
        seldlg.m_nAll = 1;
        if (seldlg.DoModal() == IDCANCEL) {
            return;
        }
        if (seldlg.m_nAll == 1) {
            bBlocked = TRUE;
        }
    }
#endif
    //--- see if the user wants to save multiple tables  ---//
    bool    bMultiSave = false;
    CPickTablesDlg dlgMulti(IDS_PICKTBLSTOSAVE);
    dlgMulti.m_pDoc = pDoc;
    CTabSet* pTabSet = pDoc->GetTableSpec();
    int iTbls = pTabSet->GetNumTables();
    bool bBlocked = false;
    if (!bBlocked && iTbls > 1)  {
        // lotsa tables and not just saving a block ...
        for (int i = 0 ; i < iTbls ; i++)  {
            dlgMulti.AddString(pTabSet->GetTable(i)->GetName());
            dlgMulti.SetCheck(i, pTabSet->GetTable(i) == GetCurTable()); // select only curr table by default
        }
        if (dlgMulti.DoModal() == IDOK)  {
            bMultiSave = true;
        }
        else  {
            // canceled out
            return false;
        }
    }
    enum SAVE_MODE {TBW_MODE=0, RTF_MODE,HTM_MODE,ASCII_MODE} eSaveMode;
    eSaveMode = TBW_MODE;
    //  Get file name

    CIMSAString csFilter;
    csFilter = _T("CSPro Tables (*.tbw)|*.tbw|HTML Files (*.html)|*.html|Rich Text Format (*.rtf)|*.rtf|Tab delimited|*.*||");
    CIMSAFileDialog dlg (FALSE, _T("*"), nullptr, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, csFilter);
    dlg.m_ofn.nFilterIndex = 1;
    BOOL bOK = FALSE;
    while (!bOK) {
        if (dlg.DoModal() == IDCANCEL) {
            return false;
        }
        csSaveAsFile = dlg.GetPathName();
        bOK = TRUE;
    }

    //  Save stuff

    CWaitCursor wait;
    TCHAR pszBuffer[65500];
    memset(pszBuffer,_T('\0'),65500);
    if ((csSaveAsFile.Right(3)).CompareNoCase(FileExtensions::Table) == 0) {
        eSaveMode = TBW_MODE;
    }
    else if((csSaveAsFile.Right(3)).CompareNoCase(_T("rtf")) == 0) {
        eSaveMode = RTF_MODE;
    }
    else if ((csSaveAsFile.Right(3)).CompareNoCase(FileExtensions::HTM) == 0) {
        eSaveMode = HTM_MODE;
    }
    else if ((csSaveAsFile.Right(4)).CompareNoCase(FileExtensions::HTML) == 0) {
        eSaveMode = HTM_MODE;
    }
    else {
        eSaveMode = ASCII_MODE;
    }

    // figure out how many tables we need to save
    const int iNumTables = pDoc->GetTableSpec()->GetNumTables();
    int iNumTablesToSave = 0;
    if (iNumTables == 1) {
        iNumTablesToSave = 1;
    }
    else {
        for(int iTab = 0; iTab < iNumTables; iTab++) {
            if(dlgMulti.GetCheck(iTab)){
                ++iNumTablesToSave;
            }
        }
    }
    ASSERT(iNumTablesToSave > 0);

    if (iNumTablesToSave > 1 && eSaveMode == HTM_MODE) {
        AfxMessageBox(_T("Save as HTML only supports one table at a time.  Please save each table seperately."));
        return false;
    }
    if (iNumTablesToSave > 1 && eSaveMode == RTF_MODE) {
        AfxMessageBox(_T("Save as RTF only supports one table at a time.  Please save each table seperately."));
        return false;
    }

    CSpecFile specFile;
    if(eSaveMode == RTF_MODE){
        specFile.SetEncoding(Encoding::Ansi); //do not write the encoding for RTF files
    }
    if (!specFile.Open(csSaveAsFile, CFile::modeWrite))  {
        CIMSAString sError;
        sError = _T("Error:  Could not open file ") + csSaveAsFile;
        AfxMessageBox(sError,MB_ICONEXCLAMATION);
        return false;
    }

    //End save tables dialog
    if(bMultiSave && eSaveMode == TBW_MODE) {
        m_pGrid->SetAreaLabels4AllTables();
        pDoc->GetTableSpec()->SaveTBWBegin(specFile,pDoc->GetDictFileName());
        for(int iIndex = 0; iIndex < iNumTables; iIndex++) {
            CTable* pTable = nullptr;
            if(!dlgMulti.GetCheck(iIndex)){
                pTable = pDoc->GetTableSpec()->GetTable(iIndex);
                pTable->m_bDoSaveinTBW = false;
                continue;
            }
            else {
                pTable = pDoc->GetTableSpec()->GetTable(iIndex);
                pTable->m_bDoSaveinTBW = true;
            }
        }
        pDoc->GetTableSpec()->SaveTBWTables(specFile);
        pDoc->GetTableSpec()->SaveTBWEnd(specFile);
        specFile.Close();
    }
    else if(bMultiSave) {//Save for multiple tables
        CTblGrid* pGrid = new CTblGrid();
        CRect rect;
        GetClientRect (&rect);
        pGrid->CreateGrid(WS_CHILD | WS_VISIBLE,rect,this,124);
        pGrid->SetParent(this);
        CTabSet* pSpec = pDoc->GetTableSpec();
        pGrid->SetSpec (pSpec);

        CTable* pTable = nullptr;
        int iNumTables = pSpec->GetNumTables();
        bool bFirst = true;
        for(int iIndex = 0; iIndex < iNumTables; iIndex++) {
            if(!dlgMulti.GetCheck(iIndex)){
                continue;
            }
            pTable = pDoc->GetTableSpec()->GetTable(iIndex);
            pGrid->SetTable(pTable);
            pGrid->Update();
            //For now just save here
            if(eSaveMode ==HTM_MODE){
//              CIMSAString sHTML;
//              char pszBuffer[65500];
//              memset(pszBuffer,'\0',65500);
                if(bFirst) {
                    bFirst = false;
                    _tofstream os(specFile.m_pStream);
                    pGrid->PutHTMLTable(os, false);
                }
                else {
                    ASSERT(!_T("Trying to save 2 tables to one HTML file, should be caught above"));
                }
            }
            else if(eSaveMode == RTF_MODE){

                if(bFirst) {
                    bFirst = false;
                    //pGrid->PutRTFBegin(aFontInfo, aColorInfo, pszBuffer, &specFile);
                    _tofstream os(specFile.m_pStream);
                    pGrid->PutRTFTable(os, false, true);
                }
                else {
                    ASSERT(!_T("Trying to save 2 tables to one RTF file, should be caught above"));
                }
            }
            else if(eSaveMode == ASCII_MODE){
                _tofstream os(specFile.m_pStream);
                pGrid->PutASCIITable(os, false);

            }
        }
        //Do end
        if(eSaveMode == HTM_MODE){
            //pGrid->PutHTMLEnd(&specFile);
        }
        else if (eSaveMode == RTF_MODE){
            //pGrid->PutRTFEnd(pszBuffer, &specFile);
        }
        specFile.Close();

        if(pGrid){
            pGrid->DestroyWindow();
            delete  pGrid;
        }
    }
    else {
        //Only one table in the XTS
        if (eSaveMode == TBW_MODE) {
            ASSERT(pDoc->GetTableSpec()->GetNumTables() == 1);
            pDoc->GetTableSpec()->SaveTBWBegin(specFile,pDoc->GetDictFileName());
            CTable* pTable = pDoc->GetTableSpec()->GetTable(0);
            pTable->m_bDoSaveinTBW = true;
            pDoc->GetTableSpec()->SaveTBWTables(specFile);
            pDoc->GetTableSpec()->SaveTBWEnd(specFile);
        }
        else if(eSaveMode ==HTM_MODE) {
//          char pszBuffer[65500];
//          memset(pszBuffer,'\0',65500);

            _tofstream os(specFile.m_pStream);
            m_pGrid->PutHTMLTable(os, false);
        }
        else if(eSaveMode ==RTF_MODE) {
            //For now just save here
           _tofstream os(specFile.m_pStream);

            m_pGrid->PutRTFTable(os, false, true);
        }
        else if (eSaveMode == ASCII_MODE){
            _tofstream os(specFile.m_pStream);
            m_pGrid->PutASCIITable(os, false);
        }
        specFile.Close();
    }

    return true;
}

/////////////////////////////////////////////////////////////////////////////////
//
//  void CTabView::DeleteUnsavedTables()
//
// this deletes tables not marked for save in TBW
/////////////////////////////////////////////////////////////////////////////////
void CTabView::DeleteUnsavedTables()
{
    CTabulateDoc* pDoc = GetDocument();
    int iNumTables = pDoc->GetTableSpec()->GetNumTables();
    CTable* pCurrTbl = GetGrid()->GetTable();
    bool bDeletedActiveTable = false;
    for(int iIndex = iNumTables-1; iIndex >= 0; iIndex--) {
        CTable* pTable = pDoc->GetTableSpec()->GetTable(iIndex);
        if (pTable->m_bDoSaveinTBW == false) {
            if (pCurrTbl == pTable) {
                // removing active table - will need to switch to first table
                bDeletedActiveTable = true;
            }
            pDoc->GetTableSpec()->DeleteTable(pTable);
            delete pTable;
        }
    }

    if (bDeletedActiveTable) {
        GetGrid()->SetTable(pDoc->GetTableSpec()->GetTable(0)); //where should we position the user ??
        GetGrid()->Update();
    }

    // update the tree control
    bool bViewer = ((CTableChildWnd*) GetParentFrame())->IsViewer();
    CTabTreeCtrl* pTreeCtrl = pDoc->GetTabTreeCtrl();
    if (bViewer) {
        TableSpecTabTreeNode* table_spec_tab_tree_node = pTreeCtrl->GetTableSpecTabTreeNode(*pDoc);
        ASSERT(table_spec_tab_tree_node != nullptr);
        pTreeCtrl->ReleaseDoc(*table_spec_tab_tree_node);
        pTreeCtrl->BuildTVTree(pDoc);
    }
    else {
        pTreeCtrl->ReBuildTree();
    }
}

/////////////////////////////////////////////////////////////////////////////////
//
//  void CTabView::OnRButtonUp (UINT nFlags, CPoint point)
//
/////////////////////////////////////////////////////////////////////////////////
// rather than using a fixed dialog box, w/entries either activated or not, i am
// going to build the popup dialog based on the location (cell/row heading/title/etc)
// of the right-click
//
/////////////////////////////////////////////////////////////////////////////////

void CTabView::OnRButtonUp(UINT /*nFlags*/, CPoint point)
{
    long        lRow = -1;
    int         iCol = -1;
    bool        bAllowAIDs = true; // allow "Add, Insert, Delete Table" opts?
    CTable*     pTable = GetCurTable();
    CTblGrid*   pGrid = GetGrid();
    CIMSAString sQuoteStr;
    CIMSAString sStr;

    const bool bViewer = ((CTableChildWnd*) GetParentFrame())->IsViewer();

    SetCurVar(nullptr);

    if (point.x == 0 && point.y == 0) {
        iCol = pGrid->GetCurrentCol();
        lRow = pGrid->GetCurrentRow();
        CRect rect;
        pGrid->GetCellRect(iCol, lRow, &rect);
        point.x = (rect.left + rect.right) / 2;
        point.y = (rect.top + rect.bottom) / 2;
    }
    else {
        pGrid->GetCellFromPoint (point.x, point.y, &iCol, &lRow); // func in zGridO
    }
    bool bPointInSubtableRect = IsPointInSubTableRect(point);
    BCMenu hMenu;
    hMenu.CreatePopupMenu();
   // bool bDeadSpace = false;

    if ( pGrid->IsGridEmpty() ) {  // case #1: table completely empty, no items dragged out

        bAllowAIDs = false; // can't add/ins new table if nothing on current one!
    }

    else if (iCol == -1 || lRow == -1) // case 9:  user clicked outside of table bounds
    {
        // the current table is not empty, so add/ins/del all allowed
   //     bDeadSpace = true;
    }

    bool bEmptyTable = pTable->GetRowRoot()->GetNumChildren() ==0 && pTable->GetColRoot()->GetNumChildren() ==0;
    ///Menu build start
    FMT_ID eGridComp = FMT_ID_INVALID;
    CGTblOb* pGTblOb = nullptr;
    m_pGrid->GetComponent(iCol,lRow,eGridComp,&pGTblOb);
    if (!bViewer) {
        if(eGridComp == FMT_ID_SPANNER || eGridComp == FMT_ID_COLHEAD){//Get the multiple vars
            CGTblCol* pCol = DYNAMIC_DOWNCAST(CGTblCol,pGTblOb);
            SetCurVar(pCol->GetTabVar());

            if(pCol->GetTabVar()->GetName().CompareNoCase(WORKVAR_TOTAL_NAME) != 0){
                sQuoteStr = _T("Tally Attributes (") + pCol->GetTabVar()->GetText() + _T(")");
                hMenu.AppendMenu(MF_STRING, ID_EDIT_VAR_TALLYATTRIB, sQuoteStr);
            }

            if(pCol->GetTabVar() && pCol->GetTabVar()->GetParent() != pTable->GetColRoot()){//add the parent item also
                sQuoteStr = _T("Tally Attributes (") + pCol->GetTabVar()->GetParent()->GetText() + _T(")");
                hMenu.AppendMenu(MF_STRING, ID_EDIT_VAR_TALLYATTRIB1, sQuoteStr);
            }

        }
        else if(eGridComp == FMT_ID_CAPTION || eGridComp == FMT_ID_STUB){//for 1 -var do later in stub head
            CGTblRow* pRow = DYNAMIC_DOWNCAST(CGTblRow,pGTblOb);
            SetCurVar(pRow->GetTabVar());

            if(pRow->GetTabVar()->GetName().CompareNoCase(WORKVAR_TOTAL_NAME) != 0){
                sQuoteStr = _T("Tally Attributes (") + pRow->GetTabVar()->GetText() + _T(")");
                hMenu.AppendMenu(MF_STRING, ID_EDIT_VAR_TALLYATTRIB, sQuoteStr);
            }

            if(pRow->GetTabVar() && pRow->GetTabVar()->GetParent() != pTable->GetRowRoot()){//add the parent item also
                sQuoteStr = _T("Tally Attributes (") + pRow->GetTabVar()->GetParent()->GetText() + _T(")");
                hMenu.AppendMenu(MF_STRING, ID_EDIT_VAR_TALLYATTRIB1, sQuoteStr);
            }
        }
        sQuoteStr = _T("Tally Attributes (Table)") ;
        CIMSAString sSubTableAttrib =_T("Tally Attributes (Subtable)") ;

      /*  int iGridCol = -1;
        long iGridRow = -1;
            // determine cursor position (device units)
        CPoint point;
        ::GetCursorPos(&point);
        // convert cursor position to logical units, which are compatible with the CPageOb layout rects
        m_pGrid->ScreenToClient(&point);


        m_pGrid->GetCellFromPoint (point.x, point.y, &iGridCol, &iGridRow); // func in zGridO
        */
        bool bIsDataCell = m_pGrid->IsDataCell(iCol,lRow);
        if(!bEmptyTable){
            hMenu.AppendMenu(MF_STRING, ID_EDIT_TBL_TALLYATTRIB,sQuoteStr);
            CArray<CStringArray,CStringArray&> arrValidSubtableUnitNames;
            CStringArray arrValidTableUnitNames;
            GetDocument()->GetTableSpec()->UpdateSubtableList(pTable, arrValidSubtableUnitNames, arrValidTableUnitNames);
            if(arrValidSubtableUnitNames.GetSize() > 1 && bIsDataCell && bPointInSubtableRect) {
                hMenu.AppendMenu(MF_STRING, ID_EDIT_SUBTBL_TALLYATTRIB,sSubTableAttrib);
            }
            else {
                   hMenu.AppendMenu(MF_GRAYED, ID_EDIT_SUBTBL_TALLYATTRIB,sSubTableAttrib);
            }
        }
        else {
            hMenu.AppendMenu(MF_GRAYED, ID_EDIT_TBL_TALLYATTRIB,sQuoteStr);
            CArray<CStringArray,CStringArray&> arrValidSubtableUnitNames;
            CStringArray arrValidTableUnitNames;
            GetDocument()->GetTableSpec()->UpdateSubtableList(pTable, arrValidSubtableUnitNames, arrValidTableUnitNames);
            if(arrValidSubtableUnitNames.GetSize() > 1) {
                hMenu.AppendMenu(MF_GRAYED, ID_EDIT_SUBTBL_TALLYATTRIB,sSubTableAttrib);
            }
        }
        hMenu.AppendMenu(MF_SEPARATOR, NULL, nullptr);
    }
    else {//for the viewer to split spanners
         if(eGridComp == FMT_ID_SPANNER || eGridComp == FMT_ID_COLHEAD){//Get the multiple vars
            CGTblCol* pCol = DYNAMIC_DOWNCAST(CGTblCol,pGTblOb);
            SetCurVar(pCol->GetTabVar());
         }
    }

    CIMSAString sComponent = m_pGrid->GetComponentString(iCol, lRow);
    sQuoteStr = _T("Format (") +sComponent +_T(")") ;
    if(pGTblOb && eGridComp == FMT_ID_DATACELL){
        m_iColRClick = iCol;
        m_lRowRClick =lRow;
        //Get RowFmt
        CGTblCell* pGTblCell = DYNAMIC_DOWNCAST(CGTblCell, pGTblOb);
        ASSERT(pGTblCell);
        CGTblRow* pGTblRow = pGTblCell->GetTblRow();
        if(pGTblRow->GetTabVal()){
            if(m_pGrid->IsMultiSelectState()){//cannot format data cells in multi state
                hMenu.AppendMenu(MF_GRAYED, ID_EDIT_COMPONENT_FMT, sQuoteStr);
            }
            else {
                hMenu.AppendMenu(MF_STRING, ID_EDIT_COMPONENT_FMT, sQuoteStr);
            }
        }
        else {
            hMenu.AppendMenu(MF_GRAYED, ID_EDIT_COMPONENT_FMT, sQuoteStr);
        }
    }
    else if(pGTblOb && eGridComp != FMT_ID_INVALID){
        m_iColRClick = iCol;
        m_lRowRClick =lRow;
        if(m_pGrid->IsMultiSelectState()){
            CArray<CTblBase*,CTblBase*> arrTblBase;
            if(m_pGrid->GetMultiSelObj(arrTblBase,eGridComp)){
                hMenu.AppendMenu(MF_STRING, ID_EDIT_COMPONENT_FMT, sQuoteStr);
                bool bEnableJoin = true;
                if(eGridComp == FMT_ID_SPANNER){
                    for(int iIndex = 0; iIndex < arrTblBase.GetCount(); iIndex++){
                        if(((CTabVar*)arrTblBase[iIndex])->GetNumChildren() >0){
                            bEnableJoin = false;
                            break;
                        }
                    }
                    sQuoteStr  = _T("Join Spanners");
                    bEnableJoin ? hMenu.AppendMenu(MF_STRING, ID_EDIT_JOIN_SPANNERS, sQuoteStr) : hMenu.AppendMenu(MF_GRAYED, ID_EDIT_JOIN_SPANNERS, sQuoteStr);
                    sQuoteStr  = _T("Split Spanners");
                    hMenu.AppendMenu(MF_GRAYED, ID_EDIT_SPLIT_SPANNERS, sQuoteStr);
                }
            }
            else {
                hMenu.AppendMenu(MF_GRAYED, ID_EDIT_COMPONENT_FMT, sQuoteStr);
                sQuoteStr  = _T("Join Spanners");
                hMenu.AppendMenu(MF_GRAYED, ID_EDIT_JOIN_SPANNERS, sQuoteStr);
                sQuoteStr  = _T("Split Spanners");
                hMenu.AppendMenu(MF_GRAYED, ID_EDIT_SPLIT_SPANNERS, sQuoteStr);
            }
            hMenu.AppendMenu(MF_SEPARATOR, NULL, nullptr);
        }
        else {
            hMenu.AppendMenu(MF_STRING, ID_EDIT_COMPONENT_FMT, sQuoteStr);
            sQuoteStr  = _T("Join Spanners");
            hMenu.AppendMenu(MF_GRAYED, ID_EDIT_JOIN_SPANNERS, sQuoteStr);
            bool bEnableSplit = false;
            if(eGridComp == FMT_ID_SPANNER){
                CGTblCol* pCol = DYNAMIC_DOWNCAST(CGTblCol,pGTblOb);
                if(pCol->GetTabVar() && pCol->GetTabVar()->GetFmt()){
                    CDataCellFmt* pDataCellFmt = DYNAMIC_DOWNCAST(CDataCellFmt,pCol->GetTabVar()->GetFmt());
                    bEnableSplit = pDataCellFmt->GetNumJoinSpanners() > 0;
                }
                sQuoteStr  = _T("Split Spanners");
                bEnableSplit ? hMenu.AppendMenu(MF_STRING, ID_EDIT_SPLIT_SPANNERS, sQuoteStr) : hMenu.AppendMenu(MF_GRAYED, ID_EDIT_SPLIT_SPANNERS, sQuoteStr) ;
            }
            hMenu.AppendMenu(MF_SEPARATOR, NULL, nullptr);
        }
    }
    else {
        m_iColRClick = -1;
        m_lRowRClick =-1;
        sQuoteStr=_T("Format");
        hMenu.AppendMenu(MF_GRAYED, ID_EDIT_COMPONENT_FMT, sQuoteStr);
    }

    sQuoteStr = _T("Format (Table)");
    hMenu.AppendMenu(MF_STRING, ID_EDIT_TBL_FMT,sQuoteStr);

    sQuoteStr = _T("Format (Application)");
    hMenu.AppendMenu(MF_STRING, ID_EDIT_APP_FMT,sQuoteStr);

    hMenu.AppendMenu(MF_SEPARATOR, NULL, nullptr);
    sQuoteStr = _T("Format Print (Table)");
    hMenu.AppendMenu(MF_STRING, ID_EDIT_TBL_PRINTFMT,sQuoteStr);
    hMenu.AppendMenu(MF_SEPARATOR, NULL, nullptr);


    if (iCol > 0 && lRow > pGrid->GetNumHeaderRows() - 1) {
        hMenu.AppendMenu(MF_STRING, ID_EDIT_COPY, _T("Copy\tCtrl+C"));
        hMenu.AppendMenu(MF_STRING, ID_EDIT_COPYCELLSONLY, _T("Copy Cells Only"));
        hMenu.AppendMenu(MF_STRING, ID_EDIT_COPYTABLESTRUCTURE, _T("Copy Table Spec"));
        CTabulateDoc* pDoc = GetDocument();
        if (IsClipboardFormatAvailable(pDoc->GetClipBoardFormat(TD_TABLE_FORMAT))) {
            hMenu.AppendMenu(MF_STRING, ID_EDIT_PASTETABLE, _T("Paste Table Spec"));
        }
        else {
            hMenu.AppendMenu(MF_GRAYED, ID_EDIT_PASTETABLE, _T("Paste Table Spec"));
        }
        hMenu.AppendMenu(MF_SEPARATOR, NULL, nullptr);
        hMenu.AppendMenu(MF_STRING, ID_SELECTALL, _T("Select All\tCtrl+A"));
        hMenu.AppendMenu(MF_STRING, ID_DESELECTALL, _T("Cancel Selection\tEsc"));

        hMenu.AppendMenu(MF_SEPARATOR, NULL, nullptr);

    }


#if 0

    // other cases to handle (refer to 2.3 code for possible code relevance):

        // 1: if the col heading is frequency or cumulative, which it will be when
        // you have one row variable, then i'm getting a quoted empty string on
        // the popup menu which is obv wrong, as you can't delete this in the 1st place

        // 2: area processing, when "$AreaName$ appears in header

        // 3: "Total" or "Percent" in col hdg if one of % opts being used

#endif

    // common block for all popups

    if (!bViewer) {
        //Savy for Right click Menu for copy paste 10/31/2008
        hMenu.AppendMenu(MF_STRING, ID_EDIT_COPYTABLESTRUCTURE, _T("Copy Table Spec"));
        CTabulateDoc* pDoc = GetDocument();
        if (IsClipboardFormatAvailable(pDoc->GetClipBoardFormat(TD_TABLE_FORMAT))) {
            hMenu.AppendMenu(MF_STRING, ID_EDIT_PASTETABLE, _T("Paste Table Spec"));
        }
        else {
            hMenu.AppendMenu(MF_GRAYED, ID_EDIT_PASTETABLE, _T("Paste Table Spec"));
        }
        hMenu.AppendMenu(MF_SEPARATOR, NULL, nullptr);
        //End Savy Changes

        if (bAllowAIDs) {
            hMenu.AppendMenu(MF_STRING, ID_ADD_TABLE, _T("Add Table"));
            hMenu.AppendMenu(MF_STRING, ID_INSERT_TABLE, _T("Insert Table"));
            hMenu.AppendMenu(MF_STRING, ID_DELETE_TABLE, _T("Delete Table"));
        }
        else {
            hMenu.AppendMenu(MF_GRAYED, ID_ADD_TABLE, _T("Add Table"));
            hMenu.AppendMenu(MF_GRAYED, ID_INSERT_TABLE, _T("Insert Table"));
            hMenu.AppendMenu(MF_GRAYED, ID_DELETE_TABLE, _T("Delete Table"));
        }
    }

    // remove trailing separator in menu if one exists
//    int iLastItem = hMenu.GetMenuItemCount() - 1;
//    if (hMenu.GetMenuState(iLastItem, MF_BYPOSITION) & MF_SEPARATOR) {
//        hMenu.RemoveMenu(iLastItem, MF_BYPOSITION);
//    }

    hMenu.LoadToolbar(IDR_TABLE_FRAME);
//  GetCursorPos(&point);
    CRect rectWin;
    pGrid->GetWindowRect(rectWin);
    int h = pGrid->GetTH_Height();
    int w = pGrid->GetSH_Width();
    hMenu.TrackPopupMenu (TPM_RIGHTBUTTON, rectWin.left + w + point.x, rectWin.top + h + point.y, this);
}

void CTabView::OnSelectAll()
{
    GetGrid()->SelectRange(0,0,GetGrid()->GetNumberCols()-1,GetGrid()->GetNumberRows()-1);
}

void CTabView::OnDeselectAll()
{
    int iCol;
    long lRow;
    GetGrid()->EnumFirstSelected(&iCol,&lRow);
    if (GetGrid()->EnumNextSelected(&iCol,&lRow) == 1) {
        // None selected - If Table Viewer called then exit
        if (((CTblViewApp*) AfxGetApp())->m_bCalledAsChild) {
            AfxGetMainWnd()->SendMessage(WM_COMMAND, ID_APP_EXIT);
        }
    }
    else {
        // Selected - Clear Selections
        GetGrid()->ClearSelections();
    }
}

void CTabView::OnMouseMove(UINT nFlags, CPoint point)
{
    /*CTabulateDoc* pDoc = GetDocument();

    CTabSet* pSpec = pDoc->GetTableSpec();

    CTable* pTable = m_pGrid->GetTable();
    CDataDict*      pDataDict = pSpec->GetDict();*/

    CView::OnMouseMove(nFlags, point);
}



/////////////////////////////////////////////////////////////////////////////////
//
//  int CTabView::GetDropCursor(CPoint point)
//
/////////////////////////////////////////////////////////////////////////////////
int CTabView::GetDropCursor(CPoint point)
{
    int iRet = -1;
    CTabulateDoc* pDoc = GetDocument();
    CRect rect;

    /////////////////////////////////////////////////////////////////////////////

    // make sure point is inside the view
    GetWindowRect(&rect);
    if(!rect.PtInRect(point)){
        return DictCursor::Arrow;
    }
    if (!IsDropPointValid(point)) {
        return DictCursor::NoDrop;
    }
    CDDTreeCtrl* pDictTree = pDoc->GetTabTreeCtrl()->GetDDTreeCtrl();
    if(pDictTree && pDictTree->m_bDragging){
        HTREEITEM hItem  = pDictTree->GetSelectedItem();
        LPARAM lParam = pDictTree->GetItemData(hItem);
        DictTreeNode* dict_tree_node = reinterpret_cast<DictTreeNode*>(lParam);
        if(dict_tree_node->GetDictElementType() == DictElementType::Dictionary ||
           dict_tree_node->GetDictElementType() == DictElementType::Level ||
           dict_tree_node->GetDictElementType() == DictElementType::Record )
        {
            return DictCursor::NoDrop;
        }
    }
    m_pGrid->ScreenToClient(&point);
    point.x = point.x - m_pGrid->m_GI->m_sideHdgWidth;
    point.y = point.y - m_pGrid->m_GI->m_topHdgHeight;


    TB_DROP_TYPE eDropType = GetDropType(point);
    TB_DROP_OPER eDropOper = GetDropOperation(point);
    iRet = DictCursor::NoDrop;

    if (eDropType == TB_DROP_ROW) {
        switch( eDropOper) {
            case TB_DROP_PLUS:
                iRet = DictCursor::RowPlus;
                break;
            case TB_DROP_STAR:
                iRet = DictCursor::RowStar;
                break;
            case TB_DROP:
                iRet = DictCursor::Row;
                break;
            case TB_DROP_STARPLUS:
                iRet = DictCursor::RowStarPlus;
                break;
        }
    }
    else if (eDropType == TB_DROP_COL) {
        switch( eDropOper) {
            case TB_DROP_PLUS:
                iRet = DictCursor::ColumnPlus;
                break;
            case TB_DROP_STAR:
                iRet = DictCursor::ColumnStar;
                break;
            case TB_DROP:
                iRet = DictCursor::Column;
                break;
            case TB_DROP_STARPLUS:
                iRet = DictCursor::ColumnStarPlus;
                break;
        }
    }
    else {
        return DictCursor::NoDrop;
    }
    if(m_pGrid->IsGridVarDrop() && iRet != DictCursor::NoDrop){
        CTabVar* pTargetTabVar = GetTargetItem(point);
        ASSERT(pTargetTabVar);
        CTabVar* pSItem = m_pGrid->GetDragSourceItem();
        if(!pSItem){
            iRet = DictCursor::NoDrop;
        }
        else if(pSItem == pTargetTabVar || pTargetTabVar->GetParent() == pSItem){
            iRet = DictCursor::NoDrop;
        }
    }
    return iRet;
}

bool CTabView::IsTableBlank()
{
    bool bRet = true;
    if(this->GetGrid()){
        CTable* pTable = this->GetGrid()->GetTable();
        int iNumColChildren = pTable->GetColRoot()->GetNumChildren();
        int iNumRowChildren = pTable->GetRowRoot()->GetNumChildren();
        bRet = (iNumColChildren ==0 &&  iNumRowChildren ==0);
    }
    return bRet;
}

bool CTabView::IsTblOnlyRoworOnlyCol()
{
    bool bOnlyColOrOnlyRowState = false;
    if(this->GetGrid()){
        CTable* pTable = this->GetGrid()->GetTable();
        int iNumColChildren = pTable->GetColRoot()->GetNumChildren();
        int iNumRowChildren = pTable->GetRowRoot()->GetNumChildren();
        bOnlyColOrOnlyRowState = ( iNumColChildren ==0 && iNumRowChildren !=0 )|| ( iNumColChildren!=0 && iNumRowChildren ==0 );
    }
    return bOnlyColOrOnlyRowState;
}

bool CTabView::IsDropValidDictItem(const DictTreeNode& dict_tree_node)
{
    const DictLevel* pDictLevel = nullptr;
    const CDictRecord* pDictRecord = nullptr;
    const CDictItem* pDictItem = nullptr;
    bool bRet = false;

    CIMSAString csName, csLabel;
    CRect rect;

    CTabulateDoc* pDoc = GetDocument();
    CTabSet* pSpec = pDoc->GetTableSpec();

    CTable* pTable = m_pGrid->GetTable();
    const CDataDict* pDataDict = dict_tree_node.GetDDDoc()->GetDict();
    ASSERT(pDataDict);
    bool bValidDict = pDataDict == pSpec->GetDict() || pSpec->GetWorkDict() == pDataDict;

    if(!bValidDict){
        return bRet;
    }
    DictElementType dict_element_type = dict_tree_node.GetDictElementType();

    int iLevel = dict_tree_node.GetLevelIndex();
    int iRec   = dict_tree_node.GetRecordIndex();
    int iItem  = dict_tree_node.GetItemIndex();
    int iVSet  = dict_tree_node.GetValueSetIndex();

    switch (dict_element_type)
    {
        case DictElementType::Item :
        {
            pDictItem = pDataDict->GetLevel(iLevel).GetRecord(iRec)->GetItem(iItem);
            if (!pDictItem->HasValueSets()) {
                if (pDictItem->GetContentType() != ContentType::Numeric) {
                    AfxMessageBox(_T("Items must have value sets to be tabulated."));
                }
                else {
                    AfxMessageBox(_T("Items must have value sets to be tabulated.\n\nYou can use Generate Value Set in the Data Dictionary to help you."));
                }
                return bRet;
            }
            else {
                iVSet = 0;
                const DictValueSet& dict_value_set = pDictItem->GetValueSet(iVSet);
                if (!dict_value_set.HasValues()) {
                    if (pDictItem->GetContentType() != ContentType::Numeric) {
                        AfxMessageBox(_T("Items must have value sets with values to be tabulated."));
                    }
                    else {
                        AfxMessageBox(_T("Items must have value sets with values to be tabulated.\n\nYou can use Generate Value Set in the Data Dictionary to help you."));
                    }
                    return bRet;
                }
                csName = dict_value_set.GetName();
                csLabel = dict_value_set.GetLabel();
            }
            break;
        }
        case DictElementType::ValueSet :
        {
            pDictItem = pDataDict->GetLevel(iLevel).GetRecord(iRec)->GetItem(iItem);
            const DictValueSet& dict_value_set = pDictItem->GetValueSet(iVSet);
            if (!dict_value_set.HasValues()) {
                if (pDictItem->GetContentType() != ContentType::Numeric) {
                    AfxMessageBox(_T("Items must have value sets with values to be tabulated."));
                }
                else {
                    AfxMessageBox(_T("Items must have value sets with values to be tabulated.\n\nYou can use Generate Value Set in the Data Dictionary to help you."));
                }
                return bRet;
            }
            csName = dict_value_set.GetName();
            csLabel = dict_value_set.GetLabel();
            break;
        }
        default:
        {
            return bRet;
        }
    }

    const DictValueSet* pDictVSet;
    const CDataDict* pDict = pSpec->LookupName(csName, &pDictLevel, &pDictRecord, &pDictItem, &pDictVSet);
    if (!pDict) {
        CIMSAString csMsg = csName;
        csMsg += _T(" not found in dictionary");
        AfxMessageBox (csMsg);
        return bRet;
    }
    return true;
}

TB_DROP_TYPE CTabView::GetDropType(CPoint point)
{
    TB_DROP_TYPE eDropType = TB_DROP_INVALID;

    if(IsTableBlank()){//Is Blank Drop
        int iX = point.x ;//- m_pGrid->m_GI->m_sideHdgWidth;
        int iY = point.y ;//- m_pGrid->m_GI->m_topHdgHeight;
        if (iX > iY) {// diagonal rule
            eDropType = TB_DROP_COL;
        }
        else {
            eDropType = TB_DROP_ROW;
        }
    }
    else if (IsStubDrop(point)){
        eDropType = GetDropType4Stub(point);
    }
    else if (IsTblOnlyRoworOnlyCol()){//IsTblOnlyRoworOnlyCol Drop
        eDropType = GetDropType4RorColOnly(point);
    }
    else {
            eDropType = GetDropType4RC(point);
        }
    return eDropType;
}

TB_DROP_TYPE CTabView::GetDropType4RorColOnly(CPoint point)
{
    TB_DROP_TYPE eDropType = TB_DROP_INVALID;

    ASSERT(IsTblOnlyRoworOnlyCol()); //Only Special case of only row or only col variable in the table

    CTable* pTable = m_pGrid->GetTable();
    int iNumColChildren = pTable->GetColRoot()->GetNumChildren();
    int iNumRowChildren = pTable->GetRowRoot()->GetNumChildren();

    int iGridCol;
    long iGridRow;
    iGridCol = iGridRow =INVALID_ROW_COL;

    CUGCell cellGrid;
    m_pGrid->GetCellFromPoint (point.x, point.y, &iGridCol, &iGridRow); // func in zGridO
    m_pGrid->GetCell (iGridCol,iGridRow,&cellGrid);

    /*CIMSAString sMsg;
    sMsg.Format("iCol = %d , iRow =%d\n" ,iGridCol,iGridRow);
    TRACE(sMsg);*/

    if(iNumColChildren == 0 ) {
        if(iGridCol == INVALID_ROW_COL) {
            eDropType =  TB_DROP_COL;
        }
        else if (iGridRow== INVALID_ROW_COL) {
            eDropType = TB_DROP_ROW;
        }
        else if (iGridCol != INVALID_ROW_COL) {
            eDropType = TB_DROP_ROW;
        }
    }
    else if(iNumRowChildren == 0){
        if(iGridRow == INVALID_ROW_COL) {
            eDropType = TB_DROP_ROW;
        }
        else if (iGridCol == INVALID_ROW_COL) {
            eDropType = TB_DROP_COL;
        }
        else if (iGridRow != INVALID_ROW_COL) {
            eDropType = TB_DROP_COL;
        }
    }
    return eDropType;
}

TB_DROP_OPER CTabView::GetDropOperation(CPoint point)
{
    TB_DROP_OPER  eDropOper = TB_DROP_NOOP;
    if(IsTableBlank()){//Is Blank Drop
        eDropOper = TB_DROP;
    }
    else if(IsStubDrop(point)){
        eDropOper = TB_DROP_PLUS;
    }
    else if (IsTblOnlyRoworOnlyCol()){//IsTblOnlyRoworOnlyCol Drop
        eDropOper = GetDropOp4RorColOnly(point);
    }
    else {
        eDropOper = GetDropOp4RC(point);
    }

    return eDropOper;

}

TB_DROP_OPER CTabView::GetDropOp4RorColOnly(CPoint point)
{
    TB_DROP_OPER  eDropOper = TB_DROP_NOOP;
    ASSERT(IsTblOnlyRoworOnlyCol()); //Only Special case of only row or only col variable in the table

    CTable* pTable = m_pGrid->GetTable();
    int iNumColChildren = pTable->GetColRoot()->GetNumChildren();
    int iNumRowChildren = pTable->GetRowRoot()->GetNumChildren();

    int iGridCol;
    long iGridRow;
    iGridCol = iGridRow =INVALID_ROW_COL;

    CUGCell cellGrid;
    m_pGrid->GetCellFromPoint (point.x, point.y, &iGridCol, &iGridRow); // func in zGridO
    m_pGrid->GetCell (iGridCol,iGridRow,&cellGrid);

    /*CIMSAString sMsg;
    sMsg.Format("iCol = %d , iRow =%d\n" ,iGridCol,iGridRow);
    TRACE(sMsg);*/
    if(iNumColChildren == 0 ) {
        if(iGridCol == INVALID_ROW_COL) {
            eDropOper =  TB_DROP;
        }
        else if (iGridRow== INVALID_ROW_COL) {
            eDropOper =  TB_DROP_PLUS;
        }
        else if (iGridCol != INVALID_ROW_COL) {
            if(IsTransitionPoint(point)){
                eDropOper =  TB_DROP_PLUS;
            }
            else if(!IsStarPlusDrop(point)){
                eDropOper =  TB_DROP_STAR;
            }
            else {
                eDropOper =  TB_DROP_STARPLUS;
            }
        }
    }
    else if(iNumRowChildren == 0){
        if(iGridRow == INVALID_ROW_COL) {
            eDropOper =  TB_DROP;
        }
        else if (iGridCol == INVALID_ROW_COL) {
            eDropOper =  TB_DROP_PLUS;
        }
        else if (iGridRow != INVALID_ROW_COL) {
            if(IsTransitionPoint(point)){
                eDropOper =  TB_DROP_PLUS;
            }
            else if(!IsStarPlusDrop(point)){
                eDropOper =  TB_DROP_STAR;
            }
            else {
                eDropOper =  TB_DROP_STARPLUS;
            }
        }
    }
    /*if(m_pGrid->IsGridVarDrop() && eDropOper !=TB_DROP_NOOP){
        CTabVar* pTargetTabVar = GetTargetItem(point);
        ASSERT(pTargetTabVar);
        CTabVar* pSItem = m_pGrid->GetDragSourceItem();
        ASSERT(pSItem);
        if(pSItem == pTargetTabVar || pTargetTabVar->GetParent() == pSItem){
            eDropOper =  TB_DROP_NOOP;
        }
    }*/
    return eDropOper;

}
bool CTabView::GetDictInfo(const DictTreeNode& dict_tree_node, DICT_LOOKUP_INFO& lookupInfo)
{
    bool bRet = false;
    const DictLevel* pDictLevel = nullptr;
    const CDictRecord* pDictRecord = nullptr;
    const CDictItem* pDictItem = nullptr;

    CIMSAString csName, csLabel;
    CRect rect;

    CTabulateDoc* pDoc = GetDocument();
    CTabSet* pSpec = pDoc->GetTableSpec();

    CTable* pTable = m_pGrid->GetTable();
    const CDataDict* pDataDict = dict_tree_node.GetDDDoc()->GetDict();

    lookupInfo.iOcc = -1;
    DictElementType dict_element_type = dict_tree_node.GetDictElementType();

    int iLevel = dict_tree_node.GetLevelIndex();
    int iRec   = dict_tree_node.GetRecordIndex();
    int iItem  = dict_tree_node.GetItemIndex();
    int iVSet  = dict_tree_node.GetValueSetIndex();
    if(dict_tree_node.GetItemOccurs() >= 0) {
        lookupInfo.iOcc = dict_tree_node.GetItemOccurs();
    }
    switch (dict_element_type) 
    {
        case DictElementType::Item:
        {
            pDictItem = pDataDict->GetLevel(iLevel).GetRecord(iRec)->GetItem(iItem);
            if (!pDictItem->HasValueSets()) {
                csName =    pDictItem->GetName();
                csLabel =   pDictItem->GetLabel();
            }
            else {
                iVSet = 0;
                const DictValueSet& dict_value_set = pDataDict->GetLevel(iLevel).GetRecord(iRec)->GetItem(iItem)->GetValueSet(iVSet);
                csName = dict_value_set.GetName();
                csLabel = dict_value_set.GetLabel();
            }
            break;
        }
        case DictElementType::ValueSet:
        {
            pDictItem = pDataDict->GetLevel(iLevel).GetRecord(iRec)->GetItem(iItem);
            const DictValueSet& dict_value_set = pDictItem->GetValueSet(iVSet);
            csName = dict_value_set.GetName();
            csLabel = dict_value_set.GetLabel();
            break;
        }
        default:
        {
            return bRet;
        }
    }
    lookupInfo.csName = csName;
    const CDataDict* pDict = pSpec->LookupName(lookupInfo.csName, &lookupInfo.pLevel, &lookupInfo.pRecord, &lookupInfo.pItem, &lookupInfo.pVSet);
    if (pDict) {
        pSpec->LookupName(lookupInfo.csName, &lookupInfo.iLevel, &lookupInfo.iRecord, &lookupInfo.iItem, &lookupInfo.iVSet);
        return true;
    }
    return bRet;
}
/////////////////////////////////////////////////////////////////////////////////
//
//  void CTabView::RemoveSystemTotalVar()
//
/////////////////////////////////////////////////////////////////////////////////
void CTabView::RemoveSystemTotalVar()
{
    CTable* pTable = m_pGrid->GetTable();
    CTabVar* pRowRoot = pTable->GetRowRoot();
    CTabVar* pColRoot = pTable->GetColRoot();
    CTabulateDoc* pDoc = GetDocument();
    CTabSet* pSpec = pDoc->GetTableSpec();
    int iTotalVarIndex = -1;
    bool bHasSystemTotalInRow = false;
    bool bHasSystemTotalInCol = false;
    if(pRowRoot->GetNumChildren() > 0 ){
        for(int iIndex =0 ; iIndex < pRowRoot->GetNumChildren(); iIndex++){
            if(pRowRoot->GetChild(iIndex)->GetName().CompareNoCase(WORKVAR_TOTAL_NAME) == 0){
                iTotalVarIndex = iIndex;
                bHasSystemTotalInRow = true;
                break;
            }
        }
    }
    if(pColRoot->GetNumChildren() > 0 ){
        for(int iIndex =0 ; iIndex < pColRoot->GetNumChildren(); iIndex++){
            if(pColRoot->GetChild(iIndex)->GetName().CompareNoCase(WORKVAR_TOTAL_NAME) == 0){
                iTotalVarIndex = iIndex;
                bHasSystemTotalInCol = true;
                break;
            }
        }
    }
    ASSERT(!(bHasSystemTotalInCol && bHasSystemTotalInRow));

    if(bHasSystemTotalInRow) {
        bool bRemoveVar = false;
        if(pTable->GetColRoot()->GetNumChildren() ==0){
            bRemoveVar  =true;
        }
        else if(pTable->GetRowRoot()->GetNumChildren() > 1){
            bRemoveVar  =true;
        }
        if(bRemoveVar){
            CTabVar* pRemoveVar = pTable->GetRowRoot()->GetChild(iTotalVarIndex);
            if(pTable->GetRowRoot()->GetChild(iTotalVarIndex)->GetName().CompareNoCase(WORKVAR_TOTAL_NAME) == 0){
                // sys total has special fmt that should be cleaned up
                CTallyFmt* pFmt = pRemoveVar->GetTallyFmt();
                ASSERT(pFmt);
                pSpec->GetFmtRegPtr()->Remove(pFmt->GetID(), pFmt->GetIndex());
                pRemoveVar->Remove();
                delete pRemoveVar;
            }
        }
    }
    else if(bHasSystemTotalInCol){
        bool bRemoveVar = false;
        if(pTable->GetRowRoot()->GetNumChildren() ==0){
            bRemoveVar  =true;
        }
        else if(pTable->GetColRoot()->GetNumChildren() > 1){
            bRemoveVar  =true;
        }

        if(bRemoveVar){
            CTabVar* pRemoveVar = pTable->GetColRoot()->GetChild(iTotalVarIndex);
            if(pTable->GetColRoot()->GetChild(iTotalVarIndex)->GetName().CompareNoCase(WORKVAR_TOTAL_NAME) == 0){
                // sys total has special fmt that should be cleaned up
                CTallyFmt* pFmt = pRemoveVar->GetTallyFmt();
                ASSERT(pFmt);
                pSpec->GetFmtRegPtr()->Remove(pFmt->GetID(), pFmt->GetIndex());
                pRemoveVar->Remove();
                delete pRemoveVar;
            }
        }
    }
}
/////////////////////////////////////////////////////////////////////////////////
//
//  void CTabView::AddSystemTotalVar()
//
/////////////////////////////////////////////////////////////////////////////////
void CTabView::AddSystemTotalVar()
{
    bool bRow =true;
    const DictValueSet* pDictVSet =nullptr;
    const CDictItem* pDictItem = nullptr;
    const CDictRecord* pDictRecord = nullptr;
    const DictLevel* pDictLevel = nullptr;

    CTabulateDoc* pDoc = GetDocument();
    CTabSet* pSpec = pDoc->GetTableSpec();
    CTable* pTable = m_pGrid->GetTable();
    const CDataDict* pDataDict = pSpec->GetDict();
    const CDataDict* pWorkDict = pSpec->GetWorkDict();
     if(pTable->GetRowRoot()->GetNumChildren() ==0 && pTable->GetColRoot()->GetNumChildren() ==0){
         return;
     }

    if(pTable->GetRowRoot()->GetNumChildren() ==0){
        bRow = true;
    }
    else if(pTable->GetColRoot()->GetNumChildren() ==0){
        bRow = false;
    }
    else {//Remove If it already exists ??
        return ;
    }
    ASSERT(pWorkDict);

    const CDataDict* pDict = pSpec->LookupName(WORKVAR_TOTAL_NAME,&pDictLevel,&pDictRecord,&pDictItem,&pDictVSet);
    if(!pDict){
        ASSERT(FALSE);
        CIMSAString sMsg;
        sMsg.Format(_T("Cannot find variable %s"), (LPCTSTR)WORKVAR_TOTAL_NAME);
        AfxMessageBox(sMsg);
        return;
    }
    CTabVar* pAddToTabVar = nullptr;
    bRow ? pAddToTabVar = pTable->GetRowRoot() : pAddToTabVar = pTable->GetColRoot();
    ASSERT(pAddToTabVar->GetNumChildren() <= 1);
    if(pAddToTabVar->GetNumChildren() == 1) {
        bool bSystemTotalExists = pAddToTabVar->GetChild(0)->GetName().CompareNoCase(WORKVAR_TOTAL_NAME) ==0;
        ASSERT(bSystemTotalExists);
        return;
    }
    else {
        //Do lookup and make sure the item exists .
        CTallyFmt* pDefTallyFmt = nullptr;
        FMT_ID eFmtID = FMT_ID_INVALID;
        bRow ? eFmtID = FMT_ID_TALLY_ROW :eFmtID = FMT_ID_TALLY_COL;
        pDefTallyFmt= DYNAMIC_DOWNCAST(CTallyFmt,pSpec->GetFmtReg().GetFmt(eFmtID));


        ASSERT(pDefTallyFmt);

        //Create a new TabVar for SYSTEM_TOTAL_NAME;
        CStringArray arrVals;
       // arrVals.Add("Total"); //Get the string from foriegn keys
        MakeDummyVSet(pDictItem,arrVals);

        // make special fmt for sys total that contains only total stat (no freqs)
        CTallyFmt* pSysTotalVarFmt = new CTallyFmt;
        MakeDefTallyFmt4Dlg(pSysTotalVarFmt,pDefTallyFmt);
        pSysTotalVarFmt->ClearStats();
        pSysTotalVarFmt->AddStat(CTallyVarStatFmtFactory::GetInstance()->Create(_T("Total")));
        pSpec->GetFmtRegPtr()->AddFmt(pSysTotalVarFmt);

        CTabVar* pTabVar = new CTabVar(pDictItem,arrVals,*pSysTotalVarFmt);
        pTabVar->SetOcc(-1);
        pTabVar->SetTallyFmt(pSysTotalVarFmt);
        pAddToTabVar->AddChildVar(pTabVar);
    }
}

CTabVar* CTabView::AddRowVarPlus(DICT_LOOKUP_INFO& lookupInfo,CTabVar* pTargetItem, bool bAfter /*= true*/ )
{
    const DictValueSet* pDictVSet =nullptr;
    const CDictItem* pDictItem = nullptr;
    CTabulateDoc* pDoc = GetDocument();
    CTabSet* pSpec = pDoc->GetTableSpec();
    CTabVar* pTabVar = nullptr;
    CTable* pTable = m_pGrid->GetTable();
    const CDataDict* pDataDict = pSpec->GetDict();
    int iTabVarOccs =-1;

    CTallyFmt* pTableTallyFmt = nullptr;
    pTableTallyFmt= DYNAMIC_DOWNCAST(CTallyFmt,pSpec->GetFmtReg().GetFmt(FMT_ID_TALLY_ROW));
    ASSERT(pTableTallyFmt);


    bool bGridVarDrop = m_pGrid->IsGridVarDrop();
    if(!bGridVarDrop){
        pDictItem = lookupInfo.pItem;
        pDictVSet = lookupInfo.pVSet;
    }
    else {
        pTabVar = m_pGrid->GetDragSourceItem();
        ASSERT(pTabVar);
        pTabVar->Remove();//Remove it from its current position without deleting
    }
    if(!bGridVarDrop && pDictItem){
        iTabVarOccs = GetOcc4TabVar(lookupInfo);
    }
    if(!pTargetItem || pTargetItem == pTable->GetRowRoot()) {
        CTabVar* pRowRoot =  pTable->GetRowRoot();
        if(!pTabVar){
            if(pDictVSet) {
                pTabVar= new CTabVar(pDictVSet,*pTableTallyFmt);
            }
            else {
                CStringArray arrVals;
                MakeDummyVSet(pDictItem,arrVals);
                pTabVar= new CTabVar(pDictItem,arrVals,*pTableTallyFmt);
            }
            pTabVar->SetOcc(iTabVarOccs);

        }
        pRowRoot->AddChildVar(pTabVar);
    }
    else if(pTargetItem){
        //check if this target item has children  if yes
        CTabVar* pParentVar = nullptr;
        CTabVar* pAfterItem = nullptr;
        CTabVar* pRowRoot =  pTable->GetRowRoot();
        if(pTargetItem->GetNumChildren() > 0 ) {
            pParentVar =pTargetItem;
            pAfterItem = nullptr;
        }
        else if(pTargetItem->GetParent() != pRowRoot){
            pParentVar = pTargetItem->GetParent();
            if(bAfter){
                pAfterItem = pTargetItem;
            }
        }
        else { //Transition drop //Check if this causes problems
            pParentVar = pTargetItem->GetParent();
            if(bAfter){
                pAfterItem = pTargetItem;
            }
        }
        //check if the taget item parent is not rowroot
        //now add the var and set the parent appropriately
        if(pParentVar){
            if(!pTabVar) {
                if(pDictVSet) {
                    pTabVar= new CTabVar(pDictVSet,*pTableTallyFmt);
                }
                else {
                    CStringArray arrVals;
                    MakeDummyVSet(pDictItem,arrVals);
                    pTabVar= new CTabVar(pDictItem,arrVals,*pTableTallyFmt);
                }

                pTabVar->SetOcc(iTabVarOccs);
            }
            if(!pAfterItem){
                pAfterItem = pParentVar->GetChild(0);
                pAfterItem->InsertSibling(pTabVar);
            }
            else {
                pAfterItem->InsertSibling(pTabVar,true);
            }
        }

    }
    return pTabVar;
}

CTabVar* CTabView::AddColVarPlus(DICT_LOOKUP_INFO& lookupInfo,CTabVar* pTargetItem, bool bAfter  /*= true*/  )
{
    const DictValueSet* pDictVSet =nullptr;
    const CDictItem* pDictItem = nullptr;
    CTabulateDoc* pDoc = GetDocument();
    CTabSet* pSpec = pDoc->GetTableSpec();
    CTabVar* pTabVar = nullptr;
    CTable* pTable = m_pGrid->GetTable();
    const CDataDict* pDataDict = pSpec->GetDict();
    int iTabVarOccs =-1;

    CTallyFmt* pTableTallyFmt = nullptr;
    pTableTallyFmt = DYNAMIC_DOWNCAST(CTallyFmt,pSpec->GetFmtReg().GetFmt(FMT_ID_TALLY_COL));
    ASSERT(pTableTallyFmt);

    bool bGridVarDrop = m_pGrid->IsGridVarDrop();
    if(!bGridVarDrop){
        pDictItem = lookupInfo.pItem;
        pDictVSet = lookupInfo.pVSet;
    }
    else {
        pTabVar = m_pGrid->GetDragSourceItem();
        ASSERT(pTabVar);
        pTabVar->Remove();//Remove it from its current position without deleting
    }
    if(!bGridVarDrop && pDictItem){
        iTabVarOccs = GetOcc4TabVar(lookupInfo);
    }
    if(!pTargetItem || pTargetItem == pTable->GetColRoot()){
        CTabVar* pColRoot =  pTable->GetColRoot();
        if(!pTabVar){
            if(pDictVSet) {
                pTabVar= new CTabVar(pDictVSet,*pTableTallyFmt);
            }
            else {
                CStringArray arrVals;
                MakeDummyVSet(pDictItem,arrVals);
                pTabVar= new CTabVar(pDictItem,arrVals,*pTableTallyFmt);
            }
            pTabVar->SetOcc(iTabVarOccs);
        }
        pColRoot->AddChildVar(pTabVar);
    }
    else if(pTargetItem){
        //check if this target item has children  if yes
        CTabVar* pParentVar = nullptr;
        CTabVar* pAfterItem = nullptr;
        CTabVar* pRowRoot =  pTable->GetColRoot();
        if(pTargetItem->GetNumChildren() > 0 ) {
            pParentVar =pTargetItem;
            pAfterItem = nullptr;
        }
        else if(pTargetItem->GetParent() != pRowRoot){
            pParentVar = pTargetItem->GetParent();
            if(bAfter){
                pAfterItem = pTargetItem;
            }
        }
        else { //Transition point code . Check if this causes problems
            pParentVar = pTargetItem->GetParent();
            if(bAfter){
                pAfterItem = pTargetItem;
            }
        }
        //check if the taget item parent is not rowroot
        //now add the var and set the parent appropriately
        if(pParentVar){
            if(!pTabVar){
                if(pDictVSet) {
                    pTabVar= new CTabVar(pDictVSet,*pTableTallyFmt);
                }
                else {
                    CStringArray arrVals;
                    MakeDummyVSet(pDictItem,arrVals);
                    pTabVar= new CTabVar(pDictItem,arrVals,*pTableTallyFmt);
                }

                pTabVar->SetOcc(iTabVarOccs);
            }
            if(!pAfterItem){
                pAfterItem = pParentVar->GetChild(0);
                pAfterItem->InsertSibling(pTabVar);
            }
            else {
                pAfterItem->InsertSibling(pTabVar,true);
            }
        }
    }
    return pTabVar;
}

CTabVar* CTabView::AddRowVarStar(DICT_LOOKUP_INFO& lookupInfo,CTabVar* pTargetItem, bool bAfter  /*= true*/  )
{
    const DictValueSet* pDictVSet =nullptr;
    const CDictItem* pDictItem = nullptr;
    CTabulateDoc* pDoc = GetDocument();
    CTabSet* pSpec = pDoc->GetTableSpec();
    CTabVar* pTabVar = nullptr;
    CTable* pTable = m_pGrid->GetTable();
    const CDataDict*      pDataDict = pSpec->GetDict();
    int iTabVarOccs =-1;

    CTallyFmt* pTableTallyFmt = nullptr;
    pTableTallyFmt = DYNAMIC_DOWNCAST(CTallyFmt,pSpec->GetFmtReg().GetFmt(FMT_ID_TALLY_ROW));
    ASSERT(pTableTallyFmt);

    bool bGridVarDrop = m_pGrid->IsGridVarDrop();
    if(!bGridVarDrop){
        pDictItem = lookupInfo.pItem;
        pDictVSet = lookupInfo.pVSet;
    }
    else {
        pTabVar = m_pGrid->GetDragSourceItem();
        ASSERT(pTabVar);
        pTabVar->Remove();//Remove it from its current position without deleting
    }
    if(!bGridVarDrop && pDictItem){
        iTabVarOccs = GetOcc4TabVar(lookupInfo);
    }
    if(pTargetItem) {
        if(!pTabVar){
            if(pDictVSet) {
                pTabVar= new CTabVar(pDictVSet,*pTableTallyFmt);
            }
            else {
                CStringArray arrVals;
                MakeDummyVSet(pDictItem,arrVals);
                pTabVar= new CTabVar(pDictItem,arrVals,*pTableTallyFmt);
            }

/*
better to set these when prepping for print view, since what appears to be a stub might turn out to be a caption ... csc 7/21/04
            // the variable has a default row group fmt ... csc 5/27/04
            ASSERT_VALID(pSpec->GetFmtReg().GetFmt(TFT_FMT_ID_ROWGRP));
            CFmt* pVarFmt = (CFmt*)pSpec->GetFmtReg().GetFmt(TFT_FMT_ID_ROWGRP);
            pTabVar->SetFmt(pVarFmt);

            // the variable's values have default row fmt ... csc 5/27/04
            ASSERT_VALID(pSpec->GetFmtReg().GetFmt(TFT_FMT_ID_ROW));
            CFmt* pValFmt = (CFmt*)pSpec->GetFmtReg().GetFmt(TFT_FMT_ID_ROW);
            for (int iChild=0 ; iChild<pTabVar->GetNumValues() ; iChild++) {
                pTabVar->GetValue(iChild)->SetFmt(pValFmt);
            }
*/
            pTabVar->SetOcc(iTabVarOccs);
        }
        pTargetItem->AddChildVar(pTabVar);
    }
    return pTabVar;
}

CTabVar* CTabView::AddColVarStar(DICT_LOOKUP_INFO& lookupInfo,CTabVar* pTargetItem, bool bAfter  /*= true*/  )
{
    const DictValueSet* pDictVSet =nullptr;
    const CDictItem* pDictItem = nullptr;
    CTabulateDoc* pDoc = GetDocument();
    CTabSet* pSpec = pDoc->GetTableSpec();
    CTabVar* pTabVar = nullptr;
    CTable* pTable = m_pGrid->GetTable();
    const CDataDict* pDataDict = pSpec->GetDict();
    int iTabVarOccs =-1;

    CTallyFmt* pTableTallyFmt = nullptr;
    pTableTallyFmt = DYNAMIC_DOWNCAST(CTallyFmt,pSpec->GetFmtReg().GetFmt(FMT_ID_TALLY_COL));
    ASSERT(pTableTallyFmt);

    bool bGridVarDrop = m_pGrid->IsGridVarDrop();
    if(!bGridVarDrop){
        pDictItem = lookupInfo.pItem;
        pDictVSet = lookupInfo.pVSet;
    }
    else {
        pTabVar = m_pGrid->GetDragSourceItem();
        ASSERT(pTabVar);
        pTabVar->Remove();//Remove it from its current position without deleting
        pTabVar->RemoveAllPrtViewInfo();
        for(int iIndex =0 ; iIndex < pTabVar->GetNumValues(); iIndex++){
            pTabVar->GetValue(iIndex)->RemoveAllPrtViewInfo();
        }
    }
    if(!bGridVarDrop && pDictItem){
        iTabVarOccs = GetOcc4TabVar(lookupInfo);
    }
    if(pTargetItem){
        if(!pTabVar){
            if(pDictVSet) {
                pTabVar= new CTabVar(pDictVSet,*pTableTallyFmt);
            }
            else {
                CStringArray arrVals;
                MakeDummyVSet(pDictItem,arrVals);
                pTabVar= new CTabVar(pDictItem,arrVals,*pTableTallyFmt);
            }

/*
better to set these when prepping for print view, since what appears to be a stub might turn out to be a caption ... csc 7/21/04
            // the variable has a default row group fmt ... csc 5/27/04
            ASSERT_VALID(pSpec->GetFmtReg().GetFmt(TFT_FMT_ID_ROWGRP));
            CFmt* pVarFmt = (CFmt*)pSpec->GetFmtReg().GetFmt(TFT_FMT_ID_ROWGRP);
            pTabVar->SetFmt(pVarFmt);

            // the variable's values have default row fmt ... csc 5/27/04
            ASSERT_VALID(pSpec->GetFmtReg().GetFmt(TFT_FMT_ID_ROW));
            CFmt* pValFmt = (CFmt*)pSpec->GetFmtReg().GetFmt(TFT_FMT_ID_ROW);
            for (int iChild=0 ; iChild<pTabVar->GetNumValues() ; iChild++) {
                pTabVar->GetValue(iChild)->SetFmt(pValFmt);
            }
*/
            pTabVar->SetOcc(iTabVarOccs);
        }
        pTargetItem->AddChildVar(pTabVar);
    }
    return pTabVar;
}
int CTabView::GetOcc4TabVar(const CDictItem* pDictItem)
{
    ASSERT(FALSE); ///Do not use this anymore
    int iTabVarOccs = -1;  // assign 1 to avoid compiler warnings; it's usually right
    int iDictOccs = pDictItem->GetOccurs();
    if (pDictItem->GetItemType() == ItemType::Subitem && pDictItem->GetOccurs() == 1) { // BMD 19 Sep 2001
        iDictOccs = pDictItem->GetParentItem()->GetOccurs();
    }
    //SAVY&&& check this when Occs are in
    int iFromTreeOccs = 0;// dict_tree_node->GetItemOccurs();
    if (iDictOccs == 1) {
        iTabVarOccs = -1;
    }
    if (iDictOccs > 1) {
        if (iFromTreeOccs == -1) {
            iTabVarOccs = -1;
        }
        else {
            iTabVarOccs = iFromTreeOccs + 1;
        }
    }
    return iTabVarOccs;
}
int CTabView::GetOcc4TabVar(DICT_LOOKUP_INFO& lookupInfo)
{
    const CDictItem* pDictItem = lookupInfo.pItem;
    int iTabVarOccs = -1;  // assign 1 to avoid compiler warnings; it's usually right
    int iDictOccs = pDictItem->GetOccurs();
    if (pDictItem->GetItemType() == ItemType::Subitem && pDictItem->GetOccurs() == 1) { // BMD 19 Sep 2001
        iDictOccs = pDictItem->GetParentItem()->GetOccurs();
    }
    if (iDictOccs == 1) {
        iTabVarOccs = -1;
    }
    if (iDictOccs > 1) {
        iTabVarOccs = lookupInfo.iOcc;//zero based
    }
    return iTabVarOccs;
}
CTabVar* CTabView::GetTargetItem(CPoint point)
{
    CTabVar* pRetVar = nullptr;
    CTable* pTable = m_pGrid->GetTable();
    TB_DROP_TYPE eDropType = GetDropType(point);
    TB_DROP_OPER eDropOper = GetDropOperation(point);


    if(IsTableBlank()){//Is Blank Drop
        if(eDropType == TB_DROP_ROW) {
            pRetVar = pTable->GetRowRoot();
        }
        else if(eDropType == TB_DROP_COL) {
            pRetVar = pTable->GetColRoot();
        }
        else {
            pRetVar = nullptr; //do nothing
        }
    }
    else if(IsStubDrop(point)){
        if(eDropType == TB_DROP_ROW) {
            pRetVar = pTable->GetRowRoot();
            if(pRetVar->GetNumChildren() >0){
                pRetVar = pRetVar->GetChild(0);
            }
        }
        else if(eDropType == TB_DROP_COL) {
            pRetVar = pTable->GetColRoot();
            if(pRetVar->GetNumChildren() >0){
                pRetVar = pRetVar->GetChild(0);
            }
        }
        else {
            pRetVar = nullptr; //do nothing
        }
    }
    else if (IsTblOnlyRoworOnlyCol()){//IsTblOnlyRoworOnlyCol Drop
        pRetVar = GetTabVarFromPoint(point);
        if(eDropType == TB_DROP_ROW) {
            if(!pRetVar) {
                pRetVar = pTable->GetRowRoot();
            }
        }
        else if(eDropType == TB_DROP_COL) {
            if(!pRetVar) {
                pRetVar = pTable->GetColRoot();
            }
        }
        else {
            pRetVar = nullptr; //do nothing
        }
    }
    else {
        pRetVar = GetTabVarFromPoint(point);
        TB_DROP_TYPE eDropType = GetDropType4RC(point);
        if(eDropType != TB_DROP_INVALID && pRetVar == nullptr) {
            if(eDropType == TB_DROP_ROW){
                pRetVar = pTable->GetRowRoot();
            }
            else if(eDropType == TB_DROP_COL){
                pRetVar = pTable->GetColRoot();
            }
        }
    }

    return pRetVar;
}


CTabVar* CTabView::GetTabVarFromPoint(CPoint point)
{
    CUGCell cellGrid;
    CTabVar* pRetVar = nullptr;
    int iGridCol;
    long iGridRow;
    iGridCol = iGridRow =INVALID_ROW_COL;

    m_pGrid->GetCellFromPoint (point.x, point.y, &iGridCol, &iGridRow); // func in zGridO

    m_pGrid->GetCell (iGridCol,iGridRow,&cellGrid);

    CTblOb** pTblOb = (CTblOb**) cellGrid.GetExtraMemPtr();
    if(pTblOb && (*pTblOb) && (*pTblOb)->IsKindOf(RUNTIME_CLASS(CGTblCol))){
        pRetVar = ((CGTblCol*)(*pTblOb))->GetTabVar();
    }
    else if(pTblOb && (*pTblOb) && (*pTblOb)->IsKindOf(RUNTIME_CLASS(CGTblRow))){
        pRetVar = ((CGTblRow*)(*pTblOb))->GetTabVar();
    }
    return pRetVar;
}
TB_DROP_TYPE CTabView::GetDropType4RC(CPoint point)
{
    //Get Drop type when both Row and col vars are present
    TB_DROP_TYPE eDropType = TB_DROP_INVALID;
    int iGridCol;
    long iGridRow;
    iGridCol = iGridRow =INVALID_ROW_COL;

    CUGCell cellGrid;
    m_pGrid->GetCellFromPoint (point.x, point.y, &iGridCol, &iGridRow); // func in zGridO
    m_pGrid->GetCell (iGridCol,iGridRow,&cellGrid);

    if(iGridCol == INVALID_ROW_COL && iGridRow != INVALID_ROW_COL){
        eDropType = TB_DROP_COL;
    }
    else if(iGridRow == INVALID_ROW_COL && iGridCol != INVALID_ROW_COL){
        eDropType = TB_DROP_ROW;
    }
    else {
        CTblOb** pTblOb = (CTblOb**) cellGrid.GetExtraMemPtr();
        if(pTblOb && (*pTblOb) && (*pTblOb)->IsKindOf(RUNTIME_CLASS(CGTblCol))){
            eDropType = TB_DROP_COL;
        }
        else if(pTblOb && (*pTblOb) && (*pTblOb)->IsKindOf(RUNTIME_CLASS(CGTblRow))){
            eDropType = TB_DROP_ROW;
        }
    }
    return eDropType;

}

TB_DROP_OPER CTabView::GetDropOp4RC(CPoint point)
{
    TB_DROP_OPER  eDropOper = TB_DROP_NOOP;

    CTable* pTable = m_pGrid->GetTable();

    int iGridCol;
    long iGridRow;
    iGridCol = iGridRow =INVALID_ROW_COL;

    CUGCell cellGrid;
    m_pGrid->GetCellFromPoint (point.x, point.y, &iGridCol, &iGridRow); // func in zGridO
    m_pGrid->GetCell (iGridCol,iGridRow,&cellGrid);

    if(iGridCol == INVALID_ROW_COL && iGridRow != INVALID_ROW_COL){
        eDropOper = TB_DROP_PLUS;
    }
    else if(iGridRow == INVALID_ROW_COL && iGridCol != INVALID_ROW_COL){
        eDropOper = TB_DROP_PLUS;
    }
    else {
        if(IsTransitionPoint(point)){
            eDropOper =  TB_DROP_PLUS;
        }
        else if(IsStarPlusDrop(point)){
            eDropOper = TB_DROP_STARPLUS;
        }
        else {
            CTblOb** pTblOb = (CTblOb**) cellGrid.GetExtraMemPtr();
            if(pTblOb && (*pTblOb) && (*pTblOb)->IsKindOf(RUNTIME_CLASS(CGTblCol))){
                CGTblCol* pTblCol = DYNAMIC_DOWNCAST(CGTblCol,*pTblOb);
                if(pTblCol->GetTabVar() && pTblCol->GetTabVar()->GetName().CompareNoCase(WORKVAR_TOTAL_NAME) ==0){
                    eDropOper = TB_DROP_PLUS;
                }
                else {
                    eDropOper = TB_DROP_STAR;
                }
            }
            else if(pTblOb && (*pTblOb) && (*pTblOb)->IsKindOf(RUNTIME_CLASS(CGTblRow))){
                CGTblRow* pTblRow= DYNAMIC_DOWNCAST(CGTblRow,*pTblOb);
                if(pTblRow->GetTabVar() && pTblRow->GetTabVar()->GetName().CompareNoCase(WORKVAR_TOTAL_NAME) ==0){
                    eDropOper = TB_DROP_PLUS;
                }
                else {
                    eDropOper = TB_DROP_STAR;
                }
            }
        }
    }
    return eDropOper;

}
bool  CTabView::IsStarPlusDrop(CPoint point)
{
    bool bRet = false;
    CTable* pTable = m_pGrid->GetTable();

    int iGridCol;
    long iGridRow;
    iGridCol = iGridRow =INVALID_ROW_COL;

    CUGCell cellGrid;
    m_pGrid->GetCellFromPoint (point.x, point.y, &iGridCol, &iGridRow); // func in zGridO
    m_pGrid->GetCell (iGridCol,iGridRow,&cellGrid);
    if(iGridCol != INVALID_ROW_COL && iGridRow != INVALID_ROW_COL) {
        CTabVar* pTabVar = nullptr;
        CTblOb** pTblOb = (CTblOb**) cellGrid.GetExtraMemPtr();
        bool bParentIsRoot =false;
        if(pTblOb && (*pTblOb) && (*pTblOb)->IsKindOf(RUNTIME_CLASS(CGTblCol))){
            pTabVar = ((CGTblCol*)(*pTblOb))->GetTabVar();
            if (pTabVar->GetParent() == pTable->GetColRoot()){
                bParentIsRoot = true;
            }
        }
        else if(pTblOb && (*pTblOb) && (*pTblOb)->IsKindOf(RUNTIME_CLASS(CGTblRow))){
            pTabVar = ((CGTblRow*)(*pTblOb))->GetTabVar();
            if (pTabVar && pTabVar->GetParent() == pTable->GetRowRoot()){
                bParentIsRoot = true;
            }
        }
        if(pTabVar) {
            if(pTabVar->GetNumChildren() > 0) {
                bRet = true;
            }
            else if (!bParentIsRoot){
                bRet = true;
            }
        }
    }
    return bRet;
}

bool  CTabView::IsTransitionPoint(CPoint point)
{
    bool bRet  = false;
    CTable* pTable = m_pGrid->GetTable();

    //Rule for transition
    //between The drop item and the next item (if it is not the first one )
    //The tabvars shld be different
    //atleast one ParentVar shld  be the root
    //If both the ParentVars are root then transition is true if the point test is valid
    //if none of the parentvars are root the transition is false
    //if only one parentvar is root (say TabVar1) then the other item shldnot be the child of (TabVar1)
    //and point test shld be valid in this  for transition to be true
    int iGridCol;
    long iGridRow;
    iGridCol = iGridRow =INVALID_ROW_COL;
    int iNumCols = m_pGrid->GetNumberCols();
    int iHeaderRows = m_pGrid->GetNumHeaderRows();
    CUGCell cellGrid;
    m_pGrid->GetCellFromPoint (point.x, point.y, &iGridCol, &iGridRow); // func in zGridO
    m_pGrid->GetCell (iGridCol,iGridRow,&cellGrid);
    CTabVar* pTabVar1 = nullptr;
    CTabVar* pTabVar2 = nullptr;

    CTabVar* pRowRoot = pTable->GetRowRoot();
    CTabVar* pColRoot = pTable->GetColRoot();

    if(iGridCol != INVALID_ROW_COL && iGridRow != INVALID_ROW_COL) {
        CTblOb** pTblOb = (CTblOb**) cellGrid.GetExtraMemPtr();
        bool bParentIsRoot =false;
        if(pTblOb && (*pTblOb) && (*pTblOb)->IsKindOf(RUNTIME_CLASS(CGTblCol))){
            pTabVar1 = ((CGTblCol*)(*pTblOb))->GetTabVar();
            int iStartCol = iGridCol ;
            long iStartRow  = iGridRow;
            int iEndCol = iGridCol;
            long iEndRow = iGridRow;
            m_pGrid->GetJoinRange(&iStartCol,&iStartRow,&iEndCol,&iEndRow);



            bool bForward = true;
            bool bPass = false;
            //Get Next TabVar
            if(iEndCol + 1  < iNumCols ) {
                m_pGrid->GetCell (iEndCol + 1,iGridRow,&cellGrid);
                CTblOb** pTblOb2 = (CTblOb**) cellGrid.GetExtraMemPtr();
                if(pTblOb2 && (*pTblOb2) && (*pTblOb2)->IsKindOf(RUNTIME_CLASS(CGTblCol))){
                    pTabVar2 = ((CGTblCol*)(*pTblOb2))->GetTabVar();
                }
                bPass = DoRuleCheck(pTabVar1 ,pTabVar2,true);
            }
            if(!bPass) { //Do the backward check
                if(iStartCol - 1  > 0) {
                    iStartCol = iStartCol - 1;
                    iStartRow = iGridRow;
                    m_pGrid->GetJoinStartCell(&iStartCol,&iStartRow,&cellGrid);
                    //                  m_pGrid->GetCell (iStartCol - 1 ,iGridRow,&cellGrid);
                }
                else {
                    return bRet;
                }
                CTblOb** pTblOb2 = (CTblOb**) cellGrid.GetExtraMemPtr();
                if(pTblOb2 && (*pTblOb2) && (*pTblOb2)->IsKindOf(RUNTIME_CLASS(CGTblCol))){
                    pTabVar2 = ((CGTblCol*)(*pTblOb2))->GetTabVar();
                }
                else {
                    return bRet;
                }

                bPass = DoRuleCheck(pTabVar1 ,pTabVar2,true);
                bForward = false;
            }
            if (bPass){
                //Temp test ( you shld do the above conditions on next item to drop and prev item to drop
                //the this test will vary depending on the current item /prev item combo success
                //or the current item /next item combo
                //Get the cell rect
                CRect rect ;
                m_pGrid->GetCellRect(iGridCol,iGridRow,&rect);
                //Is point in 5 pixel zone
                if(bForward){
                    bRet =rect.right - point.x  <= 5;
                }
                else {
                    bRet = point.x - rect.left   <= 5;
                }
            }

        }
        else if(pTblOb && (*pTblOb) && (*pTblOb)->IsKindOf(RUNTIME_CLASS(CGTblRow))){
            pTabVar1 = ((CGTblRow*)(*pTblOb))->GetTabVar();
            bool bForward = true;
            //Get Next TabVar
            if(iGridRow + 1  < m_pGrid->GetNumberRows() ) {
                m_pGrid->GetCell (iGridCol,iGridRow + 1,&cellGrid);
            }
            else {
                return bRet;
            }
            CTblOb** pTblOb2 = (CTblOb**) cellGrid.GetExtraMemPtr();
            if(pTblOb2 && (*pTblOb2) && (*pTblOb2)->IsKindOf(RUNTIME_CLASS(CGTblRow))){
                pTabVar2 = ((CGTblCol*)(*pTblOb2))->GetTabVar();
            }
            else {
                return bRet;
            }
            bool bPass = DoRuleCheck(pTabVar1 ,pTabVar2,false);
            if(!bPass) { //Do the backward check
                //Get Next TabVar
                if(iGridRow - 1  >0 ) {
                    m_pGrid->GetCell (iGridCol,iGridRow - 1,&cellGrid);
                }
                else {
                    return bRet;
                }
                CTblOb** pTblOb2 = (CTblOb**) cellGrid.GetExtraMemPtr();
                if(pTblOb2 && (*pTblOb2) && (*pTblOb2)->IsKindOf(RUNTIME_CLASS(CGTblRow))){
                    pTabVar2 = ((CGTblCol*)(*pTblOb2))->GetTabVar();
                }
                else {
                    return bRet;
                }
                bPass = DoRuleCheck(pTabVar1 ,pTabVar2,false);
                bForward = false;
            }
            if(bPass){
                //Temp test ( you shld do the above conditions on next item to drop and prev item to drop
                //the this test will vary depending on the current item /prev item combo success
                //or the current item /next item combo
                //Get the cell rect
                CRect rect ;
                m_pGrid->GetCellRect(iGridCol,iGridRow,&rect);
                //Is point in 5 pixel zone
                if(bForward){
                    bRet = rect.bottom - point.y <= 5;
                }
                else{
                    bRet = point.y - rect.top <= 5;
                }
            }
        }
    }
    return bRet;
}

bool  CTabView::DoRuleCheck(CTabVar* pTabVar1 , CTabVar* pTabVar2,bool bCol /*=true*/)
{
    bool bRet = false;
    bool bAtLeastOne  = false;
    bool bBoth = false;
    bool bIsChildVar =false;

    if(pTabVar1 == pTabVar2 || !pTabVar1 || !pTabVar2){////The tabvars shld be different
        return bRet;
    }
    CTabVar* pParent1 = pTabVar1->GetParent();
    CTabVar* pParent2 = pTabVar2->GetParent();

    CTable* pTable = m_pGrid->GetTable();
    CTabVar* pRowRoot = pTable->GetRowRoot();
    CTabVar* pColRoot = pTable->GetColRoot();
    if(bCol){
        bAtLeastOne = (pParent1 == pColRoot) || (pParent2 == pColRoot);
        bBoth = (pParent1 == pColRoot) &&(pParent2 == pColRoot);
    }
    else {
        bAtLeastOne = (pParent1 == pRowRoot) || (pParent2 == pRowRoot);
        bBoth = (pParent1 == pRowRoot) &&(pParent2 == pRowRoot);
    }


    if(!bBoth) {//if only one parentvar is root (say TabVar1) then the other item shldnot be the child of (TabVar1)
        bIsChildVar= ((pTabVar1->GetParent() == pTabVar2) || (pTabVar2->GetParent() == pTabVar1));
        if(bIsChildVar)
            return bRet;
    }


    if(!bAtLeastOne) {//if none of the parentvars are root the transition is false
        return bRet;
    }
    else if (bBoth || !bIsChildVar){//If both the ParentVars are root then transition is true if the point test is valid
        return true;
    }

    return bRet;
    //and point test shld be valid in this  for transition to be true
}

void CTabView::RefreshViewAfterDrop()
{
    CTabulateDoc* pDoc = GetDocument();
    CTable* pTable = m_pGrid->GetTable();
    //Generate  title
    pTable->GenerateTitle();
    /////////////////////////////////////////////////////////////////////////////
    // update the tree
    CTabTreeCtrl* pTreeCtrl = pDoc->GetTabTreeCtrl();
    pTreeCtrl->ReBuildTree();

    /////////////////////////////////////////////////////////////////////////////
    // update the grid
    pTable->RemoveAllData();
    // force design view when variable is dropped JH 8/05
    ((CTableChildWnd*) GetParentFrame())->SetDesignView(TRUE);
    m_pGrid->Update();
    Invalidate();
    pDoc->SetModifiedFlag(TRUE);
}

bool CTabView::RefreshLogicAfterDrop()
{
    bool bRet = true;
    CTabulateDoc* pDoc = GetDocument();
    CTable* pTable = m_pGrid->GetTable();
    CIMSAString sString;
    pDoc->MakeCrossTabStatement(pTable,sString);
    //Send a message to CSPro and putsource code for this
    TBL_PROC_INFO tblProcInfo;
    tblProcInfo.pTabDoc = pDoc;
    tblProcInfo.pTable = pTable;
    tblProcInfo.sTblLogic = sString;
    tblProcInfo.eEventType = CSourceCode_Tally;
    tblProcInfo.pLinkTable = nullptr;
    tblProcInfo.bGetLinkTables = true;

    if(AfxGetMainWnd()->SendMessage(UWM::Table::PutTallyProc, 0, (LPARAM)&tblProcInfo) == -1){
        AfxMessageBox(_T("Failed to put logic in App"));
        return false;
    }
    return bRet;
}
bool CTabView::IsStubDrop(CPoint point)
{
    bool bRet = false;
    CTable* pTable = m_pGrid->GetTable();
    int iNumColChildren = pTable->GetColRoot()->GetNumChildren();
    int iNumRowChildren = pTable->GetRowRoot()->GetNumChildren();

    if(iNumColChildren > 0 || iNumRowChildren >0 ){
        int iGridCol;
        long iGridRow;
        iGridCol = iGridRow =INVALID_ROW_COL;

        CUGCell cellGrid;
        m_pGrid->GetCellFromPoint (point.x, point.y, &iGridCol, &iGridRow); // func in zGridO
        m_pGrid->GetJoinStartCell(&iGridCol,&iGridRow,&cellGrid);
        if(iGridCol == 0) {
            bRet = iGridRow > 0 && iGridRow <= m_pGrid->GetNumHeaderRows() -1;
        }
    }
    return bRet;
}

TB_DROP_TYPE CTabView::GetDropType4Stub(CPoint point)
{
    TB_DROP_TYPE eDropType = TB_DROP_INVALID;

    //Get the cell rect
    bool bRet = false;
    CTable* pTable = m_pGrid->GetTable();
    int iNumColChildren = pTable->GetColRoot()->GetNumChildren();
    int iNumRowChildren = pTable->GetRowRoot()->GetNumChildren();

    if(iNumColChildren > 0 || iNumRowChildren >0 ){
        int iGridCol;
        long iGridRow;
        iGridCol = iGridRow =INVALID_ROW_COL;

        CUGCell cellGrid;
        m_pGrid->GetCellFromPoint (point.x, point.y, &iGridCol, &iGridRow); // func in zGridO
        m_pGrid->GetJoinStartCell(&iGridCol,&iGridRow,&cellGrid);
        if(iGridCol == 0) {
            if(iGridRow > 0 && iGridRow <= m_pGrid->GetNumHeaderRows() -1){
                CRect rect;
                m_pGrid->GetCellRect(iGridCol,iGridRow,&rect);
                int iHeight = m_pGrid->GetTH_Height();
                int iX1 = point.x - rect.left;
                float iY1 = (float)(point.y  - rect.top);   //csc 7/15/04, compiler warning
                int iX2 = rect.right - rect.left;
                float iY2 = (float)(rect.bottom - rect.top);   //csc 7/15/04, compiler warning
                if (iX1 == 0) {
                    eDropType = TB_DROP_ROW;
                }
                else if( iY1/iX1 > iY2/iX2 ){
                    eDropType = TB_DROP_ROW;
                }
                else {
                    eDropType = TB_DROP_COL;
                }
            }
        }
    }
    return eDropType;
    //convert the point to
}

/////////////////////////////////////////////////////////////////////////////////
//
//  void CTabView::MakeDummyVSet (const CDictItem* pDictItem, CStringArray& arrVals)
//
/////////////////////////////////////////////////////////////////////////////////
void CTabView::MakeDummyVSet (const CDictItem* pDictItem, CStringArray& arrVals)
{
    // It's an item with no value sets
    if(pDictItem->GetName().CompareNoCase(WORKVAR_TOTAL_NAME) ==0){
      //  arrVals.Add("Total");
        return;
    }
    int len = pDictItem->GetLen();
    int iVals = (len > 1 ? 11 : 10);
    CIMSAString sValue;
    for(int iIndex =0;iIndex < iVals ; iIndex++ ){
        if (len > 1) {
            if (iIndex == 0) {
//              sValue = "< 0";
                //serpro's syntax does not take negative values
                continue;
            }
            else {
                TCHAR pszTemp[20];
                GetPrivateProfileString(_T("intl"), _T("sDecimal"), _T("."), pszTemp, 10, _T("WIN.INI"));
                TCHAR cDec = pszTemp[0];

                CIMSAString csFrom(_T('0'), len);
                CIMSAString csTo(_T('9'), len);
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
                sValue = csFrom;
                sValue += _T(" ");
                sValue += _T("to");
                sValue += _T(" ");
                sValue += csTo;
            }
        }
        else {
            TCHAR pszTemp[20];
            GetPrivateProfileString(_T("intl"), _T("sDecimal"), _T("."), pszTemp, 10, _T("WIN.INI"));
            TCHAR cDec = pszTemp[0];

            CIMSAString csFrom(_T('0'), len);
            csFrom.SetAt(0, (TCHAR)(_T('0') + iIndex));

            UINT uDec = pDictItem->GetDecimal();
            if (uDec > 0 && pDictItem->GetDecChar()) {
                csFrom.SetAt(len - uDec - 1, pszTemp[0]);
            }
            double dTemp = atod(csFrom, uDec);
            csFrom = dtoa(dTemp, pszTemp, uDec, cDec, false);
            sValue = csFrom;
        }
        if(!sValue.IsEmpty()){
            arrVals.Add(sValue);
            sValue=_T("");
        }
    }
}
/////////////////////////////////////////////////////////////////////////////////
//
//                              CTabView::OnArea
//
/////////////////////////////////////////////////////////////////////////////////

void CTabView::OnArea()
{
    CTabulateDoc* pDoc = GetDocument();
    CTabSet* pSpec = pDoc->GetTableSpec();
    CIMSAString sMsg;
    pSpec->ReconcileCon(sMsg);
    CAreaDlg dlg;
    dlg.m_pCurrDict = pSpec->GetDict();
    dlg.m_pConsolidate = pSpec->GetConsolidate();
    if (dlg.DoModal() == IDOK) {
        if (dlg.m_bIsModified) {
            TBL_PROC_INFO tblProcInfo;
            tblProcInfo.pTabDoc = pDoc;//Used for knowing the active tab app
            tblProcInfo.pTable = nullptr;//unused
            tblProcInfo.sTblLogic = _T("");//unused
            tblProcInfo.eEventType = CSourceCode_NoProc;//unused
            tblProcInfo.pLinkTable = nullptr;//unused
            tblProcInfo.bGetLinkTables = false; //unused
            //If you change the area breaks your preproc for level is replaced with break by
            //YOU WILL LOSE THE Preproc of LEVEL 1 . -->GSF / JOSH request
            AfxGetMainWnd()->SendMessage(UWM::Table::ReplaceLevelProcForLevel, 0, (LPARAM)&tblProcInfo);
            pDoc->SetModifiedFlag();
            m_pGrid->GetTable()->RemoveAllData();
            m_pGrid->Update();
        }
    }
}
/////////////////////////////////////////////////////////////////////////////////
//
//                           CTabView::OnUpdateArea
//
/////////////////////////////////////////////////////////////////////////////////

void CTabView::OnUpdateArea(CCmdUI *pCmdUI)
{
    pCmdUI->Enable();
}

/////////////////////////////////////////////////////////////////////////////////
//
//  void CTabView::OnEditCopy()
//
/////////////////////////////////////////////////////////////////////////////////
void CTabView::OnEditCopy()
{
    CopyCellsToClipboard(true);
}

/////////////////////////////////////////////////////////////////////////////////
//
//  void CTabView::OnEditCopycellsonly()
//
/////////////////////////////////////////////////////////////////////////////////
void CTabView::OnEditCopycellsonly()
{
    CopyCellsToClipboard(false);
}

/////////////////////////////////////////////////////////////////////////////////
//
//  void CTabView::OnUpdateEditCopy(CCmdUI *pCmdUI)
//
/////////////////////////////////////////////////////////////////////////////////
void CTabView::OnUpdateEditCopy(CCmdUI *pCmdUI)
{
    CTabulateDoc* pDoc = GetDocument();
    CTable* pTbl = m_pGrid->GetTable();

    pCmdUI->Enable(pTbl != nullptr && pTbl->GetTabDataArray().GetSize() > 0);

//  Make sure they can see what they're getting (focus rectangle)
}

/////////////////////////////////////////////////////////////////////////////////
//
//  void CTabView::CopyCellsToClipboard
//
/////////////////////////////////////////////////////////////////////////////////
void CTabView::CopyCellsToClipboard(bool bIncludeParents)
{
    CTabulateDoc* pDoc = GetDocument();
    CTable* pTbl = m_pGrid->GetTable();

    if(pTbl->GetTabDataArray().GetSize() ==0)
        return;

    // GHM 20100818 there is no reason to copy the RTF to the clipboard if the user doesn't want it
    // holding shift while copying the cells will mean ASCII only
    bool onlyWantASCII = GetKeyState(VK_SHIFT) & 0x8000;

//  Make sure they can see what they're getting (focus rectangle)

    m_pGrid->SetFocus();

    CWaitCursor wait;
//  Fill string with tab delimited text stuff
    const bool bBlockedOnly = true;
    m_pGrid->UpdateTableBlocking(false);
    m_pGrid->UpdateTableBlocking(true);

    std::basic_ostringstream<wchar_t> osASCII;

    m_pGrid->PutASCIITable(osASCII, bBlockedOnly, bIncludeParents);
    osASCII << _T('\0'); // add null terminator to stream (not one by default)


//  Fill global memory buffer with tab delimited text stuff

    BOOL bRetCode;
    std::wstring txtBuffer  = (std::wstring(osASCII.str()));

    const TCHAR* pszASCIIString = txtBuffer.c_str();
    long lSize = (long) _tcslen(pszASCIIString);

    HGLOBAL hGlobalText;
    VERIFY ( (hGlobalText = GlobalAlloc (GHND, (lSize+ +1)*sizeof(TCHAR))) != nullptr );
    TCHAR FAR *lpGlobalText = (TCHAR FAR *) GlobalLock (hGlobalText);
    _tmemcpy(lpGlobalText, pszASCIIString, lSize);
    bRetCode = ! GlobalUnlock (hGlobalText);
    ASSERT ( bRetCode );

    HGLOBAL hGlobalRTF;

    if( !onlyWantASCII )
    {

    //  Fill string with RTF stuff

    std::basic_ostringstream<wchar_t> osRTF;

    m_pGrid->PutRTFTable(osRTF, bBlockedOnly, bIncludeParents);

    //  Fill global memory buffer with RTF stuff
        osRTF << _T('\0'); // add null terminator to stream (not one by default)

    std::wstring txtBuffer  = (std::wstring(osRTF.str()));

        const TCHAR* pszRTFString = txtBuffer.c_str();
        lSize = (long) _tcslen(pszRTFString);

        VERIFY ( (hGlobalRTF = GlobalAlloc (GHND, (lSize + 1)*sizeof(TCHAR))) != nullptr );
        TCHAR FAR *lpGlobalRTF = (TCHAR FAR *) GlobalLock (hGlobalRTF);

        //For RTF all the unicode characters at this point would have been converted to \uCodePoint?(\u536?)
        //So there are no unicode charcters in the string buffer so just convert it to the ANSI from WidetoMultibyte for storing in the clipboard
        WideCharToMultiByte( CP_ACP, 0 ,pszRTFString ,lSize, (LPSTR)lpGlobalRTF,lSize,nullptr,nullptr);

        bRetCode = ! GlobalUnlock (hGlobalRTF);
        ASSERT ( bRetCode );

    }

//  Give global buffers to clipboard

    UINT uFormat = RegisterClipboardFormat(_T("Rich Text Format"));
    ASSERT (uFormat > 0);
    bRetCode = OpenClipboard ();
    ASSERT ( bRetCode );
    bRetCode = EmptyClipboard ();
    ASSERT ( bRetCode );

    bRetCode = (SetClipboardData (CF_UNICODETEXT, hGlobalText) != nullptr);

    ASSERT ( bRetCode );
    if( !onlyWantASCII )
    {
        bRetCode = (SetClipboardData (uFormat, hGlobalRTF) != nullptr);
        ASSERT ( bRetCode );
    }
    bRetCode = CloseClipboard ();
    ASSERT ( bRetCode );
}

/////////////////////////////////////////////////////////////////////////////////
//
//  void CTabView::OnEditVarTallyAttributes()
//
/////////////////////////////////////////////////////////////////////////////////

BOOL CTabView::OnEditVarTallyAttributes(UINT nID)
{
    BOOL bRet = TRUE;

    CTallyVarDlg varTallyFmtDlg;
    CTabulateDoc* pDoc = GetDocument();

    CTabSet* pSpec = pDoc->GetTableSpec();

    CTabVar* pTabVar = GetCurVar();
    nID == ID_EDIT_VAR_TALLYATTRIB1 ? pTabVar = pTabVar->GetParent(): pTabVar = pTabVar;

    bool bIsRowVar = true;
    CTabVar* pParent = pTabVar->GetParent();
    while(pParent){
        if(pParent == m_pGrid->GetTable()->GetRowRoot()){
            bIsRowVar = true;
            break;
        }
        if(pParent == m_pGrid->GetTable()->GetColRoot()){
            bIsRowVar = false;
            break;
        }
        pParent = pParent->GetParent();
    }

    ASSERT(pTabVar);
    CIMSAString csTitle = _T("Tally Attributes (") + pTabVar->GetText() + _T(")");
    varTallyFmtDlg.m_sTitle = csTitle;

//TO DO Parent stuff . are we doing command range ???
    CTallyFmt* pTallyFmt = pTabVar->GetTallyFmt();
    pTabVar->GetNumChildren() > 0 ? varTallyFmtDlg.m_bDisablePct = true: varTallyFmtDlg.m_bDisablePct = false;
    CTallyFmt* pDefTallyFmt = nullptr;

    FMT_ID eFmtID = FMT_ID_INVALID;
    bIsRowVar ? eFmtID = FMT_ID_TALLY_ROW :eFmtID = FMT_ID_TALLY_COL;
    pDefTallyFmt= DYNAMIC_DOWNCAST(CTallyFmt,pSpec->GetFmtReg().GetFmt(eFmtID));
    ASSERT(pDefTallyFmt);

    CTallyFmt*  pCopyOfVarFmt = nullptr;
    if(pTallyFmt && pTallyFmt->GetIndex()!= 0 ){//make a copy
        pCopyOfVarFmt = new CTallyFmt(*pTallyFmt);
    }
    else {
        pCopyOfVarFmt = new CTallyFmt;
        MakeDefTallyFmt4Dlg(pCopyOfVarFmt,pDefTallyFmt);
    }
   varTallyFmtDlg.m_pDefaultFmt = pDefTallyFmt;
   varTallyFmtDlg.m_pFmtSelected= pCopyOfVarFmt;
   bool bUpdate = false;

   // get min and max values in vset for this variable to use as defaults and limits
   // for ranges on median and ntiles
    double dValMin = DBL_MAX;
    double dValMax = DBL_MIN;
    const CDictItem* pDictItem;
    const DictValueSet* pDictVSet;
    BOOL bFound = TRUE;
    const CDataDict* pDataDict = pSpec->GetDict();
    const CDataDict* pWorkDict = pSpec->GetWorkDict();
    bFound = pDataDict->LookupName(pTabVar->GetName(), nullptr, nullptr, &pDictItem, &pDictVSet);
    if(!bFound && pWorkDict){
        bFound = pWorkDict->LookupName(pTabVar->GetName(), nullptr, nullptr, &pDictItem, &pDictVSet);
    }

    std::tie(varTallyFmtDlg.m_dVarMin, varTallyFmtDlg.m_dVarMax) = pDictVSet->GetMinMax();
    varTallyFmtDlg.m_dVarMinDefault = varTallyFmtDlg.m_dVarMin;
    varTallyFmtDlg.m_dVarMaxDefault = varTallyFmtDlg.m_dVarMax;

    varTallyFmtDlg.m_sDefaultPropRange = _T("1"); // default if we can't get it from vset
    ASSERT(pDictVSet->HasValues());
    size_t iVal = 0;
    // skip the specials
    while (iVal < pDictVSet->GetNumValues() && pDictVSet->GetValue(iVal).IsSpecial()) {
        ++iVal;
    }
    if (iVal < pDictVSet->GetNumValues()) {
        varTallyFmtDlg.m_sDefaultPropRange = pDictVSet->GetValue(iVal).GetRangeString();
    }
    if(varTallyFmtDlg.DoModal() == IDOK){
       CTallyFmt defTallyCompare;
       MakeDefTallyFmt4Dlg(&defTallyCompare,pDefTallyFmt);
       CTallyFmt* pCompareFmt = pTallyFmt? pTallyFmt:&defTallyCompare;
       if(*pCopyOfVarFmt != *pCompareFmt || varTallyFmtDlg.m_bOrderChanged){
           if(!pTallyFmt){ //new fmt so add to registry
              pSpec->GetFmtRegPtr()->AddFmt(pCopyOfVarFmt);
              pTabVar->SetTallyFmt(pCopyOfVarFmt);
           }
           else {
               *pTallyFmt = *pCopyOfVarFmt;
           }
           m_pGrid->GetTable()->OnTallyAttributesChange(pTabVar, varTallyFmtDlg.m_aMapStatNewToOldPos,
                                                        pSpec->GetDict(),pSpec->GetWorkDict());
           pDoc->SetModifiedFlag();
           bUpdate = true;
           if (pTallyFmt) {
               delete pCopyOfVarFmt ; //done with the copy now delete it
           }
       }
       else {
           delete pCopyOfVarFmt;
       }
   }
   else {
       delete pCopyOfVarFmt;
   }
   if(bUpdate) {
       m_pGrid->GetTable()->RemoveAllData();
       m_pGrid->Update();
       // update class var info

       CIMSAString sCrossTabStmt;
       pDoc->MakeCrossTabStatement(m_pGrid->GetTable(),sCrossTabStmt);
       //Send a message to CSPro and putsource code for this
       TBL_PROC_INFO tblProcInfo;
       tblProcInfo.pTabDoc = pDoc;
       tblProcInfo.pTable = m_pGrid->GetTable();
       tblProcInfo.sTblLogic = sCrossTabStmt;
       tblProcInfo.eEventType = CSourceCode_Tally;
       tblProcInfo.pLinkTable = nullptr;
       tblProcInfo.bGetLinkTables = true;

       if(AfxGetMainWnd()->SendMessage(UWM::Table::PutTallyProc, 0, (LPARAM)&tblProcInfo) == -1){
           AfxMessageBox(_T("Failed to put logic in App"));
       }
   }
   return TRUE;
}
/////////////////////////////////////////////////////////////////////////////////
//
//  void CTabView::OnEditTableTallyAttributes()
//
/////////////////////////////////////////////////////////////////////////////////

void CTabView::OnEditTableTallyAttributes()
{

    /*CTblTallyFmtDlg tblTallyFmtDlg;
    tblTallyFmtDlg.DoModal();*/
    ((CTableChildWnd*)GetParentFrame())->OnParms();
}
/////////////////////////////////////////////////////////////////////////////////
//
//  void CTabView::OnEditSubTableTallyAttributes()
//
/////////////////////////////////////////////////////////////////////////////////
void CTabView::OnEditSubTableTallyAttributes()
{

    /*CTblTallyFmtDlg tblTallyFmtDlg;
    tblTallyFmtDlg.DoModal();*/
    ((CTableChildWnd*)GetParentFrame())->OnParms(true);
}

/////////////////////////////////////////////////////////////////////////////////
//
//  void CTabView::OnEditTblCompFmt()
//
/////////////////////////////////////////////////////////////////////////////////
void CTabView::OnEditTblCompFmt()
{
    /*if(m_lRowRClick !=-1 && m_iColRClick != -1){
    m_pGrid->ProcessFmtDlg(m_iColRClick,m_lRowRClick);
    }*/

    CCompFmtDlg compFmtdlg;
    FMT_ID eGridComp = FMT_ID_INVALID;
    CGTblOb* pGTblOb = nullptr;

    CTable* pTable = m_pGrid->GetTable();
    CFmtReg* pFmtReg = pTable->GetFmtRegPtr();
    m_pGrid->GetComponent(m_iColRClick,m_lRowRClick,eGridComp,&pGTblOb);
    CTblOb* pTblOb = nullptr;
    switch(eGridComp){
        case FMT_ID_TITLE:
            if(pGTblOb){
                pTblOb = pTable->GetTitle();
                DoFmtDlg4TblOb(pTblOb);
            }
            break;
            case FMT_ID_STUBHEAD:
            if(pGTblOb){
                pTblOb = pTable->GetStubhead(0);
                DoFmtDlg4TblOb(pTblOb);
            }
            break;
            case FMT_ID_SUBTITLE:
            if(pGTblOb){
                pTblOb = pTable->GetSubTitle();
                DoFmtDlg4TblOb(pTblOb);
            }
            break;
            case FMT_ID_PAGENOTE:
                if(pGTblOb){
                    pTblOb = pTable->GetPageNote();
                    DoFmtDlg4TblOb(pTblOb);
                }
            break;
            case FMT_ID_ENDNOTE:
                if(pGTblOb){
                    pTblOb = pTable->GetEndNote();
                    DoFmtDlg4TblOb(pTblOb);
                }
            break;
            case FMT_ID_SPANNER:
                if(pGTblOb){
                    CGTblCol* pCol = DYNAMIC_DOWNCAST(CGTblCol,pGTblOb);
                    ASSERT(pCol);
                    CTabVar* pTabVar = pCol->GetTabVar();
                    ASSERT(pTabVar);
                    DoFmtDlg4SpannerOrCaption(pTabVar);
                }
                break;
            case FMT_ID_CAPTION:
                if(pGTblOb){
                    CGTblRow* pRow = DYNAMIC_DOWNCAST(CGTblRow,pGTblOb);
                    ASSERT(pRow);
                    CTabVar* pTabVar = pRow->GetTabVar();
                    ASSERT(pTabVar);
                    DoFmtDlg4SpannerOrCaption(pTabVar);
                }
             break;
            case FMT_ID_AREA_CAPTION:
                if(pGTblOb){
                    pTblOb = pTable->GetAreaCaption();
                    ASSERT(pTblOb);
                    DoFmtDlg4TblOb(pTblOb);
                }
             break;
            case FMT_ID_COLHEAD:
                if(pGTblOb){
                    CGTblCol* pCol = DYNAMIC_DOWNCAST(CGTblCol,pGTblOb);
                    ASSERT(pCol);
                    CTabValue* pTabVal = pCol->GetTabVal();
                    ASSERT(pTabVal);
                    DoFmtDlg4ColheadOrStub(pTabVal);
                }
            break;
            break;
            case FMT_ID_STUB:
                if(pGTblOb){
                    CGTblRow* pRow = DYNAMIC_DOWNCAST(CGTblRow,pGTblOb);
                    ASSERT(pRow);
                    CTabValue* pTabVal = pRow->GetTabVal();
                    ASSERT(pTabVal);
                    DoFmtDlg4ColheadOrStub(pTabVal);
                }
            break;
          case FMT_ID_DATACELL:
            if(pGTblOb){
                DoFmtDlg4DataCell(pGTblOb);
            }
            break;
        default:
            break;
    }
    //delete from here
  //For restore position. Check to see if need to remove  m_pGrid->GotoCell(m_iColRClick,m_lRowRClick);
    SaveScrollPos();
    m_pGrid->GotoCell(m_iColRClick,m_lRowRClick);
    RestoreScrollPos();
}

/////////////////////////////////////////////////////////////////////////////////
//
//  void CTabView::OnEditTablePrintFmt()
//
//
// see also implementation in CTabPrtView::OnEditTablePrintFmt
//
/////////////////////////////////////////////////////////////////////////////////
void CTabView::OnEditTablePrintFmt()
{
    CTable* pTbl = m_pGrid->GetTable();
    CFmtReg* pFmtReg = pTbl->GetFmtRegPtr();
    CTabulateDoc* pDoc = DYNAMIC_DOWNCAST(CTabulateDoc, GetDocument());
    CTabSet* pSet = pDoc->GetTableSpec();

//    CTblPrintFmt fmtTblPrint(*pTbl->GetTblPrintFmt());  //working copy
    CTblPrintFmt fmtTblPrint;   // working copy
    if (pTbl->GetTblPrintFmt()) {
        fmtTblPrint=*pTbl->GetTblPrintFmt();
    }

    // prep working copy of the tblprintfmt
    int iIndex=pFmtReg->GetNextCustomFmtIndex(fmtTblPrint);
    if (fmtTblPrint.GetIndex()==0) {
        // was using the stock default ... make the working copy use just defaults (except for printer driver/output/device and units)
        fmtTblPrint.SetTblLayout(TBL_LAYOUT_DEFAULT);
        fmtTblPrint.SetPageBreak(PAGE_BREAK_DEFAULT);
        fmtTblPrint.SetCtrHorz(CENTER_TBL_DEFAULT);
        fmtTblPrint.SetCtrVert(CENTER_TBL_DEFAULT);
        fmtTblPrint.SetPaperType(PAPER_TYPE_DEFAULT);
        fmtTblPrint.SetPageOrientation(PAGE_ORIENTATION_DEFAULT);
        fmtTblPrint.SetStartPage(START_PAGE_DEFAULT);
        fmtTblPrint.SetPageMargin(CRect(PAGE_MARGIN_DEFAULT,PAGE_MARGIN_DEFAULT,PAGE_MARGIN_DEFAULT,PAGE_MARGIN_DEFAULT));
        fmtTblPrint.SetHeaderFrequency(HEADER_FREQUENCY_DEFAULT);
        fmtTblPrint.SetUnits(pSet->GetTabSetFmt()->GetUnits());
    }
    fmtTblPrint.SetIndex(iIndex);

    CTblPrintFmtDlg dlg;

    // initialize dialog variables ...
    dlg.m_pFmtTblPrint=&fmtTblPrint;
    dlg.m_pDefaultFmtTblPrint=DYNAMIC_DOWNCAST(CTblPrintFmt, pFmtReg->GetFmt(FMT_ID_TBLPRINT,0));

    // note that we specify that our header/footer font is "default" if --
    // - the CFmtFont is null (that is, IsFontCustom()==false), or
    // - the format's fmt registry ID is zero
    bool bDefaultHeaderFont;
    if (pTbl->GetHeader(0)->GetDerFmt()) {
        iIndex=pTbl->GetHeader(0)->GetDerFmt()->GetIndex();
        bDefaultHeaderFont=dlg.m_bDefaultHeaderFont=(!pTbl->GetHeader(0)->GetDerFmt()->IsFontCustom() || iIndex==0);
    }
    else {
        iIndex=0;
        bDefaultHeaderFont=dlg.m_bDefaultHeaderFont=true;
    }
    LOGFONT lfHeaderCurrent; //currently active header font
    if (dlg.m_bDefaultHeaderFont) {
        ::ZeroMemory(&dlg.m_lfHeader, sizeof(LOGFONT));
    }
    else {
        pTbl->GetHeader(0)->GetDerFmt()->GetFont()->GetLogFont(&dlg.m_lfHeader);
    }
    (DYNAMIC_DOWNCAST(CFmt, pFmtReg->GetFmt(FMT_ID_HEADER_LEFT,0)))->GetFont()->GetLogFont(&dlg.m_lfDefaultHeader);
    lfHeaderCurrent=bDefaultHeaderFont?dlg.m_lfDefaultHeader:dlg.m_lfHeader;

    bool bDefaultFooterFont;
    if (pTbl->GetFooter(0)->GetDerFmt()) {
        iIndex=pTbl->GetFooter(0)->GetDerFmt()->GetIndex();
        bDefaultFooterFont=dlg.m_bDefaultFooterFont=(!pTbl->GetFooter(0)->GetDerFmt()->IsFontCustom() || iIndex==0);
    }
    else {
        iIndex=0;
        bDefaultFooterFont=dlg.m_bDefaultFooterFont=true;
    }
    LOGFONT lfFooterCurrent; //currently active footer font
    if (dlg.m_bDefaultFooterFont) {
        ::ZeroMemory(&dlg.m_lfFooter, sizeof(LOGFONT));
    }
    else {
        pTbl->GetFooter(0)->GetDerFmt()->GetFont()->GetLogFont(&dlg.m_lfFooter);
    }
    (DYNAMIC_DOWNCAST(CFmt, pFmtReg->GetFmt(FMT_ID_FOOTER_LEFT,0)))->GetFont()->GetLogFont(&dlg.m_lfDefaultFooter);
    lfFooterCurrent=bDefaultFooterFont?dlg.m_lfDefaultFooter:dlg.m_lfFooter;

    SetupFolioText(pTbl->GetHeader(0), FMT_ID_HEADER_LEFT, dlg.m_sHeaderLeft, pFmtReg);
    SetupFolioText(pTbl->GetHeader(1), FMT_ID_HEADER_CENTER, dlg.m_sHeaderCenter, pFmtReg);
    SetupFolioText(pTbl->GetHeader(2), FMT_ID_HEADER_RIGHT, dlg.m_sHeaderRight, pFmtReg);
    SetupFolioText(pTbl->GetFooter(0), FMT_ID_FOOTER_LEFT, dlg.m_sFooterLeft, pFmtReg);
    SetupFolioText(pTbl->GetFooter(1), FMT_ID_FOOTER_CENTER, dlg.m_sFooterCenter, pFmtReg);
    SetupFolioText(pTbl->GetFooter(2), FMT_ID_FOOTER_RIGHT, dlg.m_sFooterRight, pFmtReg);

    bool bBuild=false;  //=true if something changed
    if (dlg.DoModal()==IDOK)  {
        bool bTblPrintChanged=(fmtTblPrint.GetHeaderFrequency()!=dlg.m_eHeaderFrequency
            || fmtTblPrint.GetTblLayout()!=dlg.m_eTblLayout
            || fmtTblPrint.GetStartPage()!=dlg.m_iStartPage
            || fmtTblPrint.GetPageMargin()!=dlg.m_rcPageMargin);

        if (bTblPrintChanged) {
            // need to add a custom tblprintfmt to the registry ...
            fmtTblPrint.SetHeaderFrequency(dlg.m_eHeaderFrequency);
            fmtTblPrint.SetTblLayout(dlg.m_eTblLayout);
            fmtTblPrint.SetStartPage(dlg.m_iStartPage);
            fmtTblPrint.SetPageMargin(dlg.m_rcPageMargin);

            CTblPrintFmt* pFmtTblPrintNew=new CTblPrintFmt(fmtTblPrint);

            iIndex=pFmtReg->GetNextCustomFmtIndex(fmtTblPrint);
            pFmtTblPrintNew->SetIndex(iIndex);
            if (!pFmtReg->AddFmt(pFmtTblPrintNew)) {
                ASSERT(FALSE);
            }
            pTbl->SetTblPrintFmt(pFmtTblPrintNew);
            bBuild=true;
        }

        if (UpdateFolioText(pTbl->GetHeader(0), FMT_ID_HEADER_LEFT, dlg.m_sHeaderLeft, pFmtReg)) {
            bBuild = true;
        }
        if (UpdateFolioText(pTbl->GetHeader(1), FMT_ID_HEADER_CENTER, dlg.m_sHeaderCenter, pFmtReg)) {
            bBuild = true;
        }
        if (UpdateFolioText(pTbl->GetHeader(2), FMT_ID_HEADER_RIGHT, dlg.m_sHeaderRight, pFmtReg)) {
            bBuild = true;
        }
        if (UpdateFolioText(pTbl->GetFooter(0), FMT_ID_FOOTER_LEFT, dlg.m_sFooterLeft, pFmtReg)) {
            bBuild = true;
        }
        if (UpdateFolioText(pTbl->GetFooter(1), FMT_ID_FOOTER_CENTER, dlg.m_sFooterCenter, pFmtReg)) {
            bBuild = true;
        }
        if (UpdateFolioText(pTbl->GetFooter(2), FMT_ID_FOOTER_RIGHT, dlg.m_sFooterRight, pFmtReg)) {
            bBuild = true;
        }

        LOGFONT lfHeader;
        if (dlg.m_bDefaultHeaderFont) {
            lfHeader=dlg.m_lfDefaultHeader;
        }
        else {
            lfHeader=dlg.m_lfHeader;
        }
        bool bHeaderFmtChanged=(memcmp(&lfHeader, &lfHeaderCurrent, sizeof(LOGFONT))!=0 || (bDefaultHeaderFont!=dlg.m_bDefaultHeaderFont));
        if (bHeaderFmtChanged) {
            // we'll create 3 new header formats, one each for left/center/right, and apply them

            // ... left header
            CFmt* pHeaderLeftFmt=new CFmt;
            if (pTbl->GetHeader(0)->GetDerFmt()) {
                *pHeaderLeftFmt=*pTbl->GetHeader(0)->GetDerFmt();
            }
            else {
                pHeaderLeftFmt->SetID(FMT_ID_HEADER_LEFT);
            }
            iIndex=pFmtReg->GetNextCustomFmtIndex(FMT_ID_HEADER_LEFT);
            pHeaderLeftFmt->SetIndex(iIndex);
            if (dlg.m_bDefaultHeaderFont) {
                pHeaderLeftFmt->SetFont((CFont*)nullptr);
            }
            else {
                pHeaderLeftFmt->SetFont(&dlg.m_lfHeader);
            }

            if (!pFmtReg->AddFmt(pHeaderLeftFmt)) {
                ASSERT(FALSE);
            }
            pTbl->GetHeader(0)->SetFmt(pHeaderLeftFmt);

            // ... center header
            CFmt* pHeaderCenterFmt=new CFmt;
            if (pTbl->GetHeader(1)->GetDerFmt()) {
                *pHeaderCenterFmt=*pTbl->GetHeader(1)->GetDerFmt();
            }
            else {
                pHeaderCenterFmt->SetID(FMT_ID_HEADER_CENTER);
            }
            iIndex=pFmtReg->GetNextCustomFmtIndex(FMT_ID_HEADER_CENTER);
            pHeaderCenterFmt->SetIndex(iIndex);
            if (dlg.m_bDefaultHeaderFont) {
                pHeaderCenterFmt->SetFont((CFont*)nullptr);
            }
            else {
                pHeaderCenterFmt->SetFont(&dlg.m_lfHeader);
            }

            if (!pFmtReg->AddFmt(pHeaderCenterFmt)) {
                ASSERT(FALSE);
            }
            pTbl->GetHeader(1)->SetFmt(pHeaderCenterFmt);

            // ... right header
            CFmt* pHeaderRightFmt=new CFmt;
            if (pTbl->GetHeader(2)->GetDerFmt()) {
                *pHeaderRightFmt=*pTbl->GetHeader(2)->GetDerFmt();
            }
            else {
                pHeaderRightFmt->SetID(FMT_ID_HEADER_RIGHT);
            }
            iIndex=pFmtReg->GetNextCustomFmtIndex(FMT_ID_HEADER_RIGHT);
            pHeaderRightFmt->SetIndex(iIndex);
            if (dlg.m_bDefaultHeaderFont) {
                pHeaderRightFmt->SetFont((CFont*)nullptr);
            }
            else {
                pHeaderRightFmt->SetFont(&dlg.m_lfHeader);
            }

            if (!pFmtReg->AddFmt(pHeaderRightFmt)) {
                ASSERT(FALSE);
            }
            pTbl->GetHeader(2)->SetFmt(pHeaderRightFmt);

            bBuild=true;
        }

        LOGFONT lfFooter;
        if (dlg.m_bDefaultFooterFont) {
            lfFooter=dlg.m_lfDefaultFooter;
        }
        else {
            lfFooter=dlg.m_lfFooter;
        }
        bool bFooterFmtChanged=(memcmp(&lfFooter, &lfFooterCurrent, sizeof(LOGFONT))!=0 || (bDefaultFooterFont!=dlg.m_bDefaultFooterFont));
        if (bFooterFmtChanged) {
            // we'll create 3 new footer formats, one each for left/center/right, and apply them

            // ... left footer
            CFmt* pFooterLeftFmt=new CFmt;
            if (pTbl->GetFooter(0)->GetDerFmt()) {
                *pFooterLeftFmt=*pTbl->GetFooter(0)->GetDerFmt();
            }
            else {
                pFooterLeftFmt->SetID(FMT_ID_FOOTER_LEFT);
            }
            iIndex=pFmtReg->GetNextCustomFmtIndex(FMT_ID_FOOTER_LEFT);
            pFooterLeftFmt->SetIndex(iIndex);
            if (dlg.m_bDefaultFooterFont) {
                pFooterLeftFmt->SetFont((CFont*)nullptr);
            }
            else {
                pFooterLeftFmt->SetFont(&dlg.m_lfFooter);
            }

            if (!pFmtReg->AddFmt(pFooterLeftFmt)) {
                ASSERT(FALSE);
            }
            pTbl->GetFooter(0)->SetFmt(pFooterLeftFmt);

            // ... center footer
            CFmt* pFooterCenterFmt=new CFmt;
            if (pTbl->GetFooter(1)->GetDerFmt()) {
                *pFooterCenterFmt=*pTbl->GetFooter(1)->GetDerFmt();
            }
            else {
                pFooterCenterFmt->SetID(FMT_ID_FOOTER_CENTER);
            }
            iIndex=pFmtReg->GetNextCustomFmtIndex(FMT_ID_FOOTER_CENTER);
            pFooterCenterFmt->SetIndex(iIndex);
            if (dlg.m_bDefaultFooterFont) {
                pFooterCenterFmt->SetFont((CFont*)nullptr);
            }
            else {
                pFooterCenterFmt->SetFont(&dlg.m_lfFooter);
            }

            if (!pFmtReg->AddFmt(pFooterCenterFmt)) {
                ASSERT(FALSE);
            }
            pTbl->GetFooter(1)->SetFmt(pFooterCenterFmt);

            // ... right footer
            CFmt* pFooterRightFmt=new CFmt;
            if (pTbl->GetFooter(2)->GetDerFmt()) {
                *pFooterRightFmt=*pTbl->GetFooter(2)->GetDerFmt();
            }
            else {
                pFooterRightFmt->SetID(FMT_ID_FOOTER_RIGHT);
            }
            iIndex=pFmtReg->GetNextCustomFmtIndex(FMT_ID_FOOTER_RIGHT);
            pFooterRightFmt->SetIndex(iIndex);
            if (dlg.m_bDefaultFooterFont) {
                pFooterRightFmt->SetFont((CFont*)nullptr);
            }
            else {
                pFooterRightFmt->SetFont(&dlg.m_lfFooter);
            }

            if (!pFmtReg->AddFmt(pFooterRightFmt)) {
                ASSERT(FALSE);
            }
            pTbl->GetFooter(2)->SetFmt(pFooterRightFmt);

            bBuild=true;
        }

        if (bBuild) {
            pDoc->SetModifiedFlag(TRUE);
        }
    }
}





/*
    CTable* pTbl = m_pGrid->GetTable();
    CFmtReg* pFmtReg = pTbl->GetFmtRegPtr();
    CTabulateDoc* pDoc = DYNAMIC_DOWNCAST(CTabulateDoc, GetDocument());
    CTabSet* pSet = pDoc->GetTableSpec();
    CTblPrintFmt fmtTblPrint(*pTbl->GetTblPrintFmt());  //working copy

    // prep working copy of the tblprintfmt
    int iIndex=pFmtReg->GetNextCustomFmtIndex(fmtTblPrint);
    if (fmtTblPrint.GetIndex()==0) {
        // was using the stock default ... make the working copy use just defaults (except for printer driver/output/device and units)
        fmtTblPrint.SetTblLayout(TBL_LAYOUT_DEFAULT);
        fmtTblPrint.SetPageBreak(PAGE_BREAK_DEFAULT);
        fmtTblPrint.SetCtrHorz(CENTER_TBL_DEFAULT);
        fmtTblPrint.SetCtrVert(CENTER_TBL_DEFAULT);
        fmtTblPrint.SetPaperType(PAPER_TYPE_DEFAULT);
        fmtTblPrint.SetPageOrientation(PAGE_ORIENTATION_DEFAULT);
        fmtTblPrint.SetStartPage(START_PAGE_DEFAULT);
        fmtTblPrint.SetPageMargin(CRect(PAGE_MARGIN_DEFAULT,PAGE_MARGIN_DEFAULT,PAGE_MARGIN_DEFAULT,PAGE_MARGIN_DEFAULT));
        fmtTblPrint.SetHeaderFrequency(HEADER_FREQUENCY_DEFAULT);
    }
    fmtTblPrint.SetIndex(iIndex);

    CTblPrintFmtDlg dlg;
    dlg.m_pFmtTblPrint=&fmtTblPrint;
    dlg.m_pDefaultFmtTblPrint=DYNAMIC_DOWNCAST(CTblPrintFmt, pFmtReg->GetFmt(FMT_ID_TBLPRINT,0));

    if (dlg.DoModal()==IDOK)  {
        bool bChanged=(fmtTblPrint.GetHeaderFrequency()!=dlg.m_eHeaderFrequency
            || fmtTblPrint.GetTblLayout()!=dlg.m_eTblLayout
            || fmtTblPrint.GetStartPage()!=dlg.m_iStartPage
            || fmtTblPrint.GetPageMargin()!=dlg.m_rcPageMargin
            || pTbl->GetHeader(0)->GetText()!=dlg.m_sHeaderLeft
            || pTbl->GetHeader(1)->GetText()!=dlg.m_sHeaderCenter
            || pTbl->GetHeader(2)->GetText()!=dlg.m_sHeaderRight
            || pTbl->GetFooter(0)->GetText()!=dlg.m_sFooterLeft
            || pTbl->GetFooter(1)->GetText()!=dlg.m_sFooterCenter
            || pTbl->GetFooter(2)->GetText()!=dlg.m_sFooterRight);

        if (bChanged) {
            fmtTblPrint.SetHeaderFrequency(dlg.m_eHeaderFrequency);
            fmtTblPrint.SetTblLayout(dlg.m_eTblLayout);
            fmtTblPrint.SetStartPage(dlg.m_iStartPage);
            fmtTblPrint.SetPageMargin(dlg.m_rcPageMargin);

            CTblPrintFmt* pFmtTblPrintNew=new CTblPrintFmt(fmtTblPrint);

            pFmtTblPrintNew->SetIndex(iIndex);
            if (!pFmtReg->AddFmt(pFmtTblPrintNew)) {
                ASSERT(FALSE);
            }
            pTbl->SetTblPrintFmt(pFmtTblPrintNew);
            pTbl->GetHeader(0)->SetText(dlg.m_sHeaderLeft);
            pTbl->GetHeader(1)->SetText(dlg.m_sHeaderCenter);
            pTbl->GetHeader(2)->SetText(dlg.m_sHeaderRight);
            pTbl->GetFooter(0)->SetText(dlg.m_sFooterLeft);   // todo -- fonts also!
            pTbl->GetFooter(1)->SetText(dlg.m_sFooterCenter);
            pTbl->GetFooter(2)->SetText(dlg.m_sFooterRight);
            pDoc->SetModifiedFlag(TRUE);
        }
    }
}
*/


/////////////////////////////////////////////////////////////////////////////////
//
//  void CTabView::OnEditAppFmt()
//
/////////////////////////////////////////////////////////////////////////////////
void CTabView::OnEditAppFmt()
{
    DoFmtDlg4App();
    /*CAppFmtDlg appFmtdlg;
    appFmtdlg.DoModal();*/
}

/////////////////////////////////////////////////////////////////////////////////
//
//  void CTabView::OnEditTableFmt()
//
/////////////////////////////////////////////////////////////////////////////////
void CTabView::OnEditTableFmt()
{
    /*CTblFmtDlg tblFmtdlg;
    tblFmtdlg.DoModal();*/
    DoFmtDlg4Table();
}

/////////////////////////////////////////////////////////////////////////////////
//
//  void CTabView::OnEditTblJoinSpanners()
//
/////////////////////////////////////////////////////////////////////////////////
void CTabView::OnEditTblJoinSpanners()
{
    CArray<CTblBase*,CTblBase*> arrTblBase;
    FMT_ID eGridComp = FMT_ID_SPANNER;
    if(m_pGrid->GetMultiSelObj(arrTblBase,eGridComp)){
        CDataCellFmt* pFmt = (CDataCellFmt*)arrTblBase[0]->GetFmt();
        CTabulateDoc* pDoc = GetDocument();
        CTabSet* pSpec = pDoc->GetTableSpec();
        CTable* pTable = m_pGrid->GetTable();
        CFmtReg* pFmtReg = pTable->GetFmtRegPtr();
        CDataCellFmt* pDataCellFmt = pFmt;
        if(!pFmt){
            CDataCellFmt* pDefTblFmt = DYNAMIC_DOWNCAST(CDataCellFmt,m_pGrid->GetTable()->GetFmtRegPtr()->GetFmt(FMT_ID_SPANNER));
            pDataCellFmt = new CDataCellFmt(*pDefTblFmt);
            arrTblBase[0]->SetFmt(pDataCellFmt);
            pFmt = pDataCellFmt;
            pDataCellFmt->SetIndex(m_pGrid->GetTable()->GetFmtRegPtr()->GetNextCustomFmtIndex(*pDataCellFmt));
            pFmtReg->AddFmt(pDataCellFmt);
        }
        else if(pFmt->GetIndex() == 0){
            pDataCellFmt = new CDataCellFmt(*pFmt);
            pDataCellFmt->SetNumJoinSpanners(0);
            pFmtReg->AddFmt(pDataCellFmt);
        }


        int iNumJoinSpanners = pDataCellFmt->GetNumJoinSpanners();
        pDataCellFmt->SetNumJoinSpanners(iNumJoinSpanners+arrTblBase.GetCount()-1);
        pDoc->SetModifiedFlag();
        m_pGrid->Update();

    }
}

/////////////////////////////////////////////////////////////////////////////////
//
//  void CTabView::OnEditTblSplitSpanners()
//
/////////////////////////////////////////////////////////////////////////////////
void CTabView::OnEditTblSplitSpanners()
{
    CTabVar* pTabVar = GetCurVar();
    if(pTabVar && pTabVar->GetFmt()){
        CTabulateDoc* pDoc = GetDocument();
        ((CDataCellFmt*)pTabVar->GetFmt())->SetNumJoinSpanners(0);
        pDoc->SetModifiedFlag();
        m_pGrid->Update();
    }
}
/////////////////////////////////////////////////////////////////////////////////
//
//  void CTabView::DoFmtDlg4TblOb(CTblOb* pTblOb)
//
/////////////////////////////////////////////////////////////////////////////////
void CTabView::DoFmtDlg4TblOb(CTblOb* pTblOb)
{
    CFmt* pFmt = nullptr;
    CFmt* pDefFmt = nullptr;
    CFmt* pFmtCopy = nullptr;
    bool bUpdate = false;

    CTabulateDoc* pDoc = GetDocument();
    CTabSet* pSpec = pDoc->GetTableSpec();
    CTable* pTable = m_pGrid->GetTable();
    CFmtReg* pFmtReg = pTable->GetFmtRegPtr();

    CCompFmtDlg compFmtdlg;
    FMT_ID eGridComp = FMT_ID_INVALID;

    CGTblOb* pGTblOb = nullptr;

    m_pGrid->GetComponent(m_iColRClick,m_lRowRClick,eGridComp,&pGTblOb);


    //Get the dialog title

    CIMSAString sComponent = m_pGrid->GetComponentString(m_iColRClick, m_lRowRClick);
    CIMSAString  sTitle = _T("Format (") +sComponent +_T(")") ;

    m_pGrid->GetComponent(m_iColRClick,m_lRowRClick,eGridComp,&pGTblOb);
    compFmtdlg.m_sTitle =  sTitle;
    compFmtdlg.m_eGridComp=eGridComp;

    ASSERT(pTblOb);
    pFmt = pTblOb->GetDerFmt();
    if (!pFmt) {
        compFmtdlg.m_bDefaultIndentation=TRUE;   // csc
    }
    else {
        compFmtdlg.m_bDefaultIndentation=(pFmt->GetIndent(LEFT)==INDENT_DEFAULT);   // csc
    }

    pDefFmt = DYNAMIC_DOWNCAST(CFmt,pFmtReg->GetFmt(eGridComp));
    if(pTblOb->GetFmt() && pFmt->GetIndex()!=0){
        pFmt = DYNAMIC_DOWNCAST(CFmt,pTblOb->GetFmt());
        pFmtCopy = new CFmt(*pFmt);   // a copy of the ob's formats, which will be modified (possibly) through the dlg box
    }
    else {
        if(pFmt && pFmt->GetIndex() == 0){//default fmt
            pFmt = nullptr;
        }
        pFmtCopy = new CFmt(*pDefFmt);   // a copy of the ob's formats, which will be modified (possibly) through the dlg box
        MakeDefFmt4Dlg(pFmtCopy,pDefFmt);

    }
    compFmtdlg.m_pFmt = pFmtCopy;
    compFmtdlg.m_pDefFmt = pDefFmt;
    enum HIDDEN eHiddenOld = HIDDEN_NOT_APPL;
    if(eGridComp == FMT_ID_AREA_CAPTION) {
        if(m_pGrid->ForceHideAreaCaptionInOneRowTable()){
            pFmtCopy->SetHidden(HIDDEN_YES);
            eHiddenOld = pDefFmt->GetHidden();
            pDefFmt->SetHidden(HIDDEN_YES);
        }
    }
    SaveScrollPos();
    if(compFmtdlg.DoModal() == IDOK){
        CFmt defCompareFmt;
        MakeDefFmt4Dlg(&defCompareFmt,pDefFmt);
        if(eGridComp == FMT_ID_AREA_CAPTION) {
            if(m_pGrid->ForceHideAreaCaptionInOneRowTable()){
                defCompareFmt.SetHidden(HIDDEN_YES);
                pDefFmt->SetHidden(eHiddenOld);
            }
        }
        CFmt* pCompareFmt = pFmt? pFmt:&defCompareFmt;

        if(*pFmtCopy != *pCompareFmt){
            if(!pFmt ){ //new fmt so add to registry
                pDoc->SetModifiedFlag();
                pFmtReg->AddFmt(pFmtCopy);
                pTblOb->SetFmt(pFmtCopy);
                bUpdate = true;
            }
            else {
                *pFmt = *pFmtCopy;
                pDoc->SetModifiedFlag();
                delete pFmtCopy ; //done with the copy now delete it
                bUpdate = true;
            }
        }
        else {
            delete pFmtCopy;
        }
    }
    else {
        if(eGridComp == FMT_ID_AREA_CAPTION) {
            if(m_pGrid->ForceHideAreaCaptionInOneRowTable()){
                pDefFmt->SetHidden(eHiddenOld);
            }
        }
        delete pFmtCopy;
        return;
    }
    if(bUpdate){
      m_pGrid->Update();
      RedrawWindow();
      RestoreScrollPos();
    }
}

/////////////////////////////////////////////////////////////////////////////////
//
//  void CTabView::DoFmtDlg4SpannerOrCaption(CTabVar* pTabVar)
//
/////////////////////////////////////////////////////////////////////////////////
void CTabView::DoFmtDlg4SpannerOrCaption(CTabVar* pTabVar)
{
    CDataCellFmt* pFmt = nullptr;
    CDataCellFmt* pDefFmt = nullptr;
    CDataCellFmt* pFmtCopy = nullptr;
    bool bUpdate = false;

    CTabulateDoc* pDoc = GetDocument();
    CTabSet* pSpec = pDoc->GetTableSpec();
    CTable* pTable = m_pGrid->GetTable();
    CFmtReg* pFmtReg = pTable->GetFmtRegPtr();

    CCompFmtDlg compFmtdlg;
    compFmtdlg.m_pTabView =this;
    FMT_ID eGridComp = FMT_ID_INVALID;

    CGTblOb* pGTblOb = nullptr;
    bool m_bEnableMultiSelect = false;
    bool m_bIsMultiSelectState = false;
    m_bIsMultiSelectState = m_pGrid->IsMultiSelectState();
    CArray<CTblBase*,CTblBase*>arrTblBase;

    m_pGrid->GetComponent(m_iColRClick,m_lRowRClick,eGridComp,&pGTblOb);
    //Get the dialog title

    CIMSAString sComponent = m_pGrid->GetComponentString(m_iColRClick, m_lRowRClick);
    CIMSAString  sTitle = _T("Format (") +sComponent +_T(")") ;

    m_pGrid->GetComponent(m_iColRClick,m_lRowRClick,eGridComp,&pGTblOb);
    compFmtdlg.m_sTitle =  sTitle;
    compFmtdlg.m_eGridComp=eGridComp;

    if(m_bIsMultiSelectState){
        m_bEnableMultiSelect = m_pGrid->GetMultiSelObj(arrTblBase,eGridComp);
        m_bEnableMultiSelect = arrTblBase.GetSize() > 0;
        m_bEnableMultiSelect = true;
        compFmtdlg.m_pArrTblBase = &arrTblBase;
    }
    if (eGridComp == FMT_ID_COLHEAD || eGridComp == FMT_ID_STUB){
        compFmtdlg.m_bDisableZeroHide= false;
    }
    else {
        compFmtdlg.m_bDisableZeroHide= true;
    }
    CGRowColOb* pGTblRowCol = DYNAMIC_DOWNCAST(CGRowColOb,pGTblOb);
    CTabValue* pTabVal = nullptr;
    if(pGTblRowCol && pGTblRowCol->GetTabVal()){ //A tabval which is a spanner has to have its hide disabled
     compFmtdlg.m_bDisableHide = true;
     compFmtdlg.m_bDisableZeroHide= true;
     pTabVal = pGTblRowCol->GetTabVal();
     pFmt = pGTblRowCol->GetTabVal()->GetDerFmt();
    }
    else {
        ASSERT(pTabVar);
        pFmt = pTabVar->GetDerFmt();
    }
    pDefFmt = DYNAMIC_DOWNCAST(CDataCellFmt,pFmtReg->GetFmt(eGridComp));
    if (!pFmt) {
        compFmtdlg.m_bDefaultIndentation=TRUE;   // csc
    }
    else {
        compFmtdlg.m_bDefaultIndentation=(pFmt->GetIndent(LEFT)==INDENT_DEFAULT);   // csc
    }
    if(pFmt && pFmt->GetIndex()!=0){
        pFmtCopy = new CDataCellFmt(*pFmt);   // a copy of the ob's formats, which will be modified (possibly) through the dlg box
    }
    else {
        if(pFmt && pFmt->GetIndex() == 0){//default fmt
            pFmt = nullptr;
        }
        pFmtCopy = new CDataCellFmt(*pDefFmt);   // a copy of the ob's formats, which will be modified (possibly) through the dlg box
        pFmtCopy->Init();
        MakeDefFmt4Dlg(pFmtCopy,pDefFmt);
    }
    SaveScrollPos();
    compFmtdlg.m_pFmt = pFmtCopy;
    compFmtdlg.m_pDefFmt = pDefFmt;
    bUpdate  = (compFmtdlg.DoModal() == IDOK);
    if(bUpdate&& !m_bEnableMultiSelect){
        bUpdate = false;
        CDataCellFmt defCompareFmt;
        MakeDefFmt4Dlg(&defCompareFmt,pDefFmt);
        CDataCellFmt* pCompareFmt = pFmt? pFmt:&defCompareFmt;
        if(*pFmtCopy != *pCompareFmt){
            if(!pFmt ){ //new fmt so add to registry
                pDoc->SetModifiedFlag();
                pFmtReg->AddFmt(pFmtCopy);
                if(pTabVal){
                   pTabVal->SetFmt(pFmtCopy);
                }
                else {
                    pTabVar->SetFmt(pFmtCopy);
                }
                bUpdate = true;
            }
            else {
                *pFmt = *pFmtCopy;
                pDoc->SetModifiedFlag();
                delete pFmtCopy ; //done with the copy now delete it
                bUpdate = true;
            }
        }
        else {
            delete pFmtCopy;
        }
    }
     else {
        delete pFmtCopy;
        if(!bUpdate){
            return;
        }
    }
    if(bUpdate){
      m_pGrid->Update();
      RedrawWindow();
      RestoreScrollPos();
    }
}
/////////////////////////////////////////////////////////////////////////////////
//
//  void CTabView::DoFmtDlg4ColheadOrStub(CTabValue* pTabVal)
//
/////////////////////////////////////////////////////////////////////////////////
void CTabView::DoFmtDlg4ColheadOrStub(CTabValue* pTabVal)
{
    CDataCellFmt* pFmt = nullptr;
    CDataCellFmt* pDefFmt = nullptr;
    CDataCellFmt* pFmtCopy = nullptr;
    bool bUpdate = false;

    CTabulateDoc* pDoc = GetDocument();
    CTabSet* pSpec = pDoc->GetTableSpec();
    CTable* pTable = m_pGrid->GetTable();
    CFmtReg* pFmtReg = pTable->GetFmtRegPtr();

    CCompFmtDlg compFmtdlg;
    compFmtdlg.m_pTabView =this;
    FMT_ID eGridComp = FMT_ID_INVALID;

    CGTblOb* pGTblOb = nullptr;
    bool m_bEnableMultiSelect = false;
    bool m_bIsMultiSelectState = false;
    m_bIsMultiSelectState = m_pGrid->IsMultiSelectState();
    CArray<CTblBase*,CTblBase*>arrTblBase;


    m_pGrid->GetComponent(m_iColRClick,m_lRowRClick,eGridComp,&pGTblOb);
    //Get the dialog title

    CIMSAString sComponent = m_pGrid->GetComponentString(m_iColRClick, m_lRowRClick);
    CIMSAString  sTitle = _T("Format (") +sComponent +_T(")") ;

    m_pGrid->GetComponent(m_iColRClick,m_lRowRClick,eGridComp,&pGTblOb);
    compFmtdlg.m_sTitle =  sTitle;
    compFmtdlg.m_eGridComp=eGridComp;

    if(m_bIsMultiSelectState){
        m_bEnableMultiSelect = m_pGrid->GetMultiSelObj(arrTblBase,eGridComp);
        m_bEnableMultiSelect = arrTblBase.GetSize() > 0;
        m_bEnableMultiSelect = true;
        compFmtdlg.m_pArrTblBase = &arrTblBase;
    }

    ASSERT(pTabVal);
    pFmt = pTabVal->GetDerFmt();
    pDefFmt = DYNAMIC_DOWNCAST(CDataCellFmt,pFmtReg->GetFmt(eGridComp));

    if (!pFmt) {
        compFmtdlg.m_bDefaultIndentation=TRUE;   // csc
    }
    else {
        compFmtdlg.m_bDefaultIndentation=(pFmt->GetIndent(LEFT)==INDENT_DEFAULT);   // csc
    }
    if(pTabVal->GetFmt() && pFmt->GetIndex()!=0){
        pFmt = DYNAMIC_DOWNCAST(CDataCellFmt,pTabVal->GetFmt());
        pFmtCopy = new CDataCellFmt(*pFmt);   // a copy of the ob's formats, which will be modified (possibly) through the dlg box
    }
    else {
        if(pFmt && pFmt->GetIndex() == 0){//default fmt
            pFmt = nullptr;
        }
        pFmtCopy = new CDataCellFmt();   // a copy of the ob's formats, which will be modified (possibly) through the dlg box
        pFmtCopy->Init(); //call for the decimals initialisation
        MakeDefFmt4Dlg(pFmtCopy,pDefFmt);

    }
    compFmtdlg.m_pFmt = pFmtCopy;
    compFmtdlg.m_pDefFmt = pDefFmt;
    SaveScrollPos();
    bUpdate  = (compFmtdlg.DoModal() == IDOK);
    if(bUpdate && !m_bEnableMultiSelect){
        bUpdate = false;
        CDataCellFmt defCompareFmt;
        defCompareFmt.Init(); //call for the decimals initialisation
        MakeDefFmt4Dlg(&defCompareFmt,pDefFmt);
        CDataCellFmt* pCompareFmt = pFmt? pFmt:&defCompareFmt;

        if(*pFmtCopy != *pCompareFmt){
            if(!pFmt ){ //new fmt so add to registry
                pDoc->SetModifiedFlag();
                pFmtReg->AddFmt(pFmtCopy);
                pTabVal->SetFmt(pFmtCopy);
                bUpdate = true;
            }
            else {
                *pFmt = *pFmtCopy;
                pDoc->SetModifiedFlag();
                delete pFmtCopy ; //done with the copy now delete it
                bUpdate = true;
            }
        }
        else {
            delete pFmtCopy;
        }
    }
    else {
        delete pFmtCopy;
        if(!bUpdate){
            return;
        }
    }
    if(arrTblBase.GetSize() > 0) {
        for(int iIndex =0; iIndex < arrTblBase.GetSize(); iIndex++){
            CTabValue* pTabValue = (DYNAMIC_DOWNCAST(CTabValue,arrTblBase.GetAt(iIndex)));
            ASSERT(pTabValue);
            ProcessSpecials(pTabValue);
        }
    }
    else {
        ProcessSpecials(pTabVal);
    }


    if(bUpdate){
      m_pGrid->Update();
      RedrawWindow();
      RestoreScrollPos();
    }
}

/////////////////////////////////////////////////////////////////////////////////
//
//  void CTabView::DoFmtDlg4DataCell(CGTblOb* pGTblOb)
//
/////////////////////////////////////////////////////////////////////////////////
void CTabView::DoFmtDlg4DataCell(CGTblOb* pGTblOb)
{
    CDataCellFmt* pFmt = nullptr;
    CDataCellFmt* pDefFmt = nullptr;
    CDataCellFmt* pFmtCopy = nullptr;
    bool bUpdate = false;
    int iPanel = -1;
    CSpecialCell* pSpecialCell = nullptr;
    CTabulateDoc* pDoc = GetDocument();
    CTabSet* pSpec = pDoc->GetTableSpec();
    CTable* pTable = m_pGrid->GetTable();
    CFmtReg* pFmtReg = pTable->GetFmtRegPtr();
    SaveScrollPos();
    CCompFmtDlg compFmtdlg;
    FMT_ID eGridComp = FMT_ID_INVALID;

    m_pGrid->GetComponent(m_iColRClick,m_lRowRClick,eGridComp,&pGTblOb);
    //Get the dialog title

    CIMSAString sComponent = m_pGrid->GetComponentString(m_iColRClick, m_lRowRClick);
    CIMSAString  sTitle = _T("Format (") +sComponent +_T(")") ;

    m_pGrid->GetComponent(m_iColRClick,m_lRowRClick,eGridComp,&pGTblOb);
    compFmtdlg.m_sTitle =  sTitle;
    compFmtdlg.m_eGridComp=eGridComp;

    pDefFmt = DYNAMIC_DOWNCAST(CDataCellFmt,pFmtReg->GetFmt(eGridComp));
    if (!pFmt) {
        compFmtdlg.m_bDefaultIndentation=TRUE;   // csc
    }
    else {
        compFmtdlg.m_bDefaultIndentation=(pFmt->GetIndent(LEFT)==INDENT_DEFAULT);   // csc
    }
    CGTblCell* pGTblCell = DYNAMIC_DOWNCAST(CGTblCell,pGTblOb);
    ASSERT(pGTblCell);
    CGTblRow* pGTblRow = pGTblCell->GetTblRow();
    ASSERT(pGTblRow);

    CTabValue* pTabVal = pGTblRow->GetTabVal();

    CArray<CSpecialCell, CSpecialCell&>& arrSpecials = pTabVal->GetSpecialCellArr();
    if(pTabVal){
        //find if we have special cells .else make one
        iPanel  = m_pGrid->GetRowPanel(m_lRowRClick);
        if(iPanel > 0 ){
            pSpecialCell = pTabVal->FindSpecialCell(iPanel,m_iColRClick);
        }
        if(pSpecialCell){
            pFmt = pSpecialCell->GetDerFmt();
            ASSERT(pFmt); // Special cells must have special fmt
            pFmtCopy = new CDataCellFmt(*pFmt);   // a copy of the ob's formats, which will be modified (possibly) through the dlg box
        }
        else {
            pFmtCopy = new CDataCellFmt(*pDefFmt);   // a copy of the ob's formats, which will be modified (possibly) through the dlg box
            // init class members ...


            pFmtCopy->Init(); //call for the decimals initialisation
            MakeDefFmt4Dlg(pFmtCopy,pDefFmt);
        }
    }
    else {
        ASSERT(FALSE); //you cannot set data cell fmt for "caption" owned cells
        return;
    }


    compFmtdlg.m_pFmt = pFmtCopy;
    compFmtdlg.m_pDefFmt = pDefFmt;
    //CDataCellFmt originalFmt(*pFmtCopy);
    if(compFmtdlg.DoModal() == IDOK){
        CDataCellFmt defCompareFmt;
        defCompareFmt.Init(); //call for the decimals initialisation
        MakeDefFmt4Dlg(&defCompareFmt,pDefFmt);

        /*bool bHasFmtChanged = *pFmtCopy != originalFmt;
        if(!bHasFmtChanged){
            return;
        }*/
        CDataCellFmt* pCompareFmt = pFmt? pFmt:&defCompareFmt;

        if(*pFmtCopy != *pCompareFmt ){
            if(!pFmt ){ //new fmt so add to registry
                pDoc->SetModifiedFlag();
                pFmtReg->AddFmt(pFmtCopy);
                CSpecialCell specialCell(iPanel,m_iColRClick);
                specialCell.SetFmt(pFmtCopy);
                arrSpecials.Add(specialCell);
                bUpdate = true;
            }
            else {
                *pFmt = *pFmtCopy;
                pDoc->SetModifiedFlag();
                delete pFmtCopy ; //done with the copy now delete it
                bUpdate = true;
            }
        }
        else {
            delete pFmtCopy;
        }
    }
    else {
        delete pFmtCopy;
        return;
    }
    if(bUpdate){
        m_pGrid->Update();
        RedrawWindow();
        RestoreScrollPos();
    }
}

/////////////////////////////////////////////////////////////////////////////////
//
// void  CTabView::MakeDefFmt(CFmt* pCopyFmt, CFmt* pDefFmt)
//
/////////////////////////////////////////////////////////////////////////////////
void CTabView::MakeDefFmt4Dlg(CFmt* pCopyFmt, CFmt* pDefFmt)
{
    CTabulateDoc* pDoc = GetDocument();
    CTabSet* pSpec = pDoc->GetTableSpec();

    CDataCellFmt* pDataCellFmt = DYNAMIC_DOWNCAST(CDataCellFmt, pCopyFmt);
    CDataCellFmt* pDefDataCellFmt = DYNAMIC_DOWNCAST(CDataCellFmt, pDefFmt);


    ASSERT(pCopyFmt);
    ASSERT(pDefFmt);
    pCopyFmt->Init();

    pCopyFmt->SetUnits(pDefFmt->GetUnits());

    FMT_COLOR txtColor = pCopyFmt->GetTextColor();
    txtColor.m_rgb = pDefFmt->GetTextColor().m_rgb;
    pCopyFmt->SetTextColor(txtColor);

    FMT_COLOR fillColor = pCopyFmt->GetFillColor();
    fillColor.m_rgb = pDefFmt->GetFillColor().m_rgb;
    pCopyFmt->SetFillColor(fillColor);

    pCopyFmt->SetID(pDefFmt->GetID());
    pCopyFmt->SetIndex(pSpec->GetFmtReg().GetNextCustomFmtIndex(*pDefFmt));
    //The following flags are not done as "default" flags so we need to get
    //the correct flags from the def fmt as the dialog uses only check boxes
    pCopyFmt->SetSpanCells(pDefFmt->GetSpanCells());
    pCopyFmt->SetHidden(pDefFmt->GetHidden());//even though this has HIDDEN_DEFAULT //bruce says we sh'd
    //just use HIDDEN_YES / HIDDEN_NO stuff and the dialog has just a check box and we cant show this
    //stuff  so we need to get the default fmt stuff into the copy
    pCopyFmt->SetFontExtends(pDefFmt->GetFontExtends());
    pCopyFmt->SetTextColorExtends(pDefFmt->GetTextColorExtends());
    pCopyFmt->SetFillColorExtends(pDefFmt->GetFillColorExtends());
    pCopyFmt->SetLinesExtend(pDefFmt->GetLinesExtend());
    pCopyFmt->SetIndentationExtends(pDefFmt->GetIndentationExtends());

    if(pCopyFmt->GetIndent(LEFT) == INDENT_DEFAULT){
        pCopyFmt->SetIndent(LEFT,pDefFmt->GetIndent(LEFT));
    }
    if(pCopyFmt->GetIndent(RIGHT) == INDENT_DEFAULT){
        pCopyFmt->SetIndent(RIGHT,pDefFmt->GetIndent(RIGHT));
    }

    if(pDataCellFmt){
        pDataCellFmt->SetZeroHidden(pDefDataCellFmt->GetZeroHidden());
    }
    if(pDefFmt->GetID() == FMT_ID_DATACELL){
        pCopyFmt->SetLineLeft(LINE_NOT_APPL);
        pCopyFmt->SetLineRight(LINE_NOT_APPL);
        pCopyFmt->SetLineTop(LINE_NOT_APPL);
        pCopyFmt->SetLineBottom(LINE_NOT_APPL);
        pCopyFmt->SetHidden(HIDDEN_NOT_APPL);
    }

}

/////////////////////////////////////////////////////////////////////////////////
//
//  void CTabView::MakeDefFmt(CFmt* pCopyFmt, CFmt* pDefFmt)
//
/////////////////////////////////////////////////////////////////////////////////
void CTabView::MakeDefTallyFmt4Dlg(CTallyFmt* pCopyFmt, CTallyFmt* pDefFmt)
{
    CTabulateDoc* pDoc = GetDocument();
    CTabSet* pSpec = pDoc->GetTableSpec();

    ASSERT(pCopyFmt);
    ASSERT(pDefFmt);
    pCopyFmt->Init();

    pCopyFmt->SetID(pDefFmt->GetID());
    pCopyFmt->SetIndex(pSpec->GetFmtReg().GetNextCustomFmtIndex(*pDefFmt));
    pCopyFmt->CopyStats(pDefFmt->GetStats());
}

/////////////////////////////////////////////////////////////////////////////////
//
//  void CTabView::ProcessSpecials(CTabValue* pTabVal)
//
/////////////////////////////////////////////////////////////////////////////////
void CTabView::ProcessSpecials(CTabValue* pTabVal)
{
    CTabulateDoc* pDoc = GetDocument();
    CTabSet* pSpec = pDoc->GetTableSpec();
    CTable* pTable = m_pGrid->GetTable();
    CFmtReg* pFmtReg = pTable->GetFmtRegPtr();

    //Chk if special vals are to be created
    if(m_iColRClick==0) {//We are processing stub
        //check columns if they are custom
        int iNumCols = m_pGrid->GetNumberCols();
        int iHeaderRows = m_pGrid->GetNumHeaderRows();
        FMT_ID eGridComp = FMT_ID_INVALID;

        for (int iCol =1; iCol < iNumCols; iCol++){
            //if yes create special val and add it to this pTabVal;
            CGTblOb* pGTblOb = nullptr;
            m_pGrid->GetComponent(iCol,iHeaderRows-1,eGridComp,&pGTblOb);
            ASSERT(pGTblOb);
            CGTblCol* pTblCol = DYNAMIC_DOWNCAST(CGTblCol,pGTblOb);
            ASSERT(pTblCol);
            CTabValue* pColHeadVal = pTblCol->GetTabVal();
            ASSERT(pColHeadVal);
            bool bFmtCustom = pColHeadVal->GetDerFmt() && pColHeadVal->GetDerFmt()->GetIndex()!=0;
            if(!bFmtCustom){//Check the indentation
                CGTblOb* pRowGTblOb = nullptr;
                FMT_ID eGridCompRow = FMT_ID_INVALID;
                m_pGrid->GetComponent(0,m_lRowRClick,eGridCompRow,&pRowGTblOb);
                CFmt evaldRowFmt;
                m_pGrid->GetFmt4NonDataCell(pRowGTblOb,eGridCompRow,evaldRowFmt);
                bool bRowIndentExtends = evaldRowFmt.GetIndentationExtends();
                CFmt evaldColFmt;
                m_pGrid->GetFmt4NonDataCell(pGTblOb,eGridComp,evaldColFmt);
                bool bColIndentExtends = evaldColFmt.GetIndentationExtends();
                if(bRowIndentExtends && bColIndentExtends){
                    bFmtCustom = true;
                }

            }
            if(!bFmtCustom){
                continue;
            }
            else {//create special val and add it to this pTabVal;
                int iCurPanelRow = m_lRowRClick;
                int iPanel = 0;
                int iNextPanelRow =0;
                iPanel = m_pGrid->GetRowPanel(m_lRowRClick);
                while(iPanel){
                    CDataCellFmt* pDataCellFmt = new CDataCellFmt();
                    CDataCellFmt *pDefDataCellFmt = DYNAMIC_DOWNCAST(CDataCellFmt,pSpec->GetFmtReg().GetFmt(FMT_ID_DATACELL));
                    pDataCellFmt->SetUnits(pDefDataCellFmt->GetUnits());
                    // init class members ...
                    pDataCellFmt->SetLineLeft(LINE_NOT_APPL);
                    pDataCellFmt->SetLineRight(LINE_NOT_APPL);
                    pDataCellFmt->SetLineTop(LINE_NOT_APPL);
                    pDataCellFmt->SetLineBottom(LINE_NOT_APPL);
                    pDataCellFmt->SetHidden(HIDDEN_NOT_APPL);

                    pDataCellFmt->SetID(FMT_ID_DATACELL);
                    //int iPanel = m_pGrid->GetRowPanel(m_lRowRClick);
                    ASSERT(iPanel > 0);
                    CSpecialCell* pSpecialCell = pTabVal->FindSpecialCell(iPanel,iCol);
                    if(pSpecialCell){
                        delete pDataCellFmt;
                        //continue;
                    }
                    else {
                        CSpecialCell specialCell(iPanel,iCol);
                        specialCell.SetFmt(pDataCellFmt);
                        pDataCellFmt->SetIndex(pSpec->GetFmtReg().GetNextCustomFmtIndex(*pDefDataCellFmt));
                        CArray<CSpecialCell, CSpecialCell&>& arrSpecials = pTabVal->GetSpecialCellArr();
                        arrSpecials.Add(specialCell);
                        pFmtReg->AddFmt(pDataCellFmt);
                    }
                    iPanel = m_pGrid->GetNextPanel(iCurPanelRow,iNextPanelRow);
                    iCurPanelRow = iNextPanelRow;
                }
            }
        }
    }
    else{ //we are processing col head
        //check stubs  if they are custom
        //if yes create special val and add it to this pTabVal;
        //check columns if they are custom
        int iNumRows = m_pGrid->GetNumberRows();
        int iHeaderRows = m_pGrid->GetNumHeaderRows();
        FMT_ID eGridComp = FMT_ID_INVALID;

        for (int iRow =iHeaderRows; iRow < iNumRows; iRow++){
            //if yes create special val and add it to this pTabVal;
            CGTblOb* pGTblOb = nullptr;
            m_pGrid->GetComponent(0,iRow,eGridComp,&pGTblOb);
            ASSERT(pGTblOb);
            CGTblRow* pTblRow = DYNAMIC_DOWNCAST(CGTblRow,pGTblOb);
            if(!pTblRow){
                continue;
            }
            CTabValue* pStubVal= pTblRow->GetTabVal();
            if(!pStubVal){
                continue;
            }
            bool bFmtCustom = pStubVal->GetDerFmt() && pStubVal->GetDerFmt()->GetIndex()!=0;
            if(!bFmtCustom){
                continue;
            }
            else {//create special val and add it to this pTabVal;
                int iRowPanel = m_pGrid->GetRowPanel(iRow);
                int iColOffSet = m_iColRClick;
                iColOffSet = m_pGrid->GetColNumForFirstPanel(iColOffSet);
                while(iColOffSet){
                    CDataCellFmt* pDataCellFmt = new CDataCellFmt();
                    CDataCellFmt *pDefDataCellFmt = DYNAMIC_DOWNCAST(CDataCellFmt,pSpec->GetFmtReg().GetFmt(FMT_ID_DATACELL));
                    pDataCellFmt->SetUnits(pDefDataCellFmt->GetUnits());
                    // init class members ...
                    pDataCellFmt->SetLineLeft(LINE_NOT_APPL);
                    pDataCellFmt->SetLineRight(LINE_NOT_APPL);
                    pDataCellFmt->SetLineTop(LINE_NOT_APPL);
                    pDataCellFmt->SetLineBottom(LINE_NOT_APPL);
                    pDataCellFmt->SetHidden(HIDDEN_NOT_APPL);

                    pDataCellFmt->SetID(FMT_ID_DATACELL);
                    ASSERT(iRowPanel > 0);
                    CSpecialCell* pSpecialCell = pStubVal->FindSpecialCell(iRowPanel,iColOffSet);
                    if(pSpecialCell){
                        delete pDataCellFmt;
                    }
                    else {
                        CSpecialCell specialCell(iRowPanel,iColOffSet);
                        specialCell.SetFmt(pDataCellFmt);
                        pDataCellFmt->SetIndex(pSpec->GetFmtReg().GetNextCustomFmtIndex(*pDefDataCellFmt));
                        CArray<CSpecialCell, CSpecialCell&>& arrSpecials = pStubVal->GetSpecialCellArr();
                        arrSpecials.Add(specialCell);
                        pFmtReg->AddFmt(pDataCellFmt);
                    }
                    iColOffSet = m_pGrid->GetNextColOffSet(iColOffSet);
                }
            }
        }
    }
}


/////////////////////////////////////////////////////////////////////////////////
//
//  void CTabView::DoFmtDlg4App()
//
/////////////////////////////////////////////////////////////////////////////////
void CTabView::DoFmtDlg4App()
{
    CTabSetFmt* pTabSetFmt = nullptr;
    CTabSetFmt* pDefTabSetFmt = nullptr;
    CTabSetFmt* pCopyTabSetFmt = nullptr;
    bool bUpdate = false;

    CTabulateDoc* pDoc = GetDocument();
    CTabSet* pSpec = pDoc->GetTableSpec();
    CTable* pTable = m_pGrid->GetTable();
    CFmtReg* pFmtReg = pTable->GetFmtRegPtr();

    CAppFmtDlg appFmtdlg;

    pTabSetFmt = pSpec->GetTabSetFmt();
    pDefTabSetFmt = DYNAMIC_DOWNCAST(CTabSetFmt,pFmtReg->GetFmt(FMT_ID_TABSET));
    //Over write the default . 'cos only one tabset fmt no need of another index
    pTabSetFmt = pDefTabSetFmt;
    if(pTabSetFmt && pTabSetFmt->GetIndex()!=0){
        pCopyTabSetFmt = new CTabSetFmt(*pTabSetFmt);   // a copy of the ob's formats, which will be modified (possibly) through the dlg box
    }
    else {
        //only one tabset
        pCopyTabSetFmt = new CTabSetFmt(*pTabSetFmt);
    }
    appFmtdlg.m_pFmt = pCopyTabSetFmt;
    SaveScrollPos();
    if(appFmtdlg.DoModal() == IDOK){
        CTabSetFmt* pCompareFmt = pTabSetFmt? pTabSetFmt:pDefTabSetFmt;

        if(*pCopyTabSetFmt != *pCompareFmt){

            // all CFmts in registry have copy of units - need to synch them when units change
            const bool bUnitsChanged = (pCompareFmt->GetUnits() != pCopyTabSetFmt->GetUnits());
            if (bUnitsChanged) {
                pFmtReg->SetUnits(pTabSetFmt->GetUnits());
            }
            /*if(!pTabSetFmt ){ //new fmt so add to registry
                pDoc->SetModifiedFlag();
                //One and only one TabSet Fmt so dont change the index
                pSpec->SetTabSetFmt(pCopyTabSetFmt);
                pFmtReg->AddFmt(pCopyTabSetFmt);
                bUpdate = true;
            }
            else {*/
                *pTabSetFmt = *pCopyTabSetFmt;
                pDoc->SetModifiedFlag();
                delete pCopyTabSetFmt ; //done with the copy now delete it
                bUpdate = true;
           // }
        }
        else {
            delete pCopyTabSetFmt;
        }
    }
    else {
        delete pCopyTabSetFmt;
        return;
    }

    if(bUpdate){
      m_pGrid->GetTable()->GenerateTitle();
      m_pGrid->Update();
      RedrawWindow();
      RestoreScrollPos();
    }
}

/////////////////////////////////////////////////////////////////////////////////
//
//  void CTabView::DoFmtDlg4Table()
//
/////////////////////////////////////////////////////////////////////////////////
void CTabView::DoFmtDlg4Table()
{
    CTblFmt* pTblFmt = nullptr;
    CTblFmt* pDefTblFmt = nullptr;
    CTblFmt* pCopyTblFmt = nullptr;
    bool bUpdate = false;

    CTabulateDoc* pDoc = GetDocument();
    CTabSet* pSpec = pDoc->GetTableSpec();
    CTable* pTable = m_pGrid->GetTable();
    CFmtReg* pFmtReg = pTable->GetFmtRegPtr();

    CTblFmtDlg TblFmtDlg;
    bool bViewer = ((CTableChildWnd*) GetParentFrame())->IsViewer();
    TblFmtDlg.m_bViewer = bViewer;

    pTblFmt = pTable->GetDerFmt();
    pDefTblFmt = DYNAMIC_DOWNCAST(CTblFmt,pFmtReg->GetFmt(FMT_ID_TABLE));
    if(pTblFmt && pTblFmt->GetIndex()!=0){
        pCopyTblFmt = new CTblFmt(*pTblFmt);   // a copy of the ob's formats, which will be modified (possibly) through the dlg box
    }
    else {
        if(pTblFmt && pTblFmt->GetIndex() == 0){//default fmt
            pTblFmt = nullptr;
        }
        pCopyTblFmt = new CTblFmt();   // a copy of the ob's formats, which will be modified (possibly) through the dlg box
        pCopyTblFmt->Init();
        pCopyTblFmt->SetIncludeEndNote(pDefTblFmt->HasEndNote());
        pCopyTblFmt->SetIncludePageNote(pDefTblFmt->HasPageNote());
        pCopyTblFmt->SetIncludeSubTitle(pDefTblFmt->HasSubTitle());
    }
    bool bHasPageNote = pCopyTblFmt->HasPageNote();
    bool bHasEndNote = pCopyTblFmt->HasEndNote();

    TblFmtDlg.m_pFmt = pCopyTblFmt;
    TblFmtDlg.m_pDefFmt = pDefTblFmt;
    SaveScrollPos();
    if(TblFmtDlg.DoModal() == IDOK){
        CTblFmt defCompareFmt;
        defCompareFmt.Init();
        defCompareFmt.SetIncludeEndNote(pDefTblFmt->HasEndNote());
        defCompareFmt.SetIncludePageNote(pDefTblFmt->HasPageNote());
        defCompareFmt.SetIncludeSubTitle(pDefTblFmt->HasSubTitle());

        CTblFmt* pCompareFmt = pTblFmt? pTblFmt:&defCompareFmt;

        if(*pCopyTblFmt != *pCompareFmt){
            if(!bHasPageNote && pCopyTblFmt->HasPageNote()){//add default text
                pTable->GetPageNote()->SetText(_T("<Type page note here>"));
                if (nullptr!=pTable->GetPageNote()->GetDerFmt()) {
                    CFmt* pFmtPageNote=pTable->GetPageNote()->GetDerFmt();

                    CUSTOM custom = pFmtPageNote->GetCustom();
                    custom.m_bIsCustomized = false;
                    custom.m_sCustomText =_T("");
                    pFmtPageNote->SetCustom(custom);
                }
            }

            if(!bHasEndNote && pCopyTblFmt->HasEndNote()){//add default text
                pTable->GetEndNote()->SetText(_T("<Type end note here>"));
                if (nullptr!=pTable->GetEndNote()->GetDerFmt()) {
                    CFmt* pFmtEndNote=pTable->GetEndNote()->GetDerFmt();

                    CUSTOM custom = pFmtEndNote->GetCustom();
                    custom.m_bIsCustomized = false;
                    custom.m_sCustomText =_T("");
                    pFmtEndNote->SetCustom(custom);
                }
            }

            if(!pTblFmt ){ //new fmt so add to registry
                pDoc->SetModifiedFlag();
                pCopyTblFmt->SetIndex(pFmtReg->GetNextCustomFmtIndex(*pCopyTblFmt));
                pTable->SetFmt(pCopyTblFmt);
                pFmtReg->AddFmt(pCopyTblFmt);
                bUpdate = true;
            }
            else {
                *pTblFmt = *pCopyTblFmt;
                pDoc->SetModifiedFlag();
                delete pCopyTblFmt ; //done with the copy now delete it
                bUpdate = true;
            }

        }
        else {
            delete pCopyTblFmt;
        }
    }
    else {
        delete pCopyTblFmt;
        return;
    }

    if(bUpdate){
      m_pGrid->Update();
      RedrawWindow();
      RestoreScrollPos();
    }
}

////////////////////////////////////////////////////////////////
//
//     CTabView::OnActivateView()
//
////////////////////////////////////////////////////////////////
void CTabView::OnActivateView(BOOL bActivate, CView* pActivateView, CView* pDeactiveView)
{
    if (bActivate && pActivateView == this) {
        UpdateAreaComboBox(false); // update list of areas on activation - may be different
                                   // list for different docs
    }
    CView::OnActivateView(bActivate, pActivateView, pDeactiveView);
}


/////////////////////////////////////////////////////////////////////////////////
//
//  void CTabView::UpdateAreaComboBox()
//
/////////////////////////////////////////////////////////////////////////////////
void CTabView::UpdateAreaComboBox(bool bResetSelection)
{
    // get the combo box from the toolbar
    CWnd* pTabToolbar = AfxGetMainWnd()->GetDescendantWindow(TABTOOLBAR);
    if (pTabToolbar == nullptr) {
        return;
    }
    ASSERT_VALID(pTabToolbar);
    CComboBox* pCombo = (CComboBox*) pTabToolbar->GetDlgItem(ID_AREA_COMBO);
    if (pCombo == nullptr) {
        return; // may not have combo box in all apps e.g. viewer
    }
    ASSERT_VALID(pCombo);

    RECT rect; // GHM 20100215
    pCombo->GetWindowRect(&rect);
    pCombo->GetParent()->ScreenToClient(&rect);
    int comboWidth = 150; // rect.right - rect.left;
    int newComboWidth = comboWidth;
    CDC * comboDC = pCombo->GetDC();


    // update combo box contents
    pCombo->ResetContent();
    pCombo->AddString(_T("All"));
    pCombo->SetItemData(0, 0);
    CTable* pTable = GetCurTable();
    if (pTable == nullptr) {
        pCombo->SetCurSel(0);
        return;
    }
    const int iNumTabData = pTable->GetTabDataArray().GetSize();
    CString sNewStr;
    for (int i = 0; i < iNumTabData; ++i) {
        CTabData* pTabData = pTable->GetTabDataArray()[i];


        // compute indentation
        CString sIndent;
        CIMSAString sBreakKey = pTabData->GetBreakKey();
        do {
            CIMSAString sToken = sBreakKey.GetToken(_T(";"));
            if (!SO::IsBlank(sToken)) {
                sIndent += _T(" ");
            }
        } while (!sBreakKey.IsEmpty());

        if (pTabData->GetAreaLabel().IsEmpty()) {
            // if label is empty, may not have been computed yet.
            // Lookup based on key.
            CIMSAString sBreakKey = pTabData->GetBreakKey();
            sBreakKey.Remove(';');
            sBreakKey.Replace(_T("-"),_T(" "));
            sBreakKey.MakeUpper();
            CTabulateDoc* pDoc = (CTabulateDoc*)GetDocument();
            CMapStringToString& areaLabelLookup = pDoc->GetAreaLabelLookup();
            CIMSAString sAreaLabel;
            if (areaLabelLookup.Lookup(sBreakKey,sAreaLabel)) {
                sNewStr = sAreaLabel;
            }
            else {
                sNewStr = sBreakKey; //no area label, use code
            }
        }
        else {
            sNewStr = pTabData->GetAreaLabel();
        }

        CString indentedStr = sIndent + sNewStr;
        CIMSAString sSuppressCaption = sNewStr;
        sSuppressCaption.Trim();
        bool bIgnoreAreaNameLabel = !sSuppressCaption.IsEmpty() && sSuppressCaption[0] == _T('~');
        if(!bIgnoreAreaNameLabel){
            int iNewStr = pCombo->InsertString(-1,indentedStr);
            pCombo->SetItemData(iNewStr, i+1);

            SIZE size; // GHM 20100215
            GetTextExtentPoint(comboDC->m_hDC,indentedStr,indentedStr.GetLength(),&size);
            newComboWidth = std::max(newComboWidth,(int) size.cx + 25);
        }
    }

    pCombo->ReleaseDC(comboDC);
    rect.right = rect.left + newComboWidth;

    if( newComboWidth != comboWidth ) // GHM 20100215
    {
        RECT toolbarRect;
        pCombo->GetParent()->GetClientRect(&toolbarRect);

        rect.right = std::min(rect.right,toolbarRect.right);
        pCombo->MoveWindow(&rect);
    }


    if (bResetSelection) {
        m_iCurrSelArea = 0; // set initial selection to "all"
    }

    // select item with item data matching m_iCurrSelArea
    for (int i = 0; i < pCombo->GetCount(); ++i) {
        if (pCombo->GetItemData(i) == m_iCurrSelArea) {
            pCombo->SetCurSel(i);
            break;
        }
    }
    m_pGrid->SetCurSelArea(m_iCurrSelArea);
}


/////////////////////////////////////////////////////////////////////////////////
//
//  void CTabView::OnSelChangeAreaComboBox()
//
/////////////////////////////////////////////////////////////////////////////////
void CTabView::OnSelChangeAreaComboBox()
{
    // get the combo box from the toolbar
    CWnd* pTabToolbar = AfxGetMainWnd()->GetDescendantWindow(TABTOOLBAR);
    if (pTabToolbar == nullptr) {
        return;
    }
    ASSERT_VALID(pTabToolbar);
    CComboBox* pCombo = (CComboBox*) pTabToolbar->GetDlgItem(ID_AREA_COMBO);
    ASSERT_VALID(pCombo);

    m_iCurrSelArea = pCombo->GetItemData(pCombo->GetCurSel());
    m_pGrid->SetCurSelArea(m_iCurrSelArea);
    m_pGrid->Update();
    SetFocus();  // set focus back to main view so you don't get strange results with arrow keys afterwards.
}

/////////////////////////////////////////////////////////////////////////////////
//
//  bool CTabView::DoMultiRecordChk(const CDictRecord* pMultRecord)
//
/////////////////////////////////////////////////////////////////////////////////
bool CTabView::DoMultiRecordChk(const CDictRecord* pMultRecord,CTabVar* pTargetVar,const TB_DROP_TYPE& eDropType,const TB_DROP_OPER& eDropOper)
{
    bool bRet = false;
    ASSERT(pMultRecord);

    if(pMultRecord->GetMaxRecs()<= 1){
        return true;
    }
    CTable* pTable = m_pGrid->GetTable();
    CTabulateDoc* pDoc = GetDocument();
    CTabSet* pSpec = pDoc->GetTableSpec();
    const CDataDict* pDataDict = pSpec->GetDict();
    if(eDropType == TB_DROP_ROW){
        //check col vars for multiple form a different record
        if(!DoMultiRecordChk(pTable->GetColRoot(),pMultRecord)){
            return false;
        }
        if(pTargetVar->IsRoot() ||(pTargetVar->GetParent()->IsRoot() && eDropOper == TB_DROP_PLUS)){
            return true;
        }
        else {
            const DictLevel*      pDictLevel;
            const CDictRecord*    pDictRecord;
            const CDictItem*      pDictItem;
            const DictValueSet*   pDictVSet;
            const CDataDict* pDict = pSpec->LookupName(pTargetVar->GetName(), &pDictLevel, &pDictRecord, &pDictItem, &pDictVSet);
            if(pDict){
                if(pDictRecord && pDictRecord->GetMaxRecs() > 1 && pMultRecord != pDictRecord ){
                    return false;
                }
            }
        }
    }
    else if(eDropType == TB_DROP_COL){
        //check row vars for multiple form a different record
        if(!DoMultiRecordChk(pTable->GetRowRoot(),pMultRecord)){
            return false;
        }
        if(pTargetVar->IsRoot() ||(pTargetVar->GetParent()->IsRoot() && eDropOper == TB_DROP_PLUS)){
            return true;
        }
        else {
            const DictLevel*      pDictLevel;
            const CDictRecord*    pDictRecord;
            const CDictItem*      pDictItem;
            const DictValueSet*   pDictVSet;
            const CDataDict* pDict = pSpec->LookupName(pTargetVar->GetName(), &pDictLevel, &pDictRecord, &pDictItem, &pDictVSet);
            if(pDict){
                if(pDictRecord && pDictRecord->GetMaxRecs() > 1 && pMultRecord != pDictRecord ){
                    return false;
                }
            }
        }
    }
    else {
        ASSERT(FALSE);
    }
    return true;
}

/////////////////////////////////////////////////////////////////////////////////
//
//  bool CTabView::DoMultiRecordChk(CTabVar* pTabVar,const CDictRecord* pMultRecord)
//
/////////////////////////////////////////////////////////////////////////////////
bool CTabView::DoMultiRecordChk(CTabVar* pTabVar,const CDictRecord* pMultRecord)
{
    bool bRet = true;

    CTabulateDoc* pDoc = GetDocument();
    CTabSet* pSpec = pDoc->GetTableSpec();
    //CDataDict*        pDataDict = pSpec->GetDict();
    bRet = pSpec->DoMultiRecordChk(pTabVar,pMultRecord);
    return bRet;
}
/////////////////////////////////////////////////////////////////////////////////
//
//  bool CTabView::DoMultiItemChk(const CDictItem* pOccDictItem,CTabVar* pTargetVar,const TB_DROP_TYPE& eDropType,const TB_DROP_OPER& eDropOper)
//
/////////////////////////////////////////////////////////////////////////////////
bool CTabView::DoMultiItemChk(const CDictItem* pOccDictItem,CTabVar* pTargetVar,const TB_DROP_TYPE& eDropType,const TB_DROP_OPER& eDropOper)
{
    bool bRet = false;
    ASSERT(pOccDictItem);

    if(pOccDictItem->GetOccurs()<= 1){
        return true;
    }
    CTable* pTable = m_pGrid->GetTable();
    CTabulateDoc* pDoc = GetDocument();
    CTabSet* pSpec = pDoc->GetTableSpec();
    const CDataDict* pDataDict = pSpec->GetDict();
    if(eDropType == TB_DROP_ROW){
        //check col vars for multiple form a different record
        if(!DoMultiItemChk(pTable->GetColRoot(),pOccDictItem)){
            return false;
        }
        if(pTargetVar->IsRoot() ||(pTargetVar->GetParent()->IsRoot() && eDropOper == TB_DROP_PLUS)){
            return true;
        }
        else {
            const DictLevel*      pDictLevel;
            const CDictRecord*    pDictRecord;
            const CDictItem*      pDictItem;
            const DictValueSet*   pDictVSet;
            const CDataDict* pDict = pSpec->LookupName(pTargetVar->GetName(), &pDictLevel, &pDictRecord, &pDictItem, &pDictVSet);
            if(pDict){
                if(pDictItem && pDictItem->GetOccurs() > 1 && pOccDictItem != pDictItem ){
                    return false;
                }
            }
        }
    }
    else if(eDropType == TB_DROP_COL){
        //check row vars for multiple form a different record
        if(!DoMultiItemChk(pTable->GetRowRoot(),pOccDictItem)){
            return false;
        }
        if(pTargetVar->IsRoot() ||(pTargetVar->GetParent()->IsRoot() && eDropOper == TB_DROP_PLUS)){
            return true;
        }
        else {
            const DictLevel*      pDictLevel;
            const CDictRecord*    pDictRecord;
            const CDictItem*      pDictItem;
            const DictValueSet*   pDictVSet;
            const CDataDict* pDict = pSpec->LookupName(pTargetVar->GetName(), &pDictLevel, &pDictRecord, &pDictItem, &pDictVSet);
            if(pDict){
                if(pDictItem && pDictItem->GetOccurs() > 1 && pOccDictItem != pDictItem ){
                    return false;
                }
            }
        }
    }
    else {
        ASSERT(FALSE);
    }
    return true;
}
/////////////////////////////////////////////////////////////////////////////////
//
//  bool CTabView::DoMultiItemChk(CTabVar* pTabVar,const CDictItem* pOccDictItem)
//
/////////////////////////////////////////////////////////////////////////////////
bool CTabView::DoMultiItemChk(CTabVar* pTabVar,const CDictItem* pOccDictItem)
{
    bool bRet = true;

    if(pOccDictItem->GetOccurs() <= 1){
        return true;
    }
    CTabulateDoc* pDoc = GetDocument();
    CTabSet* pSpec = pDoc->GetTableSpec();
    bRet = pSpec->DoMultiItemChk(pTabVar,pOccDictItem);

    return bRet;
}

/////////////////////////////////////////////////////////////////////////////////
//
// CTabView::OnFilePrint()
//
/////////////////////////////////////////////////////////////////////////////////
void CTabView::OnFilePrint()
{
    // get index of current table
    CTabulateDoc* pDoc = (CTabulateDoc*)GetDocument();
    CTabSet* pTabSet = pDoc->GetTableSpec();
    int iTbls = pTabSet->GetNumTables();
    int i =0;
    for (i = 0 ; i < iTbls ; i++)  {
        if (pTabSet->GetTable(i) == GetCurTable()) {
            break;
        }
    }
    ASSERT(i < iTbls);

    ((CTableChildWnd*) GetParentFrame())->PrintFromNonPrintView(i);
}


/////////////////////////////////////////////////////////////////////////////////
//
//  bool CTabView::IsPointInSubTableRect(CPoint point)
//
/////////////////////////////////////////////////////////////////////////////////
bool CTabView::IsPointInSubTableRect(CPoint point)
{
    bool bRet = false;
    const CArray<SUBTBL_BOX,SUBTBL_BOX&>& arrBoxes= m_pGrid->GetSubTablBoxArr();
    int iNumRect = arrBoxes.GetSize();
    for(int iIndex =0; iIndex <iNumRect; iIndex++){
        CRect rect( arrBoxes[iIndex].m_rect);
        rect.DeflateRect(2,2);
        rect.PtInRect(point) ? bRet =true : bRet = false;
        if(bRet){
            break;
        }

    }
    return bRet;
}

void CTabView::SaveScrollPos()
{
    m_iVertScrollPos =m_pGrid->m_CUGVScroll->GetScrollPos();
    m_iHorzScrollPos =m_pGrid->m_CUGHScroll->GetScrollPos();
}

void CTabView::RestoreScrollPos()
{
    if(m_iVertScrollPos != -1 && m_iHorzScrollPos != -1){
        m_pGrid->m_CUGVScroll->SetScrollPos(m_iVertScrollPos,TRUE);
        m_pGrid->m_CUGHScroll->SetScrollPos(m_iHorzScrollPos,TRUE);
        m_pGrid->m_CUGGrid->RedrawWindow();
        m_pGrid->SendMessage(WM_SIZE);
        m_iVertScrollPos =-1;
        m_iHorzScrollPos =-1;
    }
}

void CTabView::OnShiftF10()
{
    CWnd* pWnd = GetFocus();
    if (pWnd->IsKindOf(RUNTIME_CLASS(CTabTreeCtrl))) {
        pWnd->PostMessage(WM_COMMAND, ID_SHIFT_F10);
    }
    else {
        m_pGrid->PostMessage(WM_COMMAND, ID_SHIFT_F10);
    }
}

void CTabView::OnEditCopytablestructure()
{
    //Get the Tab Document
    CTabulateDoc* pDoc = (CTabulateDoc*)GetDocument();

    //Get the currently selected table
    CTable* pTable = m_pGrid->GetTable();
    ASSERT(pTable);

    CSpecFile clipFile;
    if (pTable)
    {
        CString csClipFile = pDoc->GetClipFile();
        clipFile.Open(csClipFile, CFile::modeWrite);
        pDoc->GetTableSpec()->GetFmtReg().Save(clipFile);
        pTable->Save(clipFile);
        clipFile.Close();
        UINT uFormat = pDoc->GetClipBoardFormat(TD_TABLE_FORMAT);
        pDoc->FileToClip(uFormat);
        return;
    }
}

void CTabView::OnUpdateEditCopytablestructure(CCmdUI *pCmdUI)
{
    // TODO: Add your command update UI handler code here
    CTable* pTable = m_pGrid->GetTable();
    if(pTable){
        pCmdUI->Enable(TRUE);
    }
    else {
        pCmdUI->Enable(FALSE);
    }
}

void CTabView::OnEditPastetable()
{
    CIMSAString sCmd, sArg;
    CSpecFile clipFile;
    CTabulateDoc* pDoc = (CTabulateDoc*)GetDocument();

    if (IsClipboardFormatAvailable(pDoc->GetClipBoardFormat(TD_TABLE_FORMAT)))
    {
        UINT uFormat = pDoc->GetClipBoardFormat(TD_TABLE_FORMAT);
        pDoc->ClipToFile(uFormat);
    }
    else {
        ASSERT(FALSE); //if it enters this func there should be TD_TABLE_FORMAT content in the clipboard
        return;
    }
    CString csClipFile = pDoc->GetClipFile();
    clipFile.Open(csClipFile, CFile::modeRead);

    CFmtReg     fmtReg;

    while (clipFile.GetLine(sCmd, sArg) == SF_OK){
            if (sCmd.CompareNoCase(TFT_SECT_FORMAT_TABSET) == 0){  // csc 11/21/2003
                clipFile.UngetLine();
                fmtReg.Build(clipFile, CSPRO_VERSION); //Copy will always have the current version
            }
            else if (sCmd.CompareNoCase(XTS_SECT_TABLE) == 0)
            {
                CTabulateDoc* pDoc = GetDocument();
                CTabSet* pSpec = pDoc->GetTableSpec();
                CTabTreeCtrl* pTreeCtrl = pDoc->GetTabTreeCtrl();

                int iSize = pSpec->GetNumTables() - 1;
                CIMSAString sNum;
                if(iSize >= 0){
                    sNum    = pSpec->GetTable(iSize)->GetNum();
                }
                else {
                    sNum =_T("0");
                }
                sNum = pDoc->GetTableSpec()->GetNextTabNum(sNum);
                sNum = _T("TABLE") + sNum;
                CTable* pTable = new CTable();
                //pTable->Build(clipFile, pDoc->GetTableSpec()->GetFmtReg(),true);
                pTable->Build(clipFile, fmtReg, CSPRO_VERSION, true);

                //Set a new name for the table
                pTable->SetName(sNum);
                //Reconcile Format ID's
                CMap<CFmtBase*,CFmtBase*,CFmtBase*,CFmtBase*> aMapOldFmts2NewFmts;
                ReconcileFmtsForPaste(fmtReg,aMapOldFmts2NewFmts);
                //Fix Table Formats To match the reconciled ID's
                FixFmtsForPasteTable(pTable,aMapOldFmts2NewFmts);


                if(pTreeCtrl->IsPrintTree()){
                    pSpec->AddTable(pTable);
                }
                else {
                    int iLevel = pTreeCtrl->GetCurLevel();
                    pSpec->AddTable(pTable,iLevel);
                }
                CIMSAString sError;
                if(!pDoc->SyntaxCheckPasteTbl(pTable, sError)){
                    CIMSAString sMsg;
                    sMsg = _T("*** Cannot paste table due to following reasons ***\r\n");
                    sMsg += sError;

                    TBL_PROC_INFO tblProcInfo;
                    tblProcInfo.pTabDoc = pDoc;
                    tblProcInfo.pTable = pTable;

                    AfxGetMainWnd()->SendMessage(UWM::Table::DeleteLogic, (WPARAM)0, (LPARAM)&tblProcInfo);
                    pSpec->DeleteTable(pTable);

                    AfxMessageBox(sMsg);
                    clipFile.Close();
                    return;
                }
                pTable->SetFmtRegPtr(pSpec->GetFmtRegPtr());
                m_pGrid->SetTable(pTable);
                // force design view when new table is created JH 8/05
                ((CTableChildWnd*) GetParentFrame())->SetDesignView(TRUE);
                m_pGrid->Update();
                pTreeCtrl->ReBuildTree();
                pDoc->SetModifiedFlag(TRUE);
            }
        }

    clipFile.Close();
}

void CTabView::OnUpdateEditPastetable(CCmdUI *pCmdUI)
{
    bool flgclip = false;
    CTabulateDoc* pDoc = (CTabulateDoc*)GetDocument();
    flgclip = flgclip ? flgclip: IsClipboardFormatAvailable(pDoc->GetClipBoardFormat(TD_TABLE_FORMAT));
    pCmdUI->Enable(flgclip);
}
void CTabView::ReconcileFmtsForPaste(CFmtReg& fmtRegFrmClip,CMap<CFmtBase*,CFmtBase*,CFmtBase*,CFmtBase*>& aMapOldFmts2NewFmts)
{
    CTabulateDoc* pDoc = (CTabulateDoc*)GetDocument();
    pDoc->GetTableSpec()->GetFmtRegPtr()->ReconcileFmtsForPaste(fmtRegFrmClip,aMapOldFmts2NewFmts);

    //Loop through all the clipboard fmts in fmtReg
    /*for (int i=0 ; i<fmtReg.m_aFmt.GetSize() ; ++i)  {
        CFmtBase* pSrcFmt = source.m_aFmt[i];
        if (pSrcFmt->GetIndex() == 0) {
            CFmtBase* pDestFmt = GetFmt(pSrcFmt->GetID());
            ASSERT(pDestFmt);
            pDestFmt->Assign(*pSrcFmt);
        }
    }*/
    //If it is default ignore
    //If it is custom
    //Create a new format by finding the nextID which we need
    //Add it to the current TabSet fmtReg
    //Add the old pointer and the new pointer to the map
}
void CTabView::FixFmtsForPasteTable(CTable* pTable,CMap<CFmtBase*,CFmtBase*,CFmtBase*,CFmtBase*>& aMapOldFmts2NewFmts)
{
    //Loop through all the objects of the table(tabvalues, title, header ,footer) .. need to do recursive for TabVals in A*B case
    //if the fmt is null move on
    //if not set the old fmt pointer to the new fmt pointer
}
