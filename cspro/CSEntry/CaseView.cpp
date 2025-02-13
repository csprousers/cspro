// CaseView.cpp : implementation file
//
#include "StdAfx.h"
#include "CaseView.h"
#include "MainFrm.h"
#include "Rundoc.h"
#include "RunView.h"
#include <zCaseO/Case.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[]= __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CCaseView


BOOL bSelOK = FALSE;
HTREEITEM hInsertNodeItem = NULL;
BOOL bBuildingTree = FALSE;

IMPLEMENT_DYNCREATE(CCaseView, CTreeView)

CCaseView::CCaseView()
{
    m_bSelChangeUpdate = false;
    /* Moved to CMainFrame
    m_bSort = true;
    */
    m_bAllowSel = false;
    m_iCaseNumber =-1;
}

CCaseView::~CCaseView()
{
}


BEGIN_MESSAGE_MAP(CCaseView, CTreeView)
    //{{AFX_MSG_MAP(CCaseView)
    ON_NOTIFY_REFLECT(TVN_SELCHANGED, OnSelchanged)
    ON_NOTIFY_REFLECT(TVN_SELCHANGING, OnSelchanging)
    ON_NOTIFY_REFLECT(TVN_KEYDOWN, OnKeydown)
    ON_NOTIFY_REFLECT(TVN_DELETEITEM, OnDeleteitem)
    ON_NOTIFY_REFLECT(TVN_ITEMEXPANDING, OnItemexpanding)
    ON_WM_RBUTTONDOWN()
    ON_WM_GETDLGCODE()
    ON_COMMAND(ID_ADD_CASE, OnAdd)
    ON_COMMAND(ID_DEL_CASE, OnDeleteCase)
    ON_COMMAND(ID_INSRT_CASE, OnInsertCase)
    ON_COMMAND(ID_MODIFY_CASE, OnModifyCase)
    ON_COMMAND(ID_CASE_LISTING_VIEW_QUESTIONNAIRE, OnViewQuestionnaire)
    ON_NOTIFY_REFLECT(NM_DBLCLK, OnDblclk)
    ON_COMMAND(ID_FIND_CASE, OnFindCase)
    ON_COMMAND(ID_ADD_NODE, AddNode)
    ON_COMMAND(ID_DELETE_NODE, DeleteNode)
    ON_COMMAND(ID_INSERT_NODE, InsertNode)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()



/////////////////////////////////////////////////////////////////////////////
// CCaseView drawing

void CCaseView::OnDraw(CDC* pDC)
{
        UNREFERENCED_PARAMETER(pDC);
        // TODO: add draw code here
}

/////////////////////////////////////////////////////////////////////////////
// CCaseView diagnostics

#ifdef _DEBUG
void CCaseView::AssertValid() const
{
        CTreeView::AssertValid();
}
void CCaseView::Dump(CDumpContext& dc) const
{
        CTreeView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CCaseView message handlers
/////////////////////////////////////////////////////////////////////////////////
//
//      void CCaseView::BuildTree()
//
/////////////////////////////////////////////////////////////////////////////////
void CCaseView::BuildTree()
{
    CMainFrame* pFrame = (CMainFrame*)AfxGetMainWnd();
    CEntryrunDoc* pDoc = (CEntryrunDoc*) GetDocument();


    if( pDoc->GetRunApl() == NULL )
        return;

    bBuildingTree = TRUE;
    hInsertNodeItem = NULL;

    CWaitCursor wait;

    m_bSelChangeUpdate = false;
    CTreeCtrl& caseTree = GetTreeCtrl();

    bool bShowCaseLabels = pFrame->GetShowCaseLabels();

    auto pFormFile = pDoc->GetPifFile()->GetApplication()->GetRuntimeFormFiles().front();
    bool bMultiLevel = ( pFormFile->GetDictionary()->GetNumLevels() > 1 );

    // delete all the items
    SetRedraw(FALSE);
    caseTree.DeleteAllItems();

    int numCasesDisplayed = 0;

    const auto& case_summaries_to_show = ( pFrame->m_eCaseStatusToShow == CaseIterationCaseStatus::DuplicatesOnly ) ?
        pFrame->GetDuplicateCaseSummaries() : pFrame->GetCaseSummaries();

    for( const auto& case_summary : case_summaries_to_show )
    {
        if( !case_summary.IsPartial() && pFrame->m_eCaseStatusToShow == CaseIterationCaseStatus::PartialsOnly )
            continue;

        if( case_summary.GetDeleted() && pFrame->m_eCaseStatusToShow != CaseIterationCaseStatus::All )
            continue;

        AddCaseToTree(case_summary, bShowCaseLabels, bMultiLevel, numCasesDisplayed);
        ++numCasesDisplayed;
    }

    m_bSelChangeUpdate = true; // After all inserts then do this

    SetRedraw(TRUE);

    if( numCasesDisplayed == 0 && ( pFrame->m_eCaseStatusToShow == CaseIterationCaseStatus::PartialsOnly ||
        pFrame->m_eCaseStatusToShow == CaseIterationCaseStatus::DuplicatesOnly  ) )
    {
        AfxMessageBox(FormatText(_T("No %s cases are available so all cases will be shown."),
            ( pFrame->m_eCaseStatusToShow == CaseIterationCaseStatus::PartialsOnly ) ? _T("partial") : _T("duplicate")));

        pFrame->m_eCaseStatusToShow = CaseIterationCaseStatus::NotDeletedOnly;
        BuildTree();
    }

    UpdateInsertText();

    pFrame->GetRunView()->InvalidateRect(NULL);
    pFrame->GetRunView()->UpdateWindow();

    bBuildingTree = FALSE;
}


int CCaseView::PartialSaveStatusModeToImageIndex(PartialSaveMode eMode) const
{
    switch( eMode )
    {
        case PartialSaveMode::Add:
            return 3;

        case PartialSaveMode::Modify:
            return 4;

        default: // PartialSaveMode::Verify
            return 5;
    }
}


/////////////////////////////////////////////////////////////////////////////////
//
//      void CCaseView::OnInitialUpdate()
//
/////////////////////////////////////////////////////////////////////////////////
void CCaseView::OnInitialUpdate()
{
    CTreeView::OnInitialUpdate();

//-------------------------------------------
//      set styles
//-------------------------------------------
    long lStyle = GetWindowLong (this->m_hWnd, GWL_STYLE);
    lStyle |= TVS_HASBUTTONS;
    lStyle |= TVS_HASLINES;
    lStyle |= TVS_LINESATROOT;
    lStyle |= TVS_SHOWSELALWAYS;
    SetWindowLong (this->m_hWnd, GWL_STYLE, lStyle);

    if (m_cImageList.GetSafeHandle() == NULL)
    {
        m_cImageList.Create(16, 16, ILC_COLORDDB, 0, 4); // x,y of icon, flags, initial sz, sz2grow by
        m_cImageList.SetBkColor(RGB (255,255,255));

        m_cImageList.Add (AfxGetApp()->LoadIcon (IDI_EMPTY));           // 0
        m_cImageList.Add (AfxGetApp()->LoadIcon (IDI_COMPLETE));        // 1
        m_cImageList.Add (AfxGetApp()->LoadIcon (IDI_NODE));            // 2
        m_cImageList.Add (AfxGetApp()->LoadIcon (IDI_PARTIAL_ADD));     // 3
        m_cImageList.Add (AfxGetApp()->LoadIcon (IDI_PARTIAL_MOD));     // 4
        m_cImageList.Add (AfxGetApp()->LoadIcon (IDI_PARTIAL_VER));     // 5
        m_cImageList.Add(AfxGetApp()->LoadIcon(IDI_COMPLETE_VER));      // 6
        m_cImageList.Add(AfxGetApp()->LoadIcon(IDI_DELETED));           // 7

        GetTreeCtrl().SetImageList (&m_cImageList, TVSIL_NORMAL);
    }

    ((CMainFrame*)GetParentFrame())->BuildKeyArray();
    BuildTree();
    EnableWindow(TRUE);
}

/////////////////////////////////////////////////////////////////////////////////
//
//      void CCaseView::OnSelchanged(NMHDR* pNMHDR, LRESULT* pResult)
//
/////////////////////////////////////////////////////////////////////////////////
void CCaseView::OnSelchanged(NMHDR* pNMHDR, LRESULT* pResult)
{
    CEntryrunDoc* pDoc = (CEntryrunDoc*)GetDocument();

    APP_MODE mode = pDoc->GetAppMode();

    bool bProcess = ( mode == MODIFY_MODE || ( mode == VERIFY_MODE && m_bAllowSel ) );

    if( !bProcess || !m_bSelChangeUpdate || bSelOK )
    {
        *pResult = 0;
        return;
    }

    NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;
    HTREEITEM hSelItem = pNMTreeView->itemNew.hItem;

    if( hSelItem != NULL )
    {
        // REPO_TODO: will need to evaluate selections of second level nodes here
        CRunAplEntry* pRunApl = pDoc->GetRunApl();

        if( pRunApl->IsEndingModifyMode() )
            return;

        if( pRunApl->HasModifyStarted() )
            pRunApl->ModifyStop();

        pRunApl->Stop();

        pDoc->SetAppMode(NO_MODE);

        if( mode == MODIFY_MODE )
            OnModifyCase();

        else if(mode == VERIFY_MODE)
        {
            // need to have something like OnModifyCase above that will call into CMainFrame
            //pRunApl->Start(CRUNAPL_VERIFY);
        }
    }

    else {
        return;
    }

    *pResult = 0;

}
/////////////////////////////////////////////////////////////////////////////////
//
//      void CCaseView::OnSelchanging(NMHDR* pNMHDR, LRESULT* pResult)
//
/////////////////////////////////////////////////////////////////////////////////
void CCaseView::OnSelchanging(NMHDR* pNMHDR, LRESULT* pResult)
{
    NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;
    CEntryrunDoc* pDoc = (CEntryrunDoc*) GetDocument();

    if( pDoc == NULL )
        return;

    CRunAplEntry* pApl = pDoc->GetRunApl();
    APP_MODE mode = pDoc->GetAppMode();

    if( (mode == ADD_MODE || ( mode == VERIFY_MODE && !m_bAllowSel ) ) && !bSelOK )
    {
        *pResult = 1;
        return;
    }

    BOOL bProcess =  (mode == MODIFY_MODE) || (mode == VERIFY_MODE && m_bAllowSel);

    if( !bProcess )
    {
        *pResult = 0;
        return;
    }

    bool bCheckInsertNodeItem = !( !bSelOK && pNMTreeView->itemOld.hItem != pNMTreeView->itemNew.hItem &&  pApl && pDoc->GetQModified() );

    if( bCheckInsertNodeItem && hInsertNodeItem ) {
        *pResult = 1;
        return;
    }
    else if( !bSelOK && pNMTreeView->itemOld.hItem != pNMTreeView->itemNew.hItem && pApl ) {
        bool    bCancel = true;
        if(pDoc->GetAppMode() == MODIFY_MODE) {
            if(!AllowSelChange(pNMTreeView->itemNew.hItem)){
                *pResult = 1;
                return;
            }
        }

        if( pDoc->GetQModified() )
            if( AfxMessageBox (_T("Cancel current questionnaire?"), MB_YESNO | MB_DEFBUTTON2 | MB_ICONQUESTION) != IDYES )
                bCancel = false;

        if( !bCancel ) {
            CEntryrunView* pView = ((CMainFrame*)AfxGetMainWnd())->GetRunView();
            UpdateData(FALSE);
            *pResult = 1;
            pView->PostMessage(WM_SETFOCUS,(WPARAM)this,0);
            return;
        }
        else {
            pDoc->SetQModified(FALSE);

            // formally closing the CsDriver session
            pApl->ResetDoorCondition();
        }
    }

    *pResult = 0;
}

/////////////////////////////////////////////////////////////////////////////////
//
//      void CCaseView::OnKeydown(NMHDR* pNMHDR, LRESULT* pResult)
//
/////////////////////////////////////////////////////////////////////////////////
void CCaseView::OnKeydown(NMHDR* pNMHDR, LRESULT* pResult)
{
    TV_KEYDOWN* pTVKeyDown = (TV_KEYDOWN*)pNMHDR;

    if(GetKeyState(VK_CONTROL) < 0  && pTVKeyDown->wVKey == 'G') {
        CArray<UINT,UINT>* pParamArray = new CArray<UINT,UINT>;
        pParamArray->Add( pTVKeyDown->wVKey  );
        pParamArray->Add( 0/*nRepCnt not used*/);
        pParamArray->Add( 0/*nFlags not used*/);

        //The state of the virtual keys when fire the message
        bool bCtrl  = 1;
        bool bShift = GetKeyState(VK_SHIFT  )<0;
        bool bAlt   = GetKeyState(VK_MENU   )<0;

        pParamArray->Add( bCtrl  );
        pParamArray->Add( bShift );
        pParamArray->Add( bAlt   );

        AfxGetMainWnd()->PostMessage(UWM::CSEntry::AcceleratorKey, (WPARAM)pParamArray, 0);
        return;

    }

    //FABN Jan 15, 2003
    //TODO : THIS CODE MUST BE DELETED WHEN THE CASE VIEW LISTEN AGAIN THE ACCELERATOR KEYS
    bool bSerproProcess = pTVKeyDown->wVKey!=VK_DOWN && pTVKeyDown->wVKey!=VK_UP;
    if(bSerproProcess && pTVKeyDown->wVKey!=VK_DELETE && pTVKeyDown->wVKey!=VK_RETURN){

        CArray<UINT,UINT>* pParamArray = new CArray<UINT,UINT>;
        pParamArray->Add( pTVKeyDown->wVKey  );
        pParamArray->Add( 0/*nRepCnt not used*/);
        pParamArray->Add( 0/*nFlags not used*/);

        //The state of the virtual keys when fire the message
        bool bCtrl  = GetKeyState(VK_CONTROL)<0;
        bool bShift = GetKeyState(VK_SHIFT  )<0;
        bool bAlt   = GetKeyState(VK_MENU   )<0;

        pParamArray->Add( bCtrl  );
        pParamArray->Add( bShift );
        pParamArray->Add( bAlt   );

        AfxGetMainWnd()->PostMessage(UWM::CSEntry::AcceleratorKey, (WPARAM)pParamArray, 0);
    }

    *pResult = 0;
    CTreeCtrl& treeCtrl = GetTreeCtrl();

    CEntryrunDoc* pDoc = (CEntryrunDoc*) GetDocument();
    CRunAplEntry* pRunApl = pDoc->GetRunApl();

    BOOL bProcess = pDoc && pRunApl  && treeCtrl.GetCount() > 0 &&(pDoc->GetAppMode()== NO_MODE);
    if(!bProcess) {
        return;
    }

    //WE are guaranteed that a dict exists and a form exists
    auto pDict = pDoc->GetPifFile()->GetApplication()->GetRuntimeFormFiles().front()->GetDictionary();
    ASSERT(pDict);
    // TODO: Add your control notification handler code here
    switch(pTVKeyDown->wVKey ){
        case VK_DELETE:
            OnDeleteCase();
            break;
        case VK_INSERT:
            OnInsertCase();
            break;
        case VK_RETURN:
            if(pDoc && pDoc->GetAppMode() == NO_MODE) {
                //Get the currently selected hItem and send a message to
                //mainfrm to modify
                CTreeCtrl& caseTree = this->GetTreeCtrl();
                HTREEITEM hItem = caseTree.GetSelectedItem();
                int iImage = 0;
                if(hItem) {
                    caseTree.GetItemImage(hItem,iImage,iImage);
                    if(iImage == 5) {
                        GetParentFrame()->PostMessage(WM_COMMAND,ID_VERIFY);
                        CEntryrunView* pView = ((CMainFrame*)GetParentFrame())->GetRunView();
                        this->RedrawWindow();
                        pView->RedrawWindow();
                        pView->GetParent()->RedrawWindow();
                        pView->SetFocus();

                    }
                    else {
                        OnModifyCase();
                    }
                }
            }

            break;

        default:
            break;
    }
}

/////////////////////////////////////////////////////////////////////////////////
//
//      void CCaseView::DeleteCase()
//
/////////////////////////////////////////////////////////////////////////////////
void CCaseView::OnDeleteCase()
{
    CMainFrame* pFrame = (CMainFrame*)AfxGetMainWnd();
    CEntryrunDoc* pDoc = (CEntryrunDoc*)GetDocument();
    CRunAplEntry* pRunApl = pDoc->GetRunApl();

    if( pRunApl == NULL )
        return;

    if( pDoc->GetPifFile()->GetDeleteLockFlag() )
        return;

    // REPO_TEMP: can we get here if we're not in NO_MODE? if so, stop whatever is happening before deleting the case

    CTreeCtrl& caseTree = GetTreeCtrl();
    HTREEITEM hItem = caseTree.GetSelectedItem();

    if( hItem == NULL )
        return;

    NODEINFO* pNodeInfo = (NODEINFO*)caseTree.GetItemData(hItem);

    if( pNodeInfo->iNode > 0 ) // if they've selected a level node, try to delete that instead
    {
        DeleteNode();
        return;
    }

    DataRepository* pInputRepo = pRunApl->GetInputRepository();
    int iCaseNumber = pNodeInfo->case_number;

    try
    {
        // prompt when deleting a case
        bool deleted = !pNodeInfo->case_summary.GetDeleted();

        if( deleted )
        {
            CString csDeletePrompt = FormatText(MGF::GetMessageText(MGF::DeleteCase).c_str(), pNodeInfo->case_summary.GetKey().GetString());

            if( AfxMessageBox(csDeletePrompt, MB_YESNO | MB_DEFBUTTON2 | MB_ICONQUESTION) == IDNO )
                return;
        }

        pInputRepo->DeleteCase(pNodeInfo->case_summary.GetPositionInRepository(), deleted);
    }

    catch( const DataRepositoryException::Error& exception )
    {
        ErrorMessage::Display(exception);
        return;
    }

    // update the UI
    pFrame->BuildKeyArray();

    BuildTree();
    Invalidate();
    UpdateWindow();

    SetCaseNumber(iCaseNumber);
    RestoreSelectPos();

    PostMessage(WM_SETFOCUS);
}


void CCaseView::OnDeleteitem(NMHDR* pNMHDR, LRESULT* pResult)
{
    CTreeCtrl& caseTree = GetTreeCtrl();

    NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;
    HTREEITEM hItem = pNMTreeView->itemOld.hItem;

    NODEINFO* pNodeInfo = (NODEINFO*)caseTree.GetItemData(hItem);

    if( pNodeInfo != NULL )
    {
        delete pNodeInfo;
        caseTree.SetItemData(hItem,NULL);
    }

    *pResult = 0;
}

void CCaseView::OnItemexpanding(NMHDR* pNMHDR, LRESULT* pResult)
{
    NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;

    CEntryrunDoc* pDoc = (CEntryrunDoc*) GetDocument();
    if(!pDoc || !pDoc->GetRunApl() || pDoc->GetAppMode() == ADD_MODE ||pDoc->GetAppMode() == MODIFY_MODE ||pDoc->GetAppMode() == VERIFY_MODE ) {
        *pResult = 1;
        return;
    }

    HTREEITEM hNewItem  = pNMTreeView->itemNew.hItem;

    NODEINFO* pNodeInfo  = (NODEINFO*)this->GetTreeCtrl().GetItemData(hNewItem);
    if(pNodeInfo->iNode == 0) {
        HTREEITEM hChild = this->GetTreeCtrl().GetChildItem(hNewItem);
        while(hChild) {
           this->GetTreeCtrl().DeleteItem(hChild);
           hChild =  this->GetTreeCtrl().GetChildItem(hNewItem);
        }

        this->BuildNodeTree(hNewItem);
    }

    //This should be only in the multi level case

    //Collapse the old tree item
    //if the node index is zero
    //Delete the dummy item
    //Build the node

    *pResult = 0;
}

//////////////////////////////////////////////////////////////////////////////////////
//
//          void CCaseView::BuildNodeTree()
//
//////////////////////////////////////////////////////////////////////////////////////
void CCaseView::BuildNodeTree(HTREEITEM hNodeItem)
{
    CEntryrunDoc* pDoc = (CEntryrunDoc*)GetDocument();
    CRunAplEntry* pRunApl = ( pDoc == nullptr ) ? nullptr : pDoc->GetRunApl();

    if( pRunApl == nullptr || pDoc->GetAppMode() == ADD_MODE || pDoc->GetAppMode() == VERIFY_MODE || pDoc->GetAppMode() == MODIFY_MODE )
        return;

    CTreeCtrl& caseTree = GetTreeCtrl();

    NODEINFO* pNodeInfo = (NODEINFO*)caseTree.GetItemData(hNodeItem);

    // read the case to get the level nodes
    DataRepository* pInputRepo = pRunApl->GetInputRepository();
    auto data_case = pInputRepo->GetCaseAccess()->CreateCase();

    try
    {
        pInputRepo->ReadCase(*data_case, pNodeInfo->case_summary.GetPositionInRepository());
    }

    catch( const DataRepositoryException::Error& exception )
    {
        ErrorMessage::Display(exception);
        return;
    }

    const auto& case_levels = data_case->GetAllCaseLevels();

    // only items from level 2 and up have parents
    HTREEITEM hParent[MaxNumberLevels] = { nullptr };

    // Level 1 parent is TVI_ROOT
    // TVI_ROOT   => parent of level 1
    // hParent[0] => parent == Level 2
    // hParent[1] => parent == Level 3
    // hParent[2] => parent == Level 4 and so on...

    int key_sizes[MaxNumberLevels] = { 0 };

    for( size_t i = 0; i < case_levels.size(); ++i )
    {
        if( i == 0 )
        {
            hParent[0] = hNodeItem;
            continue;
        }

        const CaseLevel* case_level = case_levels[i];
        size_t level_number = case_level->GetCaseLevelMetadata().GetDictLevel().GetLevelNumber();
        CString display_key = case_level->GetLevelKey();

        // trim the parts of the key from other levels
        for( size_t j = 1; j <= level_number; ++j )
        {
            if( j >= 2 )
                display_key = display_key.Mid(key_sizes[j - 1]);

            if( j == level_number )
                key_sizes[j] = display_key.GetLength();
        }

        TV_INSERTSTRUCT TreeCtrlItem;
        TV_ITEM tvItem;

        TreeCtrlItem.hParent = hParent[level_number - 1];
        TreeCtrlItem.hInsertAfter = TVI_LAST;

        tvItem.mask = TVIF_TEXT | TVIF_PARAM;

        tvItem.pszText = display_key.GetBuffer();
        TreeCtrlItem.item = tvItem;

        NODEINFO* pNewNodeInfo = new NODEINFO { pNodeInfo->case_summary, (int)i, pNodeInfo->case_number };

        HTREEITEM hCurrentItem = caseTree.InsertItem(&TreeCtrlItem);
        caseTree.SetItemImage(hCurrentItem,2,2);
        caseTree.SetItemData(hCurrentItem,(DWORD)pNewNodeInfo);

        hParent[level_number] = hCurrentItem;
    }
}

//////////////////////////////////////////////////////////////////////////////////////
//
//          void CCaseView::UpdateInsertText()
//
//////////////////////////////////////////////////////////////////////////////////////
void CCaseView::UpdateInsertText()
{
    CEntryrunDoc* pDoc = (CEntryrunDoc*)GetDocument();

    if( pDoc->GetAppMode() != ADD_MODE )
        return;

    CTreeCtrl& caseTree = GetTreeCtrl();
    CRunAplEntry* pRunApl = pDoc->GetRunApl();

    SetRedraw(TRUE);
    HTREEITEM hItem = NULL;

    const WriteCaseParameter* write_case_parameter = pRunApl->GetEntryDriver()->GetWriteCaseParameter();

    if( write_case_parameter == nullptr )
        hItem = caseTree.InsertItem(_T("<Adding Case>"));

    else if( write_case_parameter->IsInsertParameter() )
    {
        HTREEITEM hSearch = caseTree.GetChildItem(TVI_ROOT);

        while( hSearch != NULL )
        {
            NODEINFO* pNodeInfo = (NODEINFO*)caseTree.GetItemData(hSearch);

            if( pNodeInfo->case_summary.GetPositionInRepository() == write_case_parameter->GetPositionInRepository() )
                break;

            hSearch = caseTree.GetNextSiblingItem(hSearch);
        }

        if( hSearch != NULL )
        {
            hSearch = caseTree.GetPrevSiblingItem(hSearch);

            if( hSearch == NULL )
                hSearch = TVI_FIRST;

            hItem = caseTree.InsertItem(_T("<Inserting Case>"),TVI_ROOT,hSearch);
        }
    }

    if( hItem != NULL )
    {
        bSelOK = TRUE;
        caseTree.Select(hItem,TVGN_CARET);
        bSelOK = FALSE;
    }
#ifdef REDO // REPO_TEMP: this looks like it may have to do with two-level applications
    else  {
        HTREEITEM hDelete =caseTree.GetChildItem(TVI_ROOT);
        CString sString;
        if(hInsertNodeItem) {
            caseTree.DeleteItem(hInsertNodeItem);
            hInsertNodeItem=NULL;
        }
        while(hDelete) {
            sString = caseTree.GetItemText(hDelete);
            if( sString.CompareNoCase(_T("<Inserting Case>")) == 0 ||sString.CompareNoCase(_T("<Adding Case>")) == 0 ) {
                break;
            }
            hDelete = caseTree.GetNextSiblingItem(hDelete);

        }
        if(hDelete) {
            caseTree.DeleteItem(hDelete);
        }
    }
#endif
}

//////////////////////////////////////////////////////////////////////////////////////
//
//          void CCaseView::OnRButtonDown()
//
//////////////////////////////////////////////////////////////////////////////////////
void CCaseView::OnRButtonDown(UINT nFlags, CPoint point)
{
    CEntryrunDoc* pDoc = (CEntryrunDoc*) GetDocument();
    CTreeCtrl& caseTree = GetTreeCtrl();
    HTREEITEM hItem = caseTree.HitTest(point, &nFlags );

    CMenu popMenu;
    popMenu.LoadMenu(IDR_CASE_MENU);
    CMenu* pMenu = popMenu.GetSubMenu(0);

    this->ClientToScreen(&point);

    constexpr int NumMenuItems = 12;
    ASSERT(pMenu->GetMenuItemCount() == NumMenuItems);

    if( hItem == nullptr || pDoc->GetAppMode() != NO_MODE )
    {
        for( int i = 0; i < NumMenuItems; ++i ) // disable all the options
            pMenu->EnableMenuItem(i, MF_GRAYED | MF_BYPOSITION);
    }

    else
    {
        NODEINFO* pNodeInfo = (NODEINFO*)caseTree.GetItemData(hItem);
        bool bNodeSelected = ( pNodeInfo != nullptr && pNodeInfo->iNode > 0 );

        CNPifFile* pPifFile = pDoc->GetPifFile();

        if( bNodeSelected || pPifFile->GetModifyLockFlag() )
            pMenu->EnableMenuItem(ID_MODIFY_CASE, MF_GRAYED); // modify case

        if( !bNodeSelected || pPifFile->GetModifyLockFlag() )
        {
            pMenu->EnableMenuItem(ID_ADD_NODE, MF_GRAYED); // add node
            pMenu->EnableMenuItem(ID_INSERT_NODE, MF_GRAYED); // insert node
        }

        if( pPifFile->GetAddLockFlag() )
        {
            pMenu->EnableMenuItem(ID_ADD_CASE, MF_GRAYED);
            pMenu->EnableMenuItem(ID_INSRT_CASE, MF_GRAYED);
        }

        if( pNodeInfo->case_summary.GetDeleted() )
        {
            MENUITEMINFO itemInfo;
            ZeroMemory(&itemInfo, sizeof(itemInfo));
            itemInfo.cbSize = sizeof(itemInfo);
            itemInfo.fMask = MIIM_STRING;
            pMenu->ModifyMenu(ID_DEL_CASE, MF_BYCOMMAND | MF_STRING, ID_DEL_CASE, _T("Undelete case"));
        }

        if( bNodeSelected || pPifFile->GetDeleteLockFlag() )
            pMenu->EnableMenuItem(ID_DEL_CASE, MF_GRAYED); // delete case

        if( !bNodeSelected || pPifFile->GetDeleteLockFlag() )
            pMenu->EnableMenuItem(ID_DELETE_NODE, MF_GRAYED); // delete node

        if( pPifFile->GetViewLockFlag() )
            pMenu->EnableMenuItem(ID_CASE_LISTING_VIEW_QUESTIONNAIRE, MF_GRAYED);

       caseTree.Select(hItem,TVGN_CARET);
       pMenu->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON,point.x,point.y,this);
    }
}



void CCaseView::MainFrameCallerHelper(WPARAM wParam)
{
    CMainFrame* pFrame = (CMainFrame*)AfxGetMainWnd();
    pFrame->SendMessage(WM_COMMAND,wParam);

    CEntryrunView* pView = pFrame->GetRunView();
    this->RedrawWindow();
    pView->RedrawWindow();
    pView->GetParent()->RedrawWindow();
}

void CCaseView::OnAdd()
{
    MainFrameCallerHelper(ID_ADD);
}

void CCaseView::OnInsertCase()
{
    MainFrameCallerHelper(ID_INSERT_CASE);
}

void CCaseView::OnModifyCase()
{
    MainFrameCallerHelper(ID_MODIFY);
}

void CCaseView::OnViewQuestionnaire()
{
    MainFrameCallerHelper(ID_VIEW_QUESTIONNAIRE);
}



//////////////////////////////////////////////////////////////////////////////////////
//
//          void CCaseView::OnDblclk(NMHDR* pNMHDR, LRESULT* pResult)
//
//////////////////////////////////////////////////////////////////////////////////////
void CCaseView::OnDblclk(NMHDR* pNMHDR, LRESULT* pResult)
{
    CEntryrunDoc* pRunDoc = (CEntryrunDoc*)GetDocument();

    if( pRunDoc != NULL && pRunDoc->GetAppMode() == NO_MODE )
        OnModifyCase();

    *pResult = 0;
}

void CCaseView::OnFindCase()
{
    GetParentFrame()->SendMessage(WM_COMMAND,ID_FINDCASE);
    CEntryrunView* pView = ((CMainFrame*)GetParentFrame())->GetRunView();
    this->RedrawWindow();
    pView->RedrawWindow();
    pView->GetParent()->RedrawWindow();
}


bool CCaseView::GetSelectedNode(HTREEITEM* phItem,NODEINFO** ppNodeInfo)
{
    CEntryrunDoc* pDoc = (CEntryrunDoc*)GetDocument();
    CRunAplEntry* pRunApl = ( pDoc == nullptr ) ? nullptr : pDoc->GetRunApl();

    CTreeCtrl& caseTree = GetTreeCtrl();

    if( pRunApl != nullptr && caseTree.GetCount() != 0 && pDoc->GetAppMode() == NO_MODE )
    {
        *phItem = caseTree.GetSelectedItem();

        if( *phItem != nullptr )
        {
            *ppNodeInfo = (NODEINFO*)caseTree.GetItemData(*phItem);

            if( *ppNodeInfo != nullptr && (*ppNodeInfo)->iNode > 0 )
                return true;
        }
    }

    return false;
}

void CCaseView::AddCaseToTree(const CaseSummary& case_summary, bool bShowCaseLabels, bool bMultiLevel, int case_number)
{
    const int iImageIndex = case_summary.GetDeleted() ?  7 :
                            case_summary.IsPartial()   ? PartialSaveStatusModeToImageIndex(case_summary.GetPartialSaveMode()) :
                            case_summary.GetVerified() ? 6 :
                                                         1;

    CString csKey = bShowCaseLabels ? case_summary.GetCaseLabelOrKey() : case_summary.GetKey();
    NewlineSubstitutor::MakeNewlineToUnicodeNL(csKey);

    CTreeCtrl& caseTree = GetTreeCtrl();
    HTREEITEM hItem = caseTree.InsertItem(csKey);

    NODEINFO* pNodeInfo = new NODEINFO { case_summary, 0, case_number };

    caseTree.SetItemImage(hItem, iImageIndex, iImageIndex);
    caseTree.SetItemData(hItem, (DWORD)pNodeInfo);

    if (bMultiLevel)
        caseTree.InsertItem(_T("dummy"), hItem);
}


void CCaseView::InsertNode()
{
    CTreeCtrl& caseTree = GetTreeCtrl();
    HTREEITEM hItem = nullptr;
    NODEINFO* pNodeInfo = nullptr;

    if( !GetSelectedNode(&hItem,&pNodeInfo) )
        return;

    if( ((CMainFrame*)AfxGetMainWnd())->ModifyStarterHelper(pNodeInfo,CRunAplEntry::ProcessModifyAction::InsertNode) )
    {
        HTREEITEM hParent = caseTree.GetParentItem(hItem);
        hItem = caseTree.GetPrevSiblingItem(hItem);

        if( hItem == nullptr )
            hItem = TVI_FIRST;

        hInsertNodeItem = caseTree.InsertItem(_T("<Inserting Node>"),hParent,hItem);

        if( hInsertNodeItem != nullptr )
        {
            bSelOK = TRUE;
            caseTree.Select(hInsertNodeItem,TVGN_CARET);
            bSelOK = FALSE;
        }
    }
}


void CCaseView::AddNode()
{
    HTREEITEM hItem = nullptr;
    NODEINFO* pNodeInfo = nullptr;

    if( !GetSelectedNode(&hItem,&pNodeInfo) )
        return;

    ((CMainFrame*)AfxGetMainWnd())->ModifyStarterHelper(pNodeInfo,CRunAplEntry::ProcessModifyAction::AddNode);
}


void CCaseView::DeleteNode()
{
    HTREEITEM hItem = nullptr;
    NODEINFO* pNodeInfo = nullptr;

    if( !GetSelectedNode(&hItem,&pNodeInfo) )
        return;

    if( AfxMessageBox(MGF::GetMessageText(MGF::DeleteNode).c_str(), MB_YESNO | MB_DEFBUTTON2 | MB_ICONQUESTION) == IDNO )
        return;

    CEntryrunDoc* pDoc = (CEntryrunDoc*)GetDocument();
    CRunAplEntry* pRunApl = pDoc->GetRunApl();

    if( pRunApl->DeleteNode(pNodeInfo->case_summary, pNodeInfo->iNode) )
    {
        ((CMainFrame*)AfxGetMainWnd())->BuildKeyArray();
        BuildTree();
    }
}


UINT CCaseView::OnGetDlgCode() {
    return DLGC_WANTALLKEYS;
}

bool CCaseView::IsLastCase()
{
    bool bRet = false;
    CTreeCtrl& caseTree = GetTreeCtrl();
    HTREEITEM hItem =  caseTree.GetSelectedItem();
    if(hItem) {
        hItem = caseTree.GetNextSiblingItem(hItem);
        if(!hItem)
            bRet = true;
    }
    return bRet;
}


/////////////////////////////////////////////////////////////////////////////////
//
//  bool CCaseView::AllowSelChange(HTREEITEM hSelItem)
//
/////////////////////////////////////////////////////////////////////////////////
bool CCaseView::AllowSelChange(HTREEITEM hSelItem)
{
    bool bRet = true;

    CEntryrunDoc* pDoc = (CEntryrunDoc*) GetDocument();
    CRunAplEntry* pRunApl = pDoc->GetRunApl();

    if( !hSelItem || !pDoc || pDoc->GetAppMode() != MODIFY_MODE || pRunApl == NULL || bBuildingTree )
        return bRet;

    NODEINFO* pNodeInfo = (NODEINFO*)GetTreeCtrl().GetItemData(hSelItem);

    // ensure that this isn't a case partially saved in a mode other than modify
    if( pNodeInfo->case_summary.IsPartial() && pNodeInfo->case_summary.GetPartialSaveMode() != PartialSaveMode::Modify )
    {
        AfxMessageBox(MGF::GetMessageText(MGF::PartialSaveCannotModify).c_str());
        bRet = false;
    }

    return bRet;
}


int CCaseView::GetSelectedCaseNumber()
{
    CTreeCtrl& caseTree = GetTreeCtrl();

    HTREEITEM hSelectedItem = caseTree.GetSelectedItem();
    HTREEITEM hItem = caseTree.GetRootItem();

    int iIndex = 0;

    while( hItem != NULL )
    {
        if( hItem == hSelectedItem )
            return iIndex;

        iIndex++;

        hItem = caseTree.GetNextSiblingItem(hItem);
    }

    return -1;
}

/////////////////////////////////////////////////////////////////////////////////
//
//  void CCaseView::RestoreSelectPos()
//
/////////////////////////////////////////////////////////////////////////////////
void CCaseView::RestoreSelectPos(const TCHAR* restore_using_key/* = nullptr*/)
{
    CTreeCtrl& caseTree = GetTreeCtrl();

    CMainFrame* pFrame = (CMainFrame*)AfxGetMainWnd();
    int iCount = pFrame->GetCaseSummaries().size();

    if( iCount == 0 || caseTree.GetCount() == 0 )
        return;

    if( ( m_iCaseNumber < 0 ) || m_iCaseNumber > ( (int)caseTree.GetCount() - 1 ) )
        m_iCaseNumber = caseTree.GetCount() - 1;

    HTREEITEM hItem = caseTree.GetRootItem();

    int iIndex = 0;

    while( hItem )
    {
        if( restore_using_key != nullptr )
        {
            NODEINFO* pNodeInfo = (NODEINFO*)caseTree.GetItemData(hItem);

            if( pNodeInfo->case_summary.GetKey().Compare(restore_using_key) == 0 )
                break;
        }

        else if( iIndex == m_iCaseNumber )
            break;

        hItem = caseTree.GetNextSiblingItem(hItem);
        iIndex++;
    }

    if( hItem != NULL )
    {
        caseTree.Select(hItem,TVGN_CARET);
        SetFocus();
    }
}
