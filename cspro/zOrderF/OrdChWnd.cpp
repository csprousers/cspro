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
#include "OrdChWnd.h"


IMPLEMENT_DYNCREATE(COrderChildWnd, ApplicationChildWnd)


BEGIN_MESSAGE_MAP(COrderChildWnd, ApplicationChildWnd)
    ON_WM_SYSCOMMAND()
    ON_WM_MDIACTIVATE()
    ON_COMMAND(ID_RUN, OnRun)
    ON_UPDATE_COMMAND_UI(ID_RUN, OnUpdateRunCompile)
    ON_COMMAND(ID_COMPILE, OnCompile)
    ON_UPDATE_COMMAND_UI(ID_COMPILE, OnUpdateRunCompile)
    ON_COMMAND(ID_VIEW_QUESTIONNAIRE, OnViewQuestionnaire)
    ON_UPDATE_COMMAND_UI(ID_VIEW_QUESTIONNAIRE, OnUpdateViewQuestionnaire)
    ON_COMMAND(ID_VIEW_LOGIC, OnViewLogic)
    ON_UPDATE_COMMAND_UI(ID_VIEW_LOGIC, OnUpdateViewLogic)
END_MESSAGE_MAP()


COrderChildWnd::COrderChildWnd()
    :   m_viewingQuestionnaire(false),
        m_questionnaireView(nullptr),
        m_sourceEditView(nullptr)
{
}


void COrderChildWnd::ActivateFrame(int nCmdShow)
{
    // TODO: Add your specialized code here and/or call the base class
    nCmdShow = SW_SHOWMAXIMIZED;
    __super::ActivateFrame(nCmdShow);

    AfxGetMainWnd()->SendMessage(UWM::Designer::ShowToolbar, static_cast<WPARAM>(FrameType::Order));
    WindowsDesktopMessage::Send(UWM::UtilO::SetStatusBarText, reinterpret_cast<WPARAM>(nullptr));

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
    const bool dictionary_order = ( pOrder != nullptr && pOrder->GetFormFile().IsDictOrder() );
    WindowsDesktopMessage::Send(UWM::UtilO::SetStatusBarText, dictionary_order ? L"Dictionary Order" : L"Custom Order");
}


BOOL COrderChildWnd::Create(LPCTSTR lpszClassName, LPCTSTR lpszWindowName, DWORD dwStyle, const RECT& rect, CMDIFrameWnd* pParentWnd, CCreateContext* pContext)
{
    // TODO: Add your specialized code here and/or call the base class

    if( !__super::Create(lpszClassName, lpszWindowName, dwStyle, rect, pParentWnd, pContext) )
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
    if( !m_logicDlgBar.CreateAndDock(this, pContext->m_pCurrentDoc) )
        return FALSE;

    // create the reference window
    if( !m_logicReferenceWnd.CreateAndDock(this, pContext->m_pCurrentDoc) )
        return FALSE;

    return TRUE;
}


void COrderChildWnd::OnSysCommand( UINT nID, LPARAM lParam )
{
    if(nID == SC_CLOSE)  {
        return;
    }
    else {
        __super::OnSysCommand(nID,lParam);
    }
}


/////////////////////////////////////////////////////////////////////////////
//SAVY&& Not yet updated for CSBatch support 05/22/00 you got to check for source code update on activate


void COrderChildWnd::OnMDIActivate(BOOL bActivate, CWnd* pActivateWnd, CWnd* pDeactivateWnd)
{
    __super::OnMDIActivate(bActivate, pActivateWnd, pDeactivateWnd); // mfc-gen

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
        if (m_viewingQuestionnaire) {
            OnViewQuestionnaire();
        }
        WindowsDesktopMessage::Send(UWM::UtilO::SetStatusBarText, reinterpret_cast<WPARAM>(nullptr));
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
                    if(GetOSourceView()->GetLogicCtrl()->IsModified())
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
        WindowsDesktopMessage::Send(UWM::UtilO::SetStatusBarText, reinterpret_cast<WPARAM>(nullptr));
    }*/
}


COSourceEditView* COrderChildWnd::GetOSourceView()
{
    if( m_sourceEditView == nullptr )
        m_sourceEditView = assert_nullable_cast<COSourceEditView*>(GetActiveView());

    return m_sourceEditView;
}


LogicView* COrderChildWnd::GetSourceLogicView()
{
    return GetOSourceView();
}


LogicCtrl* COrderChildWnd::GetSourceLogicCtrl()
{
    COSourceEditView* pView = GetOSourceView();
    ASSERT(pView != nullptr);
    return pView->GetLogicCtrl();
}


void COrderChildWnd::OnRun()
{
    COrderDoc* const pOrderDoc = assert_cast<COrderDoc*>(GetActiveDocument());

    if( pOrderDoc == nullptr )
        return;

    HTREEITEM hItem = pOrderDoc->GetOrderTreeCtrl()->GetSelectedItem();
    AppTreeNode* app_tree_node = pOrderDoc->GetOrderTreeCtrl()->GetTreeNode(hItem);

    if( app_tree_node != nullptr && app_tree_node->GetDocument() == pOrderDoc )
    {
        if( WindowsDesktopMessage::Send(UWM::Order::PutSourceCode, 0, app_tree_node) == -1 )
            return;
    }

    WindowsDesktopMessage::Send(UWM::Order::RunActiveApplication, 0, pOrderDoc);
}


void COrderChildWnd::OnCompile()
{
    COrderDoc* const pOrderDoc = assert_cast<COrderDoc*>(GetActiveDocument());

    if( pOrderDoc == nullptr )
        return;

    HTREEITEM hItem = pOrderDoc->GetOrderTreeCtrl()->GetSelectedItem();
    AppTreeNode* const app_tree_node = pOrderDoc->GetOrderTreeCtrl()->GetTreeNode(hItem);

    if( app_tree_node != nullptr && app_tree_node->GetDocument() == pOrderDoc )
    {
        constexpr WPARAM ForceCompile = 1;
        WindowsDesktopMessage::Send(UWM::Order::PutSourceCode, ForceCompile, app_tree_node);
        GetSourceLogicCtrl()->SetFocus();
    }
}


void COrderChildWnd::OnUpdateRunCompile(CCmdUI* const pCmdUI)
{
    pCmdUI->Enable(!m_viewingQuestionnaire);
}


void COrderChildWnd::OnViewQuestionnaire()
{
    m_logicDlgBar.ShowWindow(SW_HIDE);
    m_logicReferenceWnd.ShowWindow(SW_HIDE);
    if (!m_viewingQuestionnaire) {
        m_viewingQuestionnaire = TRUE;
        //create questionnaire view and swap
        COrderDoc* pOrderDoc = assert_cast<COrderDoc*>(GetActiveDocument());
        CRect rect(0, 0, 100, 100);
        GetClientRect(&rect);
        //GetOSourceView()->GetClientRect(&rect);
        bool refreshContent = true;
        if (!m_questionnaireView) {
            refreshContent = false;
            m_questionnaireView = new QuestionnaireView(*pOrderDoc);
            DWORD dwStyle = AFX_WS_DEFAULT_VIEW;
           // dwStyle = WS_CHILDWINDOW | WS_VISIBLE | WS_BORDER;
            CCreateContext context = CCreateContext();
            int iID = 11111;
            context.m_pCurrentDoc = pOrderDoc;
            if (!m_questionnaireView->Create(NULL, _T(""), dwStyle, rect, this, iID, &context)) {
                TRACE0("Failed to create questionnaire view\n");
                ASSERT(false);
                return;
            }
        }
        // exchange view window IDs so RecalcLayout() works
        UINT temp = GetOSourceView()->GetDlgCtrlID();
        UINT currentId = m_questionnaireView->GetDlgCtrlID();
        m_questionnaireView->SetDlgCtrlID(temp);
        GetOSourceView()->SetDlgCtrlID(currentId);

        GetOSourceView()->ShowWindow(SW_HIDE);
        m_questionnaireView->ShowWindow(SW_SHOW);
        if (refreshContent) {
            m_questionnaireView->RefreshContent(false);
        }
        m_questionnaireView->SendMessage(WM_INITIALUPDATE);
        SetActiveView(m_questionnaireView);

        RecalcLayout();
        m_questionnaireView->Invalidate();
        GetClientRect(&rect);
        __super::OnSize(SIZE_RESTORED, rect.Width(), rect.Height());
        m_questionnaireView->SendMessage(WM_SIZE);
    }
    else {
        m_questionnaireView->RefreshContent(false);
    }
}


void COrderChildWnd::OnUpdateViewQuestionnaire(CCmdUI* const pCmdUI)
{
    pCmdUI->SetCheck(m_viewingQuestionnaire);
}


void COrderChildWnd::OnViewLogic()
{
    if (m_viewingQuestionnaire || !m_questionnaireView) {
        m_viewingQuestionnaire = FALSE;
        if (m_questionnaireView && m_questionnaireView->GetSafeHwnd()) {
            m_questionnaireView->ShowWindow(SW_HIDE);
        }
        // exchange view window IDs so RecalcLayout() works
        UINT temp = GetOSourceView()->GetDlgCtrlID();
        UINT currentId = m_questionnaireView->GetDlgCtrlID();
        m_questionnaireView->SetDlgCtrlID(temp);
        GetOSourceView()->SetDlgCtrlID(currentId);

        m_logicDlgBar.ShowWindow(SW_SHOW);
        GetOSourceView()->ShowWindow(SW_SHOW);
        SetActiveView(GetOSourceView());
        GetOSourceView()->SendMessage(WM_INITIALUPDATE);
        RecalcLayout();
    }
}


void COrderChildWnd::OnUpdateViewLogic(CCmdUI* const pCmdUI)
{
    pCmdUI->SetCheck(!m_viewingQuestionnaire);
}
