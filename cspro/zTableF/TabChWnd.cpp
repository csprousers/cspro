// TabChWnd.cpp : implementation file
//

#include "StdAfx.h"
#include "TabChWnd.h"
#include "DfStylDg.h"
#include "FlashMsg.h"
#include "TabDoc.h"
#include "TabView.h"
#include "Tblgrid.h"
#include "TTallyFD.h"
#include <zUtilO/Filedlg.h>
#include <zDictF/UWM.h>


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CTableChildWnd
#define TABTOOLBAR     997         //this is defined in the CSPro CMainFrame class create

IMPLEMENT_DYNCREATE(CTableChildWnd, ApplicationChildWnd)

CTableChildWnd::CTableChildWnd()
{
    m_pSourceEditView = nullptr;
    m_pPrintView = nullptr;
    m_pOldView = nullptr;
    m_bDesignView = true;
}

CTableChildWnd::~CTableChildWnd()
{
}


BEGIN_MESSAGE_MAP(CTableChildWnd, ApplicationChildWnd)
    //{{AFX_MSG_MAP(CTableChildWnd)
    ON_WM_CREATE()
    ON_WM_MDIACTIVATE()
    ON_COMMAND(ID_RUNTAB, OnRuntab)
    ON_UPDATE_COMMAND_UI(ID_RUNTAB, OnUpdateRuntab)
    ON_COMMAND(ID_VIEW_TAB_LOGIC, OnViewLogic)
    ON_UPDATE_COMMAND_UI(ID_VIEW_TAB_LOGIC, OnUpdateViewLogic)
    ON_UPDATE_COMMAND_UI(ID_VIEW_TABLE, OnUpdateViewTable)
    ON_COMMAND(ID_TBL_COMPILE, OnTblCompile)
    ON_UPDATE_COMMAND_UI(ID_TBL_COMPILE, OnUpdateTblCompile)
    ON_COMMAND(ID_EDIT_TBL_TALLYATTRIB, OnEditTableTallyAttributes)
    ON_UPDATE_COMMAND_UI(ID_EDIT_TBL_TALLYATTRIB, OnUpdateEditTableTallyAttributes)
    ON_COMMAND(ID_VIEW_PAGEPRINTVIEW, OnViewPageprintview)
    ON_UPDATE_COMMAND_UI(ID_VIEW_PAGEPRINTVIEW, OnUpdateViewPageprintview)
    ON_COMMAND(ID_VIEW_TABLE, OnViewTable)
    ON_UPDATE_COMMAND_UI(ID_VIEW_FACING, OnUpdateViewFacing)
    ON_UPDATE_COMMAND_UI(ID_VIEW_BOOKLAYOUT, OnUpdateViewBooklayout)
    ON_UPDATE_COMMAND_UI(ID_VIEW_ZOOM, OnUpdateViewZoom)
    //}}AFX_MSG_MAP
    ON_MESSAGE (UWM::Dictionary::GetTableCursor, OnTabSetIcon)

    ON_COMMAND(ID_VIEW_PREVIOUS_TABLE, OnViewPreviousTable)
    ON_UPDATE_COMMAND_UI(ID_VIEW_PREVIOUS_TABLE, OnUpdateViewPreviousTable)
    ON_COMMAND(ID_VIEW_NEXT_TABLE, OnViewNextTable)
    ON_UPDATE_COMMAND_UI(ID_VIEW_NEXT_TABLE, OnUpdateViewNextTable)
    ON_COMMAND(ID_EDIT_SPECIFYUNIT, OnEditSpecifyunit)
    ON_UPDATE_COMMAND_UI(ID_EDIT_SPECIFYUNIT, OnUpdateEditSpecifyunit)
    ON_COMMAND(ID_EDIT_TBL_OPTIONS, OnEditOptions)
    ON_UPDATE_COMMAND_UI(ID_EDIT_TBL_OPTIONS, OnUpdateEditOptions)
    ON_COMMAND(ID_EDIT_GENERATELOGIC, OnEditGeneratelogic)
    ON_UPDATE_COMMAND_UI(ID_EDIT_GENERATELOGIC, OnUpdateEditGeneratelogic)
    ON_COMMAND(ID_FILE_PRINT_SETUP, OnFilePrintSetup)
    ON_UPDATE_COMMAND_UI(ID_FILE_PRINT_SETUP, OnUpdateFilePrintSetup)
    ON_COMMAND(ID_DESIGN_VIEW, OnDesignView)
    ON_UPDATE_COMMAND_UI(ID_DESIGN_VIEW, OnUpdateDesignView)
    ON_COMMAND(ID_TOOLS_IMPORTTFT, OnImportTFT)
    ON_COMMAND(ID_TOOLS_EXPORTTFT, OnExportTFT)
    ON_COMMAND(ID_EDIT_TABLE_EXCLUDE, OnEditExcludeTbl)
    ON_UPDATE_COMMAND_UI(ID_EDIT_TABLE_EXCLUDE, OnUpdateExcludeTbl)

    ON_COMMAND(ID_EDIT_EXCLUDEALLBUTTHIS, &CTableChildWnd::OnEditExcludeAllButThis)
    ON_UPDATE_COMMAND_UI(ID_EDIT_EXCLUDEALLBUTTHIS, &CTableChildWnd::OnUpdateEditExcludeAllButThis)
    ON_COMMAND(ID_EDIT_INCLUDEALL, &CTableChildWnd::OnEditIncludeAll)
    ON_UPDATE_COMMAND_UI(ID_EDIT_INCLUDEALL, &CTableChildWnd::OnUpdateEditIncludeAll)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTableChildWnd message handlers

int CTableChildWnd::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (COXMDIChildWndSizeDock::OnCreate(lpCreateStruct) == -1)
        return -1;

    // TODO: Add your specialized creation code here

    return 0;
}

BOOL CTableChildWnd::Create(LPCTSTR lpszClassName, LPCTSTR lpszWindowName, DWORD dwStyle, const RECT& rect, CMDIFrameWnd* pParentWnd, CCreateContext* pContext)
{
    // TODO: Add your specialized code here and/or call the base class

    BOOL bRet =  CMDIChildWnd::Create
                    (lpszClassName, lpszWindowName, dwStyle, rect, pParentWnd, pContext);

    if(bRet)
    {
        CString csApp;
        GetModuleFileName(AfxGetApp()->m_hInstance,csApp.GetBuffer(MAX_PATH),MAX_PATH);
        csApp.ReleaseBuffer();

        PathStripPath(csApp.GetBuffer(_MAX_PATH));
        csApp.ReleaseBuffer();

        if(csApp.CompareNoCase(_T("CSpro.exe")) == 0 ) {
            SetIsViewer(false);
        }
        else if(csApp.CompareNoCase(_T("TblView.exe")) == 0 ) {
            SetIsViewer(true);
        }
        else {
            SetIsViewer(true);
        }

        if(!IsViewer()) {
            // Get the window's menu
            CMenu* pMenu = GetSystemMenu (false);
            VERIFY (pMenu->GetSafeHmenu());

            // Disable the 'X'
            VERIFY(::DeleteMenu(pMenu->GetSafeHmenu(), SC_CLOSE , MF_BYCOMMAND));
            VERIFY(::DeleteMenu(pMenu->GetSafeHmenu(), 5 , MF_BYPOSITION));
        }

        if (IsViewer()) {
            SetDesignView(false); // viewer should always be in data view - JH 4/05
        }

        // Force immediate menu update
        DrawMenuBar();

        EnableDocking(CBRS_ALIGN_ANY);


        // create the compiler output and messages tabs
        if( !m_logicDlgBar.CreateAndDock(this) )
            return FALSE;

        m_logicDlgBar.ShowWindow(SW_HIDE);    

        // create the reference window
        if( !m_logicReferenceWnd.CreateAndDock(this) )
            return FALSE;

        // set up navigation bar (a toolbar within a dlg bar)
        if (!m_wndNavDlgBar.Create(this, IDD_PRTVIEW_DLGBAR, CBRS_BOTTOM|CBRS_FLYBY|CBRS_SIZE_DYNAMIC, IDD_PRTVIEW_DLGBAR)) {
            AfxMessageBox(_T("Failed to create navigation dlg bar\n"));
            ASSERT(FALSE);
        }

        m_wndNavDlgBar.CenterToolBar();
        m_wndNavDlgBar.ShowWindow(SW_HIDE);
    }

    return bRet;
}


void CTableChildWnd::OnMDIActivate(BOOL bActivate, CWnd* pActivateWnd, CWnd* pDeactivateWnd)
{
    COXMDIChildWndSizeDock::OnMDIActivate(bActivate, pActivateWnd, pDeactivateWnd);
    AfxGetMainWnd()->SendMessage(UWM::Designer::ShowToolbar, (WPARAM)FrameType::Table);
    CTabulateDoc* pTabDoc = (CTabulateDoc*)GetActiveDocument();

    if (bActivate && pTabDoc) {  // if the tab is regaining focus, perform a reconcile

        CTabSet* pTabSet = pTabDoc->GetTableSpec();

        if (pTabSet->GetDict() == nullptr){    // ptrs not initialized yet, bail
            SetRedraw(TRUE);
            return;
        }
        if(!m_pSourceEditView){
            DynamicMenu4Logic(true);
        }

        CIMSAString csErr;

        bool bReconcile = pTabDoc->Reconcile (csErr, false, true);

        if (bReconcile) {//We need to rebuild the tree ??? What about the grids
            CTabTreeCtrl* pTableTree = pTabDoc->GetTabTreeCtrl();
            pTableTree->ReBuildTree();
            //Mark Table Dirty if reconciled ???
            //For now force redraw of the grid
            POSITION pos = pTabDoc->GetFirstViewPosition();
            while (pos){
                CView* pView = pTabDoc->GetNextView(pos);
                if(pView->IsKindOf(RUNTIME_CLASS(CTabView))){
                    CTabView* pTableView = (CTabView*)pView;
                    CTblGrid* pTblGrid = pTableView->GetGrid();
                    if(pTblGrid->GetTable()->IsDirty()){
                        pTblGrid->GetTable()->RemoveAllData();
                        pTblGrid->Update();
                        pTblGrid->GetTable()->SetDirty(false);
                        pTableView->RedrawWindow();
                    }
                    break;
                }
            }
        }
    }
}


bool CTableChildWnd::IsTableFrameActive()
{
    CMDIChildWnd *pCurChild = (CMDIChildWnd *) MDIGetActive();
    CMDIChildWnd *pMe = (CMDIChildWnd *) GetActiveFrame();

    return  (pCurChild == pMe);
}

CTSourceEditView* CTableChildWnd::GetSourceView()
{
    return m_pSourceEditView;
}


CLogicView* CTableChildWnd::GetSourceLogicView()
{
    return m_pSourceEditView;
}


CLogicCtrl* CTableChildWnd::GetSourceLogicCtrl()
{
    return ( m_pSourceEditView != nullptr ) ? m_pSourceEditView->GetLogicCtrl() :
                                              nullptr;
}


CTabView* CTableChildWnd::GetTabView()
{
    CTabView *pTableView = nullptr;
    CTabulateDoc* pDoc = (CTabulateDoc*)GetActiveDocument();

    if(pDoc){
        POSITION pos = pDoc->GetFirstViewPosition();
        while(pos){
            CView* pView = pDoc->GetNextView(pos);

            if(pView->IsKindOf(RUNTIME_CLASS(CTabView))){
                pTableView= (CTabView*)pView;
                break;
            }
        }
    }
    return pTableView;
}

/////////////////////////////////////////////////////////////////////////////////
//
//  void CTableChildWnd::OnViewLogic()
//
/////////////////////////////////////////////////////////////////////////////////
void CTableChildWnd::OnViewLogic()
{
    // in the event the form tree ctrl is up, but somebody else is in the view, do this


    if (!IsTableFrameActive())
    {
        CMDIChildWnd *pMe = (CMDIChildWnd *) GetActiveFrame();

        GetMDIFrame()->MDIActivate (pMe);
    }

    if(m_pSourceEditView)   // then user trying to toggle off logic view via the menubar
    {
        OnViewTable();
        return;
    }

//  see if it's already active
     CTabulateDoc* pDoc = (CTabulateDoc*)GetActiveDocument();

     CTabView *pTableView = nullptr;

     if(pDoc)
     {
         POSITION pos = pDoc->GetFirstViewPosition();

         while (pos)
         {
             CView* pView = pDoc->GetNextView(pos);

             if(pView->IsKindOf(RUNTIME_CLASS(CTabView))){
                 pTableView = (CTabView*)pView;

                 CTabTreeCtrl* pTableTree = pTableView->GetDocument()->GetTabTreeCtrl();
                 HTREEITEM hItem = pTableTree->GetSelectedItem();
                 bool bSel = true;
                 if(hItem) {
                     TableElementTreeNode* table_element_tree_node = pTableTree->GetTreeNode(hItem);
                     if(table_element_tree_node != nullptr && table_element_tree_node->GetTabDoc() == pTableView->GetDocument()){
                         bSel = false;
                     }
                 }
                 TableSpecTabTreeNode* table_spec_tab_tree_node = pTableTree->GetTableSpecTabTreeNode(*pTableView->GetDocument());
                 ASSERT(table_spec_tab_tree_node != nullptr);
                 if(bSel){
                     pTableTree->Select(table_spec_tab_tree_node->GetHItem(),TVGN_CARET);
                 }

                 break;
             }
         }
     }
        //Set the source view as the active pane

    if (m_pSourceEditView == nullptr)
    {

        m_pSourceEditView =  new CTSourceEditView();
        CCreateContext context =  CCreateContext();

        context.m_pCurrentDoc = pDoc;


        CRect rect(0,0,100,100);
        if(pTableView)
            pTableView->GetClientRect(&rect);
        if (!m_pSourceEditView->Create(nullptr,_T(""),WS_CHILD|WS_VISIBLE,rect,this,AFX_IDW_PANE_FIRST,&context))


        {
            TRACE0("Failed to create view\n");
            ASSERT (false);
            return;
        }
        //To work with large files
        //m_pSourceEditView->GetEditCtrl()->LimitText(0xffffff);
        m_pSourceEditView->ShowWindow(SW_HIDE);
    }

    AfxGetMainWnd()->SendMessage(UWM::Table::ReconcileLinkObj, 0, reinterpret_cast<LPARAM>(pDoc));

    if(AfxGetMainWnd()->SendMessage(UWM::Table::ShowSourceCode, 0, reinterpret_cast<LPARAM>(pDoc)) != 0){
        this->OnViewTable();
        return;
    }
    m_pSourceEditView->ShowWindow(SW_SHOW);
    m_logicDlgBar.ShowWindow(SW_SHOW);
    m_pSourceEditView->GetEditCtrl()->EmptyUndoBuffer();
    SetActiveView(m_pSourceEditView);

    if(pTableView) {
        pTableView->ShowWindow(SW_HIDE);
    }

    RecalcLayout();
    Invalidate();

    AfxGetMainWnd()->SendMessage(UWM::Designer::ShowToolbar, (WPARAM)FrameType::Table);
    AfxGetMainWnd()->SendMessage(UWM::Designer::SelectTab, (WPARAM)FrameType::Table);    // GSF Request Feb 2006


    CWnd* pWnd =   AfxGetMainWnd()->GetDescendantWindow( TABTOOLBAR);
    if(!pWnd || !pWnd->IsKindOf(RUNTIME_CLASS(CToolBar)))
        return;

    DynamicMenu4Logic(false);
    CTabTreeCtrl* pTableTree = pDoc->GetTabTreeCtrl();
    if(pTableTree->IsPrintTree()){
        pTableTree->OnToggleTree();
    }
}

void CTableChildWnd::OnUpdateViewLogic(CCmdUI* pCmdUI)
{
    if(GetPrintView()){
        pCmdUI->SetCheck (IsLogicViewActive());
        pCmdUI->Enable(FALSE);
    }
    else {
        pCmdUI->SetCheck (IsLogicViewActive());
    }
}


bool CTableChildWnd::IsLogicViewActive()
{
    bool bRet = false;
    if (! IsTableFrameActive()) // who's in the view on the right, me or dict or ???
        return bRet;

    m_pSourceEditView? bRet = true : bRet = false;
    return bRet;
}


bool CTableChildWnd::IsTabViewActive()
{
    bool bRet = false;
    if (! IsTableFrameActive()) // who's in the view on the right, me or dict or ???
        return false;
    CTabView* pTabView = DYNAMIC_DOWNCAST(CTabView,GetActiveView());
    pTabView ? bRet = true : bRet = false;
    return bRet;
}

void CTableChildWnd::OnViewTable()
{
    // in the event the form tree ctrl is up, but somebody else is in the view, do this

    if (!IsTableFrameActive())
    {
        CMDIChildWnd *pMe = (CMDIChildWnd *) GetActiveFrame();
        GetMDIFrame()->MDIActivate (pMe);
    }
    DynamicMenu4Logic(true);
//  see if it's already active
    if(!m_pSourceEditView)
        return;

    CTabulateDoc* pDoc = (CTabulateDoc*)GetActiveDocument();

    CView* pView = GetActiveView();

    if (pDoc && pView->IsKindOf(RUNTIME_CLASS(CTSourceEditView))){

        HTREEITEM hItem = pDoc->GetTabTreeCtrl()->GetSelectedItem();
        TableElementTreeNode* table_element_tree_node = pDoc->GetTabTreeCtrl()->GetTreeNode(hItem);
        BOOL bSendMsg = pDoc->GetTabTreeCtrl()->GetSndMsgFlg();
        if(table_element_tree_node != nullptr && table_element_tree_node->GetTabDoc() == pDoc && bSendMsg){
            if(AfxGetMainWnd()->SendMessage(UWM::Table::PutSourceCode, 0, reinterpret_cast<LPARAM>(table_element_tree_node)) == -1)
                return;
        }//if there is an error in the logic view do not switch to the form view
        AfxGetMainWnd()->SendMessage(UWM::Table::ReconcileLinkObj, 0, reinterpret_cast<LPARAM>(pDoc));
    }


    CTabView *pTableView = nullptr;

    pTableView = GetTabView();
    ASSERT(pTableView);

    if(m_pSourceEditView) {
        m_pSourceEditView->ShowWindow(SW_HIDE);
        m_pSourceEditView->DestroyWindow();
        //Do not call delete explicitly for a View . It is done automatically
        //on PostNcDestroy
        m_pSourceEditView = nullptr;
        m_logicDlgBar.ShowWindow(SW_HIDE);
    }

    pTableView->ShowWindow(SW_SHOW);
    pTableView->SetDlgCtrlID(AFX_IDW_PANE_FIRST);
    SetActiveView(pTableView);
    CRect rect ;
    GetClientRect(&rect);
    pTableView->MoveWindow(rect.left,rect.top,rect.right,rect.bottom);

    AfxGetMainWnd()->SendMessage(UWM::Designer::ShowToolbar, (WPARAM)FrameType::Table);

    CWnd* pWnd =   AfxGetMainWnd()->GetDescendantWindow( TABTOOLBAR);
    if(!pWnd || !pWnd->IsKindOf(RUNTIME_CLASS(CToolBar)))
        return;

    DynamicMenu4Logic(true);
    CTabTreeCtrl* pTableTree = pDoc->GetTabTreeCtrl();
    if(!pTableTree->IsPrintTree()){
        pTableTree->OnToggleTree();
    }
}


/////////////////////////////////////////////////////////////////////////////////
//
//  void CTableChildWnd::OnUpdateViewTable(CCmdUI* pCmdUI)
//
/////////////////////////////////////////////////////////////////////////////////
void CTableChildWnd::OnUpdateViewTable(CCmdUI* pCmdUI)
{
    pCmdUI->SetCheck (!IsLogicViewActive());

}


/////////////////////////////////////////////////////////////////////////////////
//
//  void CTableChildWnd::OnTblCompile()
//
/////////////////////////////////////////////////////////////////////////////////
void CTableChildWnd::OnTblCompile()
{
    CTabulateDoc* pDoc = (CTabulateDoc*)GetActiveDocument();
    CView* pView = GetActiveView();

    if (pDoc && pView->IsKindOf(RUNTIME_CLASS(CTSourceEditView)))
    {
        HTREEITEM hItem = pDoc->GetTabTreeCtrl()->GetSelectedItem();
        if(!hItem)
            return;

        TableElementTreeNode* table_element_tree_node = pDoc->GetTabTreeCtrl()->GetTreeNode(hItem);

        if(table_element_tree_node != nullptr && table_element_tree_node->GetTabDoc() == pDoc){
            //wParam 1 implies force compile
            if(AfxGetMainWnd()->SendMessage(UWM::Table::PutSourceCode, 1, reinterpret_cast<LPARAM>(table_element_tree_node)) == -1) {
                ((CTSourceEditView*)pView)->GetEditCtrl()->SetFocus();
                return;
            }
            ((CTSourceEditView*)pView)->GetEditCtrl()->SetFocus();

        }
    }

}


/////////////////////////////////////////////////////////////////////////////////
//
//  void CTableChildWnd::OnUpdateTblCompile(CCmdUI* pCmdUI)
//
/////////////////////////////////////////////////////////////////////////////////
void CTableChildWnd::OnUpdateTblCompile(CCmdUI* pCmdUI)
{
    CView* pView = GetActiveView();
    if(!pView)
        return;

    if(pView->IsKindOf(RUNTIME_CLASS(CTabView))) {

        pCmdUI->Enable(FALSE);
    }
    else {

        pCmdUI->Enable(TRUE);
    }
}

void CTableChildWnd::ActivateFrame(int nCmdShow)
{
    // TODO: Add your specialized code here and/or call the base class
    nCmdShow = SW_SHOWMAXIMIZED;
    COXMDIChildWndSizeDock::ActivateFrame(nCmdShow);
}


/////////////////////////////////////////////////////////////////////////////////
//
//  LRESULT CTableChildWnd::OnTabSetIcon(WPARAM wParam,LPARAM lParam)
//
/////////////////////////////////////////////////////////////////////////////////
LRESULT CTableChildWnd::OnTabSetIcon(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    if(IsTableFrameActive()) {
        CView* pView = GetActiveView();
        if(!pView)
            return 0l;
        if(pView->IsKindOf(RUNTIME_CLASS(CTabView))) {
            CTabView* pTabView = (CTabView*)pView;
            CPoint point;
            if(GetCursorPos(&point)) {
                CTabulateDoc* pDoc = pTabView->GetDocument();
                CDDTreeCtrl* pDictTree = pDoc->GetTabTreeCtrl()->GetDDTreeCtrl();
                if(pDictTree && pDictTree->m_bDragging) { //Do the auto scroll if needed
                    pTabView->GetGrid()->DoDragScroll();
                }

                return pTabView->GetDropCursor(point);
            }
            else {
                return -1L;

            }
        }
    }
    return 0l;
}

void CTableChildWnd::OnViewPageprintview()
{
//#ifdef _UNHOOKED
    if (!IsTableFrameActive())
    {
        CMDIChildWnd *pMe = (CMDIChildWnd *) GetActiveFrame();
        GetMDIFrame()->MDIActivate (pMe);
    }
    CTabulateDoc* pDoc = (CTabulateDoc*)GetActiveDocument();

    if(m_pPrintView){ //Toggle  back to table view
        if(m_pOldView){
            m_pPrintView->ShowWindow(SW_HIDE);
            DestroyPrintView();
            m_pOldView->ShowWindow(SW_SHOW);
        }
        m_wndNavDlgBar.ShowWindow(SW_HIDE);
        return;
    }
    else {

        // get current table so we can set print view to it below
        CTabView* pView = (CTabView*)GetActiveView();
        ASSERT_VALID(pView);
        CTable* pCurrTable = pView->GetGrid()->GetTable();

        if (!CreatePrintView()) {
            AfxMessageBox(_T("No printer declared. Unable to show print view!"));
            if(m_pPrintView){ //Toggle  back to table view
                m_pPrintView->ShowWindow(SW_HIDE);
                DestroyPrintView();
                m_wndNavDlgBar.ShowWindow(SW_HIDE);
            }
            if(m_pOldView){
                m_pOldView->ShowWindow(SW_SHOW);
            }
            return;
        }

        // force print view to use current table
        CTabSet* pTabSet = pDoc->GetTableSpec();
        int iNumPrintTables = pTabSet->GetNumTables();
        for(int iIndex =0; iIndex < iNumPrintTables ; iIndex++){
            if(pTabSet->GetTable(iIndex) == pCurrTable){
                m_pPrintView->GotoTbl(iIndex);
                break;
            }
        }

        m_wndNavDlgBar.ShowWindow(SW_SHOW);
        m_pPrintView->ShowWindow(SW_SHOW);
        if(m_pOldView) {
            m_pOldView->ShowWindow(SW_HIDE);
        }
        CTabTreeCtrl* pTableTree = pDoc->GetTabTreeCtrl();
        if(!pTableTree->IsPrintTree()){
            pTableTree->OnToggleTree();
        }

        RecalcLayout();
        Invalidate();
    }
//#endif
//Do the toolbat stuff later

}

void CTableChildWnd::OnUpdateViewPageprintview(CCmdUI* pCmdUI)
{
    pCmdUI->Enable(!IsLogicViewActive());
    pCmdUI->SetCheck(GetPrintView() != nullptr);
}

// create print view and make it active
// (doesn't show it so it can be used by print from grid view)
bool CTableChildWnd::CreatePrintView()
{
    m_pOldView = GetActiveView();
    CTabulateDoc* pDoc = (CTabulateDoc*)GetActiveDocument();

    //Check for printer availability
    CTabSet* pSet = pDoc->GetTableSpec();
    const CFmtReg& fmtReg=pSet->GetFmtReg();
    CTblPrintFmt* pTblPrintFmt=DYNAMIC_DOWNCAST(CTblPrintFmt,fmtReg.GetFmt(FMT_ID_TBLPRINT,0));

    CString sPrinterDevice(pTblPrintFmt->GetPrinterDevice());
    if (sPrinterDevice.IsEmpty()) {
        return false;
    }
    m_pPrintView =  new CTabPrtView();
    CCreateContext context =  CCreateContext();
    context.m_pCurrentDoc = pDoc;

    CRect rect(0,0,100,100);
    if(m_pOldView)
        m_pOldView->GetClientRect(&rect);
    if (!m_pPrintView->Create(nullptr,_T(""),WS_CHILD|WS_BORDER,rect,this,AFX_IDW_PANE_FIRST,&context))
    {
        TRACE0("Failed to create view\n");
        ASSERT (false);
        return false;
    }
    if(!m_pPrintView->PreparePrinterDC()){
        return false;
    }

    m_pPrintView->DoInitUpdate();
    m_pPrintView->ShowWindow(SW_HIDE);
    SetActiveView(m_pPrintView);

    return true;
}

// cleanup print view and set old view as current
void CTableChildWnd::DestroyPrintView()
{
    m_pPrintView->DestroyWindow();
    //Do not call delete explicitly for a View . It is done automatically
    //on PostNcDestroy
    m_pPrintView = nullptr;
    ASSERT_VALID(m_pOldView);
    m_pOldView->SetDlgCtrlID(AFX_IDW_PANE_FIRST);
    SetActiveView(m_pOldView);
    CRect rect;
    GetClientRect(&rect);
    m_pOldView->MoveWindow(rect.left,rect.top,rect.right,rect.bottom);
}

/////////////////////////////////////////////////////////////////////////////////
//  CTabPrtView* CTableChildWnd::GetPrintView()//
//
//  Prints by creating temporary print view, calling print on it and then
//  destroying the temp print view.
/////////////////////////////////////////////////////////////////////////////////
void CTableChildWnd::PrintFromNonPrintView(int currTbl)
{
    if (!CreatePrintView()) {
        AfxMessageBox(_T("No printer declared. Unable to print!"));
        if(m_pPrintView){
            DestroyPrintView();
        }
        return;
    }

    // goto current tabl so current table button in print dlg will work
    m_pPrintView->GotoTbl(currTbl, false);

    // do the actual printing
    m_pPrintView->SendMessage(WM_COMMAND, ID_FILE_PRINT);

    DestroyPrintView();
}

/////////////////////////////////////////////////////////////////////////////////
//
//  CTabPrtView* CTableChildWnd::GetPrintView()//
/////////////////////////////////////////////////////////////////////////////////
CTabPrtView* CTableChildWnd::GetPrintView()
{
    return  m_pPrintView;

}
void CTableChildWnd::OnUpdateViewFacing(CCmdUI* /*pCmdUI*/)
{
#ifdef _UNHOOKED
    if(m_pPrintView && m_pPrintView->GetSafeHwnd()){
        // let the view handle it
        m_pPrintView->UpdateViewFacing(pCmdUI);
    }
    else {
        // view is not alive, grey out the menu choice
        pCmdUI->Enable(FALSE);
   }
#endif
}

void CTableChildWnd::OnUpdateViewBooklayout(CCmdUI* /*pCmdUI*/)
{
#ifdef _UNHOOKED
    if(m_pPrintView && m_pPrintView->GetSafeHwnd()){
        // let the view handle it
        m_pPrintView->UpdateViewBooklayout(pCmdUI);
    }
    else {
        // view is not alive, grey out the menu choice
        pCmdUI->Enable(FALSE);
    }
#endif
}

void CTableChildWnd::OnUpdateViewZoom(CCmdUI* /*pCmdUI*/)
{
#ifdef _UNHOOKED
    if(m_pPrintView && m_pPrintView->GetSafeHwnd()){
        // let the view handle it
        m_pPrintView->UpdateViewZoom(pCmdUI);
    }
    else {
        // view is not alive, grey out the menu choice
        pCmdUI->Enable(FALSE);
    }
#endif
}

void CTableChildWnd::OnViewPreviousTable()
{
    CTabulateDoc* pDoc = (CTabulateDoc*)GetActiveDocument();
    if(pDoc) {
        CTabTreeCtrl* pTableTree = pDoc->GetTabTreeCtrl();
        if(pTableTree){
            pTableTree->SelectPrevTable();
        }
    }
}

void CTableChildWnd::OnUpdateViewPreviousTable(CCmdUI *pCmdUI)
{
    bool bEnable = false;
    CTabulateDoc* pDoc = (CTabulateDoc*)GetActiveDocument();
    if(pDoc) {
        CTabTreeCtrl* pTableTree = pDoc->GetTabTreeCtrl();
        if(pTableTree && pTableTree->GetPrevTable()){
            bEnable = true;
        }
    }
    pCmdUI->Enable(bEnable);
}

void CTableChildWnd::OnViewNextTable()
{
    CTabulateDoc* pDoc = (CTabulateDoc*)GetActiveDocument();
    if(pDoc) {
        CTabTreeCtrl* pTableTree = pDoc->GetTabTreeCtrl();
        if(pTableTree){
            pTableTree->SelectNextTable();
        }
    }
}

void CTableChildWnd::OnUpdateViewNextTable(CCmdUI *pCmdUI)
{
    bool bEnable = false;
    CTabulateDoc* pDoc = (CTabulateDoc*)GetActiveDocument();
    if(pDoc) {
        CTabTreeCtrl* pTableTree = pDoc->GetTabTreeCtrl();
        if(pTableTree && pTableTree->GetNextTable()){
            bEnable = true;
        }
    }
    pCmdUI->Enable(bEnable);
}

void CTableChildWnd::OnEditSpecifyunit()
{
#ifdef _UNHOOKED
    CTabulateDoc* pDoc = (CTabulateDoc*)GetActiveDocument();
    if(pDoc) {
        CTabView* pView = (CTabView*)GetActiveView();
        CTable* pTable = pView->GetGrid()->GetTable();
        if(pTable && pTable->GetRowRoot()->GetNumChildren() > 0 && pTable->GetColRoot()->GetNumChildren() > 0){
            pDoc->GetTableSpec()->UpdateSubtableList(pTable);
            CUnitDlg unitDlg ;
            unitDlg.m_pArrUnitSpec = &pTable->GetUnitSpecArr();
            if(unitDlg.DoModal() == IDOK){
                CUnitSpec& unitSpec = pTable->GetUnitSpecArr().GetAt(unitDlg.m_iIndex);
                unitSpec.SetUniverse(unitDlg.m_sSelect);
                unitSpec.SetWeightExpr(unitDlg.m_sWeight);
                unitSpec.SetLoopingVarName(unitDlg.m_sLoopVarName);
                pDoc->SetModifiedFlag(TRUE);
            }
        }
    }
#endif
}

void CTableChildWnd::OnUpdateEditSpecifyunit(CCmdUI *pCmdUI)
{
    pCmdUI->Enable(FALSE);
#ifdef _UNHOOKED
    CTabulateDoc* pDoc = (CTabulateDoc*)GetActiveDocument();
    if(pDoc) {
        CTabulateDoc* pDoc = (CTabulateDoc*)GetActiveDocument();
        CView* pView = GetActiveView();
        if(pView->IsKindOf(RUNTIME_CLASS(CTabView))) {
            CTable* pTable = ((CTabView*)pView)->GetGrid()->GetTable();
            if(pTable && pTable->GetRowRoot()->GetNumChildren() > 0 && pTable->GetColRoot()->GetNumChildren() > 0){
                pCmdUI->Enable(TRUE);
            }
        }
    }
#endif
}
/////////////////////////////////////////////////////////////////////////////////
//
//  void CTableChildWnd::OnParms()
//
/////////////////////////////////////////////////////////////////////////////////
void CTableChildWnd::OnParms(bool bSubTable /*=false*/)
{
    CTblTallyFmtDlg dlg;
    CTabulateDoc* pDoc = (CTabulateDoc*)GetActiveDocument();
    if(pDoc) {
        CView* pView = GetActiveView();
        if(pView->IsKindOf(RUNTIME_CLASS(CTabView))) {
            dlg.m_pTabView = (CTabView*)  pView;
            CTable* pTable = ((CTabView*)pView)->GetGrid()->GetTable();
            dlg.m_arrPostCalc.Append(pTable->GetPostCalcLogic());
            CArray<CStringArray,CStringArray&> arrValidSubtableUnitNames;
            CStringArray arrValidTableUnitNames;
            CTabVar* pRowVar = nullptr;
            CTabVar* pColVar = nullptr;
            int iSubTableIndex = -1;
            if(bSubTable){
                //Get the subtable index
                pRowVar = nullptr;
                pColVar = nullptr;
                CTabView* pTabView = GetTabView();
                CTblGrid* pTblGrid = pTabView->GetGrid();


                int iCurrentCol = pTblGrid->GetCurrentCol();
                long iCurrentRow = pTblGrid->GetCurrentRow();

                //GetRowVar
                pRowVar = pTblGrid->GetRowVar(iCurrentRow);
                pColVar = pTblGrid->GetColHead(iCurrentCol);
            }
            iSubTableIndex = pDoc->GetTableSpec()->UpdateSubtableList(pTable, arrValidSubtableUnitNames, arrValidTableUnitNames,
                false,nullptr,pRowVar,pColVar);

            bSubTable? iSubTableIndex++ : iSubTableIndex=0;
            dlg.m_iCurSubTable = iSubTableIndex;
            CArray<CUnitSpec,CUnitSpec&>&arrUnits = pTable->GetUnitSpecArr();

            //Update units
            if(arrValidSubtableUnitNames.GetSize() > 0  || arrValidTableUnitNames.GetSize() > 0){
                CUnitSpec copyOfUnitspec = pTable->GetTableUnit();
                dlg.m_aUnitSpec.Add(copyOfUnitspec);
                for(int iIndex =0;iIndex< arrUnits.GetSize();iIndex++){
                    CUnitSpec copyOfUnitspec2 = arrUnits[iIndex];
                    dlg.m_aUnitSpec.Add(copyOfUnitspec2);
                }
                dlg.m_pConsolidate = pDoc->GetTableSpec()->GetConsolidate();
                dlg.m_iBreakLevel = pTable->GetBreakLevel();



                dlg.m_arrUnitNames.SetSize(arrUnits.GetSize()+1); // + 1 to leave space for entire table at item 0

                // fill in unit names for entire table
                if(arrValidSubtableUnitNames.GetSize() > 1){
                    //Get the intersection of all the subtable units names
                    CStringArray arrIntersection;
                    CStringArray& arrFirstSet = arrValidSubtableUnitNames[0];
                    for(int iUnit =0; iUnit < arrFirstSet.GetSize() ; iUnit++){
                        CIMSAString sUnitName =arrFirstSet[iUnit];
                        bool bSkipName = false;
                        for(int iOtherUnit = 1 ; iOtherUnit < arrValidSubtableUnitNames.GetSize() ; iOtherUnit++){
                            CStringArray& arrSecondSet = arrValidSubtableUnitNames[iOtherUnit];
                            bool bUnitFound = false;
                            for(int iUnitName =0; iUnitName < arrSecondSet.GetSize(); iUnitName++){
                                if(sUnitName.CompareNoCase(arrSecondSet[iUnitName]) ==0 ){
                                    bUnitFound = true;//Found in this set. Now look in the next set
                                    break;
                                }
                            }
                            if(!bUnitFound){
                                bSkipName = true;
                                break; //no need to look in other sets . Name not found . Not in intersection
                            }
                        }
                        if(!bSkipName) {
                            arrIntersection.Add(sUnitName); //in the intersection .Add it
                        }
                    }
                    if(arrIntersection.GetSize() >  0) {
                        //arrIntersection.InsertAt(0,"Select Unit -To Apply To All Subtables");
                        dlg.m_arrUnitNames[0].Copy(arrIntersection);
                    }
                }
                else {
                    dlg.m_arrUnitNames[0].Copy(arrValidTableUnitNames);
                }
                // fill in default for unit spec
                if (dlg.m_aUnitSpec[0].GetUseDefaultLoopingVar() ||
                    dlg.m_aUnitSpec[0].GetLoopingVarName().IsEmpty()) {
                        CString specStr = dlg.m_arrUnitNames.GetAt(0).GetCount() != 0 ?
                            dlg.m_arrUnitNames.GetAt(0).GetAt(0) : CString();
                        dlg.m_aUnitSpec[0].SetLoopingVarName(specStr);
                        dlg.m_aUnitSpec[0].SetUseDefaultLoopingVar(true);
                    }

                    // fill in unit names for each subtable in dialog
                    for(int iIndex =0;iIndex< arrUnits.GetSize();iIndex++){

                        dlg.m_arrUnitNames[iIndex+1].Copy(arrValidSubtableUnitNames[iIndex]);

                        // fill in default for unit spec
                        if (dlg.m_aUnitSpec[iIndex+1].GetUseDefaultLoopingVar() ||
                            dlg.m_aUnitSpec[iIndex+1].GetLoopingVarName().IsEmpty()) {
                                dlg.m_aUnitSpec[iIndex+1].SetLoopingVarName(dlg.m_arrUnitNames.GetAt(iIndex+1).GetAt(0));
                                dlg.m_aUnitSpec[iIndex+1].SetUseDefaultLoopingVar(true);
                            }
                    }
            }
            if (pTable->GetCustSpecialValSettings().GetUseCustomSpecVal()) {
                dlg.m_bUseCustomSpecVal  = BST_CHECKED;
                dlg.m_bUseSpecValDefault = pTable->GetCustSpecialValSettings().GetUseSpecValDefault() ? BST_CHECKED : BST_UNCHECKED;
                dlg.m_bUseSpecValMissing = pTable->GetCustSpecialValSettings().GetUseSpecValMissing() ? BST_CHECKED : BST_UNCHECKED;
                dlg.m_bUseSpecValRefused = pTable->GetCustSpecialValSettings().GetUseSpecValRefused() ? BST_CHECKED : BST_UNCHECKED;
                dlg.m_bUseSpecValNotAppl = pTable->GetCustSpecialValSettings().GetUseSpecValNotAppl() ? BST_CHECKED : BST_UNCHECKED;
                dlg.m_bUseSpecValUndefined = pTable->GetCustSpecialValSettings().GetUseSpecValUndefined() ? BST_CHECKED : BST_UNCHECKED;
            }
            pDoc->GetTableSpec()->DoAllSubTablesHaveSameUnit(pTable,dlg.m_sCommonUnitName);
            if(dlg.DoModal() ==IDOK){ //update the tally fmt spec for table here
                bool bModified = false;
                pTable->GetBreakLevel() != dlg.m_iBreakLevel ? bModified = true: bModified = false;
                pTable->SetBreakLevel(dlg.m_iBreakLevel);

                //for now modified is true . Fix this
                //check if postcalc is modified
                if (pTable->GetPostCalcLogic().GetSize()!= dlg.m_arrPostCalc.GetSize()){
                    bModified  = true;
                    pTable->GetPostCalcLogic().RemoveAll();
                    pTable->GetPostCalcLogic().Append(dlg.m_arrPostCalc);
                }
                for(int iIndex =0; iIndex < dlg.m_arrPostCalc.GetSize(); iIndex++){
                    if(pTable->GetPostCalcLogic().GetAt(iIndex).Compare(dlg.m_arrPostCalc[iIndex]) !=0){
                        bModified  = true;
                        pTable->GetPostCalcLogic().RemoveAll();
                        pTable->GetPostCalcLogic().Append(dlg.m_arrPostCalc);
                    }
                }
                //Set the unit stuff
                for(int iIndex =0; iIndex <  dlg.m_aUnitSpec.GetSize();iIndex++){
                    if(iIndex ==0){
                        CUnitSpec& unitSpec = pTable->GetTableUnit();
                        unitSpec != dlg.m_aUnitSpec[0] ? bModified = true: bModified = bModified;
                        unitSpec = dlg.m_aUnitSpec[0];
                    }
                    else {
                        CUnitSpec& unitSpec = arrUnits.GetAt(iIndex-1);
                        unitSpec !=  dlg.m_aUnitSpec[iIndex] ? bModified = true: bModified = bModified;
                        unitSpec = dlg.m_aUnitSpec[iIndex];
                    }
                }

                // special values
                if (dlg.m_bUseCustomSpecVal == BST_CHECKED) {
                    pTable->GetCustSpecialValSettings().GetUseCustomSpecVal()!=  true ? bModified = true: bModified = bModified;
                    pTable->GetCustSpecialValSettings().SetUseCustomSpecVal(true);

                    pTable->GetCustSpecialValSettings().GetUseSpecValNotAppl()!= (dlg.m_bUseSpecValNotAppl == BST_CHECKED) ? bModified = true: bModified = bModified;
                    pTable->GetCustSpecialValSettings().SetUseSpecValNotAppl(dlg.m_bUseSpecValNotAppl == BST_CHECKED);

                    pTable->GetCustSpecialValSettings().GetUseSpecValMissing()!= (dlg.m_bUseSpecValMissing == BST_CHECKED) ? bModified = true: bModified = bModified;
                    pTable->GetCustSpecialValSettings().SetUseSpecValMissing(dlg.m_bUseSpecValMissing == BST_CHECKED);

                    pTable->GetCustSpecialValSettings().GetUseSpecValRefused()!= (dlg.m_bUseSpecValRefused == BST_CHECKED) ? bModified = true: bModified = bModified;
                    pTable->GetCustSpecialValSettings().SetUseSpecValRefused(dlg.m_bUseSpecValRefused == BST_CHECKED);

                    pTable->GetCustSpecialValSettings().GetUseSpecValDefault()!= (dlg.m_bUseSpecValDefault == BST_CHECKED) ? bModified = true: bModified = bModified;
                    pTable->GetCustSpecialValSettings().SetUseSpecValDefault(dlg.m_bUseSpecValDefault == BST_CHECKED);

                    pTable->GetCustSpecialValSettings().GetUseSpecValUndefined()!= (dlg.m_bUseSpecValUndefined == BST_CHECKED) ? bModified = true: bModified = bModified;
                    pTable->GetCustSpecialValSettings().SetUseSpecValUndefined(dlg.m_bUseSpecValUndefined == BST_CHECKED);
                }
                else {
                    pTable->GetCustSpecialValSettings().GetUseCustomSpecVal()!=  false ? bModified = true: bModified = bModified;
                    pTable->GetCustSpecialValSettings().SetUseCustomSpecVal(false);

                    pTable->GetCustSpecialValSettings().GetUseSpecValNotAppl()!= false ? bModified = true: bModified = bModified;
                    pTable->GetCustSpecialValSettings().SetUseSpecValNotAppl(false);

                    pTable->GetCustSpecialValSettings().GetUseSpecValMissing()!= false ? bModified = true: bModified = bModified;
                    pTable->GetCustSpecialValSettings().SetUseSpecValMissing(false);

                    pTable->GetCustSpecialValSettings().GetUseSpecValRefused()!= false ? bModified = true: bModified = bModified;
                    pTable->GetCustSpecialValSettings().SetUseSpecValRefused(false);

                    pTable->GetCustSpecialValSettings().GetUseSpecValDefault()!= false ? bModified = true: bModified = bModified;
                    pTable->GetCustSpecialValSettings().SetUseSpecValDefault(false);

                    pTable->GetCustSpecialValSettings().GetUseSpecValUndefined()!= false ? bModified = true: bModified = bModified;
                    pTable->GetCustSpecialValSettings().SetUseSpecValUndefined(false);
                }
                if(bModified){
                    for(int iIndex =0; iIndex < pDoc->GetTableSpec()->GetNumTables(); iIndex++){
                        pDoc->GetTableSpec()->GetTable(iIndex)->RemoveAllData();
                    }
                    pDoc->GetTableSpec()->ReconcileLevels4Tbl(((CTabView*)pView)->GetGrid()->GetTable());
                    ((CTabView*)pView)->GetGrid()->Update();
                    //Force logic update
                    CWaitCursor wait;
                    TBL_PROC_INFO tblProcInfo;
                    tblProcInfo.pTabDoc = pDoc;
                    tblProcInfo.pTable = nullptr; //We are doing all tables
                    tblProcInfo.sTblLogic = _T("");//We are forcing an update
                    tblProcInfo.eEventType = CSourceCode_AllEvents;
                    tblProcInfo.pLinkTable = nullptr;
                    tblProcInfo.bGetLinkTables = false;

                    if(AfxGetMainWnd()->SendMessage(UWM::Table::PutTallyProc, 0, reinterpret_cast<LPARAM>(&tblProcInfo)) == -1){
                        AfxMessageBox(_T("Failed to put logic in App"));
                    }
                    pDoc->SetModifiedFlag(bModified? TRUE:FALSE);

                }
            }
        }
    }
}

/////////////////////////////////////////////////////////////////////////////////
//
//  void CTableChildWnd::OnEditOptions()
//
/////////////////////////////////////////////////////////////////////////////////
void CTableChildWnd::OnEditOptions()
{
    CTabulateDoc* pDoc = DYNAMIC_DOWNCAST(CTabulateDoc, GetActiveDocument());
    CTabSet* pSet = pDoc->GetTableSpec();
    CFmtReg& fmtReg=*(pSet->GetFmtRegPtr());

    CDefaultStylesDlg propsDlg(fmtReg,GetTabView());

    propsDlg.DoModal();
    if (propsDlg.GetStylesChanged()) {
        CTabView* pTabView = GetTabView();
        CTblGrid* pTblGrid = pTabView->GetGrid();
        pTblGrid->Update();
        if (m_pPrintView != nullptr) {
            m_pPrintView->Build(true);
            m_pPrintView->Invalidate();
        }
        pDoc->SetModifiedFlag(TRUE);
    }
}

void CTableChildWnd::OnUpdateEditOptions(CCmdUI *pCmdUI)
{
    pCmdUI->Enable(TRUE);
}

void CTableChildWnd::OnEditGeneratelogic()
{
    CTabulateDoc* pDoc = DYNAMIC_DOWNCAST(CTabulateDoc, GetActiveDocument());
#ifdef _DELME
    if(pDoc){
        bool bGenLogic = pDoc->GetTableSpec()->GetGenLogic();
        pDoc->GetTableSpec()->SetGenLogic(!bGenLogic);
        pDoc->SetModifiedFlag(TRUE);
    }
#endif
    //Get active tree selection
    //Get actvie table
    //Set the flag
    CTabView* pTableView = GetTabView();
    CTabTreeCtrl* pTableTree = pTableView->GetDocument()->GetTabTreeCtrl();
    HTREEITEM hItem = pTableTree->GetSelectedItem();
    if(hItem) {
        TableElementTreeNode* table_element_tree_node = pTableTree->GetTreeNode(hItem);
        if(table_element_tree_node && table_element_tree_node->GetTabDoc() == pTableView->GetDocument() && table_element_tree_node->GetTableElementType() == TableElementType::Table){
            bool bFlag = table_element_tree_node->GetTable()->GetGenerateLogic();
            table_element_tree_node->GetTable()->SetGenerateLogic(!bFlag);
            pDoc->SetModifiedFlag(TRUE);
            if (table_element_tree_node->GetTable()->GetGenerateLogic()) {
                if (table_element_tree_node->GetTable()->IsTableExcluded4Run()) {
                    pTableTree->SetItemImage(hItem,11,11);
                }
                else{
                    pTableTree->SetItemImage(hItem,1,1);
                }
            }
            else {
                if (table_element_tree_node->GetTable()->IsTableExcluded4Run()) {
                    pTableTree->SetItemImage(hItem,12,12);
                }
                else{
                    pTableTree->SetItemImage(hItem,2,2);
                }
            }
            pTableTree->Invalidate();
        }
    }
}

void CTableChildWnd::OnUpdateEditGeneratelogic(CCmdUI *pCmdUI)
{
    pCmdUI->Enable(FALSE);
    CTabulateDoc* pDoc = DYNAMIC_DOWNCAST(CTabulateDoc, GetActiveDocument());
#ifdef _DELME
    if(pDoc && pDoc->GetTableSpec()->GetGenLogic()){
        pCmdUI->SetCheck(1);
    }
    else {
        pCmdUI->SetCheck(0);
    }
#endif
    CTabView* pTableView = GetTabView();
    CTabTreeCtrl* pTableTree = pTableView->GetDocument()->GetTabTreeCtrl();
    HTREEITEM hItem = pTableTree->GetSelectedItem();
    if(hItem) {
        TableElementTreeNode* table_element_tree_node = pTableTree->GetTreeNode(hItem);
        if(table_element_tree_node && table_element_tree_node->GetTabDoc() == pTableView->GetDocument() && table_element_tree_node->GetTableElementType() == TableElementType::Table){
            if(pDoc && table_element_tree_node->GetTable()->GetGenerateLogic()){
                pCmdUI->Enable(TRUE);
                pCmdUI->SetCheck(1);
            }
            else {
                pCmdUI->Enable(TRUE);
                pCmdUI->SetCheck(0);
            }
        }
    }

}

void CTableChildWnd::OnEditExcludeTbl()
{
    CTabulateDoc* pDoc = DYNAMIC_DOWNCAST(CTabulateDoc, GetActiveDocument());

    //Get active tree selection
    //Get actvie table
    //Set the flag
    CTabView* pTableView = GetTabView();
    CTabTreeCtrl* pTableTree = pTableView->GetDocument()->GetTabTreeCtrl();
    HTREEITEM hItem = pTableTree->GetSelectedItem();
    if(hItem) {
        TableElementTreeNode* table_element_tree_node = pTableTree->GetTreeNode(hItem);
        if(table_element_tree_node && table_element_tree_node->GetTabDoc() == pTableView->GetDocument() && table_element_tree_node->GetTableElementType() == TableElementType::Table){
            bool bFlag = table_element_tree_node->GetTable()->IsTableExcluded4Run();
            table_element_tree_node->GetTable()->SetExcludeTable4Run(!bFlag);
            pDoc->SetModifiedFlag(TRUE);
            if (table_element_tree_node->GetTable()->GetGenerateLogic()) {
                if (table_element_tree_node->GetTable()->IsTableExcluded4Run()) {
                    pTableTree->SetItemImage(hItem,11,11);
                }
                else{
                    pTableTree->SetItemImage(hItem,1,1);
                }
            }
            else {
                if (table_element_tree_node->GetTable()->IsTableExcluded4Run()) {
                    pTableTree->SetItemImage(hItem,12,12);
                }
                else{
                    pTableTree->SetItemImage(hItem,2,2);
                }
            }
            pTableTree->Invalidate();
        }
    }
}

void CTableChildWnd::OnUpdateExcludeTbl(CCmdUI *pCmdUI)
{
    pCmdUI->Enable(FALSE);
    CTabulateDoc* pDoc = DYNAMIC_DOWNCAST(CTabulateDoc, GetActiveDocument());

    CTabView* pTableView = GetTabView();
    CTabTreeCtrl* pTableTree = pTableView->GetDocument()->GetTabTreeCtrl();
    HTREEITEM hItem = pTableTree->GetSelectedItem();
    if(hItem) {
        TableElementTreeNode* table_element_tree_node = pTableTree->GetTreeNode(hItem);
        if(table_element_tree_node && table_element_tree_node->GetTabDoc() == pTableView->GetDocument() && table_element_tree_node->GetTableElementType() == TableElementType::Table){
            if(pDoc && table_element_tree_node->GetTable()->IsTableExcluded4Run()){
                pCmdUI->Enable(TRUE);
                pCmdUI->SetCheck(1);
            }
            else {
                pCmdUI->Enable(TRUE);
                pCmdUI->SetCheck(0);
            }
        }
    }

}

/////////////////////////////////////////////////////////////////////////////
//
//        OnFilePrintSetup
//
/////////////////////////////////////////////////////////////////////////////
void CTableChildWnd::OnFilePrintSetup()
{
    // get current tabset's printer settings (they're in the default CTblPrintFmt)
    CTabulateDoc* pDoc = DYNAMIC_DOWNCAST(CTabulateDoc, GetActiveDocument());
    CTabSet* pSet = pDoc->GetTableSpec();
    const CFmtReg& fmtReg=pSet->GetFmtReg();
    CTable* pTbl=pSet->GetTable(0);
    bool bPrintSettingsChanged=false;

    // table print format ...
    CTblPrintFmt fmtTblPrint(*pTbl->GetTblPrintFmt());
    fmtTblPrint.CopyNonDefaultValues(DYNAMIC_DOWNCAST(CTblPrintFmt,fmtReg.GetFmt(FMT_ID_TBLPRINT,0)));

    CString sPrinterDevice(fmtTblPrint.GetPrinterDevice());

    if (sPrinterDevice.IsEmpty()) {
        // huh? no printer ... try to use default printer...

        CPrintDialog dlgQuery(FALSE);
        CIMSAString sMsg;
        if (dlgQuery.GetDefaults()) {
            // XTS printer invalid, but we can use the Windows default ...
            DEVNAMES FAR *pDevNames=(DEVNAMES FAR *)::GlobalLock(dlgQuery.m_pd.hDevNames);
            CString sDriver((LPTSTR)pDevNames + pDevNames->wDriverOffset);
            sPrinterDevice=(LPTSTR)pDevNames + pDevNames->wDeviceOffset;
            CString sOutput((LPTSTR)pDevNames + pDevNames->wOutputOffset);
            fmtTblPrint.SetPrinterDevice(sPrinterDevice);
            fmtTblPrint.SetPrinterDriver(sDriver);
            fmtTblPrint.SetPrinterOutput(sOutput);
            ::GlobalUnlock(dlgQuery.m_pd.hDevNames);
            sMsg.Format(_T("No printer selected for this application, using default Windows printer\n%s"), (LPCTSTR)sPrinterDevice);
            AfxMessageBox(sMsg,MB_ICONINFORMATION);
            bPrintSettingsChanged = true;
        }
        else {
            // XTS printer invalid, but system has no printer installed...
            sMsg.Format(_T("Error, no printer selected for this application. Unable to locate a default Windows printer."));
            AfxMessageBox(sMsg,MB_ICONEXCLAMATION);
            return;
        }
    }

    // prep the print setup dialog ...
    CPrintDialog dlg(TRUE);
    TCHAR* pszPrinterDevice=sPrinterDevice.GetBuffer(sPrinterDevice.GetLength());
    if (GetPrinterDevice(pszPrinterDevice, &dlg.m_pd.hDevNames, &dlg.m_pd.hDevMode)) {
        DEVMODE FAR* pDevMode=(DEVMODE FAR*)::GlobalLock(dlg.m_pd.hDevMode);

        // set orientation ...
        switch (fmtTblPrint.GetPageOrientation()) {
        case PAGE_ORIENTATION_PORTRAIT:
            pDevMode->dmOrientation=DMORIENT_PORTRAIT;
            break;
        case PAGE_ORIENTATION_LANDSCAPE:
            pDevMode->dmOrientation=DMORIENT_LANDSCAPE;
            break;
        default:
            ASSERT(FALSE);
            break;
        }

        // set paper type ...
        switch (fmtTblPrint.GetPaperType()) {
        case PAPER_TYPE_A4:
            pDevMode->dmPaperSize=DMPAPER_A4;
            break;
        case PAPER_TYPE_A3:
            pDevMode->dmPaperSize=DMPAPER_A3;
            break;
        case PAPER_TYPE_LETTER:
            pDevMode->dmPaperSize=DMPAPER_LETTER;
            break;
        case PAPER_TYPE_LEGAL:
            pDevMode->dmPaperSize=DMPAPER_LEGAL;
            break;
        case PAPER_TYPE_TABLOID:
            pDevMode->dmPaperSize=DMPAPER_TABLOID;
            break;
        default:
            ASSERT(FALSE);
            break;
        }

        // update printer
        ::GlobalUnlock(dlg.m_pd.hDevMode);
    }
    sPrinterDevice.ReleaseBuffer();

    // prep the print setup dialog ...
    if (dlg.DoModal()==IDOK) {
        // update table print format settings ... for now, update both the current and default values
        CTblPrintFmt* pTblPrintFmt=pTbl->GetTblPrintFmt();
        CTblPrintFmt* pDefaultTblPrintFmt=DYNAMIC_DOWNCAST(CTblPrintFmt, fmtReg.GetFmt(FMT_ID_TBLPRINT,0));

        // retrieve printer device info ...
        DEVNAMES FAR *pDevNames=(DEVNAMES FAR *)::GlobalLock(dlg.m_pd.hDevNames);
        CString sDriver((LPTSTR)pDevNames + pDevNames->wDriverOffset);
        CString sDevice((LPTSTR)pDevNames + pDevNames->wDeviceOffset);
        CString sOutput((LPTSTR)pDevNames + pDevNames->wOutputOffset);
        if (sDevice!=fmtTblPrint.GetPrinterDevice()) {
            bPrintSettingsChanged = true;
        }
        pTblPrintFmt->SetPrinterDevice(sDevice);
        pTblPrintFmt->SetPrinterDriver(sDriver);
        pTblPrintFmt->SetPrinterOutput(sOutput);
        pDefaultTblPrintFmt->SetPrinterDevice(sDevice);
        pDefaultTblPrintFmt->SetPrinterDriver(sDriver);
        pDefaultTblPrintFmt->SetPrinterOutput(sOutput);

        // retrieve page orientation and paper type ...
        DEVMODE FAR* pDevMode=(DEVMODE FAR*)::GlobalLock(dlg.m_pd.hDevMode);
        switch(pDevMode->dmPaperSize) {
        case DMPAPER_A4:
            pTblPrintFmt->SetPaperType(PAPER_TYPE_A4);
            break;
        case DMPAPER_LETTER:
            pTblPrintFmt->SetPaperType(PAPER_TYPE_LETTER);
            break;
        case DMPAPER_LEGAL:
            pTblPrintFmt->SetPaperType(PAPER_TYPE_LEGAL);
            break;
        case DMPAPER_A3:
            pTblPrintFmt->SetPaperType(PAPER_TYPE_A3);
            break;
        case DMPAPER_TABLOID:
            pTblPrintFmt->SetPaperType(PAPER_TYPE_TABLOID);
            break;
        default:
            // we'll just use the previous value
            CIMSAString sMsg, sCurrPaperType;
            switch(fmtTblPrint.GetPaperType()) {
            case PAPER_TYPE_A4:
                sCurrPaperType=_T("A4");
                break;
            case PAPER_TYPE_A3:
                sCurrPaperType=_T("A3");
                break;
            case PAPER_TYPE_LETTER:
                sCurrPaperType=_T("letter");
                break;
            case PAPER_TYPE_LEGAL:
                sCurrPaperType=_T("legal");
                break;
            case PAPER_TYPE_TABLOID:
                sCurrPaperType=_T("tabloid");
                break;
            default:
                ASSERT(FALSE);
                break;
            }
            sMsg.Format(_T("Sorry, paper type [%s] is not supported.  Using [%s] instead."), (LPTSTR)pDevMode->dmFormName, (LPCTSTR)sCurrPaperType);
            AfxMessageBox(sMsg,MB_ICONINFORMATION);
            break;
        }
        pDefaultTblPrintFmt->SetPaperType(pTblPrintFmt->GetPaperType());

        switch(pDevMode->dmOrientation) {
        case DMORIENT_PORTRAIT:
            pTblPrintFmt->SetPageOrientation(PAGE_ORIENTATION_PORTRAIT);
            break;
        case DMORIENT_LANDSCAPE:
            pTblPrintFmt->SetPageOrientation(PAGE_ORIENTATION_LANDSCAPE);
            break;
        default:
            // we'll just use the previous value
            CIMSAString sMsg, sCurrPageOrientation;
            switch(fmtTblPrint.GetPageOrientation()) {
            case PAGE_ORIENTATION_PORTRAIT:
                sCurrPageOrientation=_T("portrait");
                break;
            case PAGE_ORIENTATION_LANDSCAPE:
                sCurrPageOrientation=_T("landscape");
                break;
            default:
                ASSERT(FALSE);
                break;
            }
            sMsg.Format(_T("Sorry, that page orientation is not supported.  Using %s instead."), (LPCTSTR)sCurrPageOrientation);
            AfxMessageBox(sMsg,MB_ICONINFORMATION);
            break;
        }
        pDefaultTblPrintFmt->SetPageOrientation(pTblPrintFmt->GetPageOrientation());

        if (pDefaultTblPrintFmt->GetPaperType()!=fmtTblPrint.GetPaperType() || pDefaultTblPrintFmt->GetPageOrientation()!=fmtTblPrint.GetPageOrientation()) {
            bPrintSettingsChanged = true;
        }

        ::GlobalUnlock(dlg.m_pd.hDevNames);
        ::GlobalUnlock(dlg.m_pd.hDevMode);
    }

    if (bPrintSettingsChanged) {
        CFlashMsgDlg flash_msg_dlg;
        flash_msg_dlg.m_iSeconds=3;
        flash_msg_dlg.m_sFlashMsg=_T("Table layout being adjusted to correspond to your changed printer settings.");
        flash_msg_dlg.DoModal();

        pSet->SetSelectedPrinterChanged();
        if (m_pPrintView!=nullptr) {
            // print view is alive, need to rebuild it ...
            if (!m_pPrintView->PreparePrinterDC()) {
                AfxMessageBox(_T("CTableChildWnd::OnFilePrintSetup -- error changing printer DC"), MB_ICONEXCLAMATION);
            }
            m_pPrintView->Build(true);
        }

        // we're dirty now!
        pDoc->SetModifiedFlag(TRUE);
    }
}


void CTableChildWnd::OnUpdateFilePrintSetup(CCmdUI* /*pCmdUI*/)
{
    // TODO: Add your command update UI handler code here
}


void CTableChildWnd::OnDesignView()
{
    //Now toggle the data view /design view
    m_bDesignView = !m_bDesignView;
    ShowTabView(m_bDesignView);
    //Update the grid in tab view

}


void CTableChildWnd::ShowTabView(bool /*bDesignView = true*/)
{
    //Force the view to tab view if it is not active
    if(GetPrintView()){//Toggle back to the tab view
        OnViewPageprintview();
    }
    if(IsLogicViewActive()){   //if in the logic view mode . Toggle back to tab view mode
        OnViewLogic();
    }
   // now we are in table view mode.Update the data grid in design/data view mode
    ASSERT(IsTabViewActive());
    CTabView* pTabView = GetTabView();
    ASSERT(pTabView);

    CTblGrid* pTblGrid = pTabView->GetGrid();
    pTblGrid->Update();
}

/////////////////////////////////////////////////////////////////////////////////
//
//  void CTableChildWnd::OnUpdateDesignView(CCmdUI *pCmdUI)
//
/////////////////////////////////////////////////////////////////////////////////
void CTableChildWnd::OnUpdateDesignView(CCmdUI *pCmdUI)
{
    pCmdUI->Enable(TRUE);
    m_bDesignView ? pCmdUI->SetCheck(TRUE): pCmdUI->SetCheck(FALSE);
}

/////////////////////////////////////////////////////////////////////////////////
//
//  void CTableChildWnd::OnExportTFT
//
/////////////////////////////////////////////////////////////////////////////////
void CTableChildWnd::OnExportTFT()
{
    CIMSAString sFileOpenDefExt = _T("tft");
    CIMSAString sFileOpenFilter = _T("CSPro Table Format (*.tft)|*.tft||");
    DWORD dwFileOpenFlags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT;
    CIMSAFileDialog dlgFile(FALSE, sFileOpenDefExt, nullptr, dwFileOpenFlags,sFileOpenFilter);
    if (dlgFile.DoModal() == IDOK)  {
        CWaitCursor wc;
        CTabulateDoc* pDoc = DYNAMIC_DOWNCAST(CTabulateDoc, GetActiveDocument());
        CTabSet* pSet = pDoc->GetTableSpec();
        const CFmtReg& fmtReg = pSet->GetFmtReg();
        const bool bDefaultOnly = true;
        if (!fmtReg.Save(dlgFile.GetPathName(), bDefaultOnly)) {
            AfxMessageBox(_T("Failed to save TFT file"), MB_OK); // fixme - string table
        }
    }
}


/////////////////////////////////////////////////////////////////////////////////
//
//  void CTableChildWnd::OnImportTFT
//
/////////////////////////////////////////////////////////////////////////////////
void CTableChildWnd::OnImportTFT()
{
    CIMSAString sFileOpenDefExt = _T("tft");
    CIMSAString sFileOpenFilter = _T("CSPro Table Format (*.tft)|*.tft|All Files (*.*)|*.*||");
    DWORD dwFileOpenFlags = OFN_HIDEREADONLY;
    CIMSAFileDialog dlgFile(TRUE, sFileOpenDefExt, nullptr, dwFileOpenFlags,sFileOpenFilter);
    if (dlgFile.DoModal() == IDOK)  {
        CWaitCursor wc;
        CFmtReg importedFmt;
        if (!importedFmt.Build(dlgFile.GetPathName())) {
            AfxMessageBox(_T("Failed to load TFT file"), MB_OK); // fixme - string table
        }
        CTabulateDoc* pDoc = DYNAMIC_DOWNCAST(CTabulateDoc, GetActiveDocument());
        CTabSet* pSet = pDoc->GetTableSpec();
        CFmtReg& fmtReg = *(pSet->GetFmtRegPtr());
        fmtReg.CopyDefaults(importedFmt);
        CTabView* pTabView = GetTabView();
        CTblGrid* pTblGrid = pTabView->GetGrid();
        pTblGrid->Update();
        if (m_pPrintView != nullptr) {
            m_pPrintView->Build(true);
            m_pPrintView->Invalidate();
        }
        pDoc->SetModifiedFlag(TRUE);
    }
}

void CTableChildWnd::OnEditTableTallyAttributes()
{
    OnParms();
}

void CTableChildWnd::OnUpdateEditTableTallyAttributes(CCmdUI* pCmdUI)
{
    bool bEnable = false;
    // TODO: Add your command update UI handler code here
    if(GetPrintView()){
        bEnable = false;
    }
    else {
        CView* pView = GetActiveView();
        if(pView->IsKindOf(RUNTIME_CLASS(CTabView))) {
            CTable* pTable = ((CTabView*)pView)->GetGrid()->GetTable();
            if(pTable && pTable->GetRowRoot()->GetNumChildren()>0){
                bEnable = true;
            }
        }
    }
    pCmdUI->Enable(bEnable);
}
/////////////////////////////////////////////////////////////////////////////////
//
//  void CTableChildWnd::DynamicMenu4Logic(bool bDelete /*= *true*/)
//
/////////////////////////////////////////////////////////////////////////////////
void CTableChildWnd::DynamicMenu4Logic(bool bDelete /*= true*/)
{
    CWnd* pMain = AfxGetMainWnd();

    if (pMain != nullptr){
        CMenu* pMenu = pMain->GetMenu();
        bool bRedrawMenuBar=false;
        if(pMenu) {
            if(bDelete){//here delete if it exists
                if (pMenu != nullptr && pMenu->GetMenuItemCount() > 0){
                    CMenu* pSubMenu = pMenu->GetSubMenu(2);
                    if (pSubMenu != nullptr && pSubMenu->GetMenuItemCount() > 0){
                        int iCount = pSubMenu->GetMenuItemCount();
                        pSubMenu->DeleteMenu(ID_TBL_EDIT_FIND, MF_BYCOMMAND);
                        pSubMenu->DeleteMenu(ID_FIND_NEXT, MF_BYCOMMAND);
                        pSubMenu->DeleteMenu(ID_REPLACE, MF_BYCOMMAND);
                        pSubMenu->DeleteMenu(ID_EDIT_GENERATELOGIC, MF_BYCOMMAND);
                        if(iCount > pSubMenu->GetMenuItemCount()){
                            bRedrawMenuBar = true;
                            pSubMenu->DeleteMenu(pSubMenu->GetMenuItemCount()-1,MF_BYPOSITION);
                        }
                    }
                }
            }
            else { //here insert if it is missing
                if (pMenu != nullptr && pMenu->GetMenuItemCount() > 0){
                    CMenu* pSubMenu = pMenu->GetSubMenu(2);
                    if (pSubMenu != nullptr && pSubMenu->GetMenuItemCount() > 0){
                        UINT state = pSubMenu->GetMenuState(ID_TBL_EDIT_FIND, MF_BYCOMMAND);
                        if(state == 0xFFFFFFFF){
                            bRedrawMenuBar = true;
                            pSubMenu->AppendMenu(MF_SEPARATOR);
                            //pSubMenu->InsertMenu(ID_EDIT_GENERATELOGIC,MF_BYCOMMAND,ID_TBL_EDIT_FIND,"&Find \tCtrl+F");
                            pSubMenu->AppendMenu(MF_STRING,ID_TBL_EDIT_FIND,_T("&Find \tCtrl+F"));
                        }
                        state = pSubMenu->GetMenuState(ID_FIND_NEXT, MF_BYCOMMAND);
                        if(state == 0xFFFFFFFF){
                            bRedrawMenuBar = true;
                           // pSubMenu->InsertMenu(ID_EDIT_GENERATELOGIC,MF_BYCOMMAND,ID_FIND_NEXT,"Find &Next \tF3");
                            pSubMenu->AppendMenu(MF_STRING,ID_FIND_NEXT,_T("Find &Next \tF3"));
                        }
                        state = pSubMenu->GetMenuState(ID_REPLACE, MF_BYCOMMAND);
                        if(state == 0xFFFFFFFF){
                            bRedrawMenuBar = true;
                            //pSubMenu->InsertMenu(ID_EDIT_GENERATELOGIC,MF_BYCOMMAND,ID_REPLACE,"R&eplace... \tCtrl+H");
                            pSubMenu->AppendMenu(MF_STRING,ID_REPLACE,_T("R&eplace... \tCtrl+H"));
                        }
                        state = pSubMenu->GetMenuState(ID_EDIT_GENERATELOGIC, MF_BYCOMMAND);
                        if(state == 0xFFFFFFFF){
                            bRedrawMenuBar = true;
                            //pSubMenu->InsertMenu(ID_EDIT_GENERATELOGIC,MF_BYCOMMAND,ID_REPLACE,"R&eplace... \tCtrl+H");
                            pSubMenu->AppendMenu(MF_STRING,ID_EDIT_GENERATELOGIC,_T("Generate Logic"));
                        }
                    }
                }
            }
            if(bRedrawMenuBar){
                pMain->DrawMenuBar();
            }
        }
        // No need to delete pMenu because it is an MFC
        // temporary object.
    }
}

/////////////////////////////////////////////////////////////////////////////////
//
//  void CTableChildWnd::OnUpdateRuntab(CCmdUI* pCmdUI)
//
/////////////////////////////////////////////////////////////////////////////////
void CTableChildWnd::OnUpdateRuntab(CCmdUI* pCmdUI)
{
    CTabulateDoc* pDoc = (CTabulateDoc*)GetActiveDocument();
    if(pDoc->GetTableSpec()->GetNumTables() > 0 && !m_pPrintView) {
        pCmdUI->Enable(TRUE);
    }
    else {
        pCmdUI->Enable(FALSE);
    }
}

/////////////////////////////////////////////////////////////////////////////////
//
//  void CTableChildWnd::OnRuntab()
//
/////////////////////////////////////////////////////////////////////////////////
void CTableChildWnd::OnRuntab()
{
    // TODO: Add your command handler code here
   CTabulateDoc* pDoc = (CTabulateDoc*)GetActiveDocument();
//    AfxMessageBox("Do Reconcile");
//    m_pGrid->SetCurSelArea(0);
    AfxGetMainWnd()->SendMessage(UWM::Table::RunActiveApplication, 1, reinterpret_cast<LPARAM>(pDoc));

    //Save Everything
}

void CTableChildWnd::OnEditExcludeAllButThis()
{
    CTabulateDoc* pDoc = DYNAMIC_DOWNCAST(CTabulateDoc, GetActiveDocument());

    //Get active tree selection
    //Get actvie table
    //Set the flag
    CTabView* pTableView = GetTabView();
    CTabTreeCtrl* pTableTree = pTableView->GetDocument()->GetTabTreeCtrl();
    TableSpecTabTreeNode* table_spec_tab_tree_node= pTableTree->GetTableSpecTabTreeNode(*pTableView->GetDocument());
    HTREEITEM hChildItem = pTableTree->GetSelectedItem();

    if(table_spec_tab_tree_node != nullptr && table_spec_tab_tree_node->GetHItem() != nullptr){
        HTREEITEM hItem = pTableTree->GetChildItem(table_spec_tab_tree_node->GetHItem());

        //loop through the tables in the tree
        //for the active item include in run
        //for the rest exclude from it
        while(hItem) {
            TableElementTreeNode* table_element_tree_node = pTableTree->GetTreeNode(hItem);
            if(table_element_tree_node && table_element_tree_node->GetTabDoc() == pTableView->GetDocument() && table_element_tree_node->GetTableElementType() == TableElementType::Table){
                if(hItem == hChildItem){
                    table_element_tree_node->GetTable()->SetExcludeTable4Run(false);//include the active table
                }
                else {
                    table_element_tree_node->GetTable()->SetExcludeTable4Run(true);//exclude the rest
                }

                pDoc->SetModifiedFlag(TRUE);
                if (table_element_tree_node->GetTable()->GetGenerateLogic()) {
                    if (table_element_tree_node->GetTable()->IsTableExcluded4Run()) {
                        pTableTree->SetItemImage(hItem,11,11);
                    }
                    else{
                        pTableTree->SetItemImage(hItem,1,1);
                    }
                }
                else {
                    if (table_element_tree_node->GetTable()->IsTableExcluded4Run()) {
                        pTableTree->SetItemImage(hItem,12,12);
                    }
                    else{
                        pTableTree->SetItemImage(hItem,2,2);
                    }
                }
            }
            hItem = pTableTree->GetNextSiblingItem(hItem);
        }
        pTableTree->Invalidate();
    }
}


void CTableChildWnd::OnUpdateEditExcludeAllButThis(CCmdUI *pCmdUI)
{
    pCmdUI->Enable(FALSE);

    CTabView* pTableView = GetTabView();
    CTabTreeCtrl* pTableTree = pTableView->GetDocument()->GetTabTreeCtrl();
    HTREEITEM hItem = pTableTree->GetSelectedItem();
    if(hItem) {
        TableElementTreeNode* table_element_tree_node = pTableTree->GetTreeNode(hItem);
        if(table_element_tree_node && table_element_tree_node->GetTabDoc() == pTableView->GetDocument() && table_element_tree_node->GetTableElementType() == TableElementType::Table){
            pCmdUI->Enable(TRUE);
            pCmdUI->SetCheck(0);
        }
    }
}


void CTableChildWnd::OnEditIncludeAll()
{
    CTabulateDoc* pDoc = DYNAMIC_DOWNCAST(CTabulateDoc, GetActiveDocument());

    //Get active tree selection
    //Get actvie table
    //Set the flag
    CTabView* pTableView = GetTabView();
    CTabTreeCtrl* pTableTree = pTableView->GetDocument()->GetTabTreeCtrl();
    TableSpecTabTreeNode* table_spec_tab_tree_node = pTableTree->GetTableSpecTabTreeNode(*pTableView->GetDocument());

    if(table_spec_tab_tree_node != nullptr && table_spec_tab_tree_node->GetHItem() != nullptr){
        HTREEITEM hItem = pTableTree->GetChildItem(table_spec_tab_tree_node->GetHItem());

        //loop through the tables in the tree
        //for the active item include in run
        //for the rest exclude from it
        while(hItem) {
            TableElementTreeNode* table_element_tree_node = pTableTree->GetTreeNode(hItem);
            if(table_element_tree_node && table_element_tree_node->GetTabDoc() == pTableView->GetDocument() && table_element_tree_node->GetTableElementType() == TableElementType::Table){
                bool bFlag = table_element_tree_node->GetTable()->IsTableExcluded4Run();
                if(bFlag){
                    table_element_tree_node->GetTable()->SetExcludeTable4Run(false);//include the active table
                    pDoc->SetModifiedFlag(TRUE);
                }
                if (table_element_tree_node->GetTable()->GetGenerateLogic()) {
                    if (table_element_tree_node->GetTable()->IsTableExcluded4Run()) {
                        pTableTree->SetItemImage(hItem,11,11);
                    }
                    else{
                        pTableTree->SetItemImage(hItem,1,1);
                    }
                }
                else {
                    if (table_element_tree_node->GetTable()->IsTableExcluded4Run()) {
                        pTableTree->SetItemImage(hItem,12,12);
                    }
                    else{
                        pTableTree->SetItemImage(hItem,2,2);
                    }
                }
            }
            hItem = pTableTree->GetNextSiblingItem(hItem);
        }
        pTableTree->Invalidate();
    }
}


void CTableChildWnd::OnUpdateEditIncludeAll(CCmdUI *pCmdUI)
{
    pCmdUI->Enable(FALSE);
    CTabulateDoc* pDoc = DYNAMIC_DOWNCAST(CTabulateDoc, GetActiveDocument());

    if(pDoc && pDoc->GetTableSpec()->GetNumTables() > 0) {
        pCmdUI->Enable(TRUE);
        pCmdUI->SetCheck(0);
    }
}
