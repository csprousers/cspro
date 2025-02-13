//***************************************************************************
//  File name: OrdChWnd.cpp
//
//  Description:
//       Order child window implementation
//
//  History:    Date       Author   Comment
//              ---------------------------
//              04 Jun 99   gsf     copied from FrmChWnd.cpp
//
//***************************************************************************

#include "StdAfx.h"
#include "OrdChWnd.H"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// COrderChildWnd

IMPLEMENT_DYNCREATE(COrderChildWnd, ApplicationChildWnd)

COrderChildWnd::COrderChildWnd() : m_bQuestionnaireView(FALSE), m_pQuestionnaireView(NULL), m_pOSourceEditView(NULL)
{
}

COrderChildWnd::~COrderChildWnd()
{
}


BEGIN_MESSAGE_MAP(COrderChildWnd, ApplicationChildWnd)
    //{{AFX_MSG_MAP(COrderChildWnd)
    ON_WM_SYSCOMMAND()
    ON_WM_MDIACTIVATE()
    ON_COMMAND(ID_RUNBATCH, OnRun)
    ON_COMMAND(ID_ORD_COMPILE, OnCompile)
    ON_COMMAND(ID_VIEW_PEEK_LOGIC_WORD, OnPeekLogicWord)
    ON_COMMAND(ID_VIEW_GOTO_LOGIC_WORD, OnGotoLogicWord)
    ON_COMMAND(ID_COMMENT_CODE, OnCommentCode)
    ON_COMMAND(ID_DUPLICATE_LINE, OnDuplicateLine)
    ON_COMMAND(ID_LOGIC_FORMAT, OnFormatLogic)
    ON_COMMAND(ID_VIEW_QUESTIONNAIRE, &COrderChildWnd::OnViewQuestionnaire)
    ON_UPDATE_COMMAND_UI(ID_VIEW_QUESTIONNAIRE, &COrderChildWnd::OnUpdateViewQuestionnaire)
    //}}AFX_MSG_MAP
    ON_COMMAND(ID_VIEW_LOGIC, &COrderChildWnd::OnViewLogic)
    ON_UPDATE_COMMAND_UI(ID_VIEW_LOGIC, &COrderChildWnd::OnUpdateViewLogic)
    ON_UPDATE_COMMAND_UI(ID_ORD_COMPILE, &COrderChildWnd::OnUpdateOrdCompile)
    ON_UPDATE_COMMAND_UI(ID_LOGIC_FORMAT, &COrderChildWnd::OnUpdateLogicFormat)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// COrderChildWnd message handlers


/////////////////////////////////////////////////////////////////////////////////
//
//  BOOL COrderChildWnd::PreCreateWindow(CREATESTRUCT& cs)
//
/////////////////////////////////////////////////////////////////////////////////
BOOL COrderChildWnd::PreCreateWindow(CREATESTRUCT& cs)
{
    // TODO: Add your specialized code here and/or call the base class
//  cs.style &= ~(WS_SYSMENU);
    return COXMDIChildWndSizeDock::PreCreateWindow(cs);
}


/////////////////////////////////////////////////////////////////////////////////
//
//  void COrderChildWnd::ActivateFrame(int nCmdShow)
//
/////////////////////////////////////////////////////////////////////////////////
void COrderChildWnd::ActivateFrame(int nCmdShow)
{
    // TODO: Add your specialized code here and/or call the base class
    nCmdShow = SW_SHOWMAXIMIZED;
    COXMDIChildWndSizeDock::ActivateFrame(nCmdShow);

    AfxGetMainWnd()->SendMessage(UWM::Designer::ShowToolbar, static_cast<WPARAM>(FrameType::Order));
    AfxGetMainWnd()->SendMessage(WM_IMSA_SET_STATUSBAR_PANE, NULL);

    //To Make the tree selection in sync with the correct document
    COrderDoc* pOrderDoc = assert_cast<COrderDoc*>(GetActiveDocument());
    if(pOrderDoc){
        COrderTreeCtrl* pOrderTree= pOrderDoc->GetOrderTreeCtrl();
        if(!pOrderTree) return;
        HTREEITEM hItem = pOrderTree->GetSelectedItem();
        if(hItem) {
            pOrderTree->GetSelectedItem();
            const AppTreeNode* app_tree_node = pOrderTree->GetTreeNode(hItem);
            if(app_tree_node->GetDocument() != pOrderDoc) {
                //Then we need to go in sync
                const FormOrderAppTreeNode* form_order_app_tree_node = pOrderTree->GetFormOrderAppTreeNode(*pOrderDoc);
                if(form_order_app_tree_node != nullptr && form_order_app_tree_node->GetHItem() != nullptr){
                    pOrderTree->Select(form_order_app_tree_node->GetHItem(), TVGN_CARET);
                }
            }

        }
    }

    COrderDoc* pOrder = assert_cast<COrderDoc*>(GetActiveDocument());
    if (pOrder && pOrder->GetFormFile().IsDictOrder()) {
        AfxGetMainWnd()->SendMessage(WM_IMSA_SET_STATUSBAR_PANE, reinterpret_cast<WPARAM>(_T("Dictionary Order")));
    }
    else {
        AfxGetMainWnd()->SendMessage(WM_IMSA_SET_STATUSBAR_PANE, reinterpret_cast<WPARAM>(_T("Custom Order")));
    }
}

/////////////////////////////////////////////////////////////////////////////////
//
//  BOOL COrderChildWnd::Create(LPCTSTR lpszClassName, LPCTSTR lpszWindowName, DWORD dwStyle, const RECT& rect, CMDIFrameWnd* pParentWnd, CCreateContext* pContext)
//
/////////////////////////////////////////////////////////////////////////////////
BOOL COrderChildWnd::Create(LPCTSTR lpszClassName, LPCTSTR lpszWindowName, DWORD dwStyle, const RECT& rect, CMDIFrameWnd* pParentWnd, CCreateContext* pContext)
{
    // TODO: Add your specialized code here and/or call the base class

    if( !COXMDIChildWndSizeDock::Create(lpszClassName, lpszWindowName, dwStyle, rect, pParentWnd, pContext) )
        return FALSE;

    // Get the window's menu
    CMenu* pMenu = GetSystemMenu(FALSE);
    VERIFY(pMenu->GetSafeHmenu());

    // Disable the 'X'
    VERIFY(::DeleteMenu(pMenu->GetSafeHmenu(), SC_CLOSE , MF_BYCOMMAND));
    VERIFY(::DeleteMenu(pMenu->GetSafeHmenu(), 5 , MF_BYPOSITION));

    // Force immediate menu update
    DrawMenuBar();


    EnableDocking(CBRS_ALIGN_ANY);

    // create the compiler output and messages tabs
    if( !m_logicDlgBar.CreateAndDock(this) )
        return FALSE;

    // create the reference window
    if( !m_logicReferenceWnd.CreateAndDock(this) )
        return FALSE;

    return TRUE;
}

/////////////////////////////////////////////////////////////////////////////////
//
//  void COrderChildWnd::OnSysCommand( UINT nID, LPARAM lParam )
//
/////////////////////////////////////////////////////////////////////////////////
void COrderChildWnd::OnSysCommand( UINT nID, LPARAM lParam )
{
    if(nID == SC_CLOSE)  {
        return;
    }
    else {
        COXMDIChildWndSizeDock::OnSysCommand(nID,lParam);
    }
}
/////////////////////////////////////////////////////////////////////////////
//SAVY&& Not yet updated for CSBatch support 05/22/00 you got to check for source code update on activate

/////////////////////////////////////////////////////////////////////////////////
//
//  void COrderChildWnd::OnMDIActivate(BOOL bActivate, CWnd* pActivateWnd, CWnd* pDeactivateWnd)
//
/////////////////////////////////////////////////////////////////////////////////
void COrderChildWnd::OnMDIActivate(BOOL bActivate, CWnd* pActivateWnd, CWnd* pDeactivateWnd)
{
    COXMDIChildWndSizeDock::OnMDIActivate(bActivate, pActivateWnd, pDeactivateWnd); // mfc-gen

    if (bActivate) {
        //TO DO THE RECONCILE //SAVY&&&
        COrderDoc* pDoc = assert_cast<COrderDoc*>(GetActiveDocument());
        if(!pDoc)
            return;
        CDEFormFile* pOrder = &pDoc->GetFormFile();
        CString csErr;
        COrderTreeCtrl* pTree = pDoc->GetOrderTreeCtrl();
        if(pTree)
            pTree->SetRedraw(FALSE);
        else
            return;

        bool bFlag = pOrder->OReconcile(csErr, false, true);   // not silent, yes autofix

        if (!bFlag && pTree){
            // if a change (i.e., deletion!) was made in the form file, redo some stuff
            pDoc->SetModifiedFlag();

            pTree->SetSndMsgFlg(FALSE);
            pTree->SelectItem (pTree->GetRootItem());
            pTree->ReBuildTree(0,NULL,false);
            pTree->SetRedraw(TRUE);
            pTree->SetSndMsgFlg(TRUE);

            AfxGetMainWnd()->SendMessage(UWM::Order::ShowSourceCode, 0, reinterpret_cast<LPARAM>(pDoc));
        }
        pDoc->GetFormFile().UpdatePointers();
        pTree->SetRedraw(TRUE);
        if (m_bQuestionnaireView) {
            OnViewQuestionnaire();
        }
        AfxGetMainWnd()->SendMessage(WM_IMSA_SET_STATUSBAR_PANE, NULL);
    }

    if(bActivate && pActivateWnd && pActivateWnd->IsKindOf(RUNTIME_CLASS(COrderChildWnd))) {
        // when activating the frame, if the toolbar for drawing boxes was displayed
        // the last go-around, put it back up!
        //To Make the tree selection in sync with the correct document
        COrderDoc* pDoc = assert_cast<COrderDoc*>(GetActiveDocument());
        if(pDoc){
            COrderTreeCtrl* pOrderTree= pDoc->GetOrderTreeCtrl();
            if(pOrderTree) {
                HTREEITEM hItem = pOrderTree->GetSelectedItem();
                if(hItem) {
                    pOrderTree->GetSelectedItem();
                    const AppTreeNode* app_tree_node = pOrderTree->GetTreeNode(hItem);
                    if(app_tree_node->GetDocument() != pDoc) {
                        //Then we need to go in sync
                        FormOrderAppTreeNode* form_order_app_tree_node = pOrderTree->GetFormOrderAppTreeNode(*pDoc);
                        if(form_order_app_tree_node != nullptr && form_order_app_tree_node->GetHItem() != nullptr){
                            pOrderTree->Select(form_order_app_tree_node->GetHItem(),TVGN_CARET);
                        }
                    }

                }
            }
        }

        AfxGetMainWnd()->SendMessage(UWM::Designer::ShowToolbar, static_cast<WPARAM>(FrameType::Order));
        AfxGetMainWnd()->SendMessage(UWM::Designer::SelectTab, static_cast<WPARAM>(FrameType::Order));
    }

    if(!bActivate && pDeactivateWnd && pDeactivateWnd->IsKindOf(RUNTIME_CLASS(COrderChildWnd))) {
        // likewise, when deactivating the frame, if the toolbar for box drawing is
        // being displayed, trash it (it will get reinstated next time around)

        COrderDoc* pDoc = assert_cast<COrderDoc*>(GetActiveDocument());
        COrderTreeCtrl* pOrderTree= pDoc->GetOrderTreeCtrl();
        if(pOrderTree) {
            HTREEITEM hItem = pOrderTree->GetSelectedItem();
            if(hItem) {
                AppTreeNode* app_tree_node = pOrderTree->GetTreeNode(hItem);
               // BOOL bSendMsg = pOrderTree->GetSndMsgFlg(); SAVY&& check this
                if(app_tree_node != nullptr && app_tree_node->GetDocument() == pDoc /*&& bSendMsg*/)
                {
                    if(GetOSourceView()->GetEditCtrl()->IsModified())
                        (AfxGetMainWnd()->SendMessage(UWM::Order::PutSourceCode, 0, reinterpret_cast<LPARAM>(app_tree_node)) == -1);

                }
            }
        }

        if(!pActivateWnd || !pActivateWnd->IsKindOf(RUNTIME_CLASS(COrderChildWnd)))
            AfxGetMainWnd()->SendMessage(UWM::Designer::HideToolbar);
    }
   /* if (bActivate) {
        //TO DO THE RECONCILE //SAVY&&&
        COrderDoc* pDoc = assert_cast<COrderDoc*>(GetActiveDocument());
        if(!pDoc)
            return;
        CDEFormFile* pOrder = pDoc->GetOrderSpec();
        CString csErr;
        bool bFlag = pOrder->OReconcile(csErr, false, true);   // not silent, yes autofix

        if (!bFlag){
            // if a change (i.e., deletion!) was made in the form file, redo some stuff
            pDoc->SetModifiedFlag();
            COrderTreeCtrl* pTree = pDoc->GetOrderTreeCtrl();
            pTree->SetSndMsgFlg(FALSE);

            pTree->SelectItem (pTree->GetRootItem());
            pTree->ReBuildTree();

            pTree->SetSndMsgFlg(TRUE);

        }
        AfxGetMainWnd()->SendMessage(WM_IMSA_SET_STATUSBAR_PANE, NULL);
    }*/
}

/////////////////////////////////////////////////////////////////////////////////
//
//  void COrderChildWnd::OnRun()
//
/////////////////////////////////////////////////////////////////////////////////
void COrderChildWnd::OnRun()
{
    COrderDoc* pOrderDoc = assert_cast<COrderDoc*>(GetActiveDocument());
    if(!pOrderDoc)
        return;

    CView* pView = GetActiveView();
    if(!pView)
        return;

    if(pView->IsKindOf(RUNTIME_CLASS(COSourceEditView))) {

        HTREEITEM hItem = pOrderDoc->GetOrderTreeCtrl()->GetSelectedItem();
        AppTreeNode* app_tree_node = pOrderDoc->GetOrderTreeCtrl()->GetTreeNode(hItem);

        if(app_tree_node != nullptr && app_tree_node->GetDocument() == pOrderDoc) {

            if(AfxGetMainWnd()->SendMessage(UWM::Order::PutSourceCode, 0, reinterpret_cast<LPARAM>(app_tree_node)) == -1)
                return;
        }

        AfxGetMainWnd()->SendMessage(UWM::Order::RunActiveApplication, 0, reinterpret_cast<LPARAM>(pOrderDoc));
        return;
    }

}
/////////////////////////////////////////////////////////////////////////////////
//
//  COSourceEditView* COrderChildWnd::GetOSourceView()
//
/////////////////////////////////////////////////////////////////////////////////
COSourceEditView* COrderChildWnd::GetOSourceView()
{
    if (!m_pOSourceEditView) {
        m_pOSourceEditView = assert_nullable_cast<COSourceEditView*>(GetActiveView());
    }
    return m_pOSourceEditView;
}


CLogicView* COrderChildWnd::GetSourceLogicView()
{
    return GetOSourceView();
}


CLogicCtrl* COrderChildWnd::GetSourceLogicCtrl()
{
    COSourceEditView* pView = GetOSourceView();
    ASSERT(pView != nullptr);
    return pView->GetLogicCtrl();
}

/////////////////////////////////////////////////////////////////////////////////
//
//  void COrderChildWnd::OnCompile()
//
/////////////////////////////////////////////////////////////////////////////////
void COrderChildWnd::OnCompile()
{
    COrderDoc* pDoc = assert_cast<COrderDoc*>(GetActiveDocument());
    CView* pView = GetActiveView();

    if (pDoc && pView->IsKindOf(RUNTIME_CLASS(COSourceEditView)))
    {
        HTREEITEM hItem = pDoc->GetOrderTreeCtrl()->GetSelectedItem();
        AppTreeNode* app_tree_node = pDoc->GetOrderTreeCtrl()->GetTreeNode(hItem);

        if( app_tree_node != nullptr && app_tree_node->GetDocument() == pDoc )
        {
            //wParam 1 implies force compile
            AfxGetMainWnd()->SendMessage(UWM::Order::PutSourceCode, 1, reinterpret_cast<LPARAM>(app_tree_node));
            ((COSourceEditView*)pView)->GetEditCtrl()->SetFocus();
        }
    }
}

void COrderChildWnd::OnPeekLogicWord()
{
    AfxGetMainWnd()->SendMessage(ZEDIT2O_LOGIC_REFERENCE, ZEDIT2O_LOGIC_REFERENCE, reinterpret_cast<LPARAM>(GetOSourceView()->GetEditCtrl()));
}

void COrderChildWnd::OnGotoLogicWord()
{
    AfxGetMainWnd()->SendMessage(ZEDIT2O_LOGIC_REFERENCE, ZEDIT2O_LOGIC_REFERENCE_GOTO, reinterpret_cast<LPARAM>(GetOSourceView()->GetEditCtrl()));
}


template<typename CF>
void COrderChildWnd::ModifyDocumentAndDoWithLogicCtrl(CF callback_function)
{
    CView* pView = GetActiveView();

    if( pView != nullptr && pView->IsKindOf(RUNTIME_CLASS(COSourceEditView)) )
    {
        COrderDoc* pOrderDoc = assert_cast<COrderDoc*>(GetActiveDocument());

        if( pOrderDoc != nullptr )
        {
            pOrderDoc->SetModifiedFlag();
            callback_function(((COSourceEditView*)pView)->GetEditCtrl());
        }
    }
}


void COrderChildWnd::OnCommentCode()
{
    ModifyDocumentAndDoWithLogicCtrl(
        [](CLogicCtrl* logic_control)
        {
            logic_control->CommentCode();
        });
}


void COrderChildWnd::OnDuplicateLine()
{
    ModifyDocumentAndDoWithLogicCtrl(
        [](CLogicCtrl* logic_control)
        {
            if( logic_control->GetSelectionEmpty() )
                logic_control->LineDuplicate();

            else
                logic_control->SelectionDuplicate();
        });
}


void COrderChildWnd::OnFormatLogic()
{
    ModifyDocumentAndDoWithLogicCtrl(
        [](CLogicCtrl* logic_control)
        {
            logic_control->FormatLogic();
        });
}


void COrderChildWnd::OnViewQuestionnaire()
{
    m_logicDlgBar.ShowWindow(SW_HIDE);
    m_logicReferenceWnd.ShowWindow(SW_HIDE);
    if (!m_bQuestionnaireView) {
        m_bQuestionnaireView = TRUE;
        //create questionnaire view and swap
        COrderDoc* pOrderDoc = assert_cast<COrderDoc*>(GetActiveDocument());
        CRect rect(0, 0, 100, 100);
        GetClientRect(&rect);
        //GetOSourceView()->GetClientRect(&rect);
        bool refreshContent = true;
        if (!m_pQuestionnaireView) {
            refreshContent = false;
            m_pQuestionnaireView = new QuestionnaireView(*pOrderDoc);
            DWORD dwStyle = AFX_WS_DEFAULT_VIEW;
           // dwStyle = WS_CHILDWINDOW | WS_VISIBLE | WS_BORDER;
            CCreateContext context = CCreateContext();
            int iID = 11111;
            context.m_pCurrentDoc = pOrderDoc;
            if (!m_pQuestionnaireView->Create(NULL, _T(""), dwStyle, rect, this, iID, &context)) {
                TRACE0("Failed to create questionnaire view\n");
                ASSERT(false);
                return;
            }
        }
        // exchange view window IDs so RecalcLayout() works
        UINT temp = GetOSourceView()->GetDlgCtrlID();
        UINT currentId = m_pQuestionnaireView->GetDlgCtrlID();
        m_pQuestionnaireView->SetDlgCtrlID(temp);
        GetOSourceView()->SetDlgCtrlID(currentId);

        GetOSourceView()->ShowWindow(SW_HIDE);
        m_pQuestionnaireView->ShowWindow(SW_SHOW);
        if (refreshContent) {
            m_pQuestionnaireView->RefreshContent(false);
        }
        m_pQuestionnaireView->SendMessage(WM_INITIALUPDATE);
        SetActiveView(m_pQuestionnaireView);

        RecalcLayout();
        m_pQuestionnaireView->Invalidate();
        GetClientRect(&rect);
        COXMDIChildWndSizeDock::OnSize(SIZE_RESTORED, rect.Width(), rect.Height());
        m_pQuestionnaireView->SendMessage(WM_SIZE);
    }
    else {
        m_pQuestionnaireView->RefreshContent(false);
    }
}


void COrderChildWnd::OnUpdateViewQuestionnaire(CCmdUI* pCmdUI)
{
    pCmdUI->SetCheck(m_bQuestionnaireView);
}


void COrderChildWnd::OnViewLogic()
{
    if (m_bQuestionnaireView || !m_pQuestionnaireView) {
        m_bQuestionnaireView = FALSE;
        if (m_pQuestionnaireView && m_pQuestionnaireView->GetSafeHwnd()) {
            m_pQuestionnaireView->ShowWindow(SW_HIDE);
        }
        // exchange view window IDs so RecalcLayout() works
        UINT temp = GetOSourceView()->GetDlgCtrlID();
        UINT currentId = m_pQuestionnaireView->GetDlgCtrlID();
        m_pQuestionnaireView->SetDlgCtrlID(temp);
        GetOSourceView()->SetDlgCtrlID(currentId);

        m_logicDlgBar.ShowWindow(SW_SHOW);
        GetOSourceView()->ShowWindow(SW_SHOW);
        SetActiveView(GetOSourceView());
        GetOSourceView()->SendMessage(WM_INITIALUPDATE);
        RecalcLayout();
    }
}


void COrderChildWnd::OnUpdateViewLogic(CCmdUI* pCmdUI)
{
    if (m_bQuestionnaireView) {
        pCmdUI->SetCheck(FALSE);
    }
    else {
        pCmdUI->SetCheck(TRUE);
    }
}


void COrderChildWnd::OnUpdateOrdCompile(CCmdUI* pCmdUI)
{
    m_bQuestionnaireView ? pCmdUI->Enable(FALSE) : pCmdUI->Enable(TRUE);
}


void COrderChildWnd::OnUpdateLogicFormat(CCmdUI* pCmdUI)
{
    m_bQuestionnaireView ? pCmdUI->Enable(FALSE) : pCmdUI->Enable(TRUE);
}

