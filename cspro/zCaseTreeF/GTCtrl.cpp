// GTCtrl.cpp : implementation file
//

#include "StdAfx.h"
#include "GTCtrl.h"
#include "TItmInfo.h"
#include "CaseTree.h"
#include "CEUtils.h"
#include <zToolsO/Tools.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[]= __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CGenericTreeCtrl


CGenericTreeCtrl::CGenericTreeCtrl( CWnd*   pParent,
                                    bool    bCanDestroyItemInfo,
                                    int     iTypeOfTree,
                                    bool    bIsToggleView,
                                    bool    bSingleRoot,
                                    bool    bCloseOnEscKey,
                                    bool    bSingleClick,
                                    bool    bTwoIcons)
{
    InitDefaults();

    m_bCanDestroyItemInfo   = bCanDestroyItemInfo;
    m_iTypeOfTree           = iTypeOfTree;
    m_bIsToggleView         = bIsToggleView;
    m_bSingleRoot           = bSingleRoot;
    m_pParent               = pParent;
    m_bCloseOnEscKey        = bCloseOnEscKey;
    m_bSingleClick          = bSingleClick;
    m_bTwoIcons             = bTwoIcons;


}
CGenericTreeCtrl::CGenericTreeCtrl()
{
    InitDefaults();
}
void CGenericTreeCtrl::InitDefaults()
{
    m_bIsToggleView         = false;
    m_bFiltrate             = false;
}





CGenericTreeCtrl::~CGenericTreeCtrl()
{
    DestroyInfo();

    if(IsWindow(GetSafeHwnd())){ //<-- to prevent delete childs of a not created tree
        DeleteChilds(GetRootItem());
    }

    if( m_pRootItem ){
        delete(m_pRootItem);
        m_pRootItem = NULL;
    }
}


BEGIN_MESSAGE_MAP(CGenericTreeCtrl, CxTreeCtrl)
        //{{AFX_MSG_MAP(CGenericTreeCtrl)
        ON_WM_CHAR()
        ON_NOTIFY_REFLECT(NM_CLICK, OnClick)
        ON_NOTIFY_REFLECT(TVN_SELCHANGING, OnSelchanging)
        ON_WM_HSCROLL()
        ON_WM_VSCROLL()
        //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CGenericTreeCtrl message handlers


BOOL CGenericTreeCtrl::OnCommand(WPARAM wParam, LPARAM lParam)
{
    /*It doesn't take the ENTER key*/
    return CxTreeCtrl::OnCommand(wParam, lParam);
}




void CGenericTreeCtrl::SetParent(CWnd *pParent, int iParentType)
{
        m_pParent     = pParent;
        m_iParentType = iParentType;
}






void CGenericTreeCtrl::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags)
{
    CxTreeCtrl::OnChar(nChar, nRepCnt, nFlags);
}

void CGenericTreeCtrl::OnClick(NMHDR* /*pNMHDR*/, LRESULT* pResult)
{
    *pResult = 0;
}

bool CGenericTreeCtrl::IsToggleView()
{
    return m_bIsToggleView;
}


//le consulta a un hItem, si es que es el field buscado
bool CGenericTreeCtrl::HasField(CDEField * pWantedField, int iWantedOccurrence , HTREEITEM hItem)
{
    CTreeItemInfo * pItemInfo = (CTreeItemInfo*) GetItemData( hItem );
    if ( pItemInfo->GetTypeOfInfo()==CS_FIELD_INFO ){
        CDEField *      pField          = (CDEField*)pItemInfo->GetItem();//>GetField();
        int             iFieldOcc       = pItemInfo->GetOccurrence();
        return (pField && pWantedField && pField->GetSymbol()==pWantedField->GetSymbol() && iFieldOcc==iWantedOccurrence);
    } else {
        return false;
    }
}

//try to match item and occurrence
bool CGenericTreeCtrl::HasItemBase(CDEItemBase* pWantedItemBase, int iWantedOcc, HTREEITEM hItem)
{
    CTreeItemInfo*  pItemInfo   = (CTreeItemInfo*) GetItemData( hItem );
    CDEItemBase*    pItemBase   = pItemInfo->GetItem();
    bool            bMatchItem  = pItemBase && pWantedItemBase && pItemBase->GetSymbol()==pWantedItemBase->GetSymbol();
    bool            bMatchOcc   = iWantedOcc==pItemInfo->GetOccurrence();

    return  bMatchItem && bMatchOcc;
}

//only match item
bool CGenericTreeCtrl::HasItemBase(CDEItemBase* pWantedItemBase, HTREEITEM hItem)
{
    CTreeItemInfo*  pItemInfo   = (CTreeItemInfo*) GetItemData( hItem );
    CDEItemBase*    pItemBase   = pItemInfo->GetItem();
    bool            bMatchItem  = pItemBase && pWantedItemBase && pItemBase->GetSymbol()==pWantedItemBase->GetSymbol();
    return  bMatchItem;
}


bool CGenericTreeCtrl::HasField(CDEField * pWantedField, int iWantedOccurrence , CTreeItem * pRootItem)
{

    CTreeItemInfo * pItemInfo = pRootItem->m_pInfo;
    if ( pItemInfo->GetTypeOfInfo()==CS_FIELD_INFO ){
        CDEField *  pField          = (CDEField*)pItemInfo->GetItem();//GetField();
        int         iFieldOcc       = pItemInfo->GetOccurrence();
        return ( pField && pWantedField && pField->GetSymbol()==pWantedField->GetSymbol() && iFieldOcc==iWantedOccurrence);
    } else {
        return false;
    }

}


bool CGenericTreeCtrl::SearchField(CDEField * pWantedField, int iWantedOccurrence, HTREEITEM *hOutputItem, int* iIsInRoot)
{
        bool bFoundField = false;
        bool bSingleRoot = m_bSingleRoot;
        if( bSingleRoot ){

                bFoundField = xSearchField(pWantedField, iWantedOccurrence, this->GetRootItem(), hOutputItem);

        } else {

                if( m_pRootItem ){

                        //primero buscamos m_pRootItem (no dibujado)
                        if( HasField( pWantedField, iWantedOccurrence, m_pRootItem) ){

                                (*hOutputItem)  = NULL;
                                *iIsInRoot              = 1;

                                bFoundField             = true;

                        } else {

                                *iIsInRoot              = 0;

                                int iNumChilds = m_pRootItem->m_childItemsArray.GetSize();
                                HTREEITEM hItem;
                                for(int i=0; i<iNumChilds && !bFoundField; i++){
                                        hItem           = m_pRootItem->m_childItemsArray.GetAt(i);
                                        bFoundField = xSearchField(pWantedField, iWantedOccurrence, hItem, hOutputItem);
                                }
                        }

                } else {

                        ASSERT(0);
                        //AfxMessageBox("Failed ASSERT : CGenericTreeCtrl::SearchField => m_pRootItem==NULL! ");
                        bFoundField     = false;
                }
        }


        return bFoundField;
}

bool CGenericTreeCtrl::xSearchField(CDEField * pWantedField, int iWantedOccurrence, HTREEITEM hInputItem, HTREEITEM *hOutputItem)
{
        bool bFoundField = false;

        if( hInputItem==NULL ){

                return false;

        }

        if( HasField( pWantedField, iWantedOccurrence, hInputItem) ){

                (*hOutputItem)  = hInputItem;
                bFoundField             = true;

        } else {

                HTREEITEM hNextItem = this->GetChildItem( hInputItem );
                while( hNextItem != NULL && *hOutputItem==NULL && !bFoundField){

                        bFoundField = xSearchField( pWantedField, iWantedOccurrence, hNextItem, hOutputItem);

                        hNextItem = this->GetNextItem( hNextItem, TVGN_NEXT);
                }
        }

        return bFoundField;
}



void CGenericTreeCtrl::Filter(TreeFilterType              treeFilterType,                 //Tipo de filtro
                                                          CArray<HTREEITEM,HTREEITEM>*  filterItemsArray,       //Arreglo para almacenar elementos del árbol
                                                          bool                                                  bMatch)                                 //bMatch => almacena los elementos que calzan con un cierto criterio. !bMatch=> almacena los que no calzan.
{
        //updateTreeItemInfo();

        bool bSingleRoot = m_bSingleRoot;
        if( bSingleRoot ){

                xFilter(treeFilterType, GetRootItem(), filterItemsArray,bMatch);

        } else {

                if( m_pRootItem ){

                        int iNumChilds = m_pRootItem->m_childItemsArray.GetSize();
                        HTREEITEM hItem;
                        for(int i=0; i<iNumChilds; i++){
                                hItem           = m_pRootItem->m_childItemsArray.GetAt(i);
                                xFilter(treeFilterType, hItem, filterItemsArray, bMatch);
                        }

                } else {

                        ASSERT(0);
                        //AfxMessageBox("Failed ASSERT : CGenericTreeCtrl::Filter => m_pRootItem==NULL! ");
                }
        }
}

void CGenericTreeCtrl::xFilter(TreeFilterType treeFilterType, HTREEITEM hInputItem, CArray<HTREEITEM,HTREEITEM>* filterItemsArray, bool bMatch)
{
        CTreeItemInfo * pItemInfo = (CTreeItemInfo*) this->GetItemData( hInputItem );
        if(!pItemInfo || !hInputItem || !filterItemsArray){
                return;
        }

        bool bParseCondition = false;
        switch(treeFilterType){
            case TreeFilterType::FieldNote:
                bParseCondition = IsNoteIcon(pItemInfo->GetNonSelectedIconIndex());
                break;
            case TreeFilterType::None:
                bParseCondition = true;
                break;
            default:
                ASSERT(false);
        }

        if( (bParseCondition && bMatch) || (!bParseCondition && !bMatch) ){

                //its added to the list
                filterItemsArray->Add(hInputItem);
        }

        HTREEITEM hNextItem = GetChildItem( hInputItem );
        while( hNextItem ){
                xFilter( treeFilterType, hNextItem, filterItemsArray, bMatch );
                hNextItem = GetNextItem( hNextItem, TVGN_NEXT);
        }
}


bool CGenericTreeCtrl::IsNoteIcon(int iIconIdx)
{
    const int NoteIcons[] =
    {
        IDI_NON_SELECTED_FIELD_NOTE_ICON_3D,
        IDI_SELECTED_FIELD_NOTE_ICON_3D,
        IDI_NON_SELECTED_FIELD_NOTE_ICON_3D_0,
        IDI_SELECTED_FIELD_NOTE_ICON_3D_0,
        IDI_NON_SELECTED_FIELD_NOTE_ICON_3D_1,
        IDI_SELECTED_FIELD_NOTE_ICON_3D_1,
        IDI_NON_SELECTED_FIELD_NOTE_ICON_3D_2,
        IDI_SELECTED_FIELD_NOTE_ICON_3D_2,
        IDI_NON_SELECTED_FIELD_NOTE_ICON_3D_3,
        IDI_SELECTED_FIELD_NOTE_ICON_3D_3,
        IDI_NON_SELECTED_ROSTER_NOTE_ICON,
        IDI_SELECTED_ROSTER_NOTE_ICON,
        IDI_NON_SELECTED_LEVEL_NOTE_ICON_3D,
        IDI_SELECTED_LEVEL_NOTE_ICON_3D,
        IDI_NON_SELECTED_GROUP_NOTE_ICON_3D,
        IDI_SELECTED_GROUP_NOTE_ICON_3D,
        IDI_NON_SELECTED_OCCURRENCE_TITTLE_NOTE_ICON_3D,
        IDI_SELECTED_OCCURRENCE_TITTLE_NOTE_ICON_3D,
        IDI_NON_SELECTED_FINISHED_CASE_NOTE_ICON_3D,
        IDI_SELECTED_FINISHED_CASE_NOTE_ICON_3D,
        IDI_NON_SELECTED_NOT_FINISHED_CASE_NOTE_ICON_3D,
        IDI_SELECTED_NOT_FINISHED_CASE_NOTE_ICON_3D,
        IDI_NON_SELECTED_NOT_ALL_FINISHED_CASE_LIST_TITTLE_NOTE_ICON_3D,
        IDI_SELECTED_NOT_ALL_FINISHED_CASE_LIST_TITTLE_NOTE_ICON_3D,
        IDI_NON_SELECTED_ALL_FINISHED_CASE_LIST_TITTLE_NOTE_ICON_3D,
        IDI_SELECTED_ALL_FINISHED_CASE_LIST_TITTLE_NOTE_ICON_3D,
        IDI_NON_SELECTED_EMPTY_ROSTER_NOTE,
        IDI_SELECTED_EMPTY_ROSTER_NOTE,
        IDI_NON_SELECTED_NON_EMPTY_ROSTER_NOTE,
        IDI_SELECTED_NON_EMPTY_ROSTER_NOTE,
        IDI_GREEN_NODE_NOTE_ICON,
        IDI_BLUE_NODE_NOTE_ICON,
        IDI_GREEN_NODE_PADLOCK_NOTE_ICON
    };

    for( size_t i = 0; i < _countof(NoteIcons); ++i )
    {
        if( iIconIdx == CaseTreeIcons::GetIconIndex(NoteIcons[i]) )
            return true;
    }

    return false;
}

//true  => there was some filtrate
//false => do nothing
bool CGenericTreeCtrl::Filter(TreeFilterType treeFilterType)
{
        bool bFilter = false;

        CArray<HTREEITEM,HTREEITEM> hitemArray;
        Filter(treeFilterType,&hitemArray,false);
        int iSize = hitemArray.GetSize();

        if( iSize<GetCount() ){
            bFilter = true;
        } else {
            bFilter = false;
        }
        //bFilter         = (bool) (iSize < GetCount());

        if(!bFilter){
                return false;
        }


        updateTreeItemInfo();

        SaveShape();

        for(int i=0; i<iSize; i++){

                HTREEITEM hItem = hitemArray.GetAt( (iSize-1) -i );

                ASSERT(!GetChildItem(hItem));

                //store the CTreeItemInfo pointer
                m_TreeItemInfoArray.Add( (CTreeItemInfo*)GetItemData(hItem) );


                //search in m_pRootItem and try to remove it.
                bool bRemoved = false;
                bool bDeleted = false;
                if( m_pRootItem ){
                        m_pRootItem->Remove( hItem, true, &bRemoved, &bDeleted );
                }

                //if the item has not been deleted from m_pRootItem, then the item is only in the tree
                if(!bDeleted){


                        DeleteItem( hItem );
                }
        }


        m_bFiltrate = m_bFiltrate || bFilter;



        return bFilter;
}




void CGenericTreeCtrl::updateTreeItemInfo()
{
        bool bSingleRoot = m_bSingleRoot;
        if( bSingleRoot ){

                xUpdateTreeItemInfo(GetRootItem());

        } else {

                if( m_pRootItem ){

                        int iNumChilds = m_pRootItem->m_childItemsArray.GetSize();
                        HTREEITEM hItem;
                        for(int i=0; i<iNumChilds; i++){
                                hItem           = m_pRootItem->m_childItemsArray.GetAt(i);
                                xUpdateTreeItemInfo(hItem);
                        }

                } else {

                        ASSERT(0);
                        //AfxMessageBox("Failed ASSERT : CGenericTreeCtrl::updateTreeItemInfo => m_pRootItem==NULL! ");
                }
        }
}

void CGenericTreeCtrl::xUpdateTreeItemInfo(HTREEITEM hInputItem)
{
        CTreeItemInfo * pItemInfo = (CTreeItemInfo*) this->GetItemData( hInputItem );
        if(!pItemInfo || !hInputItem ){
                return;
        }

        pItemInfo->SetLabel( GetItemText(hInputItem) );
        pItemInfo->SetSelectedIconIndex( 1+pItemInfo->GetNonSelectedIconIndex() );

        CTreeItemInfo*  pParentTreeItemInfo = NULL;
        HTREEITEM               hParentItem                     = GetParentItem(hInputItem);
        if( hParentItem ){
                pParentTreeItemInfo = (CTreeItemInfo*)GetItemData(hParentItem);
        } else {
                if( m_pRootItem ){
                        int iItemIdx = m_pRootItem->GetIdx(hInputItem);
                        if( iItemIdx!=-1 ){
                                pParentTreeItemInfo = m_pRootItem->m_pInfo;
                        }
                }
        }
        pItemInfo->SetItemInfoParent( pParentTreeItemInfo );


        HTREEITEM hNextItem = GetChildItem( hInputItem );
        while( hNextItem ){
                xUpdateTreeItemInfo( hNextItem );
                hNextItem = GetNextItem( hNextItem, TVGN_NEXT);
        }
}


bool CGenericTreeCtrl::IsFiltered()
{
        return m_bFiltrate;
}

void CGenericTreeCtrl::Restore()
{
        if(!m_pParent){
                return;
        }

        //The restore must be controlled outside this tree, because of the tree layers.
        m_pParent->SendMessage(UWM::CaseTree::RestoreTree, 0, 0);

        //after this function return, the message will be processed
        //and then, this object will be recreated, so, the constructor will put m_bFiltrate in false again.


        m_bFiltrate = false;
}





HTREEITEM CGenericTreeCtrl::InsertNode(CHITEM *chitem, HTREEITEM hParent)
{
    CString         csItemLabel         = chitem->m_csLabel;
    HTREEITEM       hItemParent         = hParent;
    HTREEITEM       hInsertAfter        = TVI_LAST;
    int             iNonSelectedIconIdx = chitem->m_iNonSelectedIconIdx;
    int             iSelectedIconIdx    = chitem->m_iSelectedIconIdx;
    CTreeItemInfo*  pInfo               = chitem->m_pInfo;
    HTREEITEM       hInsertedItem       = Insert( csItemLabel, hItemParent, hInsertAfter, iNonSelectedIconIdx, iSelectedIconIdx );

    SetItemData( hInsertedItem, (DWORD) pInfo );


    return          hInsertedItem;
}

void CGenericTreeCtrl::UpdateParentWndText(CString csWindowText)
{
    if(!m_pParent){
        return;
    }

    CMsgParam msgParam;
    msgParam.csParam    = csWindowText;

    m_pParent->SendMessage(UWM::CaseTree::UpdateWindowText, (WPARAM)&msgParam, 0);
}



bool CGenericTreeCtrl::HasNotes()
{
        //NOTES CAUTION : In the future could be more types of notes.
        CArray<HTREEITEM,HTREEITEM> hitemArray;
        Filter( TreeFilterType::FieldNote ,&hitemArray,true);
        return (hitemArray.GetSize()>0);
}


int CGenericTreeCtrl::SearchItemBase(CDEItemBase* pWantedItemBase,
                                     const CArray<int,int>& aWantedOccs,
                                     CArray<HTREEITEM,HTREEITEM>& ahItems,
                                     CArray<bool,bool>& aFoundOccs )
{

    ahItems.RemoveAll();

    aFoundOccs.RemoveAll();
    int iNumWantedOccs = aWantedOccs.GetSize();
    for(int i=0; i<iNumWantedOccs; i++){
        aFoundOccs.Add(false);
    }

    xSearchItemBase(pWantedItemBase, aWantedOccs, GetRootItem(), ahItems, aFoundOccs);

    return ahItems.GetSize();
}
void CGenericTreeCtrl::xSearchItemBase(CDEItemBase* pWantedItemBase, const CArray<int,int>& aWantedOccs, HTREEITEM hItem, CArray<HTREEITEM,HTREEITEM>& ahItems, CArray<bool,bool>& aFoundOccs )
{
    if(!hItem){
        return;
    }

    int iNumOccs = aWantedOccs.GetSize();
    for( int i=0; i<iNumOccs; i++){
        if(HasItemBase( pWantedItemBase, aWantedOccs.GetAt(i), hItem)){
            ahItems.Add(hItem);
            aFoundOccs.SetAt(i,true);
        }
    }

    HTREEITEM hNextItem = GetChildItem( hItem );
    while( hNextItem ){
        xSearchItemBase( pWantedItemBase, aWantedOccs, hNextItem, ahItems, aFoundOccs);
        hNextItem = GetNextItem( hNextItem, TVGN_NEXT);
    }
}


int CGenericTreeCtrl::SearchItemBase(CDEItemBase* pWantedItemBase, CArray<HTREEITEM,HTREEITEM>& ahItems)
{
    ahItems.RemoveAll();
    xSearchItemBase(pWantedItemBase, GetRootItem(), ahItems);
    return ahItems.GetSize();
}
void CGenericTreeCtrl::xSearchItemBase(CDEItemBase* pWantedItemBase, HTREEITEM hItem, CArray<HTREEITEM,HTREEITEM>& ahItems)
{
    if(!hItem){
        return;
    }

    if(HasItemBase( pWantedItemBase, hItem)){
        ahItems.Add(hItem);
    }

    HTREEITEM hNextItem = GetChildItem( hItem );
    while( hNextItem ){
        xSearchItemBase( pWantedItemBase, hNextItem, ahItems);
        hNextItem = GetNextItem( hNextItem, TVGN_NEXT);
    }
}


int  CGenericTreeCtrl::GetNumTreeOccs(CDEItemBase* pWantedItemBase)
{
    CArray<HTREEITEM,HTREEITEM> aHITEMArray;
    SearchItemBase(pWantedItemBase, aHITEMArray );
    return aHITEMArray.GetSize();
}


void CGenericTreeCtrl::OnSelchanging(NMHDR* pNMHDR, LRESULT* pResult)
{
    // TODO: Add your control notification handler code here
    *pResult = 0;
}


void CGenericTreeCtrl::DestroyInfo()
{
    //Se eliminan todos los (CTreeItemInfo *) asociados a cada item.
    if( !m_bCanDestroyItemInfo ){
        return;
    }

    //se eliminan los CTreeItemInfo almacenados en los elementos visibles del arbol.
    bool bSingleRoot = this->m_bSingleRoot;
    if( bSingleRoot ){

        this->DestroyItemInfo( this->GetRootItem(), true );

    } else {

        if( m_pRootItem ){

            int iNumChilds = this->m_pRootItem->m_childItemsArray.GetSize();
            HTREEITEM hItem = NULL;
            for( int i=0; i<iNumChilds; i++)
            {
                hItem = this->m_pRootItem->m_childItemsArray.GetAt( i );
                DestroyItemInfo( hItem, true );
            }
            CTreeItemInfo * pItemInfo = this->m_pRootItem->m_pInfo;
            if( pItemInfo!=NULL ){
                delete( pItemInfo );
                pItemInfo = NULL;
            }

        } else {

            ASSERT(0);
            //AfxMessageBox(_T("WARNING !!! m_pRootItem==NULL. There will memory leaks"));
        }
    }

        //se eliminan los CTreeItemInfo* almacenados en m_TreeItemInfoArray (que antes estaban asociados a hojas que fueron recortadas)
    int iSize = m_TreeItemInfoArray.GetSize();
    for(int i=0; i<iSize; i++){
        CTreeItemInfo * pItemInfo = m_TreeItemInfoArray.GetAt(i);
        if( pItemInfo ){
            delete( pItemInfo );
            pItemInfo = NULL;
            m_TreeItemInfoArray.SetAt(i,NULL);
        }
    }
    m_TreeItemInfoArray.RemoveAll();

}

/*virtual*/
bool CGenericTreeCtrl::OnBeforeKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
    bool bProcessKey = true;

    //CONTROL
    if( GetKeyState(VK_CONTROL) < 0 ){

        switch(nChar){


            //CONTROL+Z
        case _T('Z') : {  m_pParent->PostMessage(UWM::CaseTree::CloseAllTreeLayers, 0, 0); } break;

            //CONTROL+F
        case _T('F') : {    if(m_iTypeOfTree!=TREE_OF_CASES){


            if(!m_bFiltrate){

                m_bFiltrate = Filter(TreeFilterType::FieldNote);
                //m_bFiltrate = true;

            } else {

                if(!m_pParent){
                    return false;
                }

                //The restore must be controlled outside this tree, because of the tree layers.
                m_pParent->SendMessage(UWM::CaseTree::RestoreTree, 0, 0);
                bProcessKey = false;

                //after this function return, the message will be processed
                //and then, this object will be recreated, so, the constructor will put m_bFiltrate in false again.
            }

                   }
        } break;

        }
    }

    if( nChar==ENTER_KEY_ASCII || nChar==SPACE_KEY_ASCII ){

        HTREEITEM SelectedItem = this->GetSelectedItem();

        Expand( SelectedItem, TVE_TOGGLE );


        SCROLLINFO ScrollInfoH, ScrollInfoV;

        GetScrollInfo(SB_HORZ,&ScrollInfoH);
        GetScrollInfo(SB_VERT,&ScrollInfoV);

        int iHScrollPos = ScrollInfoH.nPos;
        int iVScrollPos = ScrollInfoV.nPos;

        CMsgParam * pMsgParam                               = new CMsgParam();
        pMsgParam->hParam                                   = SelectedItem;
        pMsgParam->bParam                                   = true;
        pMsgParam->dwArrayParam.Add( iHScrollPos );
        pMsgParam->dwArrayParam.Add( iVScrollPos );
        pMsgParam->bMustBeDestroyedAfterLastCatchMessage    = true;

        m_pParent->PostMessage(UWM::CaseTree::SelectTreeItem, (WPARAM)pMsgParam, 0);

        bProcessKey = false;


    } else if( nChar==ESC_KEY_ASCII && m_bCloseOnEscKey){

        m_pParent->PostMessage(UWM::CaseTree::CloseCurrentTreeLayer, 0, 0);
        bProcessKey = false;

    } else {

        CArray<UINT,UINT>* pParamArray = new CArray<UINT,UINT>;
        pParamArray->Add( nChar  );
        pParamArray->Add( nRepCnt);
        pParamArray->Add( nFlags );

        //The state of the virtual keys when fire the message
        bool bCtrl  = GetKeyState(VK_CONTROL)<0;
        bool bShift = GetKeyState(VK_SHIFT  )<0;
        bool bAlt   = GetKeyState(VK_MENU   )<0;

        pParamArray->Add( bCtrl  );
        pParamArray->Add( bShift );
        pParamArray->Add( bAlt   );

        m_pParent->PostMessage(UWM::CaseTree::UnknownKey, (WPARAM)pParamArray, 0);
    }

    return bProcessKey;
}
/*virtual*/
void CGenericTreeCtrl::OnAfterKeyDown(UINT /*nChar*/, UINT /*nRepCnt*/, UINT /*nFlags*/)
{
}


/*virtual*/
bool CGenericTreeCtrl::OnBeforeKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags)
{
    bool bProcessKey = true;
    return bProcessKey;
}

/*virtual*/
void CGenericTreeCtrl::OnAfterKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags)
{
}

/*virtual*/
bool CGenericTreeCtrl::OnBeforeLButtonDblClk(UINT nFlags, CPoint point){
    return false;
}

/*virtual*/
void CGenericTreeCtrl::OnAfterLButtonDblClk (UINT nFlags, CPoint point){
    DoSelect( nFlags, point );
}

void CGenericTreeCtrl::DoSelect( UINT nFlags, CPoint point ){

    if(m_bSingleClick){

        //here the tree get the focus... but nobody was selected
        m_pParent->PostMessage(UWM::CaseTree::SelectTreeItemUnsuccessful);

        return;
    }

    if(!m_pParent){
        return;
    }

    HTREEITEM SelectedItem  = GetSelectedItem();
    HTREEITEM hItem         = HitTest(point);
    if( !hItem || hItem!=SelectedItem){
        return;
    }

    if( SelectedItem){

        SCROLLINFO ScrollInfoH, ScrollInfoV;

        GetScrollInfo(SB_HORZ,&ScrollInfoH);
        GetScrollInfo(SB_VERT,&ScrollInfoV);

        int iHScrollPos = ScrollInfoH.nPos;
        int iVScrollPos = ScrollInfoV.nPos;

        CMsgParam * pMsgParam                               = new CMsgParam();
        pMsgParam->hParam                                   = SelectedItem;
        pMsgParam->bParam                                   = true;
        pMsgParam->dwArrayParam.Add( iHScrollPos );
        pMsgParam->dwArrayParam.Add( iVScrollPos );
        pMsgParam->bMustBeDestroyedAfterLastCatchMessage    = true;

        m_pParent->PostMessage(UWM::CaseTree::SelectTreeItem, (WPARAM)pMsgParam, 0);
    }
}
