// FreqView.cpp : implementation of the CSFreqView class
//

#include "StdAfx.h"
#include "FreqView.h"
#include <zInterfaceF/resource_shared.h>


IMPLEMENT_DYNCREATE(CSFreqView, CFormView)

BEGIN_MESSAGE_MAP(CSFreqView, CFormView)
    ON_WM_CREATE()
    ON_WM_SIZE()
    ON_NOTIFY(NM_CLICK, IDC_DATADICT_TREE, OnClickDatadictTree)
    ON_NOTIFY(TVN_KEYDOWN, IDC_DATADICT_TREE, OnKeydownDatadictTree)
END_MESSAGE_MAP()


CSFreqView::CSFreqView()
    :   CFormView(CSFreqView::IDD)
{
}


BOOL CSFreqView::Create(LPCTSTR lpszClassName, LPCTSTR lpszWindowName, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, CCreateContext* pContext)
{
    // TODO: Add your specialized code here and/or call the base class
    dwStyle &=~WS_HSCROLL;
    dwStyle &=~WS_VSCROLL;
    return CFormView::Create(lpszClassName, lpszWindowName, dwStyle, rect, pParentWnd, nID, pContext);
}


int CSFreqView::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    lpCreateStruct->style &= ~( WS_HSCROLL | WS_VSCROLL );

    return CFormView::OnCreate(lpCreateStruct);
}


// Resizing of the views
void CSFreqView::OnSize(UINT nType, int cx, int cy)
{
    CFormView::OnSize(nType, cx, cy);

    if( m_dicttree.m_hWnd != NULL )
    {
        CRect rc;
        GetClientRect(&rc);
        m_dicttree.MoveWindow(0, 0, rc.Width(), rc.Height(), TRUE);
    }
}


void CSFreqView::DoDataExchange(CDataExchange* pDX)
{
    CFormView::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_DATADICT_TREE, m_dicttree);
}


//  Initializing of all the form
void CSFreqView::OnInitialUpdate()
{
    CFormView::OnInitialUpdate();

    ASSERT (GetDocument() != NULL);
    ASSERT (GetDocument()->GetDataDict() != NULL);

    if( m_ilState.m_hImageList == NULL )
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
        m_ilNormal.Add(AfxGetApp()->LoadIcon(IDI_ITEMUNAVAIL));
        m_ilNormal.Add(AfxGetApp()->LoadIcon(IDI_DICTIONARY_SUBITEM));
        m_dicttree.SetImageList(&m_ilNormal, TVSIL_NORMAL);    // attach the state  image list
    }

    m_dicttree.SetParent(this);

    CImageList* pmyImageList, *pstate;
    pmyImageList = m_dicttree.GetImageList(TVSIL_NORMAL);
    pstate = m_dicttree.GetImageList(TVSIL_STATE);

    // make sure that parent frame is large enough to completely show the form view
    SetScrollSizes(MM_TEXT,CSize(0,0));
    GetParentFrame()->RecalcLayout();
    UpdateWindow();
    InitializeView();
}


// Add all record and its items in the tree structure.
bool CSFreqView::AddRecordIntree(const CDictRecord* pRecord, HTREEITEM htreeLabel)
{
    TVINSERTSTRUCT tvistrRecord,tvistrItem,tvistrVSet;
    HTREEITEM htreeRecord,hItem = NULL,htreeVSet,hParentItem;
    tvistrRecord.item.mask = TVIF_PARAM | TVIF_TEXT;
    tvistrItem.item.mask = TVIF_PARAM | TVIF_TEXT;
    tvistrVSet.item.mask = TVIF_PARAM | TVIF_TEXT;

    CString cs = SharedSettings::ViewNamesInTree () ? pRecord->GetName() : pRecord->GetLabel();
    tvistrRecord.item.pszText = cs.GetBuffer(0);
    tvistrRecord.item.cchTextMax = cs.GetLength();
    tvistrRecord.item.lParam = (LPARAM) -1;//s.GetId();
    tvistrRecord.hParent = htreeLabel;//TVI_ROOT;
    tvistrRecord.hInsertAfter = 0;
    tvistrRecord.item.state = TVIS_EXPANDED;
    htreeRecord = m_dicttree.InsertItem(&tvistrRecord);

    if (pRecord->GetSonNumber() == COMMON)
    {
        m_dicttree.SetItemImage( htreeRecord, 3, 3);
    }
    else
    {
        m_dicttree.SetItemImage( htreeRecord, 2, 2);
    }

    for (int i = 0 ; i < pRecord->GetNumItems() ; i++)
    {
        const CDictItem* pItem = pRecord->GetItem(i);

        if( !pItem->AddToTreeFor80() )
            continue;

        int totocc = pItem->GetOccurs();
        if (pItem->GetParentItem() != NULL)
            totocc = pItem->GetParentItem()->GetOccurs()*totocc;

        CString csItem = SharedSettings::ViewNamesInTree () ? pItem->GetName() : pItem->GetLabel();
        int occurrence = -1;
        if (totocc > 1)
            occurrence = 0;

        tvistrItem.item.pszText = csItem.GetBuffer(0);
        tvistrItem.item.cchTextMax = csItem.GetLength();
        tvistrItem.hParent = htreeRecord;
        tvistrItem.hInsertAfter = 0;

        if (pItem->HasValueSets())
        {
            tvistrItem.item.lParam = GetDocument()->GetPositionInList(pItem->GetValueSet(0).GetName(),occurrence);
        }
        else
        {
            tvistrItem.item.lParam = GetDocument()->GetPositionInList(pItem->GetName(),occurrence);
        }

        hParentItem = m_dicttree.InsertItem(&tvistrItem);
        m_dicttree.SetCheck(hParentItem,GetDocument()->IsChecked(tvistrItem.item.lParam));

        if (pItem->GetContentType() == ContentType::Alpha && !pItem->HasValueSets())
        {
            m_dicttree.SetItemImage( hParentItem, 6, 6);
        }
        else
        {
            if (pItem->GetParentItem() == NULL)
                m_dicttree.SetItemImage( hParentItem, 4, 4);
            else
                m_dicttree.SetItemImage( hParentItem, 7, 7);
        }
        SetParentStates(hParentItem); // Set all the parents

        if (pItem->GetNumValueSets() > 1)
        {
            for( const auto& dict_value_set : pItem->GetValueSets() )
            {
                csItem = SharedSettings::ViewNamesInTree () ? dict_value_set.GetName() : dict_value_set.GetLabel();
                tvistrVSet.item.pszText = csItem.GetBuffer(0);
                tvistrVSet.item.cchTextMax = csItem.GetLength();
                int vSetoccurrence = -1;
                if (totocc > 1)
                    vSetoccurrence = 0;

                tvistrVSet.item.lParam = GetDocument()->GetPositionInList(dict_value_set.GetName(),vSetoccurrence,true); // GHM 20111228 true (reverse search) added due to issues with selections on items that have multiple value sets
//              tvistrVSet.item.lParam = (LPARAM) -2;
                tvistrVSet.hParent = hParentItem;
                tvistrVSet.hInsertAfter = 0;
                htreeVSet = m_dicttree.InsertItem(&tvistrVSet);
                m_dicttree.SetCheck(htreeVSet,GetDocument()->IsChecked(tvistrVSet.item.lParam));
                m_dicttree.SetItemImage( htreeVSet, 5, 5);
                SetParentStates(htreeVSet);
            }
        }

        //totocc = 1;
        if ( totocc > 1)
        {
            BOOL bParentCheck = m_dicttree.GetCheck(hParentItem);
            for(int occ = 0; occ < totocc;occ++)
            {
                CString csTree = SharedSettings::ViewNamesInTree () ? pItem->GetName() : pItem->GetLabel();
                CString csoccLabel;
                csoccLabel.Format(_T("(%d)"),occ+1);
                int itemOccurence = 0;
                if (totocc > 1)
                {
                    itemOccurence = occ+1;
                    if( pItem->GetOccurs() == 1 || pItem->GetOccurrenceLabels().GetLabel(occ).IsEmpty() )
                        csTree += csoccLabel;
                    else
                        csTree = pItem->GetOccurrenceLabels().GetLabel(occ);
                }
                tvistrItem.item.pszText = csTree.GetBuffer(0);
                tvistrItem.item.cchTextMax = csTree.GetLength();
                tvistrItem.hParent = hParentItem;
                tvistrItem.hInsertAfter = 0;

                if (pItem->HasValueSets())
                {
                    tvistrItem.item.lParam = GetDocument()->GetPositionInList(pItem->GetValueSet(0).GetName(),itemOccurence);
                }
                else
                {
                    tvistrItem.item.lParam = GetDocument()->GetPositionInList(pItem->GetName(),itemOccurence);
                }

                hItem = m_dicttree.InsertItem(&tvistrItem);
                m_dicttree.SetCheck(hItem,GetDocument()->IsChecked(tvistrItem.item.lParam));

                if (pItem->GetContentType() == ContentType::Alpha && !pItem->HasValueSets())
                {
                    m_dicttree.SetItemImage( hItem, 6, 6);
                }
                else
                {
                    if (pItem->GetParentItem() == NULL)
                        m_dicttree.SetItemImage( hItem, 4, 4);
                    else
                        m_dicttree.SetItemImage( hItem, 7, 7);
                }

                if (pItem->GetNumValueSets() > 1)
                {
                    for( const auto& dict_value_set : pItem->GetValueSets() )
                    {
                        csTree = SharedSettings::ViewNamesInTree () ? dict_value_set.GetName() : dict_value_set.GetLabel();
                        tvistrVSet.item.pszText = csTree.GetBuffer(0);
                        tvistrVSet.item.cchTextMax = csTree.GetLength();
                        int vSetoccurence = 0;
                        if (totocc > 1)
                        {
                            vSetoccurence = occ+1;
                        }
                        //tvistrVSet.item.lParam = (LPARAM) -2;
                        tvistrVSet.hParent = hItem;
                        tvistrVSet.hInsertAfter = 0;
                        tvistrVSet.item.lParam = GetDocument()->GetPositionInList(dict_value_set.GetName(),vSetoccurence,true);  // GHM 20111228 true (reverse search) added due to issues with selections on items that have multiple value sets
                        htreeVSet = m_dicttree.InsertItem(&tvistrVSet);
                        m_dicttree.SetCheck(htreeVSet,GetDocument()->IsChecked(tvistrVSet.item.lParam));
                        m_dicttree.SetItemImage( htreeVSet, 5, 5);
                    }
                }
            }
            SetParentStates(hItem);
            m_dicttree.SetCheck(hParentItem,bParentCheck);

        }
    }
    return true;
}


// InitializeView of the form
void CSFreqView::InitializeView()
{
    CWaitCursor wait;

    const CDataDict* pDataDict = GetDocument()->GetDataDict();
    m_dicttree.DeleteAllItems ();

    if (pDataDict->GetNumLevels() == 0)
        return;

    TVINSERTSTRUCT tvistrLabel,tvistrRoot;
    HTREEITEM htreeLabel,htreeRoot;
    tvistrLabel.item.mask = TVIF_PARAM | TVIF_TEXT;
    tvistrRoot.item.mask = TVIF_PARAM | TVIF_TEXT;
    CString cs;
    cs = SharedSettings::ViewNamesInTree () ? pDataDict->GetName() : pDataDict->GetLabel();
    tvistrRoot.item.pszText = cs.GetBuffer(0);
    tvistrRoot.item.cchTextMax = cs.GetLength();
    tvistrRoot.item.lParam = (LPARAM) -1;//s.GetId();
    tvistrRoot.hParent = TVI_ROOT;
    tvistrRoot.hInsertAfter = 0;
    tvistrRoot.item.state = TVIS_EXPANDED;
    htreeRoot = m_dicttree.InsertItem(&tvistrRoot);
    m_dicttree.SetItemImage( htreeRoot, 0, 0);

    for( const DictLevel& dict_level : pDataDict->GetLevels() )
    {
        CString csLevel = SharedSettings::ViewNamesInTree () ? dict_level.GetName() : dict_level.GetLabel();
        tvistrLabel.item.pszText = csLevel.GetBuffer(0);
        tvistrLabel.item.cchTextMax = csLevel.GetLength();
        tvistrLabel.item.lParam = (LPARAM) -1;//s.GetId();
        tvistrLabel.hParent = htreeRoot;
        tvistrLabel.hInsertAfter = 0;
        tvistrLabel.item.state = TVIS_EXPANDED;
        htreeLabel = m_dicttree.InsertItem(&tvistrLabel);
        m_dicttree.SetItemImage( htreeLabel, 1, 1);
        AddRecordIntree(dict_level.GetIdItemsRec(),htreeLabel);

        for (int j = 0 ; j < dict_level.GetNumRecords() ; j++) {
            AddRecordIntree(dict_level.GetRecord(j),htreeLabel);
        }
        m_dicttree.Expand(htreeLabel,TVE_EXPAND);
    }
    m_dicttree.Expand(htreeRoot,TVE_EXPAND);
    m_dicttree.SetFocus();

    ((CFrameWnd*)AfxGetMainWnd())->SetActiveView(this);
}


// Clicking of the treeitem
void CSFreqView::TreeItemClicked(HTREEITEM hItem,int recursive )
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
    int occ = 0;
    bool flagcont = true;

    if (recursive != 2)
    {
        int item = m_dicttree.GetItemData(hItem);
        if (item > 0)
        {
            occ = GetDocument()->GetItemOcc (item);
            if (occ == 0) {
                // clicked on item with occurences (not vset)
                if (recursive == 1) {
                    // didn't click directly on this, recursing from above
                    flagcont = true;
                }
                else {
                    // check to see if user wants
                    // to continue recursion to children
                    m_dicttree.Expand(hItem, TVE_EXPAND);
                    if (AfxMessageBox(_T("Apply to all individual occurrences?"), MB_YESNO) == IDYES) {
                        flagcont = true;
                    }
                    else {
                        flagcont = false;
                    }
                }
            }
        }
        if (flagcont && m_dicttree.ItemHasChildren(hItem))  {
            // If the item has children set check for all the childrens



            ASSERT(m_dicttree.ItemHasChildren(hItem));
            HTREEITEM hTblTitle = m_dicttree.GetChildItem(hItem);
            while (hTblTitle != NULL)  {
                // This expands the node when clicked.. bruce requested to remove it
//              if (m_dicttree.GetItemData(hItem) == -1 && recursive == 0)
//              {
//                  m_dicttree.Expand( hItem,TVE_EXPAND );  //(hItem)->
//              }
                if (m_dicttree.GetItemData(hItem) == 0 && recursive == 0)
                {
                    m_dicttree.Expand( hItem,TVE_EXPAND );  //(hItem)->
                }
                m_dicttree.SetCheck(hTblTitle, m_dicttree.GetCheck(hItem));
                //CString cs = m_dicttree.GetItemText(hTblTitle);
                TreeItemClicked(hTblTitle, 1);
                hTblTitle = m_dicttree.GetNextItem(hTblTitle, TVGN_NEXT);
            }
        }
    }
    int item = m_dicttree.GetItemData(hItem);
    if (item < 0)
    {
        if (recursive > 0) return;
        SetParentStates(hItem);
        return;
    }

    occ = GetDocument()->GetItemOcc (item);
/*  if (occ == -1)
    {
        AfxMessageBox("Test");
    }
*/  if (occ == -2)
    {
        occ = m_dicttree.GetItemData(m_dicttree.GetParentItem(hItem));
    }
    int img=0,selimg=0;
    m_dicttree.GetItemImage(hItem,img,selimg);
    /*if (img == 6) allow alpha numeric in the new system
    {
        if (recursive == 0 && m_dicttree.GetCheck(hItem))
        {
            m_dicttree.SetCheck(hItem, FALSE);
            AfxMessageBox("Alphanumeric items must have value sets to be tabulated",MB_ICONINFORMATION);

        }
        m_dicttree.SetCheck(hItem, FALSE);
        return;
    }*/
//  if (occ!=0)
    {
        GetDocument()->SetItemCheck(item,(bool)m_dicttree.GetCheck(hItem));
    }
    //GetDocument()->AddItemtoList(GetDetails(hItem), (bool)m_dicttree.GetCheck(hItem),occ);
    if (recursive > 0) return;
    /*
    if (occ > 0)
    {
        HTREEITEM hParent = m_dicttree.GetParentItem(hItem);
        int checked = (m_dicttree.GetItemState(hParent, TVIS_STATEIMAGEMASK) >> 12) - 1;
        if (checked ==0)
        {
            m_dicttree.SetItemState(hParent,(int)3<<12,TVIS_STATEIMAGEMASK);
            SetParentStates(hParent);
            m_dicttree.SetCheck(m_dicttree.GetParentItem(hItem),0);
            return;
        }
        SetParentStates(hParent);
        return;
    }
    */
    if (occ > 0)
    {
        SetParentStates(m_dicttree.GetParentItem(hItem));
    }
    else
        SetParentStates(hItem);
}


// Click Data dictionaries.
void CSFreqView::OnClickDatadictTree(NMHDR* pNMHDR, LRESULT* pResult)
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


// When the user checks the tree check box.
LRESULT CSFreqView::OnTvCheckbox(WPARAM, LPARAM lp)
{
    HTREEITEM hitem = (HTREEITEM) lp;
    int checked = (m_dicttree.GetItemState(hitem, TVIS_STATEIMAGEMASK) >> 12) - 1;
    if (checked == 2 ) {
        m_dicttree.SetCheck(hitem,FALSE);
    }
    TreeItemClicked(hitem);

    return 0;
}


// Get the name of the item clicked.
CString CSFreqView::GetDetails(HTREEITEM hItem)
{
    ASSERT(hItem != NULL);

    HTREEITEM htempRoot = m_dicttree.GetRootItem();
    int level = 0, record = 0, item = 0, vset = 0, iocc = 0;
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
                vset = 0;
            //  int occ = 0;
            //  occ = m_dicttree.GetItemData(hItem);
                if (htempItem == hItem)
                {
                    if (m_dicttree.GetChildItem(htempItem)==NULL)
                        return GetDocument()->GetNameat(level, record-1,item, vset,iocc);
                    else
                    {
                        HTREEITEM htemp = m_dicttree.GetChildItem(htempItem);
                        int itemdata = m_dicttree.GetItemData( htemp);
                        if (itemdata >= 0)
                            return GetDocument()->GetNameat(level, record-1,item, vset,iocc);
                        else
                            return GetDocument()->GetNameat(level, record-1,item, vset,iocc);
                    }
                }
                for (HTREEITEM htempVSetOcc = m_dicttree.GetChildItem(htempItem); htempVSetOcc != NULL;
                htempVSetOcc = m_dicttree.GetNextItem(htempVSetOcc,TVGN_NEXT))
                {
                    if (htempVSetOcc == hItem)
                    {
                        if (m_dicttree.GetItemData(htempVSetOcc) == -2)
                            return GetDocument()->GetNameat(level, record-1,item, vset,iocc);
                        else
                            if (m_dicttree.GetChildItem(htempVSetOcc)==NULL)
                                return GetDocument()->GetNameat(level, record-1,item, vset,iocc);
                            return _T("");
                    }
                    if (m_dicttree.GetItemData(htempVSetOcc) == -2)
                        vset++;
                    else
                    {
                        int vseto=0;
                        for (HTREEITEM htempVSets = m_dicttree.GetChildItem(htempVSetOcc); htempVSets != NULL;
                        htempVSets = m_dicttree.GetNextItem(htempVSets,TVGN_NEXT))
                        {
                            if (htempVSets == hItem)
                            {
                                //if (m_dicttree.GetItemData(htempVSetOcc) == -2)
                                return GetDocument()->GetNameat(level, record-1,item, vseto,iocc);
                                //else
                                //  if (m_dicttree.GetChildItem(htempItem)==NULL)
                                //      return GetDocument()->GetNameat(level, record-1,item, vset,iocc);
                                //  return "";
                            }
                            vseto++;
                        }
                        iocc++;
                    }
                }
                item++;
            }
            record++;
        }
        level++;
    }
    return _T("");
}


// Set all the parents of setting the parents of checked.
void CSFreqView::SetParentStates(HTREEITEM hItem)
{
    HTREEITEM hParent = m_dicttree.GetParentItem(hItem);
    if (hParent == NULL) return;

    UINT bparentstate = 1;
    BOOL flagfalse = FALSE;
    bool bFirstRepeatingitem = true;
    hItem = m_dicttree.GetChildItem(hParent);  // first kid
    ASSERT(hItem!=NULL);
    while (hItem != NULL)  {
        int img=0,selimg=0;
        m_dicttree.GetItemImage(hItem,img,selimg);
        /*if (img == 6) item unavailable for item with occs  bug --this image is used for item with occs let it go through
        {
            hItem = m_dicttree.GetNextSiblingItem(hItem);
            continue;
        }*/

        int item = m_dicttree.GetItemData(hItem);
        int occ = GetDocument()->GetItemOcc(item);
        if (occ == 0)
        {
            /*if(m_dicttree.GetCheck(m_dicttree.GetParentItem(hItem)) && bparentstate == 1){//see if the item itself is checked before u see its occs
                bparentstate =2 ;
            }*/
            HTREEITEM hSubItem = m_dicttree.GetChildItem(hItem);  // first kid
            while(hSubItem!=NULL)
            {
                switch (m_dicttree.GetCheck(hSubItem))
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
                hSubItem = m_dicttree.GetNextSiblingItem(hSubItem );
            }
        }


        if (occ <= 0)
        {
            if (bparentstate == 3) break;
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
        }

        else // GHM 20120516 the state of repeating items was being ignored and then, even when the values were all checked, was leading to partially-selected checks on parent nodes
        {
            int checkState = m_dicttree.GetCheck(hItem);

            if( bFirstRepeatingitem )
            {
                bFirstRepeatingitem = false;
                bparentstate = checkState + 1;
            }

            else if( checkState == 0 )
                bparentstate = 1;
        }

        hItem = m_dicttree.GetNextSiblingItem(hItem);
    }
    if (bparentstate == 2 && flagfalse) bparentstate = 3;
    m_dicttree.SetItemState(hParent,bparentstate<<12,TVIS_STATEIMAGEMASK);
    SetParentStates(hParent);
}


void CSFreqView::OnKeydownDatadictTree(NMHDR* pNMHDR, LRESULT* pResult)
{
  TV_KEYDOWN* pTVKeyDown = (TV_KEYDOWN*)pNMHDR;

  HTREEITEM it = m_dicttree.GetSelectedItem();
    //TreeItemClicked(hitem);
  //VK_END
  if (it != NULL ) {
      //short keystate = GetAsyncKeyState(VK_SPACE);
      //if (keystate != 0 )
      if( pTVKeyDown->wVKey == VK_SPACE ) // GHM 20120516 changed
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
        // the check box was hit.
        OnTvCheckbox(pNMHDR->idFrom, (LPARAM) it);
      }
  }
  *pResult = 0;
}


void CSFreqView::OnActivateView(BOOL bActivate, CView* pActivateView, CView* pDeactiveView)
{
    if (bActivate)
    {
        CString csDictFname = GetDocument()->GetDictFileName();
        if (!csDictFname.IsEmpty())
        {
            CFileStatus fStatus;
            bool flg = CFile::GetStatus(csDictFname,fStatus);

            if( !flg ) // they deleted or moved the dictionary
            {
                GetDocument()->CloseUponFileNonExistance();
                InitializeView();

                AfxMessageBox(FormatText(_T("The dictionary file %s no longer exists"), (LPCTSTR)csDictFname));
            }

            else if( fStatus.m_mtime != GetDocument()->m_tDCFTime )
            {
                GetDocument()->OpenDictFile(true);
                GetDocument()->CheckValueSetChanges();
                InitializeView();
            }
        }
    }

    CFormView::OnActivateView(bActivate, pActivateView, pDeactiveView);
}


void CSFreqView::RefreshTree()
{
    int iImage;
    int iSelImage;

    if (SharedSettings::ViewNamesInTree())
    {
        HTREEITEM htempRoot = m_dicttree.GetRootItem();
        int level = 0, record = 0, item = 0, iocc = 0, iVS = 0;
        if (htempRoot == NULL)  return ;
        m_dicttree.SetItemText(htempRoot,GetDocument()->GetDataDict()->GetName());
        for (HTREEITEM htemplevel = m_dicttree.GetChildItem(htempRoot); htemplevel != NULL;
        htemplevel = m_dicttree.GetNextItem(htemplevel,TVGN_NEXT))
        {
            m_dicttree.SetItemText(htemplevel,GetDocument()->GetDataDict()->GetLevel(level).GetName());
            record = -1;
            for (HTREEITEM htempRecord = m_dicttree.GetChildItem(htemplevel); htempRecord != NULL;
            htempRecord = m_dicttree.GetNextItem(htempRecord,TVGN_NEXT))
            {
                item = 0;
                if (record == -1)
                    m_dicttree.SetItemText(htempRecord,GetDocument()->GetDataDict()->GetLevel(level).GetIdItemsRec()->GetName());
                else
                    m_dicttree.SetItemText(htempRecord,GetDocument()->GetDataDict()->GetLevel(level).GetRecord(record)->GetName());
                for (HTREEITEM htempItem = m_dicttree.GetChildItem(htempRecord); htempItem != NULL;
                        htempItem = m_dicttree.GetNextItem(htempItem,TVGN_NEXT)) {
                    iocc = 0;
                    iVS = 0;
                    if (record == -1)
                        m_dicttree.SetItemText(htempItem,GetDocument()->GetDataDict()->GetLevel(level).GetIdItemsRec()->GetItem(item)->GetName());
                    else
                        m_dicttree.SetItemText(htempItem,GetDocument()->GetDataDict()->GetLevel(level).GetRecord(record)->GetItem(item)->GetName());

                    for (HTREEITEM htempOcc = m_dicttree.GetChildItem(htempItem); htempOcc != NULL;
                            htempOcc = m_dicttree.GetNextItem(htempOcc,TVGN_NEXT)) {
                        m_dicttree.GetItemImage(htempOcc,iImage,iSelImage);
                        if (iImage == 5) {
                            if (record == -1)
                                m_dicttree.SetItemText(htempOcc,GetDocument()->GetDataDict()->GetLevel(level).GetIdItemsRec()->GetItem(item)->GetValueSet(iVS).GetName());
                            else {
                                m_dicttree.SetItemText(htempOcc,GetDocument()->GetDataDict()->GetLevel(level).GetRecord(record)->GetItem(item)->GetValueSet(iVS).GetName());
                            }
                            iVS++;
                        }
                        else {
                            CString csoccLabel;
                            csoccLabel.Format(_T("(%d)"),iocc+1);
                            if (record == -1)
                                m_dicttree.SetItemText(htempOcc,GetDocument()->GetDataDict()->GetLevel(level).GetIdItemsRec()->GetItem(item)->GetName()+csoccLabel);
                            else {
                                m_dicttree.SetItemText(htempOcc,GetDocument()->GetDataDict()->GetLevel(level).GetRecord(record)->GetItem(item)->GetName()+csoccLabel);
                            }
                            int nVS = 0;
                            for (HTREEITEM htempVS = m_dicttree.GetChildItem(htempOcc); htempVS != NULL;
                                    htempVS = m_dicttree.GetNextItem(htempVS,TVGN_NEXT)) {
                                if (record == -1)
                                    m_dicttree.SetItemText(htempVS,GetDocument()->GetDataDict()->GetLevel(level).GetIdItemsRec()->GetItem(item)->GetValueSet(nVS).GetName());
                                else {
                                    m_dicttree.SetItemText(htempVS,GetDocument()->GetDataDict()->GetLevel(level).GetRecord(record)->GetItem(item)->GetValueSet(nVS).GetName());
                                }
                                nVS++;
                            }
                            iocc++;
                        }
                    }
                    item++;
                }
                record++;
            }
            level++;
        }
    }
    else
    {
        HTREEITEM htempRoot = m_dicttree.GetRootItem();
        int level = 0, record = 0, item = 0, iocc = 0, iVS = 0;
        if (htempRoot == NULL)  return ;
        m_dicttree.SetItemText(htempRoot,GetDocument()->GetDataDict()->GetLabel());
        for (HTREEITEM htemplevel = m_dicttree.GetChildItem(htempRoot); htemplevel != NULL;
        htemplevel = m_dicttree.GetNextItem(htemplevel,TVGN_NEXT))
        {
            m_dicttree.SetItemText(htemplevel,GetDocument()->GetDataDict()->GetLevel(level).GetLabel());
            record = -1;
            for (HTREEITEM htempRecord = m_dicttree.GetChildItem(htemplevel); htempRecord != NULL;
            htempRecord = m_dicttree.GetNextItem(htempRecord,TVGN_NEXT))
            {
                item = 0;
                if (record == -1)
                    m_dicttree.SetItemText(htempRecord,GetDocument()->GetDataDict()->GetLevel(level).GetIdItemsRec()->GetLabel());
                else
                    m_dicttree.SetItemText(htempRecord,GetDocument()->GetDataDict()->GetLevel(level).GetRecord(record)->GetLabel());
                for (HTREEITEM htempItem = m_dicttree.GetChildItem(htempRecord); htempItem != NULL;
                htempItem = m_dicttree.GetNextItem(htempItem,TVGN_NEXT))
                {
                    iocc = 0;
                    iVS = 0;
                    if (record == -1)
                        m_dicttree.SetItemText(htempItem,GetDocument()->GetDataDict()->GetLevel(level).GetIdItemsRec()->GetItem(item)->GetLabel());
                    else
                        m_dicttree.SetItemText(htempItem,GetDocument()->GetDataDict()->GetLevel(level).GetRecord(record)->GetItem(item)->GetLabel());


                    for (HTREEITEM htempOcc = m_dicttree.GetChildItem(htempItem); htempOcc != NULL;
                            htempOcc = m_dicttree.GetNextItem(htempOcc,TVGN_NEXT)) {
                        m_dicttree.GetItemImage(htempOcc,iImage,iSelImage);
                        if (iImage == 5) {
                            if (record == -1)
                                m_dicttree.SetItemText(htempOcc,GetDocument()->GetDataDict()->GetLevel(level).GetIdItemsRec()->GetItem(item)->GetValueSet(iVS).GetLabel());
                            else
                                m_dicttree.SetItemText(htempOcc,GetDocument()->GetDataDict()->GetLevel(level).GetRecord(record)->GetItem(item)->GetValueSet(iVS).GetLabel());
                            iVS++;
                        }
                        else {
                            // GHM 20140226 rewritten slightly to properly support occurrence labels
                            const CDictItem* pItem = GetDocument()->GetDataDict()->GetLevel(level).GetRecord(record == -1 ? COMMON : record)->GetItem(item);
                            CString csItemLabel;

                            if( pItem->GetOccurs() == 1 || pItem->GetOccurrenceLabels().GetLabel(iocc).IsEmpty() )
                                csItemLabel.Format(_T("%s(%d)"), (LPCTSTR)pItem->GetLabel(), iocc + 1);

                            else
                                csItemLabel = pItem->GetOccurrenceLabels().GetLabel(iocc);

                            m_dicttree.SetItemText(htempOcc,csItemLabel);

                            iVS = 0;
                            for (HTREEITEM htempVS = m_dicttree.GetChildItem(htempOcc); htempVS != NULL;
                                    htempVS = m_dicttree.GetNextItem(htempVS,TVGN_NEXT)) {
                                if (record == -1)
                                    m_dicttree.SetItemText(htempVS,GetDocument()->GetDataDict()->GetLevel(level).GetIdItemsRec()->GetItem(item)->GetValueSet(iVS).GetLabel());
                                else {
                                    m_dicttree.SetItemText(htempVS,GetDocument()->GetDataDict()->GetLevel(level).GetRecord(record)->GetItem(item)->GetValueSet(iVS).GetLabel());
                                }
                                iVS++;
                            }
                            iocc++;
                        }
                    }
                    item++;
                }
                record++;
            }
            level++;
        }
    }
}
