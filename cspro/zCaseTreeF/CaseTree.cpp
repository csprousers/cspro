// CaseTree.cpp : implementation file
//
#include "StdAfx.h"
#include "CaseTree.h"
#include "CEUtils.h"
#include "GTCtrl.h"
#include "IItemCnf.h"
#include "CRunAplE.h"
#include <engine/Tables.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[]= __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// CCaseTree
CCaseTree::CCaseTree(   int                 iTypeOfParentWindow,
                        int                 iTypeOfTree,
                        bool                bIsInToggleView,
                        CDEField*           pWantedField,
                        int                 iWantedOcc,
                        CWinApp*            pWinApp,
                        CCapiRunAplEntry*   pCapiRunAplEntry,
                        CWnd*               pParent)
{
    m_pCapiRunAplEntry              = pCapiRunAplEntry;
    m_pWinApp                       = pWinApp;
    m_pParent                       = pParent;
    m_bFinishCase                   = false;
    m_iTypeOfParentWindow           = iTypeOfParentWindow;
    m_iTypeOfTree                   = iTypeOfTree;
    m_bIsInToggleView               = bIsInToggleView;
    m_pRootItem                     = NULL;
    m_bFullTreeOfQuestionsAnswers   = true;
    m_pTreeFont                     = NULL;
    m_pLastHITEMParent              = NULL;
    m_pLastInsertedItem             = NULL;
    m_bLockedStack                  = false;
    m_pWantedField                  = pWantedField;
    m_iWantedOcc                    = iWantedOcc;
    m_treeFilterType                = TreeFilterType::None;
    m_pValidField                   = NULL;
    m_iValidOcc                     = -1;
    m_pLastSelectedField            = NULL;
    m_iLastSelectedOcc              = -1;
    m_bRecursiveClose               = false;
    m_iToggleTreeLayers             = 0;
    m_pImageList                    = NULL;
    m_pTree                         = NULL;
    m_pTreeArray                    = NULL;
    m_iNumberOfTogglePressed        = 0;
    m_bRefreshLocked                = false;
    m_iLastOpenNodeIdx              = -1;
    m_ptLastScrollPos.x             = -1;
    m_ptLastScrollPos.y             = -1;

    Create(NULL,_T(""),WS_VISIBLE|WS_CHILD , CRect(0,0,0,0), m_pParent, 1234 );

    InitFonts();

    m_bIsClean          = false;
    m_bVerifyFlag       = true;
    m_bFilteredTree     = false;
    m_pWhiteBackGround  = NULL;
    m_pShape            = NULL;
    m_bShowNames        = true;
}

CCaseTree::~CCaseTree()
{
    if(m_pTreeArray){
        int iSize = m_pTreeArray->GetSize();
        CGenericTreeCtrl* pTree;
        for( int i=0; i<iSize; i++){
            pTree = m_pTreeArray->GetAt(iSize-i-1);
            if( IsWindow(pTree->GetSafeHwnd()) ){
                pTree->ShowWindow(SW_HIDE);
            }
            delete( pTree );
            m_pTreeArray->SetAt(iSize-i-1,NULL);
        }
        m_pTreeArray->RemoveAll();
        delete( m_pTreeArray );
        m_pTreeArray = NULL;
    }

    if(m_pImageList){
        m_pImageList->DeleteImageList();
        delete(m_pImageList);
        m_pImageList = NULL;
    }

    if(m_pTreeFont){
        delete(m_pTreeFont);
        m_pTreeFont = NULL;
    }

    if(m_pWhiteBackGround){
        delete(m_pWhiteBackGround);
        m_pWhiteBackGround = NULL;
    }

    if(m_pShape){
        m_pShape->RemoveAll();
        delete(m_pShape);
        m_pShape = NULL;
    }
}


BEGIN_MESSAGE_MAP(CCaseTree, CWnd)
    //{{AFX_MSG_MAP(CCaseTree)
    ON_MESSAGE(UWM::CaseTree::SelectTreeItem, OnSelectTreeItem )
    ON_MESSAGE(UWM::CaseTree::UpdateWindowText, OnUpdateWindowText)
    ON_MESSAGE(UWM::CaseTree::CloseAllTreeLayers, OnCloseAllTreeLayers)
    ON_MESSAGE(UWM::CaseTree::RestoreTree, OnRestoreTree)
    ON_MESSAGE(UWM::CaseTree::CloseCurrentTreeLayer, OnCloseCurrentTreeLayer)
    ON_MESSAGE(UWM::CaseTree::Refresh, OnRefresh)
    ON_MESSAGE(UWM::CaseTree::UnknownKey, OnUnknownKey)
    ON_MESSAGE(UWM::CaseTree::ShowWindow, OnShowWindow)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

void CCaseTree::GetWindowRect(LPRECT lpRect ) const
{
        lpRect->left    = m_ixo;
        lpRect->top             = m_iyo;
        lpRect->right   = m_ix1;
        lpRect->bottom  = m_iy1;
}
void CCaseTree::MoveWindow( int x, int y, int nWidth, int nHeight, BOOL bRepaint)
{
        //Store new coordinates
        m_ixo = x;
        m_iyo = y;
        m_ix1 = x + nWidth;
        m_iy1 = y + nHeight;

    if( m_pWhiteBackGround && IsWindow(m_pWhiteBackGround->GetSafeHwnd()) ){
        m_pWhiteBackGround->MoveWindow(m_ixo,m_iyo,m_ix1-m_ixo,m_iy1-m_iyo,bRepaint);
    }

    int iNumLayers = GetNumTreeLayers();
    for(int i=0; i<iNumLayers; i++){
            m_pTreeArray->GetAt(i)->MoveWindow(m_ixo,m_iyo,m_ix1-m_ixo,m_iy1-m_iyo,bRepaint);
    }
}

void CCaseTree::MoveWindow( LPCRECT lpRect, BOOL bRepaint )
{

        //Store new coordinates
        m_ixo = lpRect->left;
        m_iyo = lpRect->top;
        m_ix1 = lpRect->right;
        m_iy1 = lpRect->bottom ;

        CRect rect(m_ixo,m_iyo,m_ix1,m_iy1);

    if( m_pWhiteBackGround && IsWindow(m_pWhiteBackGround->GetSafeHwnd()) ){
        m_pWhiteBackGround->MoveWindow(rect,bRepaint);
    }

    int iNumLayers = GetNumTreeLayers();
    for(int i=0; i<iNumLayers; i++){
            m_pTreeArray->GetAt(i)->MoveWindow(rect,bRepaint);
    }
}

CWnd* CCaseTree::SetFocus()
{
        return m_pTree ? m_pTree->SetFocus() : NULL;
}

LRESULT CCaseTree::OnShowWindow(WPARAM wParam, LPARAM lParam)
{
    bool bShow = (bool) wParam;

    int iNumLayers = GetNumTreeLayers();
    if( iNumLayers==0){
        return FALSE;
    }

    if(!bShow){
        for(int iLayerIdx=0; iLayerIdx<iNumLayers; iLayerIdx++){
            m_pTreeArray->GetAt(iLayerIdx)->ShowWindow( SW_HIDE );
        }
    } else {
        m_pTreeArray->GetAt(iNumLayers-1)->ShowWindow(SW_NORMAL);
    }

    return true;
}


LRESULT CCaseTree::OnSelectTreeItem(WPARAM wParam, LPARAM )
{
    CMsgParam*      pMsgParam = (CMsgParam*) wParam;

    ASSERT(pMsgParam->dwArrayParam.GetSize()>=2 );

    CPoint      ptScrollPos( (int) pMsgParam->dwArrayParam.GetAt(0), (int) pMsgParam->dwArrayParam.GetAt(1) );

    UserSelectItem( pMsgParam->hParam, ptScrollPos, pMsgParam->bParam);

    if( pMsgParam && pMsgParam->bMustBeDestroyedAfterLastCatchMessage ){
            delete(pMsgParam);
            pMsgParam = NULL;
    }

    return TRUE;
}

LRESULT CCaseTree::OnUpdateWindowText(WPARAM wParam, LPARAM /*lParam*/)
{
    CMsgParam*  pMsgParam = (CMsgParam*) wParam;

    m_pParent->SendMessage(UWM::CaseTree::UpdateWindowText, (WPARAM)pMsgParam, 0);

    if( pMsgParam && pMsgParam->bMustBeDestroyedAfterLastCatchMessage ){
            delete(pMsgParam);
            pMsgParam = NULL;
    }

    return TRUE;
}

LRESULT CCaseTree::OnCloseAllTreeLayers(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    CloseDialog();
    return TRUE;
}

LRESULT CCaseTree::OnRestoreTree(WPARAM wParam, LPARAM /*lParam*/)
{
    Restore();
    return TRUE;
}

LRESULT CCaseTree::OnCloseCurrentTreeLayer(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    CloseCurrentTreeLayer();
    return TRUE;
}

LRESULT CCaseTree::OnRefresh(WPARAM wParam, LPARAM lParam)
{
    if(m_bRefreshLocked)
        return false;

    ASSERT(m_pCapiRunAplEntry);

    CMsgParam*  pMsgParam    = NULL;
    HTREEITEM   hItem        = NULL;
    CString     csLastSelKey = _T("");
    CDEField*   pWantedField = NULL;
    int         iWantedOcc   = -1;
    bool        bDestroyArray= false;

    CArray<CDEItemBase*,CDEItemBase*>*          pItemBaseArray      = NULL;
    CArray<CArray<int,int>*,CArray<int,int>*>*  pOccsToRefreshArray = NULL;

    if(lParam==0){

        pMsgParam        = (CMsgParam*) wParam;                  ASSERT(pMsgParam->dwArrayParam.GetSize()==1);
        hItem            = pMsgParam->hParam;
        csLastSelKey     = pMsgParam->csParam;
        pWantedField     = (CDEField*) pMsgParam->dwArrayParam.GetAt(0);
        iWantedOcc       = pMsgParam->iParam;

    } else if(lParam==-1) {

        CDEItemBase* pItemBase  = (CDEItemBase*) wParam;
        pWantedField            = pItemBase ? (CDEField*) pItemBase : NULL;
        iWantedOcc              = pWantedField ? pWantedField->GetParent()->GetCurOccurrence() : -1;

    } else if(lParam==-2) {

        pMsgParam           = (CMsgParam*) wParam; ASSERT( pMsgParam->dwArrayParam.GetSize()==3 );
        pWantedField        = (CDEField*)                       pMsgParam->dwArrayParam.GetAt(0);
        iWantedOcc          = pWantedField ? pWantedField->GetParent()->GetCurOccurrence() : -1;
        pItemBaseArray      = (CArray<CDEItemBase*,CDEItemBase*>*)          pMsgParam->dwArrayParam.GetAt(1);
        pOccsToRefreshArray = (CArray<CArray<int,int>*,CArray<int,int>*>*) pMsgParam->dwArrayParam.GetAt(2);

    } else if(lParam==-3){

        CArray<DEFLD,DEFLD>* pDEFLDArray = (CArray<DEFLD,DEFLD>*) wParam;

        pItemBaseArray = new CArray<CDEItemBase*,CDEItemBase*>;

        int iSize = pDEFLDArray->GetSize();
        for(int i=0; i<iSize; i++){
            pItemBaseArray->Add( m_pCapiRunAplEntry->GetDeField(&pDEFLDArray->ElementAt(i)) );
        }

        pWantedField    = m_pWantedField;
        iWantedOcc      = m_iWantedOcc;
        csLastSelKey    = m_csWantedKey;
        bDestroyArray   = true;
    }

    if(pWantedField || csLastSelKey.GetLength()>0 ){

        Refresh( hItem, csLastSelKey, pWantedField, iWantedOcc, pItemBaseArray, pOccsToRefreshArray );  // If pItemBaseArray==NULL, then there is a full refresh (pOccsToRefreshArray is ignored)
    }

    if(bDestroyArray && pItemBaseArray){
        pItemBaseArray->RemoveAll();
        delete(pItemBaseArray);
        pItemBaseArray = NULL;
    }

    if( pMsgParam && pMsgParam->bMustBeDestroyedAfterLastCatchMessage ){
        delete( pMsgParam );
        pMsgParam = NULL;
    }

    if(!pWantedField && csLastSelKey.GetLength()==0 ){
        m_pParent->PostMessage(UWM::CaseTree::DeleteCaseTree, 0, 0);
    }

    return TRUE;
}

LRESULT CCaseTree::OnUnknownKey(WPARAM wParam, LPARAM lParam)
{
    m_pParent->PostMessage(UWM::CaseTree::UnknownKey, wParam, lParam);
    return TRUE;
}

bool CCaseTree::ShowTreeOfNodes(bool bDisplayNodeChilds, CDEField* pWantedField, int iWantedOcc )
{
    bool bSearchOK     = false;

    bDisplayNodeChilds = true;

    if(!m_pTree){
        return NULL;
    }

    CArray<CHITEM*,CHITEM*> hitemArray;
    CHITEM*                 chitem      = NULL;
    CString                 csCaseIds   = _T("");
    int                     iNumNodes   = m_pCapiRunAplEntry->GetNNodes();

    //pre scan for calculate icon idxs :
    //iArray.GetAt(j) is the index of the non selected icon for node j.
    int     iNodeIdx;
    int iIconIdx = CaseTreeIcons::GetIconIndex(IDI_BLUE_NODE_ICON);
    CArray<int,int> iArray;
    for( iNodeIdx=0; iNodeIdx<iNumNodes; iNodeIdx++ ){
        iArray.Add(-1);
    }
    for( iNodeIdx=0; iNodeIdx<iNumNodes; iNodeIdx++){
        if(m_pCapiRunAplEntry->IsOpen(iNodeIdx)){
            iArray.SetAt(iNodeIdx, CaseTreeIcons::GetIconIndex(IDI_GREEN_NODE_ICON));
        }
    }
    for( iNodeIdx=iNumNodes-1; iNodeIdx>=0; iNodeIdx--){
        if(iArray.GetAt(iNodeIdx)==CaseTreeIcons::GetIconIndex(IDI_GREEN_NODE_ICON)){
            iIconIdx = CaseTreeIcons::GetIconIndex(IDI_GREEN_NODE_PADLOCK_ICON);
        } else if(iArray.GetAt(iNodeIdx)==-1){
            iArray.SetAt(iNodeIdx,iIconIdx);
        }
    }

    //
    for( iNodeIdx=0; iNodeIdx<iNumNodes; iNodeIdx++ ){

        chitem                          = new CHITEM(NULL);

        chitem->m_csToolTip             = chitem->m_csLabel;
        chitem->m_iLevelIdx             = m_pCapiRunAplEntry->GetLevel(iNodeIdx)-1;
        chitem->m_csLabel               = m_pCapiRunAplEntry->GetNodeLabel(iNodeIdx);
        chitem->m_iNonSelectedIconIdx   = iArray.GetAt(iNodeIdx);
        chitem->m_iSelectedIconIdx      = iArray.GetAt(iNodeIdx);
        chitem->m_pInfo                 = new CTreeItemInfo( iNodeIdx,
                                                             IntToString(iNodeIdx),
                                                             iNodeIdx,
                                                             CS_NODE_INFO,
                                                             -1,
                                                             -1,
                                                             -1,
                                                             chitem->m_iNonSelectedIconIdx,
                                                             chitem->m_iSelectedIconIdx,
                                                             chitem->m_csToolTip );
        hitemArray.Add(chitem);
    }


        bSearchOK = xInsertItems( &hitemArray , _T("Tree of nodes"), pWantedField, iWantedOcc);


    for(iNodeIdx=0; iNodeIdx<iNumNodes; iNodeIdx++){
        delete(hitemArray.GetAt(iNodeIdx));
        hitemArray.SetAt(iNodeIdx,NULL);
    }

    if(m_pTree->GetRootItem()){
        bool bExpandResult = (m_pTree->Expand( m_pTree->GetRootItem(), TVE_EXPAND ) != 0 );
        if(!bExpandResult){
            TRACE(_T("bExpandResult = false\n"));
        }
    }

    if( m_InsertItemConfigArray.GetSize()>0 ){
        DoStackCalls();
    }

    return bSearchOK;
}

bool CCaseTree::xInsertItems(CArray<CHITEM*,CHITEM*>* pInOrderArray, CString csRootItemLabel, CDEField* pWantedField, int iWantedOcc)
{
    bool bSearchOK  = false;

    if(!pInOrderArray){
        return false;
    }

    int     iSize       = pInOrderArray->GetSize();

    //iNumRoots corresponde a la cantidad de items que originalmente deben colgar de NULL <=> iLevelIdx==0
    int iNumRoots   = 0;
    for(int iItemIdx=0; iItemIdx<iSize; iItemIdx++){
        if(pInOrderArray->GetAt(iItemIdx)->GetLevelIdx()==0){
            iNumRoots++;
        }
    }
    if(iNumRoots>1){
        ASSERT(0);
        //AfxMessageBox(_T("CGenericTreeCtrl::InsertItems : Not implemented for multi root trees"));
        return false;
    }

    CHITEM*         chitem;
    int             iLevelIdx;
    HTREEITEM       hParent         = NULL;
    HTREEITEM       hLastItem       = NULL;
    int             iLastLevelIdx   = -1;
    int             iNumBackSteps   = 0;
    for(int i=0; i<iSize; i++){

    //i <==> iNodeIdx

        chitem                  = pInOrderArray->GetAt(i);
        iLevelIdx               = chitem->GetLevelIdx();
        iNumBackSteps   = 1+iLastLevelIdx-iLevelIdx;
        hParent                         = hLastItem;

        for(int j=0; j<iNumBackSteps && hParent; j++){
                hParent = hParent ? m_pTree->GetParentItem(hParent) : NULL;
        }

        hLastItem = m_pTree->InsertNode(chitem,hParent);
        if( InsertNodeVars( i, m_pTree,hLastItem, pWantedField, iWantedOcc) ){
                bSearchOK = true;
        }

        iLastLevelIdx   = iLevelIdx;
    }

    return bSearchOK;
}

bool CCaseTree::DoStackCalls()
{
        m_bLockedStack  = true;

        bool bSearchOK = false;

        int iNumCalls = m_InsertItemConfigArray.GetSize();


        CDEItemBase*    pItem;
        CString                         csFieldLabel;
        int                             iFieldLabelLength;
        int                             iMaxLength = 0;
        for(int iCallIdx=0; iCallIdx<iNumCalls; iCallIdx++){
                pItem = m_InsertItemConfigArray.GetAt(iCallIdx)->m_pItem;
                ASSERT( pItem && pItem->GetItemType() == CDEFormBase::Field );
                if( m_bShowNames ){
                        csFieldLabel    = ((CDEField*)pItem)->GetDictItem()->GetName();
                } else {
                        csFieldLabel    = ((CDEField*)pItem)->GetDictItem()->GetLabel();
                }
                iFieldLabelLength       = csFieldLabel.GetLength();
                if(iFieldLabelLength>iMaxLength){
                        iMaxLength = iFieldLabelLength;
                }
        }


        for(int i=0; i<iNumCalls; i++){

            InsertItemConfig* InsertParams  = m_InsertItemConfigArray.GetAt(i);

            InsertParams->m_iMaxItemLength  = iMaxLength;

            if( InsertMultiOccItem( InsertParams->m_iNodeIdx,
                                    InsertParams->m_pItem,
                                    InsertParams->m_iItemIndex,
                                    InsertParams->m_iItemOcc,
                                    InsertParams->m_hItemParent,
                                    InsertParams->m_hInsertAfter,
                                    InsertParams->m_pWantedField,
                                    InsertParams->m_iWantedOcc,
                                    InsertParams->m_iMaxItemLength,
                                    InsertParams->m_bSpecialInsert,
                                    InsertParams->m_bOccIcon) ){
                    bSearchOK = true;
            }

            delete(InsertParams);
            InsertParams=NULL;
            m_InsertItemConfigArray.SetAt(i,NULL);

        }
        m_InsertItemConfigArray.RemoveAll();

        m_bLockedStack = false;

        return bSearchOK;
}

bool CCaseTree::InsertNodeVars(int iNodeIdx, CGenericTreeCtrl* pTree, HTREEITEM hItem, CDEField* pWantedField, int iWantedOcc)
{
    bool bSearchOK = false;

        if(!hItem){
                return false;
        }

        CTreeItemInfo* pItemInfo = (CTreeItemInfo*) pTree->GetItemData( hItem );
        if(!pItemInfo){
                return false;
        }

        //int                           iNodeIdx        = pItemInfo->GetIndex();
        bool                    bIsOpen                 = m_pCapiRunAplEntry->IsOpen(iNodeIdx);
        if( bIsOpen ){

                int             iLevelIdx       = m_pCapiRunAplEntry->GetLevel(iNodeIdx)-1;
                CDEFormFile*    pFormFile       = m_pCapiRunAplEntry->GetDEFormFile(false);
                CDELevel*       pLevel          = pFormFile ? pFormFile->GetLevel( iLevelIdx ) : NULL;

                bSearchOK                               = InsertLevel( pItemInfo->GetKey(), iNodeIdx, pLevel, hItem , iLevelIdx, pWantedField, iWantedOcc , true);
        }

        return bSearchOK;
}


/*Inserta el item pItem colgando de hItemParent.
  Cuando el item tiene una sola ocurrencia => cuelga directamente de hItemParent.
  Si el item tiene N ocurrencias => bajo hItemParent se cuelga un hItem que a su vez será el item padre de cada una de las N ocurrencias.*/
bool CCaseTree::InsertMultiOccItem(  int            iNodeIdx,
                                     CDEItemBase*   pItem,                  //
                                     int            iItemIndex,
                                     int            iItemOcc,
                                     HTREEITEM      hItemParent,
                                     HTREEITEM      hInsertAfter,
                                     CDEField*      pWantedField,
                                     int            iWantedOcc,
                                     int            iMaxItemLength,
                                     bool           bSpecialInsert,
                                     bool           bOccIcon)
{
    CString csParentKey = m_pTree->GetKey( hItemParent );

    if(hItemParent!=NULL){
        CTreeItemInfo* pItemInfoParent = (CTreeItemInfo*) m_pTree->GetItemData(hItemParent);
        int iTypeOfInfo = pItemInfoParent->GetTypeOfInfo();
        if( iTypeOfInfo==CS_GROUP_OCCURRENCES_TITTLE ){
            csParentKey = m_pTree->GetKey( m_pTree->GetParentItem(hItemParent) );
        }
    }




    bool    bSearchOK = false;

    /*                      CDEItemBase
                                     |
                     +---------------+
                     |               |
              CDEGroup        CDEField
                     |
                CDERoster
    */


    /*Paso 1 : averiguar si es single o múltiple para determinar tipo de inserción.

      Inserción single :

                    hItemParent
                     |
                     + csItemLabel/csItemName


      Inserción múltiple : inserta todas las ocurrencias del item

                    hItemParent
                     |
                     + csItemLabel/csItemName : n occurrences
                             |
                             + (1)
                             + (2)
                             :
                             + (n)

      Inserción especial : el item es múltiple, pero se quiere refrescar una ocurrencia en particular

                    hItemParent
                     |
                     + ...
                     + ...
                     + (k)
                        |
                        + item child 1
                        + item child 2
                        + ...
                     + ...


    */


    int                     iDataOccs=0;
    int                     iNumDrawableOccs;
    CDEFormBase::eItemType  eTypeOfItem;

    eTypeOfItem = pItem->GetItemType();
    switch( eTypeOfItem ){

    case CDEFormBase::Roster :{  iDataOccs           = ((CDEGroup*) pItem)->GetDataOccs();
                                 iNumDrawableOccs    = ((CDEGroup*) pItem)->GetMaxLoopOccs();       } break;

    case CDEFormBase::Group  :{  iDataOccs           = ((CDEGroup*) pItem)->GetDataOccs();
                                 iNumDrawableOccs    = ((CDEGroup*) pItem)->GetMaxLoopOccs();       } break;

    case CDEFormBase::Field  :{  iDataOccs           = 1;//pItem->GetParent()->GetDataOccs();
                                 m_pValidField       = (CDEField*) pItem;
                                 m_iValidOcc         = iItemOcc;
                 } break;
    }

    iNumDrawableOccs  = iDataOccs;

    HTREEITEM hItem   = NULL;
    CString   csItemLabel;
    int       iNonSelectedIconIndex;
    int       iSelectedIconIndex;
    int       iIconColor;
    CString   csToolTip;


    if( iNumDrawableOccs==1 || bSpecialInsert ){


        //Analizamos si este es el Field y ocurrencia buscados
        int iOccAux = iItemOcc!=-1 ? iItemOcc : 1;
        if( pItem && pItem->GetItemType() == CDEFormBase::Field && (((CDEField*)pItem)==pWantedField) && iOccAux==iWantedOcc ){
            bSearchOK = true;
        }

        //Todos los fields consecutivos, hijos de un mismo padre se guardan primero en un stack (m_bLockedStack=false)
        //y posteriormente se ingresan al árbol (m_bLockedStack=true)
        if( !m_bLockedStack ){

            if( NeedToPutInStack( pItem,hItemParent, m_pLastInsertedItem, m_pLastHITEMParent ) ){

                InsertItemConfig* pItemConfig = new InsertItemConfig(iNodeIdx, pItem, iItemIndex, iItemOcc, hItemParent, hInsertAfter, csParentKey, pWantedField, iWantedOcc, iMaxItemLength, bSpecialInsert, bOccIcon); ASSERT( bSpecialInsert==false );
                m_InsertItemConfigArray.Add( pItemConfig );
                m_pLastInsertedItem = pItem;
                m_pLastHITEMParent  = hItemParent;


                //cuando esto ocurre, el pItem es un CDEField => no tiene hijos. => no hay problema en no llamar a InsertItemChilds
                return bSearchOK;

            } else {

                if(m_InsertItemConfigArray.GetSize()>0){

                    //cuando existe acumulación de llamadas en el stack hay que vaciarlo.
                    if( DoStackCalls() ){
                        bSearchOK = true;
                    }

                    //ahora se analiza si el elemento actual debe ingresarse al stack
                    if(pItem->GetItemType() == CDEFormBase::Field){
                        m_pLastInsertedItem = pItem;
                        m_pLastHITEMParent  = hItemParent;
                        if( NeedToPutInStack( pItem,hItemParent, m_pLastInsertedItem, m_pLastHITEMParent ) ){
                            InsertItemConfig* pItemConfig = new InsertItemConfig(iNodeIdx, pItem, iItemIndex, iItemOcc, hItemParent, hInsertAfter, csParentKey, pWantedField, iWantedOcc, iMaxItemLength, bSpecialInsert, bOccIcon);  ASSERT( bSpecialInsert==false );
                            m_InsertItemConfigArray.Add( pItemConfig );
                            return bSearchOK;
                        }
                    }
                }
            }
        }

        ///////////////////////////////////
        if(m_pCapiRunAplEntry->InVerify()){
            CDEItemBase* pCurItem = m_pCapiRunAplEntry->GetCurItemBase();
            bool bItemMatch = pCurItem && pCurItem->GetItemType() == CDEFormBase::Field && pItem && pItem->GetItemType() == CDEFormBase::Field && pCurItem==pItem;
            if( bItemMatch ){
                int iOccMatch = pCurItem->GetParent()->GetCurOccurrence() == (iItemOcc!=-1 ? iItemOcc : 1) ;
                if(iOccMatch){
                    m_bVerifyFlag = false;
                }
            }
        }
        //////////////////////////////////


        //CASO 1 : EL ITEM CUELGA DIRECTAMENTE DE hItemParent
        /*
                hItemParent
                 |
                 + csItemLabel
        */
        m_pCapiRunAplEntry->GetInfo(m_bVerifyFlag,
                                    //iNodeIdx,
                                    pItem,
                                    iItemOcc!=-1 ? iItemOcc : 1,
                                    iMaxItemLength,
                                    m_bShowNames,
                                    NULL,
                                    &iNonSelectedIconIndex,
                                    &iSelectedIconIndex,
                                    &iIconColor,
                                    &csItemLabel,
                                    &csToolTip  );

        //FABN Feb 14, 2003
        if(bSpecialInsert && bOccIcon){
            int     iNumDigits          = (IntToString(iNumDrawableOccs)).GetLength();
            CString csOccNumber         = IntToString( iItemOcc/*iWantedOcc*/ );
            int     iNumZeros           = iNumDigits - csOccNumber.GetLength();
            CString csFormatedNumber    = _T("");
            for(int i=0; i<iNumZeros; i++) csFormatedNumber += _T("0");
            csFormatedNumber += csOccNumber;
            CString csOccTittle   = _T("(") + csFormatedNumber + _T(")");

            if( bOccIcon ){
                csItemLabel             = csOccTittle;
                            iNonSelectedIconIndex   = iItemOcc <= iDataOccs ? 32 : 30;
                iSelectedIconIndex      = 1 + iNonSelectedIconIndex;
            }

                        if(m_bShowNames){
                csToolTip = pItem->GetLabel() + _T(" ") + csOccTittle;
            } else {
                csToolTip = pItem->GetName() + _T(" ") + csOccTittle;
            }
        }

        //CString csOldNote   = m_pCapiRunAplEntry->GetNote( pItem, iOcc );

        CString csKey = csParentKey + _T("-") + IntToString(pItem->GetSymbol()) + /*"#" + IntToString(iItemIndex) +*/ _T("#");
                if( bSpecialInsert ){
                        csKey+=IntToString(iItemOcc);
                } else {
                        csKey+=_T("1");
                }

        CTreeItemInfo * pItemInfo       = new CTreeItemInfo( iNodeIdx, csKey, iItemIndex, bSpecialInsert && bOccIcon ? CS_OCCURRENCE_INDEX : eTypeOfItem== CDEFormBase::Field ? CS_FIELD_INFO : CS_GROUP_INFO, /*bSpecialInsert ? iWantedOcc : */ iItemOcc!=-1 ? iItemOcc : 1, iNumDrawableOccs, -1, iNonSelectedIconIndex, iSelectedIconIndex, csToolTip);
        pItemInfo->SetItem              ( pItem );

        pItemInfo->SetOccurrence( /*bSpecialInsert ? iWantedOcc : */ iItemOcc!=-1 ? iItemOcc : 1 );


        pItemInfo->SetMaxItemLabelLength( iMaxItemLength );


        //Inserción al árbol
        hItem = m_pTree->Insert( csItemLabel, hItemParent, hInsertAfter /*TVI_LAST*/, iNonSelectedIconIndex,iSelectedIconIndex);
        m_pTree->SetItemData( hItem, (DWORD) pItemInfo );

        if( InsertItemChilds(iNodeIdx, pItem, iItemIndex, /*1*/iItemOcc!=-1 ? iItemOcc : 1, hItem, csKey, pWantedField, iWantedOcc) ){
            bSearchOK = true;
        }

    } else {

        if(m_InsertItemConfigArray.GetSize()>0){

            //cuando existe acumulación de llamadas en el stack hay que vaciarlo.
            if( DoStackCalls() ){
                    bSearchOK = true;
            }
        }

        //CASO 2 : SE CREA UNA HOJA QUE CUELGA DE hGroupParent Y BAJO ÉSTA SE CUELGA CADA OCURRENCIA
        /*
                hGroupParent
                 |
                 + csGroupTittle
                         |
                         + (1)
                         + (2)
                         :
                         + (n)
        */

        HTREEITEM   hOccTittle;
        CString     csOccTittle;
        CString     csOccNumber;
        int         iNumZeros;
        CString     csFormatedNumber;

        if( m_bShowNames ){
            csItemLabel = pItem->GetName();
        } else {
            csItemLabel = pItem->GetLabel();
        }
        csItemLabel.TrimRight();

        if(csItemLabel.GetAt(csItemLabel.GetLength()-1)!=':'){
            csItemLabel += (_T(" : ") + IntToString(iNumDrawableOccs) + _T(" occurrences"));
        } else {
            csItemLabel += (_T(" ") + IntToString(iNumDrawableOccs) + _T(" occurrences"));
        }


        hItem           = m_pTree->Insert( csItemLabel, hItemParent, hInsertAfter/*TVI_LAST*/, 8,9);
        CString csKey   = csParentKey + _T("-") + IntToString(pItem->GetSymbol()) + _T("#0");

        if(m_bShowNames){
            csToolTip = pItem->GetLabel() + _T(" ( ") + IntToString(iNumDrawableOccs) + _T(" occurrences )");
        } else {
            csToolTip = pItem->GetName() + _T(" ( ") + IntToString(iNumDrawableOccs) + _T(" occurrences )");
        }

        CTreeItemInfo * pItemInfo = new CTreeItemInfo(  iNodeIdx, csKey, iItemIndex, CS_GROUP_OCCURRENCES_TITTLE, 0, iNumDrawableOccs, -1, 8, 9, csToolTip);

        //if( iNumDrawableOccs == 0 ) {
            pItemInfo->SetItem( pItem );
        //}

        //AfxMessageBox(csItemLabel);
        m_pTree->SetItemData( hItem, (DWORD) pItemInfo );


        CString csItemOccKey;
        int     iNumDigits = (IntToString(iNumDrawableOccs)).GetLength();
        CString csNotUsed;
        for(int iItemItr = 1; iItemItr<=iNumDrawableOccs; iItemItr++){

            csOccNumber       = IntToString(iItemItr);
            iNumZeros         = iNumDigits - csOccNumber.GetLength();
            csFormatedNumber  = _T("");
            for(int i=0; i<iNumZeros; i++) csFormatedNumber += _T("0");
            csFormatedNumber += csOccNumber;
            csOccTittle   = _T("(") + csFormatedNumber + _T(")");

            if(m_bShowNames){
                csToolTip = pItem->GetLabel() + _T(" ") + csOccTittle;
            } else {
                csToolTip = pItem->GetName() + _T(" ") + csOccTittle;
            }

            iNonSelectedIconIndex   = iItemItr <= iDataOccs ? 32 : 30;
            iSelectedIconIndex      = 1 + iNonSelectedIconIndex;
            hOccTittle              = m_pTree->Insert( csOccTittle, hItem, TVI_LAST, iNonSelectedIconIndex, iSelectedIconIndex);
            csItemOccKey            = csParentKey + _T("-") + IntToString(pItem->GetSymbol()) + _T("#") + IntToString(iItemItr);
            pItemInfo               = new CTreeItemInfo( iNodeIdx, csItemOccKey, iItemIndex, CS_OCCURRENCE_INDEX, iItemItr, iNumDrawableOccs, -1, iNonSelectedIconIndex , iSelectedIconIndex, csToolTip);
            pItemInfo->SetItem( pItem );
            pItemInfo->SetLabel( csOccTittle );

            m_pTree->SetItemData( hOccTittle, (DWORD) pItemInfo );

            if( InsertItemChilds(iNodeIdx, pItem, iItemIndex, iItemItr, hOccTittle, csItemOccKey, pWantedField, iWantedOcc) ){
                bSearchOK = true;
            }
        }
    }

    return bSearchOK;
}

//Inserta los hijos de la iGroupOcc-ava ocurrencia del grupo pGroup colgando de hGroup.
bool CCaseTree::InsertItemChilds(int iNodeIdx, CDEItemBase* pItem, int iItemIndex, int iItemOcc, HTREEITEM hItem, CString csItemKey, CDEField* pWantedField, int iWantedOcc )
{
        bool                    bSearchOK       = false;

        if( pItem->GetItemType() == CDEFormBase::Field  &&
                ((CDEField*)pItem)==pWantedField        &&
                iItemOcc==iWantedOcc ){

                bSearchOK = true;


        }

        int                     iNumChilds;
        CDEGroup*               pGroup;
        CDEFormBase::eItemType  eTypeOfItem;

        eTypeOfItem     = pItem->GetItemType();
        switch( eTypeOfItem ){

        case CDEFormBase::Roster :{  pGroup          = (CDEGroup*) pItem;
                                        iNumChilds      = pGroup->GetNumItems();
                                        for(int i=0; i<iNumChilds; i++){

                                                if( InsertMultiOccItem( iNodeIdx, pGroup->GetItem(i), iItemIndex, iItemOcc, hItem, TVI_LAST, /*csItemKey + _T("-") + IntToString(i),*/ pWantedField, iWantedOcc, 0, false , true ) ){
                                                        bSearchOK = true;
                                                }
                                        }

                                 } break;

        case CDEFormBase::Group  :{  pGroup          = (CDEGroup*) pItem;
                                        iNumChilds      = pGroup->GetNumItems();
                                        for(int i=0; i<iNumChilds; i++){

                                                if( InsertMultiOccItem( iNodeIdx, pGroup->GetItem(i), iItemIndex, iItemOcc, hItem, TVI_LAST, /*csItemKey + _T("-") + IntToString(i),*/ pWantedField, iWantedOcc, 0, false , true ) ){
                                                        bSearchOK = true;
                                                }
                                        }

                                 } break;

        case CDEFormBase::Field  :{  iNumChilds      = 0; } break;
        }

        return bSearchOK;
}

bool CCaseTree::InsertLevel(CString csParentKey, int iNodeIdx, CDELevel* pLevel, HTREEITEM hParentItemOfLevel, int iLevelIndex, CDEField* pWantedField, int iWantedOcc, bool bParentItemReplaceRootLevel)
{
        bool bSearchOK = false;

        bool    bShortRepresentation    = m_bShowNames;

        bParentItemReplaceRootLevel     = false;

        bool    bNewTreeItemInfo        = false;

        //Inserción del level : la raíz se inserta sólo si se escoje "single root".
        HTREEITEM  ParentGroupLevel = hParentItemOfLevel; //NULL;

        if(!bParentItemReplaceRootLevel){

                if( ( !hParentItemOfLevel || hParentItemOfLevel==TVI_ROOT) ){

                        bNewTreeItemInfo = true;
                        CString csLevelLabel;
                        if( bShortRepresentation ){
                                csLevelLabel = pLevel->GetName();
                        } else {
                                csLevelLabel = pLevel->GetLabel();
                        }
                        ParentGroupLevel = m_pTree->Insert( csLevelLabel , hParentItemOfLevel , TVI_LAST, 4,5);
                }
        }

        CString csKey = csParentKey  + _T("#") + IntToString(pLevel->GetSymbol()) + _T("#") + IntToString(iLevelIndex);

    CTreeItemInfo * pItemInfo = bNewTreeItemInfo ? new CTreeItemInfo( iNodeIdx, csKey, iLevelIndex, CS_LEVEL_INFO, pLevel->GetCurOccurrence(), pLevel->GetMaxLoopOccs(), -1, 4, 5,pLevel->GetName()  ) : NULL;
        if(pItemInfo){
                pItemInfo->SetLevel( pLevel );
        }

        if( ParentGroupLevel ){         /*single root*/

                if(pItemInfo){
                        m_pTree->SetItemData( ParentGroupLevel, (DWORD) pItemInfo );
                }

        } else { /*multiple root*/

                //En lugar de insertar una raíz en el árbol, se crea un elemento que contiene la información de la raíz.
                //Luego, todos los que se insertaan bajo esta raíz, se insertan bajo NULL.
                CString                 csRootLabel;
                if(bShortRepresentation){
                        csRootLabel     = pLevel->GetName();
                } else {
                        csRootLabel     = pLevel->GetLabel();
                }
                CTreeItem * pRootItem   = new CTreeItem(m_pTree, pItemInfo, csRootLabel, TVI_ROOT, TVI_LAST, 4, 5 );
                m_pTree->SetXRootItem( pRootItem );
        }

        CString csLevelKey      = csKey;

        // A su vez, cada Level tiene un set de elementos hijos

        m_pLastHITEMParent      = ParentGroupLevel;
        m_pLastInsertedItem     = NULL;

        CDEItemBase* pChildItem;
        //HTREEITEM    hTreeItem;
        int iNumChilds = pLevel->GetNumItems();//->GetNumGroups();


        for( int iChildIndex = 0; iChildIndex< iNumChilds; iChildIndex++ ){

                pChildItem      = pLevel->GetItem( iChildIndex );

                if( InsertMultiOccItem(iNodeIdx, pChildItem, iChildIndex, -1/*todas sus ocurrencias*/, ParentGroupLevel, TVI_LAST, /*csLevelKey,*/ pWantedField, iWantedOcc, 0, false, true) ){

                        bSearchOK = true;
                }
        }

        //
        //bool bExpandResult = (m_pTree->Expand( ParentGroupLevel, TVE_EXPAND ) != 0 );

        return bSearchOK;
}

bool CCaseTree::NeedToPutInStack(   CDEItemBase* pItem,                 //item que estamos insertando
                                    HTREEITEM hParentItem,      //nodo del árbol desde donde cuelga este item
                                    CDEItemBase* pLastInsertedItem,
                                    HTREEITEM hLastParentItem )
{
    bool bNeedToPutInStack = false;


    //Cuando no hay nada en el stack, ingresa el primer Field
    if(m_InsertItemConfigArray.GetSize()==0){

        if( pItem && pItem->GetItemType() == CDEFormBase::Field ){
            bNeedToPutInStack = true;
        }

    } else {

        if( (hParentItem==hLastParentItem) &&
            (pItem && (pItem->GetItemType() == CDEFormBase::Field)) &&
            (pLastInsertedItem && (pLastInsertedItem->GetItemType() == CDEFormBase::Field)) ){

            bNeedToPutInStack = true;
        }
    }

    return bNeedToPutInStack;
}

bool CCaseTree::AddTreeLayer(   int         iTypeOfTree,
                                bool        bFirstCall,
                                HTREEITEM   hItem,
                                CString     csSelectedKey, /*por si la llave del elemento que debe quedar seleccionado es conocida.*/
                                bool        bVisible          /*true => lo crea y lo muestra.*/,
                                CDEField*   pWantedField,
                                int         iWantedOcc)
{

    m_pWantedField  = pWantedField;
    m_iWantedOcc    = iWantedOcc;
    m_csWantedKey   = csSelectedKey;

    /*
    if(!pWantedField && csSelectedKey.GetLength()==0){
        return false;
    }*/
    bool bSearchOK = false;


    //Antes de desplegar cualquier árbol, se guarda el estado prendido
        if( m_pTree ){
                if(!m_pTree->SaveShape()){
                    ASSERT(0);
                        //AfxMessageBox(_T("Problems to save shape"));
                }
        }


        //Check about toggle view

                int iNumTreeLayers      = m_pTreeArray ? m_pTreeArray->GetSize() : 0;
                int iTopLayerIndex      = iNumTreeLayers-1;

    bool bIsToggleView           = iTopLayerIndex>=0 && m_pTreeArray->GetAt(iTopLayerIndex)->IsToggleView();

    InitTree(bIsToggleView);

        if( bFirstCall ){

            if( iTypeOfTree == TREE_OF_VARS_VALUES )
                bSearchOK = ShowTreeOfNodes(true,pWantedField,iWantedOcc);

        //FABN INIT 14/Aug/2002
        } else {



                // El actual m_pTree es el puntero almacenado en el último casillero de m_pTreeArray.
                int iNewTreeIdx = this->m_pTreeArray->GetSize()-1;
                int     iOldTreeIdx     = iNewTreeIdx - 1;


                CGenericTreeCtrl * pOldTree = m_pTreeArray->GetAt( iOldTreeIdx );
                m_pTree->CopyFrom(pOldTree/*from*/, hItem/*from*/);
                m_pTree->SetFocus();
                m_pTree->Select( m_pTree->GetRootItem(), TVGN_CARET );



        }
        //FABN END 14/Aug/2002


        //En este punto el árbol está dibujado en pantalla y él sabe si es un toggle o no.
        m_iToggleTreeLayers     = 0;
        int iNumTrees = m_pTreeArray ? m_pTreeArray->GetSize() : 0;
        CGenericTreeCtrl * pTree;
        for(int i=0; i<iNumTrees; i++){
                pTree = this->m_pTreeArray->GetAt(i);
                if( pTree->IsToggleView() ){
                        m_iToggleTreeLayers++;
                }
        }
        m_bIsInToggleView = m_iToggleTreeLayers>0 ;

        //FABN INIT 15/Aug/2002

    bSearchOK = SelectCorrectItem( pWantedField,iWantedOcc, csSelectedKey);

    //FABN 16/Aug/2002
    if( m_pTree ){
        if( bVisible ){
            m_pTree->SetFocus();
            this->m_pTree->ShowWindow( SW_NORMAL );

            updateWindowText();

        }
    }

    /**the tree must remember the index of the last open node*/
    if(m_iLastOpenNodeIdx==-1){
        m_iLastOpenNodeIdx = FirstSetOfLastNodeIdx();
    }


    return bSearchOK;
}

void CCaseTree::LoadIcons()
{
    if( m_pImageList == nullptr )
    {
        m_pImageList = new CImageList();
        m_pImageList->Create(16, 16, ILC_COLOR32, 0, CaseTreeIcons::GetNumberIcons());

        CaseTreeIcons::ForeachIcon([&](int id)
        {
            HICON hIcon = m_pWinApp->LoadIcon(id);
            m_pImageList->Add(hIcon);
        });
    }
}


void CCaseTree::updateWindowText()
{
    CString csWindowText = _T("");

        if( m_pTree ){

                HTREEITEM hSelectedItem = m_pTree->GetSelectedItem();
                if( hSelectedItem ){

                        //si existe hoja seleccionada => se calcula quién es su hoja padre.
                        CTreeItemInfo * pItemInfo = NULL;
                        HTREEITEM hParentItem   = m_pTree->GetParentItem( hSelectedItem );
                        if( hParentItem )
                            pItemInfo = (CTreeItemInfo*) m_pTree->GetItemData( hParentItem );

                        if( pItemInfo ){

                                CDEItemBase* pItem = pItemInfo->GetItem();
                                if( pItem ){

                                        if( m_bShowNames ){
                                                csWindowText = pItem->GetName() + _T(" ") + csWindowText;
                                        } else {
                                                csWindowText = pItem->GetLabel() + _T(" ") + csWindowText;
                                        }

                                        if( pItemInfo->GetTypeOfInfo()==CS_OCCURRENCE_INDEX ){
                                                csWindowText = csWindowText  + _T(" ") + pItemInfo->GetLabel();
                                        }



                                } else {

                                        if(hParentItem){
                                                csWindowText = m_pTree->GetItemText(hParentItem);
                                        } else {
                                            csWindowText = m_pTree->GetItemText( m_pTree->GetRootItem() );
                                        }
                                }


                        }
                }
        }

    if(csWindowText.GetLength()>0){
        CMsgParam msgParam;
            msgParam.csParam    = csWindowText;
            m_pParent->SendMessage(UWM::CaseTree::UpdateWindowText, (WPARAM)&msgParam, 0);
    }
}




int CCaseTree::GetNumTreeLayers()
{
        return  m_pTreeArray ? m_pTreeArray->GetSize() : 0;
}


void CCaseTree::InitTree( bool bIsToggleView)
{


    if( m_pTree ){
                m_pTree->ShowWindow( SW_HIDE );
        }


        int             iCurrentNumTrees        = m_pTreeArray ? m_pTreeArray->GetSize() : 0;

    bool        bCanDestroyItemInfo     = (iCurrentNumTrees==0);

    CGenericTreeCtrl * pTree = new CGenericTreeCtrl( this, bCanDestroyItemInfo, m_iTypeOfTree, bIsToggleView,true, iCurrentNumTrees > 0, false, false );

    //From now, m_pTree is a pointer to the last tree stored in m_pTreeArray.
    if( !m_pTreeArray){
            m_pTreeArray = new CArray<CGenericTreeCtrl*,CGenericTreeCtrl*>;
    }

    m_pTreeArray->Add( pTree );

    m_pTree = m_pTreeArray->GetAt( m_pTreeArray->GetSize()-1 );

    CRect rect(m_ixo,m_iyo,m_ix1,m_iy1);

    if(!m_pWhiteBackGround){
        m_pWhiteBackGround = new CTreeCtrl();
        m_pWhiteBackGround->Create( WS_VISIBLE | WS_CHILD | TVS_HASLINES | TVS_LINESATROOT | TVS_HASBUTTONS, rect, m_pParent, 20 );
    }

    DWORD dwStyle = WS_VISIBLE   | WS_CHILD | TVS_HASLINES | TVS_LINESATROOT | TVS_HASBUTTONS |TVS_NOTOOLTIPS;
          dwStyle |= TVS_SHOWSELALWAYS;

    m_pTree->Create(dwStyle,
                    rect,
                    m_pWhiteBackGround,
                    20 );

    LoadIcons();
    m_pTree->SetImageList(this->m_pImageList , TVSIL_NORMAL  );
    m_pTree->SetParent   (this , TREE_DIALOG );
    m_pTree->SetFont( m_pTreeFont , true);
    m_pTree->ShowWindow( SW_HIDE );

    /*m_pTree->EnableScrollBarCtrl( SB_HORZ, true );
    m_pTree->EnableScrollBarCtrl( SB_VERT, true );*/
}
/////////////////////////////////////////////////////////////////////////////
// CCaseTree message handlers

void CCaseTree::Show(int xo, int yo, int x1, int y1)
{
    ShowTree(xo,yo,x1,y1);
    if(GetNumTreeLayers()==0){
        m_pParent->PostMessage(UWM::CaseTree::DeleteCaseTree, 0, 0);
    }
}

void CCaseTree::CloseDialog()
{

        int iNumTrees = m_pTreeArray ? m_pTreeArray->GetSize() : 0;
        while( iNumTrees>1 ){
                xClose( false, false );
                iNumTrees = m_pTreeArray ? m_pTreeArray->GetSize() : 0;
        }
        xClose( false, true);
}

void CCaseTree::xClose( bool bRefresh, bool bDeleteAll )
{

        if( m_pTreeArray ){

                int iNumTrees = this->m_pTreeArray->GetSize();
                if( iNumTrees>1 ){
                        m_pTree = m_pTreeArray->GetAt(m_pTreeArray->GetSize()-1);
                        m_pTree->ShowWindow(SW_HIDE);

                        //Guardado de información valiosa del árbol que estamos eliminando.
                        if( !m_pTree->SaveShape() ){
                            ASSERT(0);
//                                AfxMessageBox(_T("Problems to save the shape"));
                        }
                        HTREEITEM hLastSelectedItem = m_pTree->GetSelectedItem();
                        if( hLastSelectedItem==NULL ){
                                hLastSelectedItem = m_pTree->GetRootItem();
                        }

                        //Cálculo de la llave a seleccionar en la capa de árbol que va a quedar activa, luego de cerrar esta.
                        CString csLastSelectedKey;
                        m_iToggleTreeLayers     = getNumToggleTreeLayers(); //AfxMessageBox("m_iToggleTreeLayers= " + IntToString(m_iToggleTreeLayers));
                        m_bIsInToggleView       = m_iToggleTreeLayers>0 ;
                        if( m_iToggleTreeLayers==1 ){
                                CGenericTreeCtrl * pTree = (CGenericTreeCtrl*) m_pTreeArray->GetAt( m_pTreeArray->GetSize()-2 );
                                csLastSelectedKey = pTree->GetKey( pTree->GetSelectedItem() ); //AfxMessageBox( "volviendo a llave " + csLastSelectedKey );
                        }
                        if( csLastSelectedKey==_T("") ){
                                CTreeItemInfo * pLastSelectedItem       = (CTreeItemInfo *) m_pTree->GetItemData(hLastSelectedItem);
                                csLastSelectedKey                       = pLastSelectedItem->GetKey();
                        }

                        //Eliminación del árbol activo
                        delete( m_pTree );
                        int i;
                        CArray<CGenericTreeCtrl*, CGenericTreeCtrl*> * pTreeArrayAux = new CArray<CGenericTreeCtrl*,CGenericTreeCtrl*>;
                        for( i=0; i<iNumTrees-1; i++){
                                pTreeArrayAux->Add( m_pTreeArray->GetAt(i) );
                        }
                        m_pTreeArray->RemoveAll();
                        iNumTrees = pTreeArrayAux->GetSize();
                        for( i=0; i<iNumTrees; i++ ){
                                m_pTreeArray->Add( pTreeArrayAux->GetAt(i) );
                        }
                        pTreeArrayAux->RemoveAll();
                        delete( pTreeArrayAux );
                        pTreeArrayAux = NULL;

                        //Seteo del nuevo árbol que será el activo.
                        m_pTree = m_pTreeArray->GetAt( m_pTreeArray->GetSize()-1 );

                        //Carga de forma
                        if(!m_pTree->LoadShape()){
                            ASSERT(0);
                                //AfxMessageBox(_T("Problems to load shape"));
                        }

                        //Recálculo de nuevas condiciones para arbolitos toggle.
                        m_iToggleTreeLayers                     = getNumToggleTreeLayers();
                        m_bIsInToggleView                       = m_iToggleTreeLayers>0 ;
                        m_iNumberOfTogglePressed        = m_bIsInToggleView ? m_iNumberOfTogglePressed : 0;

                        if(bRefresh){
                            Refresh( m_pTree->GetRootItem(), csLastSelectedKey , NULL, 0, NULL, NULL);
                        }

                        return;
                }

        }

        if( m_bIsInToggleView ){
                m_iToggleTreeLayers--;
                if( m_iToggleTreeLayers==0 ){
                        m_bIsInToggleView                = false;
                        m_iNumberOfTogglePressed = 0;
                }
        }

        if(bDeleteAll){
                m_pParent->PostMessage(UWM::CaseTree::DeleteCaseTree, 0, 0);
        }

}

void CCaseTree::Clean()
{
        if( !m_bIsClean ){

                if( m_pTreeFont ){
                        delete( m_pTreeFont );
                        m_pTreeFont = NULL;
                }
                if( m_pImageList ){
                        delete( m_pImageList );
                        m_pImageList = NULL;
                }

                if( m_pTree ){

                        bool bRemoveOK = RemoveFromArray(m_pTree);
                        ASSERT(bRemoveOK);

                        delete( m_pTree );
                        m_pTree = NULL;
                }

                m_bIsClean = true;
        }
}




bool CCaseTree::SelectCorrectItem(CDEField* pWantedField,int iWantedOcc, CString csLastSelectedKey, bool bExpand )
{
    bool bSearchOK = false;

    if( (pWantedField && iWantedOcc>0) || csLastSelectedKey.GetLength()>0 ){

        int                     iTreeLayerIdx   = -1;
        HTREEITEM   hSelItem                = NULL;
        int                         iIsInRoot               = 0;
        bool                bSearchByField  = SearchField(pWantedField,iWantedOcc, &iTreeLayerIdx, &hSelItem, &iIsInRoot );
        bool                bSearchByKey    = bSearchByField ? true : SearchField(csLastSelectedKey, &iTreeLayerIdx, &hSelItem, &iIsInRoot );
        bSearchOK           = bSearchByField || bSearchByKey;
        if(bSearchOK){
            int iNumTreeLayers  = GetNumTreeLayers();
            int iTopLayerIdx    = iNumTreeLayers - 1;
            if(iTreeLayerIdx!=-1 && iTopLayerIdx>iTreeLayerIdx){
                int i=iTopLayerIdx;
                for(i = iTopLayerIdx; i>iTreeLayerIdx; i-- ){
                    m_pTreeArray->GetAt(i)->ShowWindow(SW_HIDE);
                    delete(m_pTreeArray->GetAt(i));
                    m_pTreeArray->SetAt(i,NULL);
                }
                CArray<CGenericTreeCtrl*,CGenericTreeCtrl*>* pAuxArray = new CArray<CGenericTreeCtrl*,CGenericTreeCtrl*>;
                for(i=0; i<=iTreeLayerIdx; i++){
                    pAuxArray->Add( m_pTreeArray->GetAt(i) );
                }
                m_pTreeArray->RemoveAll();
                delete(m_pTreeArray);
                m_pTreeArray = pAuxArray;
            }
        }
        m_pTree = m_pTreeArray && m_pTreeArray->GetSize()>=1 ? m_pTreeArray->GetAt(m_pTreeArray->GetSize()-1) : NULL;
        if(m_pTree && hSelItem){

            //m_pTree->Select( hSelItem, TVGN_CARET ); <-- select and scroll

            m_pTree->SelectItem( hSelItem );            //<-- only select
            if(!m_pTree->IsVisible(hSelItem)){
                m_pTree->EnsureVisible(hSelItem);
            }

            if( bExpand ) {
                m_pTree->Expand( hSelItem, TVE_EXPAND );
            }
        }
    }

    return bSearchOK;
}

bool CCaseTree::Refresh(    HTREEITEM                                   hItem ,
                            CString                                     csLastSelectedKey,
                            CDEField*                                   pWantedField,
                            int                                         iWantedOcc,
                            CArray<CDEItemBase*,CDEItemBase*>*          pItemBaseToRefreshArray,
                            CArray<CArray<int,int>*,CArray<int,int>*>*  pOccsToRefreshArray)
{
    bool bSearchOK      = false;

    //Every time the tree is refreshed, the wanted position in the tree is stored.
    m_pWantedField  = pWantedField;
    m_iWantedOcc    = iWantedOcc;
    m_csWantedKey   = csLastSelectedKey;
    m_bVerifyFlag   = true;

    //If the tree is empty, refresh <=> add a brand new tree layer
    if(IsEmpty()){
        ASSERT(GetNumTreeLayers()==1);
        delete( m_pTree );
        m_pTree = NULL;
        m_pTreeArray->RemoveAll();
        delete(m_pTreeArray);
        m_pTreeArray = NULL;
        return AddTreeLayer(m_iTypeOfTree, true,hItem,csLastSelectedKey,true,pWantedField,iWantedOcc);
    }

    //to unselect when not found pWantedField/iWantedOcc
    m_pTree->SelectItem(NULL);


    //Before add/remove hitems : save the shape
    if(!m_pShape){
        m_pShape = new CArray<CString,CString>;
    }

    m_pTree->SaveShapeTo(m_pShape);

    //bFullRefresh : if true recreate the whole tree; else recreate only specified items
    bool bFullRefresh = false;
    if( pItemBaseToRefreshArray==NULL ){
        bFullRefresh = true;
    }

    int iNumNodes = m_pCapiRunAplEntry->GetNNodes();
    int iLastOpenNodeIdx = -1;
    for( int iNodeIdx=0; iNodeIdx<iNumNodes; iNodeIdx++){
        if(m_pCapiRunAplEntry->IsOpen(iNodeIdx)){
            iLastOpenNodeIdx = iNodeIdx;
        }
    }
    if( m_iLastOpenNodeIdx==-1 ){
        m_iLastOpenNodeIdx = FirstSetOfLastNodeIdx(iLastOpenNodeIdx);
    } else {
        if( iLastOpenNodeIdx!=m_iLastOpenNodeIdx ){
            bFullRefresh        = true;
            m_iLastOpenNodeIdx  = iLastOpenNodeIdx;
        }
    }

    if(!bFullRefresh){
        int iTotalOccsToRefresh = 0;
        int iNumArrays = pOccsToRefreshArray ? pOccsToRefreshArray->GetSize() : 0;
        for( int i=0; i<iNumArrays; i++){
            iTotalOccsToRefresh += pOccsToRefreshArray->GetAt(i)->GetSize();
        }
        if(iTotalOccsToRefresh==0){
            bFullRefresh = true;
        }
    }

    //hide before any tree operation
    m_pTree->ShowWindow(SW_HIDE);

    //main functions to refresh the tree
    if( !bFullRefresh){
        bSearchOK = RefreshItemBase(pItemBaseToRefreshArray, pOccsToRefreshArray, pWantedField,iWantedOcc);
    } else {
        bSearchOK = DestroyAndCreateAllTreeLayers( pWantedField, iWantedOcc, csLastSelectedKey );
    }
    if( m_InsertItemConfigArray.GetSize()>0 ){
        DoStackCalls();
    }

    //in bFullRefresh every node is recreated with proper label
    if(!bFullRefresh){
        if(!RefreshNodeLabels()){
            TRACE(_T("\nSome node was not found\n"));
        }
    }

    //restore the shape

    if(m_pShape){
        if( m_pTree!=NULL ){
            m_pTree->LoadShapeFrom(m_pShape);
        }
        m_pShape->RemoveAll();
        delete(m_pShape);
        m_pShape = NULL;
    }

    //Select the wanted field in the correct occ., or directly select the hitem that has the csLastSelectedKey
    bSearchOK = SelectCorrectItem( pWantedField,iWantedOcc, csLastSelectedKey );

        //Restore scroll position only if the OnRefresh receive the same last selected field
    bool bShow = true;
    bool bSameField = m_pLastSelectedField && pWantedField && m_pLastSelectedField->GetSymbol()==pWantedField->GetSymbol();
    bool bSameOcc   = m_iLastSelectedOcc==iWantedOcc;
    if( m_pTree && bSameField && bSameOcc ){
        int iHScrollPos = m_ptLastScrollPos.x;
        int iVScrollPos = m_ptLastScrollPos.y;
        if(iHScrollPos!=-1 && iVScrollPos!=-1){

            m_pTree->xSetScrollPos(iHScrollPos,iVScrollPos);

            //PATCH FOR SCROLL BUG
            m_pParent->PostMessage(UWM::CaseTree::RecalcLayout);

            bShow = false;
            m_ptLastScrollPos.x         = -1;
            m_ptLastScrollPos.y         = -1;
        }
    }

    updateWindowText();

    if(!m_pTree){
        return false;
    }

    if(bSearchOK){
        m_pTree->SetFocus();
    } else {
        m_pTree->SelectItem(NULL);
    }

    m_pTree->EnableToolTips(true);

    if(m_bFilteredTree){
        m_pTree->Filter(TreeFilterType::FieldNote);
    }
    if(bShow){
        m_pTree->ShowWindow(SW_NORMAL);
    }

    return true;
}

bool CCaseTree::xRefresh( HTREEITEM hItem, CDEField* pWantedField, int iWantedOcc, CString csWantedKey)
{

        bool bRefreshOK = true;


                //FABN INIT 22/Aug/2002
                if( m_pTree==m_pTreeArray->GetAt(0) && hItem==m_pTree->GetRootItem() ){

                        int iTypeOfTree = m_pTree->GetTypeOfTree();

                        delete( m_pTree );

                        m_pTree = NULL;
                        if( m_pTreeArray->GetSize()!=1 ){

                                //AfxMessageBox(_T("ASSERT 1"));
                        }

                        m_pTreeArray->RemoveAll();
                        delete( m_pTreeArray );
                        m_pTreeArray = NULL;


            //Re creating the first layer from scratch
                        bRefreshOK = AddTreeLayer(iTypeOfTree, true, NULL, csWantedKey, false , pWantedField,iWantedOcc);

                        return bRefreshOK;
                }

                //FABN END 22/Aug/2002

                //
                bool     bIsRefreshable;
                int      iNonSelectedIconIndex;
                int      iSelectedIconIndex;
                int      iIconColor;
                CString  CSLabel;
        CString  csToolTip;

                //Paso 1: obtención de información actualizada :
                CTreeItemInfo * pItemInfo = (CTreeItemInfo*) m_pTree->GetItemData( hItem );
                if( pItemInfo && pItemInfo->GetItem() && pItemInfo->GetTypeOfInfo()!=CS_OCCURRENCE_INDEX  /*&& pItemInfo->GetTypeOfInfo()!=CS_LEVEL_INFO*/ ){

                        bIsRefreshable = true;
                    m_pCapiRunAplEntry->GetInfo(                        m_bVerifyFlag/*,pItemInfo->GetNodeIdx()*/,
                                                            pItemInfo->GetItem(),               //input.
                                                                                                                    pItemInfo->GetOccurrence(),                         //input.
                                                                                                                    pItemInfo->GetMaxItemLabelLength(), //input.
                                                                                                                    m_bShowNames,                             //input.
                                                                                                                    pItemInfo,                                          //input.
                                                                                                                    & iNonSelectedIconIndex,            //output.
                                                                                                                    & iSelectedIconIndex,               //output.
                                                                                                                    & iIconColor,                                               //output.
                                                                                                                    & CSLabel,                                                  //
                                                                                                                    & csToolTip);                                               //output.

                        //Sólo a los Fields se les puede actualizar los íconos.
                        if( pItemInfo->GetTypeOfInfo()==CS_FIELD_INFO ){

                                pItemInfo->SetNonSelectedIconIndex( iNonSelectedIconIndex  );
                                pItemInfo->SetSelectedIconIndex   ( iSelectedIconIndex     );
                                //pItemInfo->SetIconColor           ( iIconColor             );
                                pItemInfo->SetToolTip             ( csToolTip              );

                        }

                        //En caso de tratarse de un título de ocurrencias, hay que rehacer el título :
                        if( pItemInfo->GetTypeOfInfo()==CS_GROUP_OCCURRENCES_TITTLE  ||
                            pItemInfo->GetTypeOfInfo()==CS_ROSTER_OCCURRENCES_TITTLE ){

                                CSLabel = CSLabel + _T(" : ") + IntToString( pItemInfo->GetMaxNumOccurrences() ) + _T(" occurrences ");

                        }

                } else bIsRefreshable = false;

                //Paso 2 : modificación del item, si corresponde.
                if( bIsRefreshable ){

                        HTREEITEM hRefreshedItem          = hItem;
                        UINT      nRefreshedMask          = TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
                        LPCTSTR   lpszItem                = CSLabel;
                        int       nRefreshedImage         = pItemInfo->GetNonSelectedIconIndex();
                        int       nRefreshedSelectedImage = nRefreshedImage;
                        UINT      nState                  = TVIS_BOLD;
                        UINT      nStateMask              = TVIS_BOLD;
                        LPARAM    lParam                  = 0;

                        this->m_pTree->SetItem( hRefreshedItem,
                                                nRefreshedMask,
                                                lpszItem,
                                                nRefreshedImage,
                                                nRefreshedSelectedImage,
                                                nState,
                                                nStateMask,
                                                lParam );

                        this->m_pTree->SetItemData( hRefreshedItem, (DWORD) pItemInfo );

                }

                //Paso 2 : modificamos cada hijo de hItem, si es que los hay.
                if( this->m_pTree->ItemHasChildren( hItem ) ){

                        HTREEITEM hNextItem = this->m_pTree->GetChildItem( hItem );
                        while( hNextItem != NULL ){

                                xRefresh( hNextItem, pWantedField,iWantedOcc, csWantedKey );

                                hNextItem = m_pTree->GetNextItem( hNextItem, TVGN_NEXT);
                        }
                }




        return bRefreshOK;

}

void CCaseTree::UserSelectItem(CString csItemKey, bool bVisible)
{
    HTREEITEM   hFoundItem  = NULL;
    int         iIsInRoot   = 0;
    m_pTree->SearchItem(csItemKey, &hFoundItem, &iIsInRoot);

    if( hFoundItem!=NULL ){
        //SearchItem retrieve the hitem, but not it's scroll position.
        UserSelectItem(hFoundItem, CPoint(-1,-1), bVisible);
    }
}

void CCaseTree::UserSelectItem(HTREEITEM hSelectedItem, CPoint ptScrollPos, bool bVisible)
{
    if(!hSelectedItem){
        return;
    }

    if(!IsItemSelectable(m_pTree,hSelectedItem)){

        /*the user selection is lost*/
        m_pTree->SelectItem(NULL);

        /*the last engine wanted focus is restored*/
        SelectCorrectItem( m_pWantedField,m_iWantedOcc, m_csWantedKey );
        return;
    }

    //the same effect as if IsItemSelectable return false !
    if(m_pCapiRunAplEntry->InVerify()){
        m_pParent->PostMessage(UWM::CaseTree::TreeItemWithNothingToDo, 0, 0);
        return;
    }

    CTreeItemInfo * pItemInfo   = (CTreeItemInfo*) m_pTree->GetItemData( hSelectedItem );
    int                             iTypeOfInfo = pItemInfo->GetTypeOfInfo();

    //Need to go to the selected field ?
    if( iTypeOfInfo==CS_FIELD_INFO ){
        m_pLastSelectedField    = (CDEField*)pItemInfo->GetItem();
        m_iLastSelectedOcc      = pItemInfo->GetOccurrence();
        m_ptLastScrollPos       = ptScrollPos;


        CMsgParam * pMsgParam   = new CMsgParam();
        pMsgParam->iParam       = pItemInfo->GetOccurrence();
        pMsgParam->dwArrayParam.Add( (DWORD) ((CDEField*)pItemInfo->GetItem()) );
        pMsgParam->bMustBeDestroyedAfterLastCatchMessage        = true;


        //before the engine process the new selected position,
        //the current selected position is NULL
        // clicks in a tree with different flow of the flow in progress will select nothing
        m_pTree->SelectItem(NULL);

        m_pParent->PostMessage(UWM::CaseTree::GoTo, (WPARAM)pMsgParam, 0);
        
        return;
    }

    //Need to go to a new node ?
    if( iTypeOfInfo==CS_NODE_INFO ){

        //
        int iIconIdx = pItemInfo->GetNonSelectedIconIndex();

        if( iIconIdx!=CaseTreeIcons::GetIconIndex(IDI_GREEN_NODE_ICON) &&              // the node is close
            iIconIdx!=CaseTreeIcons::GetIconIndex(IDI_GREEN_NODE_NOTE_ICON) &&         // the node is close
            iIconIdx!=CaseTreeIcons::GetIconIndex(IDI_GREEN_NODE_PADLOCK_ICON) &&      // the node is not protected
            iIconIdx!=CaseTreeIcons::GetIconIndex(IDI_GREEN_NODE_PADLOCK_NOTE_ICON)) { // the node is not protected

            //
            if(m_pTree->GetRootItem()==hSelectedItem){
                return;
            }



            CMsgParam * pMsgParam                                                           = new CMsgParam();
            pMsgParam->iParam                                                                       = pItemInfo->GetIndex();
            pMsgParam->bMustBeDestroyedAfterLastCatchMessage        = true;
            m_pParent->PostMessage(UWM::CaseTree::GoToNode, (WPARAM)pMsgParam, 0);
            return;

        }
    }

    //do  nothing
    if(m_pTree->ItemHasChildren(hSelectedItem)){
        m_pTree->Expand(hSelectedItem,TVE_TOGGLE);
    } else {
        if(m_pParent){
            m_pParent->PostMessage(UWM::CaseTree::TreeItemWithNothingToDo, 0, 0);
            return;
        }
    }

}

int CCaseTree::getNumToggleTreeLayers()
{
        int iNumToggleLayers    = 0;
        int iNumLayers                  = this->m_pTreeArray ? m_pTreeArray->GetSize() : 0;
        CGenericTreeCtrl * pTree;
        for( int i=0; i<iNumLayers; i++ ){
                pTree = (CGenericTreeCtrl*) m_pTreeArray->GetAt(i);
                if( pTree->IsToggleView() ){
                        iNumToggleLayers++;
                }
        }
        return iNumToggleLayers;
}

void CCaseTree::InitFonts()
{
    if(!m_pButtonsFont){
        m_pButtonsFont = new CFont();
        m_pButtonsFont->CreateFont ( 15,
                                    7,
                                    0,
                                    0,
                                    FW_NORMAL,
                                    FALSE,
                                    FALSE,
                                    0,
                                    DEFAULT_CHARSET,
                                    OUT_STRING_PRECIS,
                                    CLIP_STROKE_PRECIS,
                                    PROOF_QUALITY,
                                    FF_DONTCARE,
                                    _T("Arial"));
    }

    if(!m_pTreeFont ){
        m_pTreeFont = new CFont();
        m_pTreeFont->CreateFont (   DEFAULT_TREE_FONT_HEIGTH,
                                    DEFAULT_TREE_FONT_WIDTH,
                                    0,
                                    0,
                                    FW_NORMAL,
                                    FALSE,
                                    FALSE,
                                    0,
                                    DEFAULT_CHARSET,
                                    OUT_STRING_PRECIS,
                                    CLIP_STROKE_PRECIS,
                                    PROOF_QUALITY,
                                    FF_DONTCARE,
                                    _T("Courier New"));
    }
}



void CCaseTree::RefreshTreeFont()
{
    if( m_pTreeFont ){
            delete(m_pTreeFont);
            m_pTreeFont=NULL;
    }

    this->m_pTreeFont = new CFont();

    int iFontHeigth = DEFAULT_TREE_FONT_HEIGTH;
    int iFontWidth = DEFAULT_TREE_FONT_WIDTH;

    m_pTreeFont->CreateFont (   iFontHeigth,
                                iFontWidth,
                                            0,
                                            0,
                                            FW_NORMAL,
                                            FALSE,
                                            FALSE,
                                            0,
                                            DEFAULT_CHARSET,
                                            OUT_STRING_PRECIS,
                                            CLIP_STROKE_PRECIS,
                                            PROOF_QUALITY,
                                            FF_DONTCARE,
                                            _T("Courier New"));

    if(m_pTree){
        m_pTree->SetFont( this->m_pTreeFont, true );
    }
}

void CCaseTree::Restore()
{
    m_bFilteredTree = false;
    CString csLastSelectedKey = _T("");
    HTREEITEM hSelectedItem = m_pTree->GetSelectedItem();
    if( hSelectedItem ){
        CTreeItemInfo * pItemInfo = (CTreeItemInfo *) m_pTree->GetItemData(hSelectedItem);
        csLastSelectedKey = pItemInfo->GetKey();
    }

    Refresh(m_pTree->GetRootItem(), csLastSelectedKey , m_pWantedField, m_iWantedOcc, NULL, NULL);
}

void CCaseTree::CloseCurrentTreeLayer()
{
        xClose( true, true );

        updateWindowText();
}

//A case tree is empty when it has only one layer, and no items.
bool CCaseTree::IsEmpty()
{
        ASSERT(m_pTree);

        bool bIsEmpty = false;

        int     iNumTreeLayers = GetNumTreeLayers();
        if( iNumTreeLayers!=1 ){
                return false;
        }

        if(m_pTree->GetCount()==0){
                return true;
        }

        return bIsEmpty;
}


bool CCaseTree::RemoveFromArray( CGenericTreeCtrl* pTreeCtrl )
{
        int iLayerIdx   = SearchTree( pTreeCtrl );
        if(iLayerIdx!=-1){

                ASSERT(m_pTreeArray);

                CArray<CGenericTreeCtrl*,CGenericTreeCtrl*>* pAuxArray = new CArray<CGenericTreeCtrl*,CGenericTreeCtrl*>;
                int iSize = m_pTreeArray->GetSize();
                int i;
                for(i=0; i<iLayerIdx; i++){
                        pAuxArray->Add( m_pTreeArray->GetAt(i) );
                }
                for(i=iLayerIdx+1; i<iSize; i++){
                        pAuxArray->Add( m_pTreeArray->GetAt(i) );
                }
                m_pTreeArray->RemoveAll();
                delete(m_pTreeArray);
                m_pTreeArray = pAuxArray;
        }

        return (iLayerIdx!=-1);
}

int CCaseTree::SearchTree( CGenericTreeCtrl* pTreeCtrl )
{
        int iLayerIdx = -1;

        int iSize = m_pTreeArray ? m_pTreeArray->GetSize() : 0;
        for(int i=0; iLayerIdx==-1 && i<iSize; i++){
                if(m_pTreeArray->GetAt(i)==pTreeCtrl){
                        iLayerIdx = i;
                }
        }

        return iLayerIdx;
}


bool CCaseTree::SearchField(CDEField* pWantedField,int iWantedOcc,
                                                        int* iTreeLayerIdx, HTREEITEM* hSelItem, int* iIsInRoot )
{
    if(!pWantedField || iWantedOcc<=0){
                return false;
    }

    int                                 iWantedLayer    = -1;
    CGenericTreeCtrl*   pTree           = NULL;
    int                                 iNumTreeLayers  = m_pTreeArray ? m_pTreeArray->GetSize() : 0;
    int                                 iMax            = iNumTreeLayers-1;
    int                                 iMin            = 0;
    for(int i=iMax; iWantedLayer==-1 && i>=iMin; i--){
        pTree = m_pTreeArray->GetAt(i);
        if(pTree->SearchField(pWantedField, iWantedOcc, hSelItem, iIsInRoot)){
                        iWantedLayer = i;
        }
    }

    *iTreeLayerIdx  = iWantedLayer;
    return (iWantedLayer!=-1);
}
bool CCaseTree::SearchField(CString csKey,
                                                        int* iTreeLayerIdx, HTREEITEM* hSelItem, int* iIsInRoot )
{
        if(csKey.GetLength()==0){
                return false;
        }


        int                                     iWantedLayer    = -1;
        CGenericTreeCtrl*       pTree                   = NULL;
        int                                     iNumTreeLayers  = m_pTreeArray ? m_pTreeArray->GetSize() : 0;
        int                                     iMax                    = iNumTreeLayers-1;
        int                                     iMin                    = 0;
        for(int i=iMax; iWantedLayer==-1 && i>=iMin; i--){
                pTree = m_pTreeArray->GetAt(i);
                if(pTree->SearchItem(csKey, hSelItem,iIsInRoot)){
                        iWantedLayer = i;
                }
        }

        *iTreeLayerIdx  = iWantedLayer;
        return (iWantedLayer!=-1);
}



bool CCaseTree::IsNode(CGenericTreeCtrl* pTreeLayer, CString csItemKey)
{
        bool bIsNode = false;

        if(pTreeLayer){

                HTREEITEM       hFoundItem      = NULL;
                int                     iIsInRoot       = 0;
                pTreeLayer->SearchItem(csItemKey, &hFoundItem, &iIsInRoot);

                if( hFoundItem ){

                        CTreeItemInfo* pInfo = (CTreeItemInfo*) pTreeLayer->GetItemData(hFoundItem);

                        bIsNode = (pInfo->GetTypeOfInfo()==CS_NODE_INFO );
                }
        }

        return bIsNode;
}

int CCaseTree::GetNonSelectedIconIdx(CGenericTreeCtrl* pTreeLayer, CString csItemKey)
{
        int iIconIdx = -1;

        if(pTreeLayer){

                HTREEITEM       hFoundItem      = NULL;
                int                     iIsInRoot       = 0;
                pTreeLayer->SearchItem(csItemKey, &hFoundItem, &iIsInRoot);

                if( hFoundItem ){

                        CTreeItemInfo* pInfo = (CTreeItemInfo*) pTreeLayer->GetItemData(hFoundItem);
                        iIconIdx = pInfo->GetNonSelectedIconIndex();
                }
        }

        return iIconIdx;
}


void CCaseTree::ShowTree(int xo, int yo, int x1, int y1)
{
    //Store position for future use
    m_ixo = xo;
    m_iyo = yo;
    m_ix1 = x1;
    m_iy1 = y1;

    bool bEngineInitResult = true;

    RefreshTreeFont();

    if( bEngineInitResult ){
        AddTreeLayer(m_iTypeOfTree , true, NULL, _T(""), true /*se crea y se muestra*/, m_pWantedField,m_iWantedOcc );
    } else {
        CloseDialog();          //close all tree layers, if there. ok.
    }
}

void CCaseTree::ChangeView()
{
    if(m_pTree && IsWindow(m_pTree->GetSafeHwnd())){
        m_pTree->ShowWindow(SW_HIDE);
    }

    m_bShowNames = !m_bShowNames;

    if(!m_pTree){
        return;
    }

    CString csLastSelectedKey = _T("");
    HTREEITEM hSelectedItem = m_pTree->GetSelectedItem();
    CTreeItemInfo * pItemInfo = NULL;
    if( hSelectedItem ){
        pItemInfo = (CTreeItemInfo *) this->m_pTree->GetItemData(hSelectedItem);
        csLastSelectedKey = pItemInfo->GetKey();
    }

    CDEField*   pField  = NULL;
    int         iOcc    = -1;

    if(pItemInfo && pItemInfo->GetTypeOfInfo()==CS_FIELD_INFO){
        pField  = (CDEField*)pItemInfo->GetItem();//GetField();
        iOcc    = pItemInfo->GetOccurrence();
    }

    Refresh(m_pTree->GetRootItem(), csLastSelectedKey , pField,iOcc,NULL, NULL);

    m_bRefreshLocked = true;
    m_pParent->SendMessage(UWM::CaseTree::RestoreEntryRunViewFocus, 0, 0);
    m_bRefreshLocked = false;
}


bool CCaseTree::DestroyAndCreateAllTreeLayers( CDEField* pWantedField, int iWantedOcc, CString csWantedKey )
{
    bool        bSearchOK   = false;

    HTREEITEM   hRootItem   = m_pTree->GetRootItem();

    //csKeysArray   : array with the key of the first root for each tree layer
    //isToggleArray         : isToggleArray->GetAt(i)==true => layer i is a toggle
    CArray<CString,CString>         csKeysArray;
    CArray<bool,bool>               isToggleArray;

    int iFirstToggleLayerIdx    = -1;
    int iToggleTreeLayers       = 0; // should this be m_iToggleTreeLayers?
    bool bIsToggleLayer         = false;
    int iNumTrees               = m_pTreeArray ? m_pTreeArray->GetSize() : 0;

    CTreeItemInfo* pItemInfo = NULL;
    CGenericTreeCtrl* pTree = NULL;
    CString csKey;

    int i = 0;
    for( i=0; i<iNumTrees; i++){

        pTree                   = m_pTreeArray->GetAt(i);
        bIsToggleLayer  = pTree->IsToggleView();
        if(bIsToggleLayer){
            iToggleTreeLayers++;
        }
        if( iFirstToggleLayerIdx==-1 && bIsToggleLayer ){
            iFirstToggleLayerIdx = i;
        }

        isToggleArray.Add( bIsToggleLayer );    //the toggle layers are in the top of the array (ej : false,false,true,true )

        hRootItem       = pTree->GetRootItem();
        pItemInfo       = (CTreeItemInfo*) pTree->GetItemData( hRootItem );

        if( pItemInfo ){
            csKey   = pItemInfo->GetKey();
            csKeysArray.Add( csKey );
        } else {
            csKeysArray.Add(_T(" "));
        }

    }

    m_bIsInToggleView = (iToggleTreeLayers>0);


    //destroy N-1 layers
    while( iNumTrees>1 ){

        xClose( false , false);

        iNumTrees = m_pTreeArray ? m_pTreeArray->GetSize() : 0;
    }

    CArray<CString,CString> csExpandedKeysArrays;
    //m_pTree->SaveShape();
    if(!m_pTree->SaveShapeTo(&csExpandedKeysArrays)){
        ASSERT(0);
        //AfxMessageBox(_T("Problems to save shape"));
    }

    //Recreate the first layer
    bSearchOK = xRefresh( m_pTree->GetRootItem(), pWantedField, iWantedOcc, csWantedKey  );

    //recreate the other layers
    int             iSize                   = csKeysArray.GetSize();
    bool    bVisible                = false;
    bool    bIsNode;
    int             iIconIdx;
    bool    bClosedNode;
    for(i=0;  i<GetNumTreeLayers() && i<iSize-1; i++){      //take care : GetNumTreeLayers() MUST be called (because of a closed node delete some  hitems)
        m_pTree                 = m_pTreeArray->GetAt(i);


        m_pTree->LoadShapeFrom(&csExpandedKeysArrays);


        csKey           = csKeysArray.GetAt(i+1 );      //key of the first root of the layer i+1
        bVisible        = (i==iSize-2);
        if( (i+1)==iFirstToggleLayerIdx ){

        } else {

            bIsNode         = IsNode(m_pTree,csKey);
            iIconIdx        = GetNonSelectedIconIdx(m_pTree,csKey);
            bClosedNode     = bIsNode && (iIconIdx==CaseTreeIcons::GetIconIndex(IDI_GREEN_NODE_PADLOCK_ICON) || iIconIdx==CaseTreeIcons::GetIconIndex(IDI_GREEN_NODE_PADLOCK_NOTE_ICON));

            if(!bClosedNode){
                UserSelectItem( csKey, bVisible );
            }
        }
    }

    m_pTree = m_pTreeArray ? m_pTreeArray->GetAt(m_pTreeArray->GetSize()-1) : NULL;
    if(m_pTree){
        m_pTree->LoadShapeFrom(&csExpandedKeysArrays);
    }

    csKeysArray.RemoveAll();
    isToggleArray.RemoveAll();


    /**the tree must remember the index of the last open node*/
    if(m_iLastOpenNodeIdx==-1){
        m_iLastOpenNodeIdx = FirstSetOfLastNodeIdx();
    }



    return bSearchOK;
}

bool CCaseTree::RefreshItemBase(CArray<CDEItemBase*,CDEItemBase*>*          pItemBaseToRefreshArray, //Items to be updated
                                CArray<CArray<int,int>*,CArray<int,int>*>*  pOccsToRefreshArray,     //an array for the occs to be updated for each item in pItemBaseToRefreshArray
                                CDEField* pWantedField, int iWantedOcc)                              //this is where the selected HTREEITEM will be after the whole process

{
    ASSERT( pItemBaseToRefreshArray!=NULL &&
            pOccsToRefreshArray!=NULL &&
            pItemBaseToRefreshArray->GetSize()==pOccsToRefreshArray->GetSize());

    bool    bSearchOK       = false;

    int     iNumTreeLayers  = GetNumTreeLayers();
    ASSERT( iNumTreeLayers==1 );


    int     iNumItemsToRefresh  = pItemBaseToRefreshArray->GetSize();
    for(int iItemIdx=0; iItemIdx<iNumItemsToRefresh; iItemIdx++){

        CDEItemBase*        pItemBaseToRefresh  = pItemBaseToRefreshArray->GetAt(iItemIdx);
        CGenericTreeCtrl*   pTree               = m_pTree;

        //aStoredOccs           = array with the occs of pItemBaseToRefresh currently stored in the tree.
                //aStoredParents        = array with the parent keys of any stored occ
                CArray<int,int>                         aStoredOccs;
                CArray<HTREEITEM,HTREEITEM>     aStoredParents;
                CArray<HTREEITEM,HTREEITEM> ahItems;
                CArray<HTREEITEM,HTREEITEM> aStoredhInsertAfter;
                pTree->SearchItemBase(pItemBaseToRefresh, ahItems);
                int iNumHItems = ahItems.GetSize();
                for(int i=0; i<iNumHItems; i++){
                        aStoredOccs.Add( ((CTreeItemInfo*)pTree->GetItemData(ahItems.GetAt(i)))->GetOccurrence() );
                        aStoredParents.Add( pTree->GetParentItem(ahItems.GetAt(i)) );
                        HTREEITEM hPrevBrother = pTree->GetNextItem( ahItems.GetAt(i), TVGN_PREVIOUS );
                        HTREEITEM hInsertAfter = hPrevBrother ? hPrevBrother : TVI_FIRST;
                        aStoredhInsertAfter.Add( hInsertAfter );
                }
                int iNumStoredOccs = aStoredOccs.GetSize();

                //pOccs = array with the occs of pItemBaseToRefresh that will be updated / added
                CArray<int,int>* pOccs = pOccsToRefreshArray->GetAt(iItemIdx);
        if( pOccs->GetSize()==0 ){
            continue;
        }

        //aOccsToAdd = array with the occs of pItemBaseToRefresh that will be added
        //                       = occs that are in pOccs but not in aStoredOccs
        CArray<int,int> aOccsToAdd;
        int                             iNumOccs = pOccs->GetSize();
        bool                    bOccInStored;
        for(int i=0; i<iNumOccs; i++){
                bOccInStored = false;
                for(int j=0; !bOccInStored && j<iNumStoredOccs; j++){
                        if(pOccs->GetAt(i)==aStoredOccs.GetAt(j)){
                                bOccInStored = true;
                        }
                }
                if(!bOccInStored){
                        aOccsToAdd.Add(pOccs->GetAt(i));
                }
        }
        int iNumOccsToAdd = aOccsToAdd.GetSize();
        #ifdef _DEBUG
                TRACE(_T("occs that will be added for item %s\n"),(LPCTSTR)pItemBaseToRefresh->GetName());
                if(iNumOccsToAdd==0){
                        TRACE(_T("None\n"));
                }
                for(int j=0; j<iNumOccsToAdd; j++){
                        TRACE(_T("add occ = %d\n"),aOccsToAdd.GetAt(j));
                }
        #endif

        //bExistTittleOfOccs
        bool bExistTittleOfOccs = false;
        CArray<int,int> aZeroOcc;
                CArray<HTREEITEM,HTREEITEM> aTittleItem;
                CArray<bool,bool> aTittleFound;
                aZeroOcc.Add(0);
        if(pTree->SearchItemBase(pItemBaseToRefresh, aZeroOcc, aTittleItem, aTittleFound)==1){
            bExistTittleOfOccs = true;
        }

        //bNeedTittleOfOccs
        CArray<int,int> aUnion;
        CCapiEUtils::Union(aStoredOccs,*pOccs, aUnion);
        bool bNeedTittleOfOccs;
        if( ( /*1*/                      aUnion.GetSize()==1 && aUnion.GetAt(0)==1) ||
            ( /*0,1 =>1 will replace 0*/ aUnion.GetSize()==2 &&  CCapiEUtils::GetIndex(0,aUnion)!=-1 && CCapiEUtils::GetIndex(1,aUnion)!=-1) ){
            bNeedTittleOfOccs = false;
        } else {
            bNeedTittleOfOccs = true;
        }

        //iOccToRemove = occ to remove, before process the array with the occs to add (-1 => no one will be removed)
        int iOccToRemove = -1;
        if( (bNeedTittleOfOccs && !bExistTittleOfOccs) || (!bNeedTittleOfOccs && bExistTittleOfOccs)){
            ASSERT( iNumStoredOccs==0 || (iNumStoredOccs==1 && (aStoredOccs.GetAt(0)==0 || aStoredOccs.GetAt(0)==1)) );
            if( iNumStoredOccs>0 ){
                iOccToRemove = aStoredOccs.GetAt(0);
            }
            if(iOccToRemove!=0){

                CArray<int,int> aAux;
                for(int i=0; i<iNumOccsToAdd; i++){
                    aAux.Add(aOccsToAdd.GetAt(i));
                }
                aOccsToAdd.RemoveAll();
                aOccsToAdd.Add(iOccToRemove);
                for(int i=0; i<iNumOccsToAdd; i++){
                    aOccsToAdd.Add(aAux.GetAt(i));
                }
            }
        }

        //aOccsToRefresh : array with the occs to refresh
        CArray<int,int> aOccsToRefresh;
        int             iStoredOcc;
        if( (bNeedTittleOfOccs && bExistTittleOfOccs) || (!bNeedTittleOfOccs && !bExistTittleOfOccs && aUnion.GetSize()==1 && aUnion.GetAt(0)==1)){
            for(int i=0; i<iNumStoredOccs; i++){
                iStoredOcc = aStoredOccs.GetAt(i);
                if(iStoredOcc!=0 && CCapiEUtils::GetIndex(iStoredOcc,*pOccs)!=-1){
                    aOccsToRefresh.Add(iStoredOcc);
                }
            }
        }
        int iNumOccsToRefresh = aOccsToRefresh.GetSize();

                //remove occs
        HTREEITEM hInsertAfter  = NULL;
        HTREEITEM hParent       = NULL;
        if( iOccToRemove!=-1 ){
            TRACE(_T("remove occ %d\n"), iOccToRemove);
            if(RemoveItem(pItemBaseToRefresh,iOccToRemove, 0, &hInsertAfter, &hParent)){
                TRACE(_T("Done\n"));
            } else {
                ASSERT(false);
            }
        }

        //built tittle of occs
        HTREEITEM hTittleOfOccs = NULL;
        if( bNeedTittleOfOccs && !bExistTittleOfOccs){
            ASSERT(iOccToRemove!=-1); //to fill hInsertAfter and hParent
            TRACE(_T("build tittle of occs, with %d occs\n"), aOccsToAdd.GetSize() );
            int iDataOccs = ((CDEGroup*)pItemBaseToRefresh)->GetDataOccs();
            hTittleOfOccs = InsertItemOccsTittle( pItemBaseToRefresh, iDataOccs, pTree, hInsertAfter, hParent);
            TRACE(_T("Done\n"));
        } else if( bExistTittleOfOccs ){
            hTittleOfOccs = aTittleItem.GetAt(0);
        }


        //Refresh occs
        for( int i=0; i<iNumOccsToRefresh; i++){
            ASSERT( aStoredOccs.GetSize()==ahItems.GetSize() );
            TRACE(_T("refresh occ %d\n"),aOccsToRefresh.GetAt(i));
            int     ihItemIdx = CCapiEUtils::GetIndex( aOccsToRefresh.GetAt(i), aStoredOccs );

            bool bOccIcon = true;

            if( ReCreateTreeItem( pTree, ahItems.GetAt(ihItemIdx), pWantedField, iWantedOcc, bOccIcon) ){
                bSearchOK = true;
            }
            TRACE(_T("Done\n"));
        }

        //add occs
        iNumOccsToAdd = aOccsToAdd.GetSize();
        for(int i=0; i<iNumOccsToAdd; i++){
            TRACE(_T("add occ %d\n"),aOccsToAdd.GetAt(i));
            HTREEITEM hGivenParent;
            if( bNeedTittleOfOccs ){
                hGivenParent = hTittleOfOccs;
            } else {
                hGivenParent = hParent;
            }
            bool bOccIcon = true;
            if( (aUnion.GetSize()==1 && aUnion.GetAt(0)==1) || (aUnion.GetSize()==2 && CCapiEUtils::GetIndex(0,aUnion)!=-1 && CCapiEUtils::GetIndex(1,aUnion)!=-1) ){
                bOccIcon = false; //bOccIcon : false is a special case in that even the first occ doesn't have the occ icon (doesn't have the numeric occ index label too)
            }
            //int iOccIdxInStored = CCapiEUtils::GetIndex( aOccsToAdd.GetAt(i), aStoredOccs);
            if( InsertMultiOccItem( -1/*iNodeIdx not used*/,
                                    pItemBaseToRefresh,
                                    -1/*iItemIndex not used*/,
                                    aOccsToAdd.GetAt(i) /*iItemOcc*/,
                                    hGivenParent,
                                    hInsertAfter,
                                    pWantedField,
                                    iWantedOcc,
                                    -1/*iMaxItemLabelLength*/,
                                    true /*to insert ONLY the specified occ*/,
                                    bOccIcon ) ){
                bSearchOK = true;
            }


            TRACE(_T("Done\n"));
        }

        if( bNeedTittleOfOccs && bExistTittleOfOccs ){
            TRACE(_T("refresh tittle of occs\n") );
            if(RefreshTittleOfOccs(pTree,hTittleOfOccs)){
                TRACE(_T("Done\n"));
            }
        }

        if( m_InsertItemConfigArray.GetSize()>0 ){
            DoStackCalls();
        }
    }

    return  bSearchOK;
}

//REMOVE EXISTING HITEM, RE-CREATE AND ADD IN THE ORIGINAL POSITION
bool CCaseTree::ReCreateTreeItem(   CGenericTreeCtrl*   pTree,
                                    /*CDEItemBase*        pItemBaseToRefresh,
                                    int                 iItemOccToRefresh,  */
                                    HTREEITEM           hItem,
                                    CDEField*           pWantedField,
                                    int                 iWantedOcc,
                                    bool                bOccIcon)
{
    bool    bSearchOK = false;

    HTREEITEM hPrevBrother = pTree->GetNextItem( hItem, TVGN_PREVIOUS );
    HTREEITEM hInsertAfter = hPrevBrother ? hPrevBrother : TVI_FIRST;

    pTree->DeleteChilds(hItem);


    //CONSTRUCT PARAMS TO CALL InsertMultiOccItem
    int             iNodeIdx;
    CDEItemBase*    pItem;
    int             iItemIndex;
    int             iItemOcc;
    HTREEITEM       hItemParent;
    int             iMaxItemLength;
    bool            bSpecialInsert;


    CTreeItemInfo* pItemInfo = (CTreeItemInfo*) pTree->GetItemData( hItem );
    ASSERT(pItemInfo);

    iNodeIdx        = -1;   //Not used
    pItem           = pItemInfo->GetItem();//pItemBaseToRefresh;
    iItemIndex      = pItemInfo->GetIndex();
    iItemOcc        = pItemInfo->GetOccurrence();       //==0 si pItemInfo->GetTypeOfInfo()==CS_OCCURRENCES_TITTLE

    //FABN Feb 17, 2003
    if( iItemOcc==0 ){
        iItemOcc=-1;    //<- for insert every occs of pItem
    }

    hItemParent     = pTree->GetParentItem(hItem);
    iMaxItemLength  = pItemInfo->GetMaxItemLabelLength();

    //FABN Feb 14, 2003
    bSpecialInsert  = iItemOcc==-1 ? false : pItemInfo->GetTypeOfInfo()==CS_OCCURRENCE_INDEX;

    pTree->DestroyItemInfo(hItem, true);
    pTree->DeleteItem(hItem);

    bSearchOK = InsertMultiOccItem( iNodeIdx,
                                    pItem,
                                    iItemIndex,
                                    iItemOcc,
                                    hItemParent,
                                    hInsertAfter,
                                    pWantedField,
                                    iWantedOcc,
                                    iMaxItemLength,
                                    bSpecialInsert,
                                    bOccIcon);

    if( m_InsertItemConfigArray.GetSize()>0 ){
        DoStackCalls();
    }

    return bSearchOK;
}

CWnd* CCaseTree::GetWnd()
{
    return m_pTree ? (CWnd*) m_pTree : NULL;
}


HTREEITEM CCaseTree::InsertItemOccsTittle( CDEItemBase* pItem, int iNumOccsInTheTittle, CGenericTreeCtrl* pTree, HTREEITEM hInsertAfter, HTREEITEM hParent)
{
    //item label
    CString csItemLabel;
    if( m_bShowNames ){
        csItemLabel = pItem->GetName();
    } else {
        csItemLabel = pItem->GetLabel();
    }
    csItemLabel.TrimRight();
    if(csItemLabel.GetAt(csItemLabel.GetLength()-1)!=':'){
        csItemLabel += (_T(" : ") + IntToString(iNumOccsInTheTittle) + _T(" occurrences"));
    } else {
        csItemLabel += (_T(" ") + IntToString(iNumOccsInTheTittle) + _T(" occurrences"));
    }

    //tooltip
    CString csToolTip;
    if(m_bShowNames){
        csToolTip = pItem->GetLabel() + _T(" ( ") + IntToString(iNumOccsInTheTittle) + _T(" occurrences )");
    } else {
        csToolTip = pItem->GetName() + _T(" ( ") + IntToString(iNumOccsInTheTittle) + _T(" occurrences )");
    }

    //item icons
    int iNonSelectedIconIdx = 8;
    int iSelectedIconIdx    = 9;

    //item key
    CString csKey   = pTree->GetKey( hParent ) + _T("-") + IntToString(pItem->GetSymbol()) + /*"#" + IntToString(iItemIndex) +*/ _T("#0");

    //TreeItemInfo
    CTreeItemInfo * pItemInfo = new CTreeItemInfo(  -1/*ignored*/, csKey, -1/*iItemIndex ignored*/, CS_GROUP_OCCURRENCES_TITTLE, 0/*occurrence index*/, iNumOccsInTheTittle, -1, iNonSelectedIconIdx, iSelectedIconIdx, csToolTip);
    pItemInfo->SetItem( pItem );

    HTREEITEM hItem = pTree->Insert( csItemLabel, hParent, hInsertAfter, iNonSelectedIconIdx,iSelectedIconIdx);
    pTree->SetItemData( hItem, (DWORD) pItemInfo );


    return hItem;
}

bool CCaseTree::RemoveItem( CDEItemBase* pItem, int iOcc, int iLayerIdx, HTREEITEM* hInsertAfter, HTREEITEM* hParent )
{
    bool    bRemoved = false;

    ASSERT( iLayerIdx==0 ); //TODO : RemoveItem for multi layers tree

    //get the layer
    CGenericTreeCtrl* pTree = m_pTreeArray->GetAt(iLayerIdx);

    //get the hitem
    CArray<int,int> aOccs;
    aOccs.Add(iOcc);
    CArray<HTREEITEM,HTREEITEM> ahItems;
    CArray<bool,bool> aFoundOccs;
    pTree->SearchItemBase( pItem, aOccs, ahItems, aFoundOccs);

    if( ahItems.GetSize()>0 ){
        ASSERT( ahItems.GetSize()==1    &&
                aFoundOccs.GetSize()==1 &&
                aFoundOccs.GetAt(0)     &&
                ((CTreeItemInfo*)pTree->GetItemData(ahItems.GetAt(0)))->GetOccurrence()==iOcc );

        HTREEITEM hItem         = ahItems.GetAt(0);
                 *hParent       = pTree->GetParentItem(hItem);
        HTREEITEM hPrevBrother  = pTree->GetNextItem( hItem, TVGN_PREVIOUS );
                 *hInsertAfter  = hPrevBrother ? hPrevBrother : TVI_FIRST;
        pTree->DeleteChilds(hItem);
        pTree->DestroyItemInfo(hItem, true);
        pTree->DeleteItem(hItem);
        bRemoved = true;
    }

    return bRemoved;
}

bool CCaseTree::RefreshTittleOfOccs(CGenericTreeCtrl* pTree, HTREEITEM hItem)
{
    ASSERT( hItem!=NULL );
    CTreeItemInfo* pInfo =  (CTreeItemInfo*) pTree->GetItemData(hItem); ASSERT(pInfo!=NULL);

    int iTypeOfInfo = pInfo->GetTypeOfInfo();
    ASSERT( iTypeOfInfo==CS_LEVELS_TITTLE               ||
            iTypeOfInfo==CS_GROUP_OCCURRENCES_TITTLE    ||
            iTypeOfInfo==CS_ROSTER_OCCURRENCES_TITTLE  );

    CDEItemBase* pItem = pInfo->GetItem();
    int iDataOccs = ((CDEGroup*)pItem)->GetDataOccs();

    CString csItemLabel;
    if( m_bShowNames ){
        csItemLabel = pItem->GetName();
    } else {
        csItemLabel = pItem->GetLabel();
    }
    csItemLabel.TrimRight();
    if(csItemLabel.GetAt(csItemLabel.GetLength()-1)!=':'){
        csItemLabel += (_T(" : ") + IntToString(iDataOccs) + _T(" occurrences"));
    } else {
        csItemLabel += (_T(" ") + IntToString(iDataOccs) + _T(" occurrences"));
    }

    pTree->SetItemText(hItem, csItemLabel);

    return true;
}

bool CCaseTree::RefreshNodeLabels()
{
    bool bFoundAllNodes = true;
    int iNumNodes = m_pCapiRunAplEntry->GetNNodes();
    for( int iNodeIdx=0; iNodeIdx<iNumNodes; iNodeIdx++){
        if(!RefreshNodeLabel(iNodeIdx)){
            bFoundAllNodes = false;
        }
    }

    return bFoundAllNodes;
}

bool CCaseTree::RefreshNodeLabel(int iNodeIdx)
{
    bool bFoundNode = true;

    //Build the key
    CString csNodeKey = IntToString(iNodeIdx);

    //Search the hitem
    HTREEITEM   hNodeItem   = NULL;
    int         iIsInRoot   = -1;
    CString     csNodeLabel = _T("");
    if( m_pTree->SearchItem( csNodeKey, &hNodeItem, &iIsInRoot )){
        //replace the current label
        csNodeLabel = m_pCapiRunAplEntry->GetNodeLabel(iNodeIdx);

        if(hNodeItem!=NULL){
            m_pTree->SetItemText( hNodeItem, csNodeLabel );
        } else if( iIsInRoot==1){
            //for capie tree
            SetWindowText( csNodeLabel );
        }
    } else {
        bFoundNode = false;
    }

    return bFoundNode;
}

/*Used to set the first time value of m_iLastOpenNodeIdx*/
int CCaseTree::FirstSetOfLastNodeIdx(int iCurLastOpenNodeIdx/*=-1*/)
{
    ASSERT( m_iLastOpenNodeIdx==-1 );
    if( m_iLastOpenNodeIdx==-1 ){
        if(iCurLastOpenNodeIdx==-1){
            int iNumNodes = m_pCapiRunAplEntry->GetNNodes();
            int iCurLastOpenNodeIdxAux = -1;
            for( int iNodeIdx=0; iNodeIdx<iNumNodes; iNodeIdx++){
                if(m_pCapiRunAplEntry->IsOpen(iNodeIdx)){
                    iCurLastOpenNodeIdxAux = iNodeIdx;
                }
            }
            m_iLastOpenNodeIdx = iCurLastOpenNodeIdxAux;
        } else {
            m_iLastOpenNodeIdx = iCurLastOpenNodeIdx;
            ASSERT( 0<=m_iLastOpenNodeIdx && m_iLastOpenNodeIdx<m_pCapiRunAplEntry->GetNNodes() );
        }
    }
    return m_iLastOpenNodeIdx;
}

bool CCaseTree::IsItemSelectable(CGenericTreeCtrl* pTree, HTREEITEM hItem)
{
    /*not allow select for null tree or null item*/
    if(!pTree || !hItem){
        return false;
    }

    /*get the associated info and ask to the behaviour object*/
    CTreeItemInfo*  pInfo   = (CTreeItemInfo*) pTree->GetItemData(hItem);
    CDEItemBase*    pItem   = pInfo->GetItem();
    int             iOcc    = pInfo->GetOccurrence();

    ASSERT( m_pCapiRunAplEntry );
    return m_pCapiRunAplEntry ? m_pCapiRunAplEntry->IsItemSelectable( pItem, iOcc ) : false;
}
