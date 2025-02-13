// xTCtrl.cpp : implementation file
//

#include "StdAfx.h"
#include "Xtctrl.h"
#include "CEUtils.h"
#include "MsgParam.h"
#include <zToolsO/NewlineSubstitutor.h>


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[]= __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// CxTreeCtrl

CxTreeCtrl::CxTreeCtrl()
{
    m_bCanDestroyItemInfo   = false;
    m_iTypeOfTree           = -1;
    m_bSingleRoot           = true;
    m_pRootItem             = NULL;
    m_pParent               = NULL;
    m_bCloseOnEscKey        = false;
    m_bSingleClick          = false;
    m_bTwoIcons             = true;
}


BEGIN_MESSAGE_MAP(CxTreeCtrl, CTreeCtrl)
        //{{AFX_MSG_MAP(CxTreeCtrl)
    ON_WM_LBUTTONDBLCLK()
    ON_WM_LBUTTONDOWN()
    ON_WM_RBUTTONDOWN()
    ON_WM_KEYDOWN()
    ON_WM_KEYUP()
    ON_WM_SHOWWINDOW()
    ON_WM_SETFOCUS()
    ON_WM_KILLFOCUS()
    ON_WM_GETDLGCODE()
    ON_NOTIFY_REFLECT(TVN_ITEMEXPANDED, OnItemexpanded)
        //}}AFX_MSG_MAP
END_MESSAGE_MAP()


UINT CxTreeCtrl::OnGetDlgCode()
{
    return DLGC_WANTALLKEYS;
}

/////////////////////////////////////////////////////////////////////////////
// CxTreeCtrl message handlers
void CxTreeCtrl::OnLButtonDown(UINT nFlags, CPoint point)
{
    if( /*virtual*/OnBeforeLButtonDown(nFlags,point))
       CTreeCtrl::OnLButtonDown(nFlags, point);

    /*virtual*/
    OnAfterLButtonDown(nFlags,point);
}
/*virtual*/
void CxTreeCtrl::OnAfterLButtonDown(UINT /*nFlags*/, CPoint point){

    if(m_bSingleClick){

        //The same code of OnLButtonDblClk to select
        if(!m_pParent){
            return;
        }
        HTREEITEM SelectedItem  = GetSelectedItem();
        HTREEITEM hItem         = HitTest(point);
        if( !hItem || hItem!=SelectedItem){

            //here the tree get the focus... but nobody was selected
            m_pParent->PostMessage(UWM::CaseTree::SelectTreeItemUnsuccessful);

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
            pMsgParam->dwArrayParam.Add(iHScrollPos);
            pMsgParam->dwArrayParam.Add(iVScrollPos);
            pMsgParam->bMustBeDestroyedAfterLastCatchMessage    = true;
            m_pParent->PostMessage(UWM::CaseTree::SelectTreeItem, (WPARAM)pMsgParam, 0);
        }
        return;
    }
}
void CxTreeCtrl::OnRButtonDown(UINT nFlags, CPoint point)
{
    if(/*virtual*/OnBeforeRButtonDown(nFlags,point)){
        CTreeCtrl::OnRButtonDown(nFlags,point);
    }

    /*virtual*/
    OnAfterRButtonDown(nFlags,point);
}
void CxTreeCtrl::OnItemexpanded(NMHDR* pNMHDR, LRESULT* pResult)
{
    // TODO: Add your control notification handler code here
    *pResult = 0;

    /*virtual*/
    OnAfterItemExpanded(pNMHDR,pResult);
}



void CxTreeCtrl::OnLButtonDblClk(UINT nFlags, CPoint point)
{
    if(/*virtual*/OnBeforeLButtonDblClk(nFlags,point)){
        CTreeCtrl::OnLButtonDblClk(nFlags,point);
    }

    /*virtual*/
    OnAfterLButtonDblClk(nFlags,point);
}

void CxTreeCtrl::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
    if(/*virtual*/OnBeforeKeyDown(nChar,nRepCnt,nFlags)){
        CTreeCtrl::OnKeyDown(nChar, nRepCnt, nFlags);
    }

    /*virtual*/
    OnAfterKeyDown(nChar,nRepCnt,nFlags);
}

void CxTreeCtrl::OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags)
{
    if(/*virtual*/OnBeforeKeyUp(nChar,nRepCnt,nFlags)){
        CTreeCtrl::OnKeyUp(nChar,nRepCnt,nFlags);
    }

    /*virtual*/
    OnAfterKeyUp(nChar,nRepCnt,nFlags);
}


void CxTreeCtrl::OnSetFocus(CWnd* pOldWnd)
{
    if(/*virtual*/OnBeforeSetFocus(pOldWnd)){
        CTreeCtrl::OnSetFocus(pOldWnd);
    }

    /*virtual*/
    OnAfterSetFocus(pOldWnd);
}
void CxTreeCtrl::OnKillFocus(CWnd* pNewWnd)
{
    if(/*virtual*/OnBeforeKillFocus(pNewWnd)){
        CTreeCtrl::OnKillFocus(pNewWnd);
    }

        /*virtual*/
    OnAfterKillFocus(pNewWnd);
}




//INSERT FUNCTIONS
HTREEITEM CxTreeCtrl::Insert(CString CIMSA_Label, HTREEITEM hGenericTreeParentItem, HTREEITEM hInsertAfter,
                             int iNonSelectedIconIndex, int iSelectedIconIndex)
{
        //
        TV_INSERTSTRUCT tvi;
        tvi.item.mask = TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE;

        //Text
        NewlineSubstitutor::MakeNewlineToSpace(CIMSA_Label);
        tvi.item.pszText = CIMSA_Label.GetBuffer();

        //Icons
        if(m_bTwoIcons){
            tvi.item.iImage         = iNonSelectedIconIndex;
            tvi.item.iSelectedImage = iSelectedIconIndex;
        } else {
            tvi.item.iImage         = iNonSelectedIconIndex;
            tvi.item.iSelectedImage = iNonSelectedIconIndex;
        }


        //Insertion place
        tvi.hParent             = hGenericTreeParentItem;
        tvi.hInsertAfter        = hInsertAfter;

        tvi.item.lParam             = NULL;
        HTREEITEM hGenericTreeItem  = InsertItem(&tvi);

        bool bSingleRoot = m_bSingleRoot;
        if( !hGenericTreeParentItem && !bSingleRoot){
                ASSERT(m_pRootItem);
                m_pRootItem->AddChild( hGenericTreeItem );
        }

        return hGenericTreeItem;
}


//COPY FUNCTIONS
void CxTreeCtrl::RecursiveCopy( HTREEITEM hItem, HTREEITEM hParent, CxTreeCtrl * pBaseTree )
{
    //EL parent, con certeza pertenece a "this"
    CTreeItemInfo * pItemInfo = (CTreeItemInfo*) pBaseTree->GetItemData( hItem );

    //Se copia el hijo
    HTREEITEM hChildItem = this->Insert( _T("."), hParent, TVI_LAST, pItemInfo->GetNonSelectedIconIndex(),pItemInfo->GetSelectedIconIndex());
    this->SetItemText( hChildItem, pBaseTree->GetItemText( hItem ) );
    this->SetItemData( hChildItem, pBaseTree->GetItemData( hItem ) );



    HTREEITEM hNextItem = pBaseTree->GetChildItem ( hItem );
    while( hNextItem!=NULL ){

        RecursiveCopy( hNextItem, hChildItem , pBaseTree);

        hNextItem = pBaseTree->GetNextItem( hNextItem, TVGN_NEXT );
    }

    if( pBaseTree->GetItemState(hItem,TVIS_EXPANDED) & TVIS_EXPANDED ){
        this->Expand(hChildItem, TVE_EXPAND );
    }
}
void CxTreeCtrl::CopyFrom(CxTreeCtrl * pBaseTree, HTREEITEM hItem)
{
    //
    CTreeItemInfo * pItemInfo = (CTreeItemInfo*) pBaseTree->GetItemData( hItem );

    bool bSingleRoot = this->m_bSingleRoot;
    HTREEITEM hRootItem = NULL;
    if( bSingleRoot ){

        hRootItem = Insert( _T("."), TVI_ROOT, TVI_LAST, pItemInfo->GetNonSelectedIconIndex(),pItemInfo->GetSelectedIconIndex());
        this->SetItemText( hRootItem, pBaseTree->GetItemText( hItem ) );
        this->SetItemData( hRootItem, pBaseTree->GetItemData( hItem ) );


    } else {


        CTreeItem * pRootItem =         new CTreeItem(this,pItemInfo, pBaseTree->GetItemText( hItem ), TVI_ROOT, TVI_LAST, pItemInfo->GetNonSelectedIconIndex(), pItemInfo->GetSelectedIconIndex() );
        SetXRootItem( pRootItem );
    }

    //Paso 2 : inicio copiado recursivo
    //if( pBaseTree->ItemHasChildren(hItem) ){

    //Obtención del primer hijo de hItem
    HTREEITEM hNextItem     = pBaseTree->GetChildItem( hItem );
    while( hNextItem!=NULL ){

        RecursiveCopy( hNextItem, hRootItem , pBaseTree);

        //Obtención de cada hermano de hItem
        hNextItem = pBaseTree->GetNextItem( hNextItem, TVGN_NEXT);
    }
    //}

    //Paso 3 : se expande la raíz.
    if( bSingleRoot ){
        this->Expand( this->GetRootItem(), TVE_EXPAND );
    } else {


            int iNumChilds = m_pRootItem->m_childItemsArray.GetSize();
            if( iNumChilds==1 ){
                this->Expand( this->GetRootItem(), TVE_EXPAND );
            }

    }

    this->EnableToolTips(true);
}

//SEARCH FUNCTIONS


bool CxTreeCtrl::SearchItem(CString& csItemKey, HTREEITEM *hOutputItem, int*  iIsInRoot)
{
    bool bSearchOK   = false;
    bool bSingleRoot = m_bSingleRoot;
    if( bSingleRoot ){


    } else {



        if( m_pRootItem!=NULL ){

            //primero busca en m_pRootItem (que no ha sido dibujado)
            if( HasKey( csItemKey, m_pRootItem ) ){

                (*hOutputItem) = NULL;  //pero OJO : Retorna NULL indicando que no hay ningún hItem que contenga la llave,
                //sino que corresponde a la raíz no dibujada => quien llame a searchItem debe tener eso claro.
                //y consultar por pRootItem
                *iIsInRoot              = 1;

                bSearchOK               = true;

            } else {

                *iIsInRoot              = 0;

                //luego busca recorriendo cada una de las raíces que son hijos de m_pRootItem
                int iNumRoots = m_pRootItem->m_childItemsArray.GetSize();
                HTREEITEM hRootItem;
                for( int i=0; i<iNumRoots && (*hOutputItem==NULL); i++ ){
                    hRootItem = m_pRootItem->m_childItemsArray.GetAt(i);

                    if( xSearchItem( csItemKey, hRootItem, hOutputItem ) ){
                        bSearchOK = true;
                    }
                }
            }

        } else {

            AfxMessageBox(_T("Failed ASSERT : CGenericTreeCtrl::searchItem => m_pRootItem==NULL! "));
        }
    }
    return bSearchOK;
}

bool CxTreeCtrl::xSearchItem(const CString& csItemKey, HTREEITEM hInputItem, HTREEITEM *hOutputItem)
{


        bool bSearchOK = false;

        if( HasKey( csItemKey, hInputItem) ){

                (*hOutputItem) = hInputItem;
                bSearchOK      = true;

        } else {

                HTREEITEM hNextItem = this->GetChildItem( hInputItem );
                while( hNextItem != NULL && *hOutputItem==NULL){

                        if(xSearchItem( csItemKey, hNextItem, hOutputItem )){
                                bSearchOK = true;
                        }

                        hNextItem = GetNextItem( hNextItem, TVGN_NEXT);
                }
        }

        return bSearchOK;
}

bool CxTreeCtrl::HasInfo(CTreeItemInfo* pWantedInfo, CTreeItem * hItem)
{
        CTreeItemInfo * pItemInfo = hItem->m_pInfo;
        bool bHasInfo = ( pItemInfo==pWantedInfo || pItemInfo->equalKey( pWantedInfo ) );
        return bHasInfo;
}

bool CxTreeCtrl::HasInfo(CTreeItemInfo* pWantedInfo, HTREEITEM hItem)
{
        //FABN INIT 19/Aug/2002
        CTreeItemInfo * pItemInfo = (CTreeItemInfo*) GetItemData( hItem );
        bool bHasInfo = ( pItemInfo==pWantedInfo || pItemInfo->equalKey( pWantedInfo ) );
        return bHasInfo;
        //FABN END 19/Aug/2002
}

//KEY FUNCTIONS

CString CxTreeCtrl::GetKey( HTREEITEM hItem )
{


    CString csKey = _T("");
    if( hItem ){
        CTreeItemInfo* pItemInfo = (CTreeItemInfo*) GetItemData( hItem );

        if( pItemInfo ){
            csKey = pItemInfo->GetKey();
        }
    }
    return csKey;
}
bool CxTreeCtrl::SaveAllKeysTo(CArray<CString,CString> * pArray )
{
    bool bSaveOK = true;

    bool bSingleRoot = this->m_bSingleRoot;
    if( bSingleRoot  ){

        bSaveOK = xSaveAllKeysTo(pArray,GetRootItem());

    } else {

        if( !m_pRootItem ){

            //primero graba la llave de la raíz no dibujada
            pArray->Add( m_pRootItem->m_pInfo->GetKey() );

            //y ahora la llave de cada elemento hijo.
            int iNumChilds = m_pRootItem->m_childItemsArray.GetSize();
            HTREEITEM hItem;
            for( int i=0; i<iNumChilds && bSaveOK; i++ ){
                hItem = m_pRootItem->m_childItemsArray.GetAt(i);
                bSaveOK = xSaveAllKeysTo(pArray, hItem);
            }

        } else {

            AfxMessageBox(_T("Failed ASSERT : CGenericTreeCtrl::saveAllKeysTo => m_pRootItem==NULL! "));
        }
    }

    return bSaveOK;
}
bool CxTreeCtrl::xSaveAllKeysTo(CArray<CString,CString> * pArray, HTREEITEM hItem)
{
        bool bSaveOK = true;

        CTreeItemInfo * pTreeItemInfo   = (CTreeItemInfo*) this->GetItemData( hItem );
        if( pTreeItemInfo!=NULL ){

                pArray->Add( pTreeItemInfo->GetKey() );

        } else {

                bSaveOK = false;
        }

        if( bSaveOK ){

                HTREEITEM hNextItem     = this->GetChildItem( hItem );
                while( hNextItem!=NULL && bSaveOK ){
                        bSaveOK                 = xSaveShapeTo( pArray, hNextItem );
                        hNextItem       = this->GetNextItem( hNextItem, TVGN_NEXT);
                }
        }

        return bSaveOK;
}

bool CxTreeCtrl::HasKey( const CString& csItemKey, CTreeItem * hItem )
{
    CTreeItemInfo * pItemInfo = hItem->m_pInfo;
    if( pItemInfo==NULL ){
        AfxMessageBox(_T("pItemInfo==NULL in HasKey function"));
    }
    bool bHasKey = pItemInfo!=NULL && (pItemInfo->GetKey().Compare(csItemKey)==0 );
    return bHasKey;
}

bool CxTreeCtrl::HasKey( const CString& csItemKey, HTREEITEM hItem )
{
    CTreeItemInfo * pItemInfo = (CTreeItemInfo*) this->GetItemData( hItem );
    if( pItemInfo==NULL ){
        AfxMessageBox(_T("pItemInfo==NULL in HasKey function"));
    }
    bool bHasKey = pItemInfo!=NULL && (pItemInfo->GetKey().Compare(csItemKey)==0 );
    return bHasKey;
}


//SHAPE FUNCTIONS
void CxTreeCtrl::Collapse()
{
    bool bSingleRoot = this->m_bSingleRoot;
    if( bSingleRoot  ){

        xCollapse( this->GetRootItem() );

    } else {

        if( m_pRootItem!=NULL ){

            //primero colapsa la raíz no dibujada.
            m_pRootItem->m_pInfo->setExpanded(false);

            //y ahora colapsa cada una de las ramas que cuelgan de m_pRootItem
            int iNumChilds = m_pRootItem->m_childItemsArray.GetSize();
            HTREEITEM hItem;
            for( int i=0; i<iNumChilds; i++){
                hItem = m_pRootItem->m_childItemsArray.GetAt(i);
                xCollapse( hItem );
            }

        } else {

            AfxMessageBox(_T("Failed ASSERT : CGenericTreeCtrl::Collapse => m_pRootItem==NULL! "));
        }
    }
}
void CxTreeCtrl::xCollapse(HTREEITEM hItem)
{
    if(!hItem){
        AfxMessageBox(_T("Failed ASSERT : CGenericTreeCtrl::xCollapse => hItem==NULL! "));
        return;
    }

    Expand(hItem, TVE_COLLAPSE );
    HTREEITEM hNextItem = GetChildItem(hItem);
    while( hNextItem!=NULL ){
        xCollapse( hNextItem );
        hNextItem = GetNextItem( hNextItem, TVGN_NEXT );
    }
}
bool CxTreeCtrl::LoadShape()
{
    bool bLoadOK = true;
    bool bSingleRoot = m_bSingleRoot;
    if( bSingleRoot ){

        bLoadOK = xLoadShape(GetRootItem());

    } else {

        if( m_pRootItem ){

            //no tiene sentido cargar el estado expandido/colapsado de m_pRootItem
            //por defecto, m_pRootItem está expandido. (está en el título de la ventana )
            //sólo podemos confirmarle su estado
            m_pRootItem->m_pInfo->setExpanded( true );

            //ahora se carga el estado expandido/colapsado para cada uno de sus hijos.
            int iNumChilds = m_pRootItem->m_childItemsArray.GetSize();
            HTREEITEM hItem;
            for( int i=0; i<iNumChilds && bLoadOK; i++ ){
                hItem   = m_pRootItem->m_childItemsArray.GetAt(i);
                bLoadOK = xLoadShape(hItem);
            }

        } else {

            AfxMessageBox(_T("Failed ASSERT : CGenericTreeCtrl::loadShape => m_pRootItem==NULL! "));

        }
    }

    return bLoadOK;
}
bool CxTreeCtrl::xLoadShape(HTREEITEM hItem)
{
    bool bLoadOK = true;

    if( hItem==NULL ){
        AfxMessageBox(_T("Failed ASSERT : CGenericTreeCtrl::xLoadShape => hItem==NULL! "));
        return false;
    }

    CTreeItemInfo * pTreeItemInfo   = (CTreeItemInfo*) GetItemData( hItem );

    if( pTreeItemInfo ){

        xExpand(hItem,pTreeItemInfo->IsExpanded());

    } else {

        bLoadOK = false;

    }

    if( bLoadOK ){

        HTREEITEM hNextItem     = GetChildItem( hItem );
        while( hNextItem && bLoadOK){
            bLoadOK     = xLoadShape    ( hNextItem );
            hNextItem   = GetNextItem   ( hNextItem, TVGN_NEXT);
        }
    }

    return bLoadOK;
}
bool CxTreeCtrl::LoadShapeFrom(CArray<CString,CString> * pArray )
{
    if( !m_bSingleRoot){
        if( m_pRootItem ){
            m_pRootItem->m_pInfo->setExpanded( true );
        } else {
            AfxMessageBox(_T("Failed ASSERT : CGenericTreeCtrl::loadShapeFrom => m_pRootItem==NULL! "));
            return false;
        }
    }

    Collapse();

    bool bLoadOK = true;

    if(m_bSingleRoot){

        bLoadOK = xLoadShapeFrom(pArray,GetRootItem());

    } else {

        int iNumRoots = m_pRootItem->m_childItemsArray.GetSize();
        HTREEITEM hRootItem;
        for( int i=0; bLoadOK && i<iNumRoots; i++ ){
            hRootItem   = m_pRootItem->m_childItemsArray.GetAt(i);
            bLoadOK     = hRootItem && xLoadShapeFrom( pArray,hRootItem);
        }
    }

    return bLoadOK;
}
bool CxTreeCtrl::xLoadShapeFrom(CArray<CString,CString>* pArray, HTREEITEM hItem)
{
    if(!pArray || !hItem){
        return false;
    }

    bool bLoadOK = true;

    if( CCapiEUtils::InArray(pArray,GetKey(hItem)) ){
        Expand(hItem,TVE_EXPAND);
    }

    HTREEITEM hNextItem = GetChildItem( hItem );
        while(bLoadOK && hNextItem){
                bLoadOK   = xLoadShapeFrom( pArray, hNextItem);
                hNextItem = GetNextItem( hNextItem, TVGN_NEXT);
        }

    return bLoadOK;
}
bool CxTreeCtrl::SaveShapeTo(CArray<CString,CString> * pArray )
{
        if(!SaveShape()){
                return false;
        }

    bool bSaveOK = true;

    bool bSingleRoot = m_bSingleRoot;
    if( bSingleRoot ){

        xSaveShapeTo(pArray, GetRootItem() );

    } else {

        if( m_pRootItem ){

            //m_pRootItem está expandido
            pArray->Add( m_pRootItem->m_pInfo->GetKey() );

            //y ahora las ramas hijas de m_pRootItem
            int iNumChilds = m_pRootItem->m_childItemsArray.GetSize();
            HTREEITEM hItem;
            for( int i=0; i<iNumChilds && bSaveOK; i++ ){
                hItem   = m_pRootItem->m_childItemsArray.GetAt(i);
                bSaveOK = xSaveShapeTo( pArray, hItem );
            }

        } else {

            //AfxMessageBox(_T("Failed ASSERT : CGenericTreeCtrl::SaveShapeTo => m_pRootItem==NULL! "));
            return false;
        }
    }

    return bSaveOK;
}
bool CxTreeCtrl::xSaveShapeTo(CArray<CString,CString> * pArray, HTREEITEM hItem )
{
    bool bSaveOK = true;

    if( hItem==NULL ){
        //AfxMessageBox(_T("Failed ASSERT : CGenericTreeCtrl::xSaveShapeTo => hItem==NULL! "));
        ASSERT(0);
        return false;
    }

    CTreeItemInfo * pTreeItemInfo   = (CTreeItemInfo*) this->GetItemData( hItem );
    if( pTreeItemInfo!=NULL ){

        if( pTreeItemInfo->IsExpanded() ){
            pArray->Add( pTreeItemInfo->GetKey() );
        }

    } else {
        bSaveOK = false;
    }

    if( bSaveOK ){

        HTREEITEM hNextItem     = this->GetChildItem( hItem );
        while( hNextItem!=NULL && bSaveOK ){
            bSaveOK                 = xSaveShapeTo( pArray, hNextItem );
            hNextItem       = this->GetNextItem( hNextItem, TVGN_NEXT);
        }
    }

    return bSaveOK;
}
bool CxTreeCtrl::xSaveShape( HTREEITEM hItem )
{
    bool bSaveOK = true;
    if(!hItem){
        return false;
    }

    CTreeItemInfo * pTreeItemInfo   = (CTreeItemInfo*) GetItemData( hItem );
    if( pTreeItemInfo!=NULL ){
        pTreeItemInfo->setExpanded(GetItemState(hItem,TVIS_EXPANDED) & TVIS_EXPANDED );
    } else {
        bSaveOK = false;
    }

    if( bSaveOK ){

        HTREEITEM hNextItem     = this->GetChildItem( hItem );
        while( hNextItem!=NULL && bSaveOK){
            bSaveOK = xSaveShape( hNextItem );
            hNextItem = this->GetNextItem( hNextItem, TVGN_NEXT);
        }
    }

    return bSaveOK;
}
//Vuelca su estado prendido/apagado sobre elemento CTreeItemInfo
bool CxTreeCtrl::SaveShape()
{
    bool bSaveOK = true;

    bool bSingleRoot = m_bSingleRoot;
    if( bSingleRoot ){

        HTREEITEM hItem = this->GetRootItem();
        xSaveShape( hItem );

    } else {

        if( m_pRootItem ){

            //por definición, m_pRootItem está expandido, ya que está en el título de la ventana y nosotros estamos viendo sus hijos.
            m_pRootItem->m_pInfo->setExpanded( true );

            //ahora se guarda la forma de cada rama hija de m_pRootItem.
            int iNumChilds = m_pRootItem->m_childItemsArray.GetSize();
            HTREEITEM hItem;
            for( int i=0; i<iNumChilds && bSaveOK; i++ ){
                hItem = m_pRootItem->m_childItemsArray.GetAt(i);
                bSaveOK = xSaveShape( hItem );
            }

        } else {

            //AfxMessageBox(_T("Failed ASSERT : CGenericTreeCtrl::SaveShape => m_pRootItem==NULL! "));
            ASSERT(0);
        }
    }

    return bSaveOK;
}
void CxTreeCtrl::xExpand(HTREEITEM hItem, bool bExpand)
{
    if( bExpand ){

        Expand( hItem, TVE_EXPAND );

        HTREEITEM hParent = GetParentItem( hItem );

        while(hParent){

            Expand( hParent, TVE_EXPAND );
            hParent = GetParentItem( hParent );

        }

    } else {

        Expand(hItem, TVE_COLLAPSE );
    }
}
int CxTreeCtrl::GetNumExpandedItems()
{
    //SaveShape();
    CArray<CString,CString> csArray;
    SaveShapeTo(&csArray);
    int             iSize = csArray.GetSize();
    return  iSize;
}

CString CxTreeCtrl::GetShape(CString csKeySeparator){
        CString csTreeShape;
        CArray<CString,CString> aShape;
        if(SaveShapeTo(&aShape)){
                int iNumShapeAttoms = aShape.GetSize();
                for(int iShapeAttomIdx=0; iShapeAttomIdx<iNumShapeAttoms; iShapeAttomIdx++){
                        csTreeShape += aShape.GetAt(iShapeAttomIdx);
                        if(iShapeAttomIdx<iNumShapeAttoms-1){
                                csTreeShape += csKeySeparator;
                        }
                }
        }
        return csTreeShape;
}


//SCROLL FUNCTIONS
void CxTreeCtrl::xSetScrollPos( int iHScrollPos, int iVScrollPos )
{
    SCROLLINFO  ScrollInfoH, ScrollInfoV;

    GetScrollInfo(SB_HORZ,&ScrollInfoH);
    ScrollInfoH.nPos  = iHScrollPos;
    SetScrollInfo(SB_HORZ,&ScrollInfoH);

    GetScrollInfo(SB_VERT,&ScrollInfoV);
    ScrollInfoV.nPos = iVScrollPos;
    SetScrollInfo(SB_VERT,&ScrollInfoV);
}



//DESTROY FUNCTIONS

void CxTreeCtrl::DestroyItemInfo(HTREEITEM hItem, bool bRecursiveChilds)
{
    if(!hItem){
            return;
    }

    //Destroy the info associated to the given hitem
    CTreeItemInfo * pItemInfo = (CTreeItemInfo*) this->GetItemData( hItem );
    if( pItemInfo!=NULL ){
        delete( pItemInfo );
        pItemInfo = NULL;
        SetItemData(hItem,(DWORD)NULL);
    }

    //and recursive destroy the info of every hitem childs
    if(bRecursiveChilds){
        HTREEITEM hNextItem = GetChildItem(hItem);
            while( hNextItem ){
                    DestroyItemInfo( hNextItem, true );
                    hNextItem = GetNextItem( hNextItem, TVGN_NEXT);
            }
    }
}
void CxTreeCtrl::DeleteChilds( HTREEITEM hItem )
{
    if( !hItem || !ItemHasChildren(hItem) ){
        return;
    }

    CArray<HTREEITEM,HTREEITEM> hItemArray;
    HTREEITEM hDirectChildItem = GetChildItem(hItem);
    while( hDirectChildItem ){
        hItemArray.Add( hDirectChildItem );
        hDirectChildItem = GetNextItem( hDirectChildItem, TVGN_NEXT);
    }

    int iSize = hItemArray.GetSize();
    for(int i=0; i<iSize; i++){
        HTREEITEM hItemToDelete = hItemArray.GetAt(i);
        DestroyItemInfo(hItemToDelete, true);
        DeleteItem( hItemToDelete );
    }
}


//
int CxTreeCtrl::GetTypeOfTree()
{
    return m_iTypeOfTree;
}


//
int CxTreeCtrl::OnToolHitTest(CPoint point, TOOLINFO * pTI) const
{
    RECT rect;

    UINT nFlags;
    HTREEITEM       hitem = HitTest( point, &nFlags );
    if(hitem){
        CTreeItemInfo * pItemInfo = (CTreeItemInfo*) GetItemData( hitem );

        ASSERT( pItemInfo );


        if( nFlags & TVHT_ONITEMLABEL  ){

            GetItemRect( hitem, &rect, TRUE );

            ASSERT( pTI );

            if( pTI==NULL )  return -1;

            pTI->hwnd       = m_hWnd;
            pTI->uId        = (UINT)hitem;
            pTI->lpszText   = _tcsdup(pItemInfo->GetToolTip());   // MFC will free this pointer!
            pTI->rect       = rect;

            return pItemInfo->GetToolTip().GetLength()>0 ? pTI->uId : -1;
        }

    }

    return -1;
}

bool CxTreeCtrl::IsVisible(HTREEITEM hItem)
{
    bool bVisible = false;

    HTREEITEM hItemAux = GetFirstVisibleItem();
    while( !bVisible && hItemAux!=NULL){

        if( hItemAux==hItem){
            bVisible = true;
        } else {
            hItemAux = GetNextVisibleItem(hItemAux);
        }
    }

    return bVisible;
}

void CxTreeCtrl::SetXRootItem( CTreeItem * pRootItem )
{
    if( !m_pRootItem ){
        m_pRootItem     = pRootItem;
    }
}
CTreeItem * CxTreeCtrl::GetXRootItem()
{
    return m_pRootItem;
}

void CxTreeCtrl::OnShowWindow(BOOL bShow, UINT nStatus)
{
    CTreeCtrl::OnShowWindow(bShow, nStatus);
}


//VIRTUAL FUNCTIONS

/*virtual*/
bool CxTreeCtrl::OnBeforeSetFocus(CWnd* pOldWnd)
{
    return true;
}
/*virtual*/
void CxTreeCtrl::OnAfterSetFocus(CWnd* pOldWnd)
{
}

/*virtual*/
bool CxTreeCtrl::OnBeforeKillFocus(CWnd* pNewWnd)
{
    return true;
}
/*virtual*/
void CxTreeCtrl::OnAfterKillFocus(CWnd* pNewWnd)
{
}

/*virtual*/
bool CxTreeCtrl::OnBeforeKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
    return true;
}
/*virtual*/
void CxTreeCtrl::OnAfterKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
}

/*virtual*/
bool CxTreeCtrl::OnBeforeKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags)
{
    return true;
}
/*virtual*/
void CxTreeCtrl::OnAfterKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags)
{
}

/*virtual*/
bool CxTreeCtrl::OnBeforeLButtonDown(UINT nFlags, CPoint point){
    return true;
}


/*virtual*/
bool CxTreeCtrl::OnBeforeLButtonDblClk  (UINT nFlags, CPoint point){
    return true;
}
/*virtual*/
void CxTreeCtrl::OnAfterLButtonDblClk   (UINT nFlags, CPoint point){
}

/*virtual*/
bool CxTreeCtrl::OnBeforeRButtonDown    (UINT nFlags, CPoint point){
    return true;
}
/*virtual*/
void CxTreeCtrl::OnAfterRButtonDown     (UINT nFlags, CPoint point){
}

/*virtual*/
void CxTreeCtrl::OnAfterItemExpanded(NMHDR* pNMHDR, LRESULT* pResult){
}
