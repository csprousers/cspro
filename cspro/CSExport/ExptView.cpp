// ExptView.cpp : implementation of the CExportView class
//

#include "StdAfx.h"
#include "ExptView.h"
#include "CSExport.h"
#include "DictTreeView.h"
#include "ExportSplitterFrame.h"
#include "ExptDoc.h"
#include "ExportOptionsView.h"
#include "MainFrm.h"
#include <zInterfaceF/resource_shared.h>


#define DICT_RELATIONS_WARNING      _T("Items within a relation can be selected only when 'As Separate Records' is selected.")
#define DICT_RELATIONS_MULT_WARNING _T("When 'One File' is created and a relation is selected, no other multiple element can be selected.")

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CExportView

IMPLEMENT_DYNCREATE(CExportView, CFormView)

BEGIN_MESSAGE_MAP(CExportView, CFormView)
    //{{AFX_MSG_MAP(CExportView)
    ON_WM_SIZE()
    ON_NOTIFY(NM_CLICK, IDC_DATADICT_TREE, OnClickDatadictTree)
    ON_NOTIFY(TVN_KEYDOWN, IDC_DATADICT_TREE, OnKeydownDatadictTree)
    ON_COMMAND(ID_TOGGLE, OnToggle)
    ON_UPDATE_COMMAND_UI(ID_TOGGLE, OnUpdateToggle)
    ON_MESSAGE(UWM::CSExport::RefreshView, OnInitializeView)
    ON_WM_CREATE()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CExportView construction/destruction

CExportView::CExportView()
    : CFormView(CExportView::IDD)
{
    m_pExportSplitterFrame = NULL;
}

CExportView::~CExportView()
{
}

void CExportView::DoDataExchange(CDataExchange* pDX)
{
    CFormView::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CExportView)
    DDX_Control(pDX, IDC_DATADICT_TREE, m_dicttree);
    //}}AFX_DATA_MAP
}


/////////////////////////////////////////////////////////////////////////////
// CExportView drawing

void CExportView::OnDraw(CDC* pDC)
{
    UNREFERENCED_PARAMETER(pDC);
    CExportDoc* pDoc = GetDocument();
    ASSERT_VALID(pDoc);
    // TODO: add draw code for native data here
}


/////////////////////////////////////////////////////////////////////////////
// CExportView diagnostics

#ifdef _DEBUG
void CExportView::AssertValid() const
{
    CView::AssertValid();
}

void CExportView::Dump(CDumpContext& dc) const
{
    CView::Dump(dc);
}

CExportDoc* CExportView::GetDocument() // non-debug version is inline
{
    ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CExportDoc)));
    return (CExportDoc*)m_pDocument;
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CExportView message handlers

/////////////////////////////////////////////////////////////////////////////
//
//                       CExportView::AddItemIntree
//
/////////////////////////////////////////////////////////////////////////////

bool CExportView::AddItemInTree(int iRel, const CDictItem* pItem, HTREEITEM htreeLabel) {

    TVINSERTSTRUCT tvistrItem;
    HTREEITEM hParentItem;
    tvistrItem.item.mask = TVIF_PARAM | TVIF_TEXT;
    int totocc = pItem->GetOccurs();
    if (pItem->GetParentItem() != NULL) {
        totocc = pItem->GetParentItem()->GetOccurs()*totocc;
    }
    CString cs = pItem->GetLabel() ;
    tvistrItem.item.pszText = cs.GetBuffer(0);
    cs.ReleaseBuffer();
    tvistrItem.item.cchTextMax = cs.GetLength();
    int occurence = -1;
    tvistrItem.hParent = htreeLabel;
    tvistrItem.hInsertAfter = 0;
    tvistrItem.item.lParam = GetDocument()->GetPositionInList(iRel, pItem->GetName(),occurence);



    hParentItem = m_dicttree.InsertItem(&tvistrItem);

    m_dicttree.SetCheck(hParentItem,GetDocument()->IsChecked(tvistrItem.item.lParam));
    if (pItem->GetParentItem() == NULL) {
        m_dicttree.SetItemImage( hParentItem, 4, 4);
    }
    else {
        m_dicttree.SetItemImage( hParentItem, 6, 6);
    }
    SetParentStates(hParentItem); // Set all the parents
    return true;
}

/////////////////////////////////////////////////////////////////////////////
//
//                       CExportView::AddRecordIntree
//
/////////////////////////////////////////////////////////////////////////////

const CDictItem* CExportView::GetDictItem(HTREEITEM hitem){
    const CDictItem* pDictItem = NULL;
    m_aMap_DictItem_by_hitem.Lookup(hitem,pDictItem);
    return pDictItem;
}

// Add all record and its items in the tree structure.
bool CExportView::AddRecordIntree(int iRel, const CDictRecord* pRecord, HTREEITEM htreeLabel) {
    TVINSERTSTRUCT tvistrRecord,tvistrItem,tvistrVSet;
    HTREEITEM htreeRecord = NULL;
    HTREEITEM hItem = NULL;
    HTREEITEM hParentItem = NULL;
    tvistrRecord.item.mask = TVIF_PARAM | TVIF_TEXT;
    tvistrItem.item.mask = TVIF_PARAM | TVIF_TEXT;
    tvistrVSet.item.mask = TVIF_PARAM | TVIF_TEXT;

    CString cs = pRecord->GetLabel() ;
    tvistrRecord.item.pszText = cs.GetBuffer(0);
    cs.ReleaseBuffer();
    tvistrRecord.item.cchTextMax = cs.GetLength();
    tvistrRecord.item.lParam = (LPARAM) -2;//s.GetId();
    tvistrRecord.hParent = htreeLabel;//TVI_ROOT;
    tvistrRecord.hInsertAfter = 0;
    tvistrRecord.item.state = TVIS_EXPANDED;
    htreeRecord = m_dicttree.InsertItem(&tvistrRecord);
    m_dicttree.SetCheck(htreeRecord,FALSE);
    if (pRecord->GetSonNumber() == COMMON)
        m_dicttree.SetItemImage( htreeRecord, 3, 3);
    else
        m_dicttree.SetItemImage( htreeRecord, 2, 2);

    for (int i = 0 ; i < pRecord->GetNumItems() ; i++)
    {
        const CDictItem* pItem = pRecord->GetItem(i);
        if( !pItem->AddToTreeFor80() )
            continue;
        int totocc = pItem->GetOccurs();
        if (pItem->GetParentItem() != NULL)
            totocc = pItem->GetParentItem()->GetOccurs()*totocc;
        CString csItem = pItem->GetLabel() ;
        tvistrItem.item.pszText = csItem.GetBuffer(0);
        csItem.ReleaseBuffer();
        tvistrItem.item.cchTextMax = csItem.GetLength();
        int occurence = -1;
        if (totocc > 1)
            occurence = 0;

        tvistrItem.hParent = htreeRecord;
        tvistrItem.hInsertAfter = 0;
        tvistrItem.item.lParam = GetDocument()->GetPositionInList(iRel, pItem->GetName(),occurence);
        hParentItem = m_dicttree.InsertItem(&tvistrItem);

        //FABN INIT
        m_aMap_DictItem_by_hitem.SetAt( hParentItem, pItem );
        //FABN END

        m_dicttree.SetCheck(hParentItem,GetDocument()->IsChecked(tvistrItem.item.lParam));
//      if ( pItem->GetOccurs() >= 1)
//          m_dicttree.SetCheck(hParentItem,GetDocument()->IsChecked(pItem->GetName()));
//      else
//          m_dicttree.SetCheck(hParentItem,FALSE);
        if (pItem->GetParentItem() == NULL)
            m_dicttree.SetItemImage( hParentItem, 4, 4);
        else
            m_dicttree.SetItemImage( hParentItem, 6, 6);
        SetParentStates(hParentItem); // Set all the parents

        if ( totocc > 1)
        {
            for(int occ = 0; occ < totocc;occ++)
            {
                CString csLabel = pItem->GetLabel() ;
                CString csoccLabel;
                csoccLabel.Format(_T("(%d)"),occ+1);
                int occurence1 = 0;
                if (totocc > 1)
                {
                    occurence1 = occ+1;
                    if( pItem->GetOccurs() == 1 || pItem->GetOccurrenceLabels().GetLabel(occ).IsEmpty() )
                        csLabel += csoccLabel;
                    else
                        csLabel = pItem->GetOccurrenceLabels().GetLabel(occ);
                    //tvistrItem.item.lParam = (LPARAM) occ+1;
//                  cs += csoccLabel;
                }
                tvistrItem.item.pszText = csLabel.GetBuffer(0);
                csLabel.ReleaseBuffer();
                tvistrItem.item.cchTextMax = csLabel.GetLength();
                tvistrItem.hParent = hParentItem;
                tvistrItem.hInsertAfter = 0;
                tvistrItem.item.lParam = GetDocument()->GetPositionInList(iRel, pItem->GetName(),occurence1);

                hItem = m_dicttree.InsertItem(&tvistrItem);
                //m_dicttree.SetCheck(hItem,GetDocument()->IsChecked(pItem->GetName(),occ+1));
                m_dicttree.SetCheck(hItem,GetDocument()->IsChecked(tvistrItem.item.lParam));
                //m_dicttree.SetCheck(hItem,FALSE);
                if (pItem->GetParentItem() == NULL)
                    m_dicttree.SetItemImage( hItem, 4, 4);
                else
                    m_dicttree.SetItemImage( hItem, 6, 6);
            }
            SetParentStates(hItem); // Set all the parents
        }
    }
    return true;
}

// InitializeView of the form
LONG CExportView::OnInitializeView(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    CWaitCursor wait;
    const CDataDict* pDataDict = GetDocument()->m_pDataDict.get();
    if (pDataDict == NULL)
        return 0L;

    m_dicttree.DeleteAllItems ();
    m_aMapRel_Relation_by_hitem.clear();
    m_aMapRel_hitem_by_Relation.clear();

    TVINSERTSTRUCT tvistrLabel,tvistrRoot;
    HTREEITEM htreeLabel,htreeRoot;
    tvistrLabel.item.mask = TVIF_PARAM | TVIF_TEXT;
    tvistrRoot.item.mask = TVIF_PARAM | TVIF_TEXT;
    CString cs;
    cs = pDataDict->GetLabel() ;
    tvistrRoot.item.pszText = cs.GetBuffer(0);
    cs.ReleaseBuffer();
    tvistrRoot.item.cchTextMax = cs.GetLength();
    tvistrRoot.item.lParam = (LPARAM) -2;//s.GetId();
    tvistrRoot.hParent = TVI_ROOT;
    tvistrRoot.hInsertAfter = 0;
    tvistrRoot.item.state = TVIS_EXPANDED;
    htreeRoot = m_dicttree.InsertItem(&tvistrRoot);
    m_dicttree.SetCheck(htreeRoot,FALSE);
    m_dicttree.SetItemImage( htreeRoot, 0, 0);

    for( const DictLevel& dict_level : pDataDict->GetLevels() )
    {
        CString csLevel = dict_level.GetLabel();
        tvistrLabel.item.pszText = csLevel.GetBuffer(0);
        csLevel.ReleaseBuffer();
        tvistrLabel.item.cchTextMax = csLevel.GetLength();
        tvistrLabel.item.lParam = (LPARAM) -2;//s.GetId();
        tvistrLabel.hParent = htreeRoot;
        tvistrLabel.hInsertAfter = 0;
        tvistrLabel.item.state = TVIS_EXPANDED;
        htreeLabel = m_dicttree.InsertItem(&tvistrLabel);
        m_dicttree.SetCheck(htreeLabel ,FALSE);
        m_dicttree.SetItemImage( htreeLabel, 1, 1);
        AddRecordIntree(NONE, dict_level.GetIdItemsRec(),htreeLabel);

        for (int j = 0 ; j < dict_level.GetNumRecords() ; j++) {
            AddRecordIntree(NONE, dict_level.GetRecord(j),htreeLabel);
        }
        m_dicttree.Expand(htreeLabel,TVE_EXPAND);
    }

    int iRel = -1;
    for( const auto& dict_relation : pDataDict->GetRelations() ) {
        ++iRel;
        // Create relation tree item
        CString csName = dict_relation.GetName();
        tvistrLabel.item.pszText = csName.GetBuffer(0);
        csName.ReleaseBuffer();
        tvistrLabel.item.cchTextMax = csName.GetLength();
        tvistrLabel.item.lParam = (LPARAM) -2;//s.GetId();
        tvistrLabel.hParent = htreeRoot;
        tvistrLabel.hInsertAfter = 0;
        tvistrLabel.item.state = TVIS_EXPANDED;
        htreeLabel = m_dicttree.InsertItem(&tvistrLabel);

        //double mapping for fast access
        m_aMapRel_Relation_by_hitem.try_emplace(htreeLabel, &dict_relation);
        m_aMapRel_hitem_by_Relation.try_emplace(&dict_relation, htreeLabel);

        m_dicttree.SetCheck(htreeLabel, FALSE);
        m_dicttree.SetItemImage( htreeLabel, 7, 7);

        // Create primary tree item
        int iLevel = NONE;
        int iRecord = NONE;
        int iItem = NONE;
        int iVSet = NONE;
        pDataDict->LookupName(dict_relation.GetPrimaryName(), &iLevel, &iRecord, &iItem, &iVSet);
        const CDictRecord* pRecord = pDataDict->GetLevel(iLevel).GetRecord(iRecord);
        if (iItem == NONE) {
            AddRecordIntree(iRel, pRecord, htreeLabel);
        }
        else {
            AddItemInTree(iRel, pRecord->GetItem(iItem),htreeLabel);
            for (int m = iItem++ ; m < pRecord->GetNumItems() ; m++) {
                if (pRecord->GetItem(m)->GetItemType() == ItemType::Item) {
                    break;
                }
                AddItemInTree(iRel, pRecord->GetItem(m),htreeLabel);
            }
        }
        m_dicttree.Expand(htreeLabel,TVE_EXPAND);

        // Create secondary tree items
        for( const auto& dict_relation_part : dict_relation.GetRelationParts() ) {
            int iLevel1 = NONE;
            int iRecord1 = NONE;
            int iItem1 = NONE;
            int iVSet1 = NONE;
            pDataDict->LookupName(dict_relation_part.GetSecondaryName(), &iLevel1, &iRecord1, &iItem1, &iVSet1);
            const CDictRecord* pRecord1 = pDataDict->GetLevel(iLevel1).GetRecord(iRecord1);
            if (iItem1 == NONE) {
                AddRecordIntree(iRel, pRecord1,htreeLabel);
            }
            else {
                AddItemInTree(iRel, pRecord1->GetItem(iItem1),htreeLabel);
                if (pRecord1->GetItem(iItem1)->GetItemType() == ItemType::Item) {
                    for (int m = iItem1 + 1 ; m < pRecord1->GetNumItems() ; m++) {
                        if (pRecord1->GetItem(m)->GetItemType() == ItemType::Item) {
                            break;
                        }
                        AddItemInTree(iRel, pRecord1->GetItem(m),htreeLabel);
                    }
                }
            }
            m_dicttree.Expand(htreeLabel,TVE_EXPAND);
        }
    }
    m_dicttree.Expand(htreeRoot,TVE_EXPAND);

    // labels are shown by default so if names should be shown, flip the setting
    if (SharedSettings::ViewNamesInTree())
    {
        SharedSettings::ToggleViewNamesInTree();
        OnToggle();
    }

    m_dicttree.SetFocus();
    ((CMainFrame*)AfxGetMainWnd())->SetActiveView(this);
//  GetParentFrame()->RecalcLayout();

    // Update options here now that tree control selections are updated.
    // Slightly redundant but needed since code to enable/disable IDC_JOIN
    // is based on selection in tree which is not set until now when loading
    // from file.
    GetDocument()->UpdateOptionsPane();

    return TRUE;
}

const DictRelation* CExportView::GetRelation(HTREEITEM hItem)
{
    const auto& lookup = m_aMapRel_Relation_by_hitem.find(hItem);

    return ( lookup != m_aMapRel_Relation_by_hitem.cend() ) ? lookup->second :
           ( hItem != nullptr )                             ? GetRelation(m_dicttree.GetParentItem(hItem)) :
                                                              nullptr;
}

int CExportView::GetChecked(HTREEITEM hitem, bool bRecursive, bool bIncludeRelations, CArray<HTREEITEM,HTREEITEM>& aCheckedItems ){

    CTreeCtrl& tree = m_dicttree;

    const DictRelation* pDictRelation = nullptr;

    if( hitem && tree.GetCheck(hitem) ){

        if( bIncludeRelations ){
            aCheckedItems.Add( hitem );

        } else {
            pDictRelation = GetRelation(hitem);
            if(!pDictRelation){
                aCheckedItems.Add( hitem );
            }
        }
    }

    if(bRecursive && hitem){
        if( !bIncludeRelations && pDictRelation!=NULL ){
            //do nothing : each child of an item that is in relation : is in relation too.

        } else {
            hitem = tree.GetChildItem( hitem );
            while( hitem ){
                GetChecked( hitem, bRecursive, bIncludeRelations, aCheckedItems );
                hitem = tree.GetNextItem( hitem, TVGN_NEXT );
            }
        }
    }

    return aCheckedItems.GetSize();
}

int CExportView::GetSelItemsByRelation(const DictRelation& dict_relation, CArray<HTREEITEM,HTREEITEM>& raSelItems)
{
    const auto& lookup = m_aMapRel_hitem_by_Relation.find(&dict_relation);

    if( lookup != m_aMapRel_hitem_by_Relation.cend() ) {
        CArray<HTREEITEM,HTREEITEM> aCheckedItems;
        GetChecked( lookup->second, true, true, aCheckedItems );
        raSelItems.Append( aCheckedItems );
    }

    return raSelItems.GetSize();
}

void CExportView::FromHitems2DictItems(//input
                                       CArray<HTREEITEM,HTREEITEM>& aSelHitems,

                                       //output
                                       CArray<const CDictItem*,const CDictItem*>&                       rItems,
                                       CMap<const CDictItem*,const CDictItem*,CArray<int,int>*,CArray<int,int>*>&   rMapSelOccsByItem){


    CMap<const CDictItem*,const CDictItem*,const CDictItem*,const CDictItem*> aMapAddedItems;

    int n = aSelHitems.GetSize();
    for(int i=0; i<n; i++){
        HTREEITEM   hitem   = aSelHitems.ElementAt(i);
        int         pos     = (int) m_dicttree.GetItemData(hitem);
        if( 0<=pos && pos<GetDocument()->m_aItems.GetSize() ){
            const CDictItem* pDictItem = GetDocument()->m_aItems[pos].pItem;
            if( pDictItem ){

                //add the item : avoid duplicated ones
                if(!aMapAddedItems.Lookup( pDictItem, pDictItem )){

                    rItems.Add( pDictItem );

                    aMapAddedItems.SetAt( pDictItem, pDictItem );
                }


                //add occ
                int occ =   GetDocument()->m_aItems[pos].occ;

                const CDictItem* pParent = pDictItem->GetParentItem();

                bool bMultiple = false;

                //subitem inside a multiple item Item
                if( pDictItem->GetItemType() == ItemType::Subitem && pParent && pParent->GetOccurs()>1 ){
                    bMultiple = true;

                //multiple item
                } else if( pDictItem->GetOccurs()>1 ){
                    bMultiple = true;

                //any thing inside a multiple record
                } else if( pDictItem->GetRecord()->GetMaxRecs()>1 ){
                    bMultiple = true;

                }

                #ifdef _DEBUG
                    if( !bMultiple ){
                        ASSERT( occ <= 1 );
                    }
                #endif



                if( bMultiple ||  occ>1 ){

                    CArray<int,int>* pOccs = NULL;
                    if( !rMapSelOccsByItem.Lookup( pDictItem, pOccs ) ){
                        pOccs = new CArray<int,int>;
                        rMapSelOccsByItem.SetAt( pDictItem, pOccs );
                    }
                    pOccs->Add( occ );

                }
            }
        }
    }
}

int CExportView::GetSelItemsByRelation(const DictRelation& dict_relation, CArray<const CDictItem*,const CDictItem*>& raSelItems,
                                       CMap<const CDictItem*,const CDictItem*,CArray<int,int>*,CArray<int,int>*>& rMapSelOccsByItem ){

    CArray<HTREEITEM,HTREEITEM> aSelHitems;
    GetSelItemsByRelation(dict_relation, aSelHitems);

    FromHitems2DictItems( aSelHitems, raSelItems, rMapSelOccsByItem );

    return raSelItems.GetSize();

}


int CExportView::GetSelItems( bool bIncludeRelations, CArray<const CDictItem*,const CDictItem*>& raSelItems,
    CMap<const CDictItem*,const CDictItem*,CArray<int,int>*,CArray<int,int>*>& rMapSelOccsByItem, bool* pbHasAnyMultiple, bool* pbHasAnySingle, bool* pbIgnoreChildsOfSingleRecords )
{
    bool bIgnoreChildsOfSingleRecords = false;
    if(pbIgnoreChildsOfSingleRecords)
        bIgnoreChildsOfSingleRecords = *pbIgnoreChildsOfSingleRecords;
    CArray<HTREEITEM,HTREEITEM> aCheckedItems;
    GetChecked( IsWindow(m_dicttree) ? m_dicttree.GetRootItem() : NULL, true, bIncludeRelations, aCheckedItems );

    FromHitems2DictItems( /*input*/aCheckedItems, /*output*/raSelItems, rMapSelOccsByItem );

    ASSERT( rMapSelOccsByItem.GetCount() <= raSelItems.GetSize() );

    CMap<const CDictRecord*,const CDictRecord*,bool,bool> aMapFlatExport;
    if( pbHasAnyMultiple ){
        *pbHasAnyMultiple = false;
        int n = raSelItems.GetSize();
        for(int i=0; i<n; i++){
            const CDictItem* pDictItem = raSelItems.GetAt(i);
            const CDictRecord* pDictRecord = pDictItem->GetRecord();

            if( bIgnoreChildsOfSingleRecords && pDictRecord->GetMaxRecs()==1 )
                continue;


            if( pDictRecord->GetMaxRecs()>1 ){
                *pbHasAnyMultiple = true;
                break;
            }

            if(pDictItem->GetOccurs()>1){
                *pbHasAnyMultiple = true;
                break;
            }
        }
    }

    if( pbHasAnySingle ){
        *pbHasAnySingle = false;

        int n = raSelItems.GetSize();
        for(int i=0; i<n; i++){
            const CDictItem* pDictItem = raSelItems.GetAt(i);
            const CDictRecord* pDictRecord = pDictItem->GetRecord();

            if( pDictRecord->GetMaxRecs()>1 )
                continue;


            if(pDictItem->GetOccurs()>1)
                continue;

            *pbHasAnySingle = true;
            break;
        }
    }

    return raSelItems.GetSize();
}


bool CExportView::IsRelationSelected(const DictRelation& dict_relation) {
    CArray<HTREEITEM,HTREEITEM> aSelItems;
    return GetSelItemsByRelation(dict_relation, aSelItems)>0;
}

// Clicking of the treeitem
void CExportView::TreeItemClicked(HTREEITEM hItem,bool recursive )
{
    ASSERT(hItem!=NULL);
    GetDocument()->SetModifiedFlag();

    TVITEM tvi;
    LPARAM lParam;

    tvi.hItem = hItem;
    tvi.mask = TVIF_PARAM;
    tvi.hItem = hItem;
    m_dicttree.GetItem(&tvi);
    lParam = tvi.lParam;

    if (m_dicttree.ItemHasChildren(hItem))  {
        // If the item has children set check for all the childrens
        ASSERT(m_dicttree.ItemHasChildren(hItem));
        HTREEITEM hTblTitle = m_dicttree.GetChildItem(hItem);
        while (hTblTitle != NULL)  {
            //if (m_dicttree.GetItemData(hItem) == -2 && recursive == 0)
            //{
            //  m_dicttree.Expand( hItem,TVE_EXPAND );  //(hItem)->
            //}
            m_dicttree.SetCheck(hTblTitle, m_dicttree.GetCheck(hItem));
            CString cs = m_dicttree.GetItemText(hTblTitle);
            TreeItemClicked(hTblTitle, true);
            hTblTitle = m_dicttree.GetNextItem(hTblTitle, TVGN_NEXT);
        }
    }
    int occ = 0;
    int item = m_dicttree.GetItemData(hItem);
    if (item < 0)
    {
        if (recursive) return;
        SetParentStates(hItem);
        return;
    }

    occ = GetDocument()->GetItemOcc (item);
//  if (occ == -2)
//  {
//      item = m_dicttree.GetItemData(m_dicttree.GetParentItem(hItem));
//      occ = GetDocument()->GetItemOcc (item);
//  }
    if (occ!=0)
    {

/*//SAVY commented this ' relation bug  selection reported buy Juilio
        //Relation tree is like an independent tree
        DictRelation* pDictRelation = GetRelation( hItem );
        if( pDictRelation ){
            //just do nothing
        } else */

        GetDocument()->SetItemCheck(item, m_dicttree.GetCheck(hItem) != 0);
    }
    //  GetDocument()->AddItemtoList(GetDetails(hItem), (bool)m_dicttree.GetCheck(hItem),occ);
    if (recursive) return;
    SetParentStates(hItem);
}

// Resizing of the views
void CExportView::OnSize(UINT nType, int cx, int cy)
{
    CFormView::OnSize(nType, cx, cy);

    if(!m_pExportSplitterFrame)
        return;

    if(!IsWindow(m_pExportSplitterFrame->GetSafeHwnd()))
        return;

    m_pExportSplitterFrame->MoveWindow(0,0,cx,cy);
}

// Click Data dictionaries.
void CExportView::OnClickDatadictTree(NMHDR* pNMHDR, LRESULT* pResult)
{
  DWORD dw = GetMessagePos();
  CPoint p(LOWORD(dw), HIWORD(dw));
  m_dicttree.ScreenToClient(&p);

  UINT htFlags = 0;
  HTREEITEM it = m_dicttree.HitTest(p, &htFlags);

  if (it != NULL && (htFlags == TVHT_ONITEMSTATEICON)) {   //htFlags == TVHT_ONITEMLABEL ||
    HTREEITEM hitem = (HTREEITEM) it;
    int checked = (m_dicttree.GetItemState(hitem, TVIS_STATEIMAGEMASK) >> 12) - 1;
    if (checked == 2 || checked == 1 ) {
        m_dicttree.SetCheck(hitem,FALSE);
    }
    else
    {
        m_dicttree.SetCheck(hitem,TRUE);
    }
    // the check box was hit.
    OnTvCheckbox(pNMHDR->idFrom, (LPARAM) it);
  }
  *pResult = 0;
}


void CExportView::OnKeydownDatadictTree(NMHDR* /*pNMHDR*/, LRESULT* pResult)
{
  HTREEITEM it = m_dicttree.GetSelectedItem();
    //TreeItemClicked(hitem);
  if (it != NULL ) {
      short keystate = GetAsyncKeyState(VK_SPACE);
      if (keystate != 0 )
      {
        HTREEITEM hitem = (HTREEITEM) it;
        int checked = (m_dicttree.GetItemState(hitem, TVIS_STATEIMAGEMASK) >> 12) - 1;
        if (checked == 2 || checked == 1 ) {
            m_dicttree.SetCheck(hitem,FALSE);
        }
        else
        {
            m_dicttree.SetCheck(hitem,TRUE);
        }
      }
  }

    *pResult = 0;
}

// Set all the parents of setting the parents of checked.
void CExportView::SetParentStates(HTREEITEM hItem)
{
    HTREEITEM hParent = m_dicttree.GetParentItem(hItem);
    if (hParent == NULL) return;

    UINT bparentstate = 1;
    BOOL flagfalse = FALSE;
    hItem = m_dicttree.GetChildItem(hParent);  // first kid
    ASSERT(hItem!=NULL);
    while (hItem != NULL)  {
        switch (m_dicttree.GetCheck(hItem))
        {
        case 0:
            flagfalse = TRUE;
            break;
        case 1:
            bparentstate = 2;
            break;
        case 2:
            bparentstate = 3;
            break;
        }
        if (bparentstate == 3) break;

        hItem = m_dicttree.GetNextSiblingItem(hItem);
    }
    if (bparentstate == 2 && flagfalse) bparentstate = 3;
    m_dicttree.SetItemState(hParent,bparentstate<<12,TVIS_STATEIMAGEMASK);
    SetParentStates(hParent);
}


void CExportView::OnInitialUpdate()
{
    CFormView::OnInitialUpdate();

    ASSERT (GetDocument() != NULL);
    //ASSERT (GetDocument()->GetDataDict() != NULL);

    if (m_ilState.m_hImageList == NULL)
    {
        m_ilState.Create(IDB_SELECTION_STATES, 16, 0, RGB(255, 255, 255));
        m_dicttree.SetImageList(&m_ilState, TVSIL_STATE);    // attach the state  image list

        m_ilNormal.Create(16, 16, ILC_COLOR,0,7);
        m_ilNormal.SetBkColor(RGB(255, 255, 255));
        m_ilNormal.Add(AfxGetApp()->LoadIcon(IDI_DICTIONARY));
        m_ilNormal.Add(AfxGetApp()->LoadIcon(IDI_DICTIONARY_LEVEL));
        m_ilNormal.Add(AfxGetApp()->LoadIcon(IDI_DICTIONARY_RECORD));
        m_ilNormal.Add(AfxGetApp()->LoadIcon(IDI_DICTIONARY_ID_RECORD));
        m_ilNormal.Add(AfxGetApp()->LoadIcon(IDI_DICTIONARY_ITEM));
        m_ilNormal.Add(AfxGetApp()->LoadIcon(IDI_DICTIONARY_VALUESET));
        m_ilNormal.Add(AfxGetApp()->LoadIcon(IDI_DICTIONARY_SUBITEM));
        m_ilNormal.Add(AfxGetApp()->LoadIcon(IDI_DICTIONARY_RELATION));
        m_dicttree.SetImageList(&m_ilNormal, TVSIL_NORMAL);    // attach the state  image list
    }

    //FABN DEC 2004
    //do nothing, the parent of m_dicttree is m_pExportSplitterFrame->GetLeftView()
    if( m_pExportSplitterFrame && IsWindow(m_pExportSplitterFrame->GetSafeHwnd()) )
        m_dicttree.SetParent( m_pExportSplitterFrame->GetLeftView() );

    // make sure that parent frame is large enough to completely show the form view
    GetParentFrame()->RecalcLayout();
    UpdateWindow();
    PostMessage(UWM::CSExport::RefreshView);
}

// Get the name of the item clicked.
CString CExportView::GetDetails(HTREEITEM hItem)
{
    ASSERT(hItem != NULL);

    HTREEITEM htempRoot = m_dicttree.GetRootItem();

    int level = 0, record = 0, item = 0, iocc = 0;
    if (htempRoot == hItem) return _T("");
    for (HTREEITEM htemplevel = m_dicttree.GetChildItem(htempRoot); htemplevel != NULL;
    htemplevel = m_dicttree.GetNextItem(htemplevel,TVGN_NEXT))
    {
        if (htemplevel == hItem)    return _T("");//GetDocument()->GetNameat(level, record,item, vset);
        record = 0;
        for (HTREEITEM htempRecord = m_dicttree.GetChildItem(htemplevel); htempRecord != NULL;
        htempRecord = m_dicttree.GetNextItem(htempRecord,TVGN_NEXT))
        {
            item = 0;
            if (htempRecord == hItem)   return _T("");//GetDocument()->GetNameat(level, record,item, vset);
            for (HTREEITEM htempItem = m_dicttree.GetChildItem(htempRecord); htempItem != NULL;
            htempItem = m_dicttree.GetNextItem(htempItem,TVGN_NEXT))
            {
                iocc = 0;
                if (htempItem == hItem)
                {
                    if (m_dicttree.GetChildItem(htempItem)==NULL)
                        return GetDocument()->GetNameat(level, record-1,item, 0);
                    return _T("");
                }
                for (HTREEITEM htempOcc = m_dicttree.GetChildItem(htempItem); htempOcc != NULL;
                htempOcc = m_dicttree.GetNextItem(htempOcc,TVGN_NEXT))
                {
                    if (htempOcc == hItem) return GetDocument()->GetNameat(level, record-1,item,0);
                    iocc++;
                }
                item++;
            }
            record++;
        }
        level++;
    }
    return _T("");
}

// When the user checks the tree check box.
LRESULT CExportView::OnTvCheckbox(WPARAM wp, LPARAM lp)
{
    UNREFERENCED_PARAMETER(wp);

    HTREEITEM hitem = (HTREEITEM) lp;
    int checked ;
    checked = (m_dicttree.GetItemState(hitem, TVIS_STATEIMAGEMASK) >> 12) - 1;
    if (checked == 2 ) {
        m_dicttree.SetCheck(hitem,FALSE);
    }
    TreeItemClicked(hitem);

    //Items may be selected from relations only when "As separate records" is selected
    const DictRelation* pDictRelation = GetRelation(hitem);
    if( pDictRelation && m_dicttree.GetCheck(hitem) && GetDocument()->m_bAllInOneRecord ){
        AfxMessageBox( DICT_RELATIONS_WARNING );
        m_dicttree.SetCheck(hitem,FALSE);
        TreeItemClicked(hitem);

    } else {

        //
        bool bIgnoreChildsOfSingleRecord = GetDocument()->WantFlatExport();

        if( /*single file*/GetDocument()->m_bmerge  &&
                IsSelectedAnyRelation()                 &&
                    IsSelectedAnyMultiple(false, bIgnoreChildsOfSingleRecord  ) ){

        AfxMessageBox( DICT_RELATIONS_MULT_WARNING );
        m_dicttree.SetCheck(hitem,FALSE);
        TreeItemClicked(hitem);

        } else {

            //a change in the tree selection => new conditions to decide wich controls in
            //the options pane can be enabled/disabled
            GetDocument()->UpdateOptionsPane();

            GetDocument()->SyncBuff_app();
        }
    }

    GetDocument()->Checks();

    return 0;
}


void CExportView::RefreshView()
{
    int iImage;
    int iSelImage;
    int iLevel;
    int iRecord;
    int iItem;
    int iVSet;

    if (SharedSettings::ViewNamesInTree())
    {
        HTREEITEM htempRoot = m_dicttree.GetRootItem();
        int level = 0, record = 0, item = 0, iocc = 0, rel = 0, part = 0;
        if (htempRoot == NULL)  return;
        m_dicttree.SetItemText(htempRoot, GetDocument()->m_pDataDict->GetName());
        for (HTREEITEM htemplevel = m_dicttree.GetChildItem(htempRoot); htemplevel != NULL;
            htemplevel = m_dicttree.GetNextItem(htemplevel, TVGN_NEXT)) {
            m_dicttree.GetItemImage(htemplevel, iImage, iSelImage);
            if (iImage == 1) {
                m_dicttree.SetItemText(htemplevel, GetDocument()->m_pDataDict->GetLevel(level).GetName());
                record = -1;
                for (HTREEITEM htempRecord = m_dicttree.GetChildItem(htemplevel); htempRecord != NULL;
                    htempRecord = m_dicttree.GetNextItem(htempRecord, TVGN_NEXT))
                {
                    item = 0;
                    if (record == -1)
                        m_dicttree.SetItemText(htempRecord, GetDocument()->m_pDataDict->GetLevel(level).GetIdItemsRec()->GetName());
                    else
                        m_dicttree.SetItemText(htempRecord, GetDocument()->m_pDataDict->GetLevel(level).GetRecord(record)->GetName());
                    for (HTREEITEM htempItem = m_dicttree.GetChildItem(htempRecord); htempItem != NULL;
                        htempItem = m_dicttree.GetNextItem(htempItem, TVGN_NEXT)) {
                        iocc = 0;
                        if (record == -1)
                            m_dicttree.SetItemText(htempItem, GetDocument()->GetNameat(level, COMMON, item, 0));
                        else
                            m_dicttree.SetItemText(htempItem, GetDocument()->GetNameat(level, record, item, 0));


                        for (HTREEITEM htempOcc = m_dicttree.GetChildItem(htempItem); htempOcc != NULL;
                            htempOcc = m_dicttree.GetNextItem(htempOcc, TVGN_NEXT))
                        {
                            CString csoccLabel;
                            csoccLabel.Format(_T("(%d)"), iocc + 1);
                            if (record == -1)
                                m_dicttree.SetItemText(htempOcc, GetDocument()->GetNameat(level, COMMON, item, 0) + csoccLabel);
                            else
                                m_dicttree.SetItemText(htempOcc, GetDocument()->GetNameat(level, record, item, 0) + csoccLabel);
                            //if (htempOcc == hItem) return GetDocument()->GetNameat(level, record-1,item,0);
                            iocc++;
                        }
                        item++;
                    }
                    record++;
                }
                level++;
            }
            else {
                bool bFirst = true;
                for (HTREEITEM htempPart = m_dicttree.GetChildItem(htemplevel); htempPart != NULL;
                    htempPart = m_dicttree.GetNextItem(htempPart, TVGN_NEXT)) {
                    m_dicttree.GetItemImage(htempPart, iImage, iSelImage);
                    CString sPart;
                    if (bFirst) {
                        sPart = WS2CS(GetDocument()->m_pDataDict->GetRelation(rel).GetPrimaryName());
                        part = 0;
                        bFirst = false;
                    }
                    else {
                        sPart = WS2CS(GetDocument()->m_pDataDict->GetRelation(rel).GetRelationPart(part).GetSecondaryName());
                        part++;
                    }
                    if (iImage == 2) {
                        GetDocument()->m_pDataDict->LookupName(sPart, &iLevel, &iRecord, &iItem, &iVSet);
                        m_dicttree.SetItemText(htempPart, GetDocument()->m_pDataDict->GetLevel(iLevel).GetRecord(iRecord)->GetName());
                        item = 0;
                        for (HTREEITEM htempItem = m_dicttree.GetChildItem(htempPart); htempItem != NULL;
                            htempItem = m_dicttree.GetNextItem(htempItem, TVGN_NEXT)) {
                            iocc = 0;
                            m_dicttree.SetItemText(htempItem, GetDocument()->m_pDataDict->GetLevel(iLevel).GetRecord(iRecord)->GetItem(item)->GetName());
                            for (HTREEITEM htempOcc = m_dicttree.GetChildItem(htempItem); htempOcc != NULL;
                                htempOcc = m_dicttree.GetNextItem(htempOcc, TVGN_NEXT))
                            {
                                CString csoccLabel;
                                csoccLabel.Format(_T("(%d)"), iocc + 1);
                                m_dicttree.SetItemText(htempOcc, GetDocument()->m_pDataDict->GetLevel(iLevel).GetRecord(iRecord)->GetItem(item)->GetName() + csoccLabel);
                                iocc++;
                            }
                            item++;
                        }
                    }
                    else {
                        GetDocument()->m_pDataDict->LookupName(sPart, &iLevel, &iRecord, &iItem, &iVSet);
                        const CDictItem* pItem = GetDocument()->m_pDataDict->GetLevel(iLevel).GetRecord(iRecord)->GetItem(iItem);
                        m_dicttree.SetItemText(htempPart, pItem->GetName());
                        if (pItem->GetItemType() == ItemType::Item) {
                            for (int i = iItem + 1; i < GetDocument()->m_pDataDict->GetLevel(iLevel).GetRecord(iRecord)->GetNumItems(); i++) {
                                pItem = GetDocument()->m_pDataDict->GetLevel(iLevel).GetRecord(iRecord)->GetItem(i);
                                if (pItem->GetItemType() == ItemType::Item) {
                                    break;
                                }
                                htempPart = m_dicttree.GetNextItem(htempPart, TVGN_NEXT);
                                CIMSAString sBMD = pItem->GetName();
                                m_dicttree.SetItemText(htempPart, pItem->GetName());
                            }
                        }
                    }
                }
                rel++;
            }
        }
    }
    else
    {
        HTREEITEM htempRoot = m_dicttree.GetRootItem();
        int level = 0, record = 0, item = 0, iocc = 0, rel = 0, part = 0;
        if (htempRoot == NULL)  return;
        m_dicttree.SetItemText(htempRoot, GetDocument()->m_pDataDict->GetLabel());
        for (HTREEITEM htemplevel = m_dicttree.GetChildItem(htempRoot); htemplevel != NULL;
            htemplevel = m_dicttree.GetNextItem(htemplevel, TVGN_NEXT))
        {
            m_dicttree.GetItemImage(htemplevel, iImage, iSelImage);
            if (iImage == 1) {
                m_dicttree.SetItemText(htemplevel, GetDocument()->m_pDataDict->GetLevel(level).GetLabel());
                record = -1;
                for (HTREEITEM htempRecord = m_dicttree.GetChildItem(htemplevel); htempRecord != NULL;
                    htempRecord = m_dicttree.GetNextItem(htempRecord, TVGN_NEXT))
                {
                    item = 0;
                    if (record == -1)
                        m_dicttree.SetItemText(htempRecord, GetDocument()->m_pDataDict->GetLevel(level).GetIdItemsRec()->GetLabel());
                    else
                        m_dicttree.SetItemText(htempRecord, GetDocument()->m_pDataDict->GetLevel(level).GetRecord(record)->GetLabel());
                    for (HTREEITEM htempItem = m_dicttree.GetChildItem(htempRecord); htempItem != NULL;
                        htempItem = m_dicttree.GetNextItem(htempItem, TVGN_NEXT))
                    {
                        iocc = 0;
                        if (record == -1)
                            m_dicttree.SetItemText(htempItem, GetDocument()->m_pDataDict->GetLevel(level).GetIdItemsRec()->GetItem(item)->GetLabel());
                        else
                            m_dicttree.SetItemText(htempItem, GetDocument()->m_pDataDict->GetLevel(level).GetRecord(record)->GetItem(item)->GetLabel());


                        for (HTREEITEM htempOcc = m_dicttree.GetChildItem(htempItem); htempOcc != NULL;
                            htempOcc = m_dicttree.GetNextItem(htempOcc, TVGN_NEXT))
                        {
                            // GHM 20140226 rewritten slightly to properly support occurrence labels
                            const CDictItem* pItem = GetDocument()->GetDataDict()->GetLevel(level).GetRecord(record == -1 ? COMMON : record)->GetItem(item);
                            CString csItemLabel;

                            if (pItem->GetOccurs() == 1 || pItem->GetOccurrenceLabels().GetLabel(iocc).IsEmpty())
                                csItemLabel.Format(_T("%s(%d)"), (LPCTSTR)pItem->GetLabel(), iocc + 1);

                            else
                                csItemLabel = pItem->GetOccurrenceLabels().GetLabel(iocc);

                            m_dicttree.SetItemText(htempOcc, csItemLabel);

                            //if (htempOcc == hItem) return GetDocument()->GetNameat(level, record-1,item,0);
                            iocc++;
                        }
                        item++;
                    }
                    record++;
                }
                level++;
            }
            else {
                bool bFirst = true;
                for (HTREEITEM htempPart = m_dicttree.GetChildItem(htemplevel); htempPart != NULL;
                    htempPart = m_dicttree.GetNextItem(htempPart, TVGN_NEXT)) {
                    m_dicttree.GetItemImage(htempPart, iImage, iSelImage);
                    CString sPart;
                    if (bFirst) {
                        sPart = WS2CS(GetDocument()->m_pDataDict->GetRelation(rel).GetPrimaryName());
                        part = 0;
                        bFirst = false;
                    }
                    else {
                        sPart = WS2CS(GetDocument()->m_pDataDict->GetRelation(rel).GetRelationPart(part).GetSecondaryName());
                        part++;
                    }
                    if (iImage == 2) {
                        GetDocument()->m_pDataDict->LookupName(sPart, &iLevel, &iRecord, &iItem, &iVSet);
                        m_dicttree.SetItemText(htempPart, GetDocument()->m_pDataDict->GetLevel(iLevel).GetRecord(iRecord)->GetLabel());
                        item = 0;
                        for (HTREEITEM htempItem = m_dicttree.GetChildItem(htempPart); htempItem != NULL;
                            htempItem = m_dicttree.GetNextItem(htempItem, TVGN_NEXT)) {
                            iocc = 0;
                            m_dicttree.SetItemText(htempItem, GetDocument()->m_pDataDict->GetLevel(iLevel).GetRecord(iRecord)->GetItem(item)->GetLabel());
                            for (HTREEITEM htempOcc = m_dicttree.GetChildItem(htempItem); htempOcc != NULL;
                                htempOcc = m_dicttree.GetNextItem(htempOcc, TVGN_NEXT))
                            {
                                CString csoccLabel;
                                csoccLabel.Format(_T("(%d)"), iocc + 1);
                                m_dicttree.SetItemText(htempOcc, GetDocument()->m_pDataDict->GetLevel(iLevel).GetRecord(iRecord)->GetItem(item)->GetLabel() + csoccLabel);
                                iocc++;
                            }
                            item++;
                        }
                    }
                    else {
                        GetDocument()->m_pDataDict->LookupName(sPart, &iLevel, &iRecord, &iItem, &iVSet);
                        const CDictItem* pItem = GetDocument()->m_pDataDict->GetLevel(iLevel).GetRecord(iRecord)->GetItem(iItem);
                        m_dicttree.SetItemText(htempPart, pItem->GetLabel());
                        if (pItem->GetItemType() == ItemType::Item) {
                            for (int i = iItem + 1; i < GetDocument()->m_pDataDict->GetLevel(iLevel).GetRecord(iRecord)->GetNumItems(); i++) {
                                pItem = GetDocument()->m_pDataDict->GetLevel(iLevel).GetRecord(iRecord)->GetItem(i);
                                if (pItem->GetItemType() == ItemType::Item) {
                                    break;
                                }
                                htempPart = m_dicttree.GetNextItem(htempPart, TVGN_NEXT);
                                CIMSAString sBMD = pItem->GetName();
                                m_dicttree.SetItemText(htempPart, pItem->GetLabel());
                            }
                        }
                    }
                }
                rel++;
            }
        }
    }
}


void CExportView::OnToggle()
{
    SharedSettings::ToggleViewNamesInTree();
    RefreshView();
}


void CExportView::OnUpdateToggle(CCmdUI* pCmdUI)
{
    pCmdUI->SetCheck(SharedSettings::ViewNamesInTree());
}


int CExportView::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CFormView::OnCreate(lpCreateStruct) == -1)
        return -1;

    if(!m_pExportSplitterFrame){
        m_pExportSplitterFrame = new CExportSplitterFrame();

        CExportDoc* pDoc = GetDocument();
        m_pExportSplitterFrame->SetDoc( pDoc );

        CRect rc;
        GetWindowRect(rc);
        BOOL bCreateOk = m_pExportSplitterFrame->Create( NULL, _T(""), WS_CHILD | WS_MAXIMIZE | WS_VISIBLE | FWS_ADDTOTITLE, rc , this );
        ASSERT( bCreateOk );
        if( bCreateOk ){
            m_pExportSplitterFrame->GetLeftView()->SetCtrl( &m_dicttree );

            pDoc->SetTreeView   ( this );
            pDoc->SetOptionsView( m_pExportSplitterFrame->GetRightView() );
        }
    }

    return 0;
}



void CExportView::CheckRelationsOnlyInSepRecs()
{
    CExportDoc* pDoc= GetDocument();
    if( !pDoc->m_bAllInOneRecord )
        return;

    CArray<HTREEITEM,HTREEITEM> aCheckedItems;

    for( const auto& [hItem, dict_relation] :  m_aMapRel_Relation_by_hitem ) {
        CArray<HTREEITEM,HTREEITEM> aRelCheckedItems;
        if( GetChecked( hItem, true, true, aRelCheckedItems )>0 )
            aCheckedItems.Append( aRelCheckedItems );
    }
    if( aCheckedItems.GetSize()>0 ){
        AfxMessageBox( DICT_RELATIONS_WARNING );
        for(int i=0; i<aCheckedItems.GetSize(); i++)
            m_dicttree.SetCheck( aCheckedItems.ElementAt(i), FALSE );
    }
}

bool CExportView::IsFlatExport( const CDictRecord* pDictRecord, bool* pbHasAnyMultipleItem ){

    CExportDoc* pDoc        = GetDocument();
    if(!pDoc)
        return false;

    return pDoc->IsFlatExport(pDictRecord,pbHasAnyMultipleItem);
}

bool CExportView::IsSelectedAnyMultiple(bool bIncludeRelations, bool bIgnoreChildsOfSingleRecords ){

    CArray<const CDictItem*,const CDictItem*> aSelItems;
    CMap<const CDictItem*,const CDictItem*,CArray<int,int>*,CArray<int,int>*>   aMapSelOccsByItem;

    bool bHasAnyMultiple = false;
    GetSelItems( bIncludeRelations, aSelItems, aMapSelOccsByItem, &bHasAnyMultiple, NULL, &bIgnoreChildsOfSingleRecords );



    const CDictItem* pItem;
    CArray<int,int>* pOccs;
    POSITION pos = aMapSelOccsByItem.GetStartPosition();
    while(pos){
        aMapSelOccsByItem.GetNextAssoc(pos,pItem,pOccs);
        if( pOccs )
            delete( pOccs );
    }

    return bHasAnyMultiple;
}

bool CExportView::IsSelectedAnySingle(bool bIncludeRelations){
    UNREFERENCED_PARAMETER(bIncludeRelations);

    CArray<const CDictItem*,const CDictItem*> aSelItems;
    CMap<const CDictItem*,const CDictItem*,CArray<int,int>*,CArray<int,int>*>   aMapSelOccsByItem;

    bool bHasAnySingle = false;
    GetSelItems( bIncludeRelations, aSelItems, aMapSelOccsByItem, NULL, &bHasAnySingle );

    const CDictItem* pItem;
    CArray<int,int>* pOccs;
    POSITION pos = aMapSelOccsByItem.GetStartPosition();
    while(pos){
        aMapSelOccsByItem.GetNextAssoc(pos,pItem,pOccs);
        if( pOccs )
            delete( pOccs );
    }

    return bHasAnySingle;
}

bool CExportView::IsSelectedAnyRelation(){

    CExportDoc* pDoc = GetDocument();
    if(!pDoc)
        return false;

    const CDataDict* pDataDict = pDoc->m_pDataDict.get();
    if(!pDataDict)
        return false;

    bool bAnyRelationSelected = false;
    int iNumDictRelations = pDataDict->GetNumRelations();
    for(int i=0; i<iNumDictRelations; i++){
        if( IsRelationSelected( pDataDict->GetRelation(i) ) ){
            bAnyRelationSelected = true;
        }
    }
    return bAnyRelationSelected;
}
