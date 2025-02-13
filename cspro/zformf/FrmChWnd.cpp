//***************************************************************************
//  File name: FrmChWnd.cpp
//
//  Description:
//       Forms child window implementation
//
//  History:    Date       Author   Comment
//              ---------------------------
//              11 Feb 99   gsf     Created for Measure 1.0
//
//***************************************************************************

#include "StdAfx.h"
#include "FrmChWnd.h"
#include "BoxToolBar.h"
#include "DataSourceOptionsDlg.h"
#include "DragOptionsDlg.h"
#include "FormFileOptionsDlg.h"
#include "MappingOptionsDlg.h"
#include "MultipleFieldPropertiesDlg.h"
#include "QSFCndVw.h"
#include "QSFEditStyleDlg.h"
#include "QSFEView.h"
#include "RunAsBatchDlg.h"
#include "SyncParamsDlg.h"
#include <zToolsO/WinRegistry.h>
#include <zAppO/Application.h>
#include <zDictF/CapiLDlg.h>
#include <zDictF/UWM.h>
#include <zCapiO/CapiQuestionManager.h>
#include <zCapiO/QSFView.h>


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// have to have scope resolutors, else linker can't find these guys...

std::unique_ptr<BoxToolbar> CFormChildWnd::m_pBoxToolBar;
bool CFormChildWnd::m_bDrawBox = false;
std::optional<BoxType> CFormChildWnd::m_eBoxBtnSel;
std::optional<CPoint> CFormChildWnd::m_cBoxLocation;

constexpr int MIN_QUESTSZ = 60;
bool bInterfaceRequest =false;

/////////////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNCREATE(CFormChildWnd, ApplicationChildWnd)


BEGIN_MESSAGE_MAP(CFormChildWnd, ApplicationChildWnd)
    ON_WM_SYSCOMMAND()
    ON_WM_MDIACTIVATE()
    ON_COMMAND(ID_VIEW_LOGIC, OnViewLogic)
    ON_COMMAND(ID_VIEW_FORM, OnViewForm)
    ON_COMMAND(ID_SELECT_ITEMS, OnSelectItems)
    ON_UPDATE_COMMAND_UI(ID_SELECT_ITEMS, OnUpdateSelectItems)
    ON_COMMAND_RANGE(ID_SELITEMS_FROMTB, ID_DRAW_THICKBOX, OnBoxToolbar)
    ON_UPDATE_COMMAND_UI_RANGE(ID_SELITEMS_FROMTB, ID_DRAW_THICKBOX, OnUpdateBoxToolbar)
    ON_UPDATE_COMMAND_UI(ID_VIEW_FORM, OnUpdateViewForm)
    ON_UPDATE_COMMAND_UI(ID_VIEW_LOGIC, OnUpdateViewLogic)
    ON_UPDATE_COMMAND_UI(ID_GENERATE_FRM, OnUpdateGenerateForm)
    ON_COMMAND(ID_GO, OnGo)
    ON_COMMAND(ID_EDIT_OPTIONS, OnEditOptions)
    ON_COMMAND(ID_OPTIONS_DATA_SOURCES, OnOptionsDataSources)
    ON_UPDATE_COMMAND_UI(ID_OPTIONS_DATA_SOURCES, OnUpdateIfApplicationIsAvailable)
    ON_COMMAND(ID_OPTIONS_SYNCHRONIZATION, OnOptionsSynchronization)
    ON_UPDATE_COMMAND_UI(ID_OPTIONS_SYNCHRONIZATION, OnUpdateIfApplicationIsAvailable)
    ON_COMMAND(ID_OPTIONS_MAPPING, OnOptionsMapping)
    ON_UPDATE_COMMAND_UI(ID_OPTIONS_MAPPING, OnUpdateIfApplicationIsAvailable)
    ON_COMMAND(ID_COMPILE, OnCompile)
    ON_UPDATE_COMMAND_UI(ID_COMPILE, OnUpdateCompile)
    ON_COMMAND(ID_EDIT_DRAGOPTS, OnEditDragOptions)
    ON_COMMAND(ID_OPTIONS_FIELD_PROPERTIES, OnOptionsFieldProperties)
    ON_WM_DESTROY()
    ON_WM_SIZE()
    ON_COMMAND(ID_QSF_EDITOR, OnQsfEditor)
    ON_UPDATE_COMMAND_UI(ID_QSF_EDITOR, OnUpdateQsfEditor)
    ON_WM_CLOSE()
    ON_COMMAND(ID_ADDCAPI_LANG, OnAddcapiLang)
    ON_UPDATE_COMMAND_UI(ID_ADDCAPI_LANG, OnUpdateIfUsingQuestionText)
    ON_COMMAND(ID_CAPI_MACROS, OnCapiMacros)
    ON_UPDATE_COMMAND_UI(ID_CAPI_MACROS, OnUpdateIfUsingQuestionText)
    ON_COMMAND(ID_RUNAS_BCH, OnRunasBch)
    ON_UPDATE_COMMAND_UI(ID_RUNAS_BCH, OnUpdateRunasBch)
    ON_COMMAND(ID_FILE_GENERATEBINARY, OnGenerateBinary)
    ON_COMMAND(ID_FILE_PUBLISHANDDEPLOY, OnPublishAndDeploy)
    ON_COMMAND(ID_EDIT_CAPI_STYLES, OnEditStyles)
    ON_UPDATE_COMMAND_UI(ID_EDIT_CAPI_STYLES, OnUpdateIfUsingQuestionText)
    ON_MESSAGE(UWM::Designer::SwitchView, OnSwitchView)
    ON_COMMAND(ID_VIEW_QUESTIONNAIRE, &CFormChildWnd::OnViewQuestionnaire)
    ON_UPDATE_COMMAND_UI(ID_VIEW_QUESTIONNAIRE, &CFormChildWnd::OnUpdateViewQuestionnaire)
END_MESSAGE_MAP()


CFormChildWnd::CFormChildWnd()
    :   m_pFormView(nullptr),
        m_iQuestPaneSz(MIN_QUESTSZ),
        m_iQsfVSz1(0),
        m_iQsfVSz2(0),
        m_bMultiLangMode(false),
        m_bHideSecondLang(false),
        m_pSourceEditView(nullptr),
        m_pQSFEditView1(nullptr),
        m_pQSFEditView2(nullptr),
        m_pQuestionnaireView(nullptr),
        m_bUseQuestionText(false),
        m_eViewMode(FormViewMode),
        m_bAppAssociated(true),
        m_bFirstTime(true)
{
}

CFormChildWnd::~CFormChildWnd()
{
}


/////////////////////////////////////////////////////////////////////////////
// CFormChildWnd message handlers

BOOL CFormChildWnd::PreCreateWindow(CREATESTRUCT& cs)
{
    // TODO: Add your specialized code here and/or call the base class
//  cs.style &= ~(WS_SYSMENU);
    return CMDIChildWnd::PreCreateWindow(cs);
}

void CFormChildWnd::ActivateFrame(int nCmdShow)
{
    CMDIChildWnd* pWnd = ((CMDIFrameWnd*)AfxGetMainWnd())->MDIGetActive();

    if(pWnd && pWnd->IsKindOf(RUNTIME_CLASS(CDictChildWnd)))
    {
        CDDDoc* pDoc = (CDDDoc*)pWnd->GetActiveDocument();

        if(!pDoc->IsDocOK())
            return;
    }
    nCmdShow = SW_SHOWMAXIMIZED;

    CMDIChildWnd::ActivateFrame(nCmdShow);
    m_wndFSplitter.RecalcLayout();

    AfxGetMainWnd()->SendMessage(UWM::Designer::ShowToolbar, (WPARAM)FrameType::Form);
    AfxGetMainWnd()->SendMessage(WM_IMSA_SET_STATUSBAR_PANE, NULL);
    InvalidateRect(NULL);

    if(m_logicDlgBar.GetSafeHwnd() && m_logicDlgBar.IsWindowVisible()) {
        m_logicDlgBar.Invalidate();
        m_logicDlgBar.UpdateWindow();
    }

    //To Make the tree selection in sync with the correct document
    CFormDoc* pDoc = (CFormDoc*) GetActiveDocument();
    if(pDoc){
        CFormTreeCtrl* pFormTree= pDoc->GetFormTreeCtrl();
        if(!pFormTree) return;
        HTREEITEM hItem = pFormTree->GetSelectedItem();
        if(hItem) {
            pFormTree->GetSelectedItem();
            CFormID* pID = (CFormID*)pFormTree->GetItemData(hItem);
            if(pID->GetFormDoc()  != pDoc) {
                //Then we need to go in sync
                CFormNodeID* pNodeID = pFormTree->GetFormNode(pDoc);
                if(pNodeID && pNodeID->GetHItem()){
                    pFormTree->Select(pNodeID->GetHItem(),TVGN_CARET);
                }
            }

        }
    }

    if( pDoc != nullptr )
    {
        Application* pApplication = nullptr;
        AfxGetMainWnd()->SendMessage(UWM::Designer::GetApplication, (WPARAM)&pApplication, (LPARAM)pDoc);

        m_bAppAssociated = ( pApplication != nullptr );
        m_bUseQuestionText = m_bAppAssociated && pApplication->GetUseQuestionText();

        GetHeightSettings();
    }

    DisplayActiveMode();
}

// ********************************************************************************

BOOL CFormChildWnd::Create(LPCTSTR lpszClassName, LPCTSTR lpszWindowName, DWORD dwStyle, const RECT& rect, CMDIFrameWnd* pParentWnd, CCreateContext* pContext)
{
    if( !CMDIChildWnd::Create(lpszClassName, lpszWindowName, dwStyle, rect, pParentWnd, pContext) )
        return FALSE;

    // Get the window's menu
    CMenu* pMenu = GetSystemMenu (false);
    VERIFY (pMenu->GetSafeHmenu());

    // Disable the 'X'
    VERIFY(::DeleteMenu(pMenu->GetSafeHmenu(), SC_CLOSE , MF_BYCOMMAND));
    VERIFY(::DeleteMenu(pMenu->GetSafeHmenu(), 5 , MF_BYPOSITION));

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

    return TRUE;
}

// ********************************************************************************

void CFormChildWnd::OnSysCommand( UINT nID, LPARAM lParam )
{
    if(nID == SC_CLOSE)
        return;

    else
        CMDIChildWnd::OnSysCommand(nID,lParam);
}

// ********************************************************************************
void CFormChildWnd::OnMDIActivate(BOOL bActivate, CWnd* pActivateWnd, CWnd* pDeactivateWnd)
{
    bool bReconcile = true;

    CWnd* pWnd = AfxGetMainWnd()->GetDescendantWindow(ID_FORM_TOOLBAR);
    if(!pWnd || !pWnd->IsKindOf(RUNTIME_CLASS(CToolBar))){
        CMDIChildWnd::OnMDIActivate(bActivate, pActivateWnd, pDeactivateWnd);
        return;
    }
    CToolBarCtrl& pTBC = ((CToolBar*)pWnd)->GetToolBarCtrl();
    BOOL bShowLogic = pTBC.IsButtonChecked(ID_VIEW_LOGIC);

    if(bActivate && pActivateWnd && pActivateWnd->IsKindOf(RUNTIME_CLASS(CFormChildWnd))){
        // when activating the frame, if the toolbar for drawing boxes was displayed
        // the last go-around, put it back up!
        //To Make the tree selection in sync with the correct document
        CFormChildWnd* pFormWnd = (CFormChildWnd*)pActivateWnd;
        CFormDoc* pDoc = (CFormDoc*) pFormWnd->GetActiveDocument();
        //This is to fix the grid crash bug after reconcile (trevor's Bug)
        if(pDoc){
            CFormScrollView* pFV = (CFormScrollView*)pDoc->GetView();

            if (pFV != NULL)    {
                pFV->RemoveAllGrids(); //for the reconcile to work
                CDEFormFile* pFF = &pDoc->GetFormFile();
                CFormTreeCtrl* pFormTree= pDoc->GetFormTreeCtrl();
                if(!pFormTree) {
                    CMDIChildWnd::OnMDIActivate(bActivate, pActivateWnd, pDeactivateWnd);
                    return;
                }
                CFormID* pID = pFormTree->GetFormNode(pDoc);
                //SelectItem will fire CFormTreeCtrl::OnSelchanged
                //CFormTreeCtrl::OnSelchanged will fire pFrame->ActivateFrame
                //that fires OnMDIActivate !
                //==> reentrant call !

                pFormTree->SelectItem(pID->GetHItem());
                if (pFF->GetDictionary() != nullptr){  // ptrs not initialized yet, bail
                    int iCurForm = pFV->GetFormIndex();
                    CString csErr;
                    bReconcile = false;
                    pFF->UpdatePointers();//you are getting stray pointers before reconcile
                    pFormTree->SetRedraw(FALSE);     // BMD 09 Sep 2004
                    bool b = pFF->Reconcile(csErr, false, true);   // not silent, yes autofix
                    if (!b) {//rebuild the tree if reconcile has changed the formfile
                        pFormTree->ReBuildTree();//(pDF_Group->GetFormNum(), pDF_Group);
                    }
                    pFormTree->SetRedraw(TRUE);      // BMD 09 Sep 2004
                    if (!b){        // if a change (i.e., deletion!) was made in the form file, redo some stuff

                    //  if (iCurForm == 0 &&        // i.e., user was viewing the first form in the file
                    //      pFV->GetNumGrids()) {   // and there's some grids on it
                        if(true){
                            pFV->RemoveAllGrids();         // trash any grids that were created in this view
                            pFV->RemoveAllTrackers();

                            pFV->RecreateGrids (iCurForm);  // and recreate the ones needed for this view
                            pFV->SetPointers (iCurForm);
                            pFV->Invalidate();  // then i need to refresh the view (for grids to work)
                            pFV->SendMessage(WM_PAINT);
                        }
                        pDoc->SetModifiedFlag();
                        CFormTreeCtrl* pTree = pDoc->GetFormTreeCtrl();
                        pTree->SetSndMsgFlg(FALSE);
                        pTree->ReBuildTree();
                        pTree->SetSndMsgFlg(TRUE);
                        pDoc->GetUndoStack().Clear();
                    }
                    else{
                        pFV->RecreateGrids (pFV->GetFormIndex());
                        pFV->SetPointers (pFV->GetFormIndex());
                        pFV->Invalidate();
                        pFV->SendMessage(WM_PAINT);
                    }
                    pFV->PostMessage(UWM::Dictionary::MarkUsedItems);
                }
            }
        }
        CMDIChildWnd::OnMDIActivate(bActivate, pActivateWnd, pDeactivateWnd);
        if(pDoc){

            CFormTreeCtrl* pFormTree= pDoc->GetFormTreeCtrl();
            if(pFormTree) {
                HTREEITEM hItem = pFormTree->GetSelectedItem();
                if(hItem) {
                    pFormTree->GetSelectedItem();
                    CFormID* pID = (CFormID*)pFormTree->GetItemData(hItem);
                    if(pID->GetFormDoc()  != pDoc) {
                        //Then we need to go in sync
                        CFormNodeID* pNodeID = pFormTree->GetFormNode(pDoc);
                        if(pNodeID && pNodeID->GetHItem()){
                            pFormTree->Select(pNodeID->GetHItem(),TVGN_CARET);
                        }
                    }

                }
            }
            if(bShowLogic && !this->IsLogicViewActive()) {
                this->OnViewLogic();
            }
            if(!bShowLogic && this->IsLogicViewActive()) {
                this->OnViewForm();
            }

        }

        if (CanUserDrawBox())
        {
            if (m_pBoxToolBar->GetSafeHwnd() == NULL)
            {
                SetupBoxToolBar();
            }

            else // this block shld really never be executed...
            {
                m_pBoxToolBar->SetFormChildWnd(this);
                ShowControlBar(m_pBoxToolBar.get(), true, false);
            }
        }

        AfxGetMainWnd()->SendMessage(UWM::Designer::ShowToolbar, (WPARAM)FrameType::Form);
        AfxGetMainWnd()->SendMessage(UWM::Designer::SelectTab, (WPARAM)FrameType::Form);
    }

    if(!bActivate && pDeactivateWnd && pDeactivateWnd->IsKindOf(RUNTIME_CLASS(CFormChildWnd))){
        // likewise, when deactivating the frame, if the toolbar for box drawing is
        // being displayed, trash it (it will get reinstated next time around)
        CMDIChildWnd::OnMDIActivate(bActivate, pActivateWnd, pDeactivateWnd);

        if (CanUserDrawBox())
        {
            if (m_pBoxToolBar->GetSafeHwnd() != NULL)
                DeleteBoxToolBar();
        }

        if(IsLogicViewActive()){
            CFormDoc* pDoc = (CFormDoc*) GetActiveDocument();
            CFormTreeCtrl* pFormTree= pDoc->GetFormTreeCtrl();
            if(pFormTree) {
                HTREEITEM hItem = pFormTree->GetSelectedItem();
                if(hItem) {
                    CFormID* pID = (CFormID*)pFormTree->GetItemData(hItem);
                    BOOL bSendMsg = pFormTree->GetSndMsgFlg();
                    if(pID && pID->GetFormDoc() == pDoc && bSendMsg)
                    {
                        if(GetSourceView()->GetEditCtrl()->IsModified())
                            AfxGetMainWnd()->SendMessage(UWM::Form::PutSourceCode, 0, (LPARAM)pID);
                    }
                }
            }
        }

        CFormDoc* pFormDoc = (CFormDoc*) GetActiveDocument();
        if(pFormDoc){
            CQSFCndView* pView =  (CQSFCndView*)pFormDoc->GetView(FormViewType::Condition);
            if(pView && pView->m_gridCond.IsEditing()){
                pView->m_gridCond.EditChange(VK_ESCAPE,true);
            }
        }

        if(!pActivateWnd || !pActivateWnd->IsKindOf(RUNTIME_CLASS(CFormChildWnd)))
            AfxGetMainWnd()->SendMessage(UWM::Designer::HideToolbar);
    }

    if (bActivate && bReconcile)    // fix up the DDTreeCtrl's image list to reflect who's been used on the form!
    {
        CFormDoc* pDoc = (CFormDoc*) GetActiveDocument();
        if(!pDoc){
            return;
        }
        CFormScrollView* pFV = (CFormScrollView*)pDoc->GetView();

        if (pFV == NULL)    {
            SetRedraw(TRUE);
            return;         // ok to not send the msg below?
        }

        CDEFormFile* pFF = &pDoc->GetFormFile();

        if (pFF->GetDictionary() == nullptr){  // ptrs not initialized yet, bail
            SetRedraw(TRUE);
            return;
        }

        CString csErr;

        bool b = pFF->Reconcile(csErr, false, true);   // not silent, yes autofix

        if (!b)     // if a change (i.e., deletion!) was made in the form file, redo some stuff
        {
            pDoc->SetModifiedFlag();

            CFormTreeCtrl* pTree = pDoc->GetFormTreeCtrl();
            pTree->SetSndMsgFlg(FALSE);

            pTree->SelectItem (pTree->GetRootItem());
            pTree->ReBuildTree();

            pTree->SetSndMsgFlg(TRUE);

            pDoc->GetUndoStack().Clear();
        }

        pFV->PostMessage(UWM::Dictionary::MarkUsedItems);
    }
    if (bActivate)
    {
        AfxGetMainWnd()->SendMessage(WM_IMSA_SET_STATUSBAR_PANE, NULL);
        ActivateFrame(SW_SHOW); //Fixed the initialization bug related to the capi app Ctl+Tab window change Bug#12
    }

    SetRedraw(TRUE);
}

void CFormChildWnd::DeleteBoxToolBar()
{
    m_pBoxToolBar.reset();

}

// ********************************************************************************

bool CFormChildWnd::SetupBoxToolBar()
{
    ASSERT(m_pBoxToolBar->GetSafeHwnd() == NULL);

    auto pBTB = std::make_unique<BoxToolbar>(this);

    if (!pBTB->Create(this) || !pBTB->LoadToolBar(IDR_BOXTOOLBAR))
    {
        TRACE0 ("Failed to create draw box toolbar\n");
        return false;
    }

    pBTB->SetBarStyle(pBTB->GetBarStyle() | CBRS_TOOLTIPS | CBRS_FLYBY);

    pBTB->EnableDocking(0);    // 0 used if i want it to float

    if( !m_cBoxLocation.has_value() )
    {
        CRect rect;
        GetClientRect(&rect);

        m_cBoxLocation.emplace(rect.Width() / 2, rect.Height() / 2);
    }

    FloatControlBar(pBTB.get(), *m_cBoxLocation);     // use this to float it

    m_pBoxToolBar = std::move(pBTB);

    if( !m_eBoxBtnSel.has_value() )
        m_eBoxBtnSel = BoxType::Etched;  // default to etched, only because first in line...

    return true;
}

// ********************************************************************************
// when the user clicks my toolbar's Tracker btn, tell the sys we want to select stuff

void CFormChildWnd::OnSelectItems()
{
    if (m_bDrawBox)     // if true, then the box toolbar is up and the user wishes to close it
    {
        CanUserDrawBox(false);

        if (m_pBoxToolBar != nullptr) // this should be true...
        {
            DeleteBoxToolBar();
        }
    }
    else    // the user wants to draw some boxes, put up the toolbar
    {
        CanUserDrawBox(true);

        SetupBoxToolBar();
    }
}

// this is called from the view

void CFormChildWnd::ShowBoxToolBar()
{
    if (m_bDrawBox)     // if true, then the box toolbar is up and the user wishes to close it
    {
        CanUserDrawBox(false);

        if (m_pBoxToolBar != nullptr) // this should be true...
            DeleteBoxToolBar();
    }

    else    // the user wants to draw some boxes, put up the toolbar
    {
        CanUserDrawBox(true);
        SetupBoxToolBar();
    }
}


void CFormChildWnd::OnUpdateSelectItems (CCmdUI* pCmdUI)
{
    CWnd* pWnd = GetActiveView();
    pWnd&& pWnd->IsKindOf(RUNTIME_CLASS(CFormScrollView)) ? pCmdUI->Enable(true) : pCmdUI->Enable(false);
    pCmdUI->SetCheck (CanUserDrawBox());

    if (!m_bDrawBox)
    {
        if (m_pBoxToolBar->GetSafeHwnd() != NULL)
        {
            m_pBoxToolBar->DestroyWindow();
            DeleteBoxToolBar();
        }
    }
}

// ****************************************************************
// the next blk of 6 funcs are used with my Draw Box toolbar; depending
// on which one is called, unpress the curr btn and depress the one
// selected; then, change the drawing style to the box requested

namespace
{
    constexpr std::optional<BoxType> IdToBoxType(UINT nID)
    {
        if( nID == ID_DRAW_ETCHED )   return BoxType::Etched;
        if( nID == ID_DRAW_RAISED )   return BoxType::Raised;
        if( nID == ID_DRAW_THINBOX )  return BoxType::Thin;
        if( nID == ID_DRAW_THICKBOX ) return BoxType::Thick;

        return std::nullopt;
    }
}

void CFormChildWnd::OnBoxToolbar(UINT nID)
{
    m_eBoxBtnSel = IdToBoxType(nID);

    CToolBarCtrl& pTBC = m_pBoxToolBar->GetToolBarCtrl();

    for( UINT button_id : { ID_SELITEMS_FROMTB, ID_DRAW_ETCHED, ID_DRAW_RAISED, ID_DRAW_THINBOX, ID_DRAW_THICKBOX } )
        pTBC.CheckButton(button_id, ( button_id == nID ));
}

void CFormChildWnd::OnUpdateBoxToolbar(CCmdUI* pCmdUI)
{
    CWnd* pWnd = GetActiveView();
    pWnd && pWnd->IsKindOf(RUNTIME_CLASS(CFormScrollView)) ? pCmdUI->Enable(true) : pCmdUI->Enable(false);
    pCmdUI->SetCheck(( m_eBoxBtnSel == IdToBoxType(pCmdUI->m_nID) ));
}


// *************************************************************************************

CToolBar* CFormChildWnd::CreateFormToolBar(CWnd* pParentWnd)
{
    // create the form toolbar (that is controlled by the designer)
    CToolBar* pWndFormTBar = new CToolBar;

    if( !pWndFormTBar->CreateEx(pParentWnd, TBSTYLE_FLAT, WS_CHILD | WS_VISIBLE | CBRS_ALIGN_TOP, CRect(), ID_FORM_TOOLBAR) ||
        !pWndFormTBar->LoadToolBar(IDR_FORM_FRAME) )
    {
        SAFE_DELETE(pWndFormTBar);
    }

    else
    {
        auto& tool_bar_ctrl = pWndFormTBar->GetToolBarCtrl();

        tool_bar_ctrl.HideButton(ID_EDIT_CAPI_STYLES);
        tool_bar_ctrl.HideButton(ID_FORMAT_OUTLINE_BULLET);
        tool_bar_ctrl.HideButton(ID_FORMAT_OUTLINE_NUMBER);
        tool_bar_ctrl.HideButton(ID_EDIT_INSERT_IMAGE);
        tool_bar_ctrl.HideButton(ID_TOGGLE_QN);
        tool_bar_ctrl.HideButton(ID_VVIEW_LOGIC);
        tool_bar_ctrl.HideButton(ID_VQSF_EDITOR);
    }

    return pWndFormTBar;
}


bool CFormChildWnd::IsLogicViewActive()
{
    if (! IsFormFrameActive())  // who's in the view on the right, me or dict or ???

        return false;

    if (m_pSourceEditView == NULL)

        return false;
    else
        return true;
}

void CFormChildWnd::OnUpdateGenerateForm(CCmdUI* pCmdUI)
{
    if (GetViewMode() != FormViewMode) {
        pCmdUI->Enable(FALSE);
    }
    else {
        pCmdUI->Enable(TRUE);
    }
}


// if the user is currently viewing the logic, then put a check mark next to the entry

void CFormChildWnd::OnUpdateViewLogic(CCmdUI* pCmdUI)
{
    if(!m_bAppAssociated){
        pCmdUI->Enable(FALSE);
        return;
    }

    pCmdUI->Enable(TRUE);
    if(m_eViewMode == LogicViewMode){
        pCmdUI->SetCheck(TRUE);
    }
    else {
        pCmdUI->SetCheck(FALSE);
    }

}

// *************************************************************************************
// there are three places the user can invoke this func:

// [1] from the tree ctrl's pop up menu
// [2] from the form view's pop up menu
// [3] from the menubar's view option

// in 1&2, if the logic view is already up, the popup menu won't provide an option
// to select the logic view; only from the menubar can they get to this point at any
// time, since i'm now showing a check box next to the View Logic, rather than deactivating
// as before

void CFormChildWnd::OnViewLogic()
{
    if(m_eViewMode == LogicViewMode){
        DisplayActiveMode();
        return;
    }
    // in the event the form tree ctrl is up, but somebody else is in the view, do this
    if (!IsFormFrameActive())
    {
        CMDIChildWnd *pMe = (CMDIChildWnd *) GetActiveFrame();

        GetMDIFrame()->MDIActivate(pMe);
    }

    if(m_pSourceEditView)   // then user trying to toggle off logic view via the menubar
    {
        m_eViewMode = LogicViewMode;
    }

//  see if it's already active
     CFormDoc* pDoc = (CFormDoc*) GetActiveDocument();

     CFormScrollView *pFScrollView = NULL;

     if(pDoc)
     {
         POSITION pos = pDoc->GetFirstViewPosition();

         while (pos)
         {
             CView* pView = pDoc->GetNextView(pos);

             if(pView->IsKindOf(RUNTIME_CLASS(CFormScrollView)))
             {
                pFScrollView = (CFormScrollView*)pView;
                if(pFScrollView->GetCurItem()) {
                    pFScrollView->SetCurTreeItem ();    // selects the cur tracker items in the tree (if any)
                }
                else {
                    CFormTreeCtrl*  pFormTree = pFScrollView->GetDocument()->GetFormTreeCtrl();
                    HTREEITEM hItem = pFormTree->GetSelectedItem();
                    bool bSel =true;
                    if(hItem) {
                        CFormNodeID* pCurID = (CFormNodeID*)pFormTree->GetItemData(hItem);
                        if(pCurID && pCurID->GetFormDoc() == pFScrollView->GetDocument()){
                            bSel = false;
                        }
                    }
                    CFormNodeID* pID = pFormTree->GetFormNode(pFScrollView->GetDocument());
                    ASSERT(pID);
                    if(bSel){
                        pFormTree->Select(pID->GetHItem(),TVGN_CARET);
                    }
                }
                //if the g

                break;
             }
         }
     }
        //Set the source view as the active pane

    if (m_pSourceEditView == NULL)
    {

        m_pSourceEditView =  new CFSourceEditView();
        CCreateContext context =  CCreateContext();
        context.m_pCurrentDoc = pDoc;
        context.m_pCurrentFrame=this;

        CRect rect(0,0,100,100);
        if(pFScrollView)
            pFScrollView->GetClientRect(&rect);

        int iID = m_wndFSplitter.IdFromRowCol(1,0);
        CWnd* pWnd = NULL;
        if(true){
            pWnd = m_wndFSplitter.GetPane(1,0);
            if(pWnd ){
            pWnd->ShowWindow(SW_HIDE);
            pWnd->SetDlgCtrlID(2223);
        }
        }

        DWORD dwStyle = AFX_WS_DEFAULT_VIEW;
        dwStyle &= ~WS_BORDER;

        if (!m_pSourceEditView->Create(NULL,_T(""),dwStyle,rect,&m_wndFSplitter,iID,&context))
        {
            TRACE0("Failed to create view\n");
            ASSERT (false);
            return ;
        }

        // WM_INITIALUPDATE is define in AFXPRIV.H.
        m_pSourceEditView->SendMessage(WM_INITIALUPDATE, 0, 0);

        //To work with large files
        //m_pSourceEditView->GetEditCtrl()->LimitText(0xffffff);
    //    m_pSourceEditView->ShowWindow(SW_HIDE);

    }
    else {
        CWnd* pWnd = m_wndFSplitter.GetPane(1,0);
        if(pWnd && !pWnd->IsKindOf(RUNTIME_CLASS(CFSourceEditView))){
            pWnd->ShowWindow(SW_HIDE);
            int iTempID  = m_pSourceEditView->GetDlgCtrlID();
            int iID = m_wndFSplitter.IdFromRowCol(1,0);
            pWnd->SetDlgCtrlID(iTempID);
            m_pSourceEditView->SetDlgCtrlID(iID);
        }

    }
    if(AfxGetMainWnd()->SendMessage(UWM::Form::ShowSourceCode, 0, (LPARAM)pDoc) != 0){
        this->OnViewForm();
        return ;
    }

    m_eViewMode = LogicViewMode;
    if(pFScrollView) {
        pFScrollView->ShowWindow(SW_HIDE);
    }
    m_pSourceEditView->ShowWindow(SW_SHOW);
    m_pSourceEditView->GetEditCtrl()->EmptyUndoBuffer();
    SetActiveView(m_pSourceEditView);

    this->m_wndFSplitter.SetActivePane(1,0);
    m_logicDlgBar.ShowWindow(SW_SHOW);

    DisplayActiveMode();

    AfxGetMainWnd()->SendMessage(UWM::Designer::ShowToolbar, (WPARAM)FrameType::Form);
    AfxGetMainWnd()->SendMessage(UWM::Designer::SelectTab, (WPARAM)FrameType::Form);

    CWnd* pWnd = AfxGetMainWnd()->GetDescendantWindow(ID_FORM_TOOLBAR);
    if(!pWnd || !pWnd->IsKindOf(RUNTIME_CLASS(CToolBar)))
        return;

    AfxGetMainWnd()->SendMessage(WM_IMSA_SET_STATUSBAR_PANE, NULL);
    m_pSourceEditView->GetEditCtrl()->SetFocus();
}

// *************************************************************************************

bool CFormChildWnd::IsFormFrameActive()
{
    CMDIChildWnd *pCurChild = (CMDIChildWnd *) MDIGetActive();

    CMDIChildWnd *pMe = (CMDIChildWnd *) GetActiveFrame();

    return  (pCurChild == pMe);
}

// *************************************************************************************

bool CFormChildWnd::IsFormViewActive()
{
    if (! IsFormFrameActive())  // who's in the view on the right, me or dict or ???

        return false;

    if (m_pSourceEditView == NULL)

        return true;
    else
        return false;
/*
    CView* pActiveView = GetActiveView();

    if (! pActiveView->IsKindOf (RUNTIME_CLASS (CFormScrollView)) )

        return false;

    return pActiveView->ShowWindow (SW_SHOWNA);
*/
}

// if the user is currently viewing the form, then no need to
// make this menu item avail for selection!

void CFormChildWnd::OnUpdateViewForm(CCmdUI* pCmdUI)
{
    pCmdUI->Enable(TRUE);
    if(m_eViewMode == FormViewMode){
        pCmdUI->SetCheck(TRUE);
    }
    else {
        pCmdUI->SetCheck(FALSE);
    }

}

// *************************************************************************************

void CFormChildWnd::OnViewForm()
{
    // in the event the form tree ctrl is up, but somebody else is in the view, do this
    // in the event the form tree ctrl is up, but somebody else is in the view, do this
    CFormDoc* pDoc = (CFormDoc*) GetActiveDocument();
    if(m_eViewMode == FormViewMode){
        DisplayActiveMode();
        return;
    }
    if (!IsFormFrameActive())
    {
        CMDIChildWnd *pMe = (CMDIChildWnd *) GetActiveFrame();

        GetMDIFrame()->MDIActivate(pMe);
    }

//  see if it's already active
    if(!m_pSourceEditView){
        m_eViewMode = FormViewMode;
        DisplayActiveMode();
        if(m_bUseQuestionText && pDoc->GetFormTreeCtrl()->m_bSendMsg) {
            AfxGetMainWnd()->SendMessage(UWM::Form::ShowCapiText, reinterpret_cast<WPARAM>(pDoc));
        }
        AfxGetMainWnd()->SendMessage(UWM::Designer::ShowToolbar, (WPARAM)FrameType::Form);
        return;
    }

    CView* pView = GetActiveView();

    if (pDoc && pView->IsKindOf(RUNTIME_CLASS(CFSourceEditView)))
    {
        HTREEITEM hItem = pDoc->GetFormTreeCtrl()->GetSelectedItem();
        CFormID* pID = (CFormID*)pDoc->GetFormTreeCtrl()->GetItemData(hItem);
        BOOL bSendMsg = pDoc->GetFormTreeCtrl()->GetSndMsgFlg();
        if(pID && pID->GetFormDoc() == pDoc && bSendMsg)
        {
            if(AfxGetMainWnd()->SendMessage(UWM::Form::PutSourceCode, 0, (LPARAM)pID) == -1)
                return;

        }//if there is an error in the logic view do not switch to the form view
    }


    CFormScrollView *pFScrollView = NULL;

     if(pDoc)
     {
         pFScrollView = (CFormScrollView*) pDoc->GetView();
     }


    ASSERT(pFScrollView);

    int iID = m_wndFSplitter.IdFromRowCol(1,0);
    if(true){
        CWnd* pWnd = m_wndFSplitter.GetPane(1,0);
        if(pWnd && !pWnd->IsKindOf(RUNTIME_CLASS(CFormScrollView))){
            pWnd->ShowWindow(SW_HIDE);
            int iTempID  = pFScrollView->GetDlgCtrlID();
            pWnd->SetDlgCtrlID(iTempID);
        }
    }

    if(m_pSourceEditView) {
        CFormDoc* pDoc2 = (CFormDoc*)m_pSourceEditView->GetDocument();
        m_pSourceEditView->ShowWindow(SW_HIDE);
        pFScrollView->SetDlgCtrlID(iID);
        m_pSourceEditView->GetEditCtrl()->ShowWindow(SW_HIDE);
        m_pSourceEditView->DestroyWindow();
        if(pDoc2) {
            SetActiveView(pDoc2->GetView());
        }
        //Do not call delete explicitly for a View . It is done automatically
        //on PostNcDestroy
        m_pSourceEditView = NULL;
        m_logicDlgBar.ShowWindow(SW_HIDE);
        m_logicReferenceWnd.ShowWindow(SW_HIDE);
    }

    m_eViewMode = FormViewMode;
    pFScrollView->ShowWindow(SW_SHOW);
    pFScrollView->SetDlgCtrlID(iID);
    SetActiveView(pFScrollView);
    this->m_wndFSplitter.SetActivePane(1,0);

    DisplayActiveMode();
    AfxGetMainWnd()->SendMessage(UWM::Designer::ShowToolbar, (WPARAM)FrameType::Form);

    CWnd* pWnd = AfxGetMainWnd()->GetDescendantWindow(ID_FORM_TOOLBAR);
    if(!pWnd || !pWnd->IsKindOf(RUNTIME_CLASS(CToolBar)))
        return;

    if(m_bUseQuestionText && pDoc->GetFormTreeCtrl()->m_bSendMsg)
        AfxGetMainWnd()->SendMessage(UWM::Form::ShowCapiText, reinterpret_cast<WPARAM>(pDoc));
}


CFSourceEditView* CFormChildWnd::GetSourceView()
{
    return m_pSourceEditView;
}


CLogicView* CFormChildWnd::GetSourceLogicView()
{
    return m_pSourceEditView;
}


CLogicCtrl* CFormChildWnd::GetSourceLogicCtrl()
{
    return ( m_pSourceEditView != nullptr ) ? m_pSourceEditView->GetLogicCtrl() :
                                              nullptr;
}


void CFormChildWnd::OnGo()
{
    CFormDoc* pFormDoc = PreRunPublish();
    AfxGetMainWnd()->SendMessage(UWM::Form::RunActiveApplication, 0, (LPARAM)pFormDoc);
}


void CFormChildWnd::OnEditOptions()
{
    CFormDoc* pDoc = assert_cast<CFormDoc*>(GetActiveDocument());
    CFFOptionsDlg optDlg(this, &pDoc->GetFormFile());

    Application* application = nullptr;
    WindowsDesktopMessage::Send(UWM::Designer::GetApplication, &application, pDoc);

#define GetFromApplication(method, default_value) ( ( application == nullptr ) ? ( default_value ) : application->method() )

    optDlg.m_bAskOpID = GetFromApplication(GetAskOperatorId,false);
    optDlg.m_bShowEndCaseMsg = GetFromApplication(GetShowEndCaseMessage,true);
    optDlg.m_bAllowPartialSave = GetFromApplication(GetPartialSave,false);
    optDlg.m_caseTreeType = GetFromApplication(GetCaseTreeType, CaseTreeType::MobileOnly);

    optDlg.m_bUseQuestionText = GetFromApplication(GetUseQuestionText,false);
    const bool initial_use_question_text = optDlg.m_bUseQuestionText;

    optDlg.m_bShowRefusals = GetFromApplication(GetShowRefusals,true);
    optDlg.m_bCenterForms = GetFromApplication(GetCenterForms,false);
    optDlg.m_bDecimalComma = GetFromApplication(GetDecimalMarkIsComma,false);
    optDlg.m_bAutoAdvanceOnSelection = GetFromApplication(GetAutoAdvanceOnSelection,false);
    optDlg.m_bDisplayCodesAlongsideLabels = GetFromApplication(GetDisplayCodesAlongsideLabels,false);
    optDlg.m_bShowFieldLabels = GetFromApplication(GetShowFieldLabels,true);
    optDlg.m_bShowErrorMessageNumbers = GetFromApplication(GetShowErrorMessageNumbers,true);
    optDlg.m_bComboBoxShowOnlyDiscreteValues = GetFromApplication(GetComboBoxShowOnlyDiscreteValues,false);

    optDlg.m_iVerifyFreq = GetFromApplication(GetVerifyFreq,1);
    optDlg.m_iVerifyStart = GetFromApplication(GetVerifyStart,1);

    if( optDlg.m_iVerifyStart == -1 )
    {
        optDlg.m_iVerifyStart = 1;
        optDlg.m_bRandom = 1;
    }

    if( optDlg.DoModal() != IDOK )
        return;

    pDoc->SetModifiedFlag(true);

#define SetFromApplication(method,value) { if( application != nullptr ) { application->method(value); } }

    SetFromApplication(SetAskOperatorId,optDlg.m_bAskOpID);
    SetFromApplication(SetShowEndCaseMessage,optDlg.m_bShowEndCaseMsg);
    SetFromApplication(SetPartialSave,optDlg.m_bAllowPartialSave);
    SetFromApplication(SetCaseTreeType, optDlg.m_caseTreeType);

    SetFromApplication(SetUseQuestionText, optDlg.m_bUseQuestionText);
    m_bUseQuestionText = optDlg.m_bUseQuestionText;

    SetFromApplication(SetShowRefusals,optDlg.m_bShowRefusals);
    SetFromApplication(SetCenterForms,optDlg.m_bCenterForms);
    SetFromApplication(SetDecimalMarkIsComma,optDlg.m_bDecimalComma);
    SetFromApplication(SetAutoAdvanceOnSelection,optDlg.m_bAutoAdvanceOnSelection);
    SetFromApplication(SetDisplayCodesAlongsideLabels,optDlg.m_bDisplayCodesAlongsideLabels);
    SetFromApplication(SetShowFieldLabels,optDlg.m_bShowFieldLabels);
    SetFromApplication(SetShowErrorMessageNumbers,optDlg.m_bShowErrorMessageNumbers);
    SetFromApplication(SetComboBoxShowOnlyDiscreteValues,optDlg.m_bComboBoxShowOnlyDiscreteValues);

    SetFromApplication(SetVerifyFreq,optDlg.m_iVerifyFreq);
    SetFromApplication(SetVerifyStart,optDlg.m_bRandom ? -1 : optDlg.m_iVerifyStart);

    CFormScrollView* pView = (CFormScrollView*)pDoc->GetView();
    pView->UpdateGridDecimalChar(optDlg.m_bDecimalComma ? _T(',') : _T('.'));

    if( pDoc->GetFormFile().GetRTLRostersFlag() != (optDlg.m_bRTLRosters != 0) )
    {
        pDoc->GetFormFile().SetRTLRostersFlag(optDlg.m_bRTLRosters);
        CString sErr;
        pDoc->GetFormFile().Reconcile(sErr,true,true); //this call will reconcile the roster flags.
        pView->RemoveAllGrids();
        pView->RecreateGrids(pView->GetFormIndex());
    }

    if( initial_use_question_text != m_bUseQuestionText )
    {
        pDoc->UpdateAllViews(nullptr, Hint::UseQuestionTextChanged);

        // transition from CAPI to non-CAPI
        if( m_eViewMode == QSFEditorViewMode )
            m_eViewMode = FormViewMode;
    }

    DisplayActiveMode();

    CRect rect;
    GetClientRect(&rect);
    COXMDIChildWndSizeDock::OnSize(SIZE_RESTORED, rect.Width(), rect.Height());
}


bool CFormChildWnd::ShowDragOptionsDlg()
{
    CFormDoc* pDoc = assert_cast<CFormDoc*>(GetActiveDocument());
    CFormScrollView* pSV = (CFormScrollView*)pDoc->GetView();

    if( pSV->IsKindOf(RUNTIME_CLASS(CFormScrollView)) )
    {
        DragOptionsDlg dlg(m_dragOptions, pSV);

        if( dlg.DoModal() == IDOK )
            return true;
    }

    return false;
}

void CFormChildWnd::OnEditDragOptions()
{
    ShowDragOptionsDlg();
}


// *************************************************************************************

void CFormChildWnd::OnCompile()
{
    CFormDoc* pDoc = (CFormDoc*) GetActiveDocument();
    CView* pView = GetActiveView();

    if (pDoc && pView->IsKindOf(RUNTIME_CLASS(CFSourceEditView)))
    {
        HTREEITEM hItem = pDoc->GetFormTreeCtrl()->GetSelectedItem();
        CFormID* pID = (CFormID*)pDoc->GetFormTreeCtrl()->GetItemData(hItem);

        if(pID && pID->GetFormDoc() == pDoc)
        {
            //wParam 1 implies force compile
            AfxGetMainWnd()->SendMessage(UWM::Form::PutSourceCode, (WPARAM)1, (LPARAM)pID);
            ((CFSourceEditView*)pView)->GetEditCtrl()->SetFocus();
        }
    }
}

void CFormChildWnd::OnUpdateCompile(CCmdUI* pCmdUI)
{
    CView* pView = GetActiveView();
    if(!pView)
        return ;

    if(!pView->IsKindOf(RUNTIME_CLASS(CFSourceEditView))) {
        pCmdUI->Enable(FALSE);
    }
    else {
        pCmdUI->Enable(TRUE);
    }
}


BOOL CFormChildWnd::OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext)
{
    UNREFERENCED_PARAMETER(lpcs);

    Application* pApplication = nullptr;
    AfxGetMainWnd()->SendMessage(UWM::Designer::GetApplication, (WPARAM)&pApplication, (LPARAM)pContext->m_pCurrentDoc);
    m_bUseQuestionText = ( pApplication == nullptr ) ? false : pApplication->GetUseQuestionText();

    // Create splitter window
    VERIFY(m_wndFSplitter.CreateStatic(this,3,1));

    bool bViewOK = m_wndFSplitter.CreateView(0,0,RUNTIME_CLASS(QSFView), CSize(120,120), pContext);
    if(!bViewOK){
        m_wndFSplitter.DestroyWindow();
        return FALSE;
    }

    bViewOK = m_wndFSplitter.CreateView(1,0,RUNTIME_CLASS(CFormScrollView), CSize(120,120), pContext);
    if(!bViewOK){
        m_wndFSplitter.DestroyWindow();
        return FALSE;
    }

    bViewOK = m_wndFSplitter.CreateView(2,0,RUNTIME_CLASS(CQSFCndView), CSize(120,120), pContext);

    if(!bViewOK){
        m_wndFSplitter.DestroyWindow();
        return FALSE;
    }


    m_wndFSplitter.SetRowInfo(0,0,1);
    m_wndFSplitter.SetRowInfo(2,0,1);
    m_wndFSplitter.SetActivePane(0,0);

    return TRUE;
}



void CFormChildWnd::OnDestroy()
{
    this->m_wndFSplitter.DestroyWindow();
    COXMDIChildWndSizeDock::OnDestroy();

    // TODO: Add your message handler code here

}
/////////////////////////////////////////////////////////////////////////////////
//
// void CFormChildWnd::DisplayNoQuestionTextMode(bool bFormView /*=true*/)
//
/////////////////////////////////////////////////////////////////////////////////
void CFormChildWnd::DisplayNoQuestionTextMode(bool bFormView /*=true*/)
{
    CFormDoc* pFormDoc = (CFormDoc*) GetActiveDocument();

    if(!pFormDoc)
        return;

    if(m_wndFSplitter.GetSafeHwnd() && m_wndFSplitter.IsSplitterReady()){
        if(bFormView) {
            CWnd* pWnd = m_wndFSplitter.GetPane(0,0);
            if(pWnd && !pWnd->IsKindOf(RUNTIME_CLASS(QSFView))){
                pWnd->ShowWindow(SW_HIDE);          //hide the old second pane view
                QSFView* pView = (QSFView*)pFormDoc->GetView(FormViewType::QuestionText);

                int iTempID = pView->GetDlgCtrlID();
                int iID = m_wndFSplitter.IdFromRowCol(0,0);

                //Set the controlId of the qsfview3 to this pane
                pView->SetDlgCtrlID(iID);

                pView->ShowWindow(SW_SHOW);
                pWnd->SetDlgCtrlID(iTempID);
            }
            m_wndFSplitter.SetRowInfo(0, 0,10);
            m_wndFSplitter.RecalcLayout();


            //Set the form view in pane 1
            CFormScrollView* pFormView = (CFormScrollView*)pFormDoc->GetView();
            pWnd = m_wndFSplitter.GetPane(1,0);
            if(pWnd && !pWnd->IsKindOf(RUNTIME_CLASS(CFormScrollView))){
                pWnd->ShowWindow(SW_HIDE);          //hide the old second pane view
                CFormScrollView* pFormView2 = (CFormScrollView*)pFormDoc->GetView();

                int iTempID = pFormView2->GetDlgCtrlID();
                int iID = m_wndFSplitter.IdFromRowCol(1,0);

                //Set the controlId of the qsfview3 to this pane
                pFormView2->SetDlgCtrlID(iID);
                pFormView2->ShowWindow(SW_SHOW);
                pWnd->SetDlgCtrlID(iTempID);
            }
            SetActiveView(pFormView);
        }
        else if(!bFormView){
            m_wndFSplitter.SetRowInfo(0, 0,10);
        }

        m_wndFSplitter.SetActivePane(1,0);
        CRect rect;
        GetClientRect(&rect);
        COXMDIChildWndSizeDock::OnSize(SIZE_RESTORED, rect.Width(), rect.Height());


        m_wndFSplitter.SetRowInfo(0, 0,10);
        m_wndFSplitter.SetRowInfo(2, 0,10);
        m_wndFSplitter.SetRowInfo(1, rect.Height(),10);
        m_wndFSplitter.RecalcLayout();

        GetWindowRect(&rect);
        RedrawWindow(rect);
    }
}

void CFormChildWnd::OnSize(UINT nType, int cx, int cy)
{
    CRect rect;
    GetClientRect( &rect );

    COXMDIChildWndSizeDock::OnSize(nType, cx, cy);

    if(m_wndFSplitter.IsSplitterReady()){
        /*this->m_wndFSplitter.RecalcLayout();*/
        DisplayActiveMode();
    }

}
/////////////////////////////////////////////////////////////////////////////////
//
// void CFormChildWnd::DisplayQuestionTextMode(bool bFormView /*=true*/)
//
/////////////////////////////////////////////////////////////////////////////////
void CFormChildWnd::DisplayQuestionTextMode(bool bFormView /*=true*/)
{
    CFormDoc* pFormDoc = (CFormDoc*) GetActiveDocument();
    if(!pFormDoc)
        return;

    if(m_wndFSplitter.GetSafeHwnd() && m_wndFSplitter.IsSplitterReady()){
        CRect rect;
        GetClientRect(&rect);
        //COXMDIChildWndSizeDock::OnSize(SIZE_RESTORED, rect.Width(), rect.Height());
        int iCur ,iMin;
        iCur=iMin=0;

        CFormScrollView* pFormView = (CFormScrollView*)pFormDoc->GetView();

        m_wndFSplitter.SetRowInfo(0, pFormView->GetCurForm()->GetQuestionTextHeight(), 10);
        m_wndFSplitter.RecalcLayout();

        if(bFormView) {
            CWnd* pWnd = m_wndFSplitter.GetPane(0,0);
            if(pWnd && !pWnd->IsKindOf(RUNTIME_CLASS(QSFView))){
                pWnd->ShowWindow(SW_HIDE);          //hide the old second pane view
                QSFView* pView = (QSFView*)pFormDoc->GetView(FormViewType::QuestionText);

                int iTempID = pView->GetDlgCtrlID();
                int iID = m_wndFSplitter.IdFromRowCol(0,0);

                //Set the controlId of the qsfview3 to this pane
                pView->SetDlgCtrlID(iID);

                pView->ShowWindow(SW_SHOW);
                pWnd->SetDlgCtrlID(iTempID);
            }
            else if(pWnd && !pWnd->IsWindowVisible()){
                pWnd->ShowWindow(SW_SHOW);
            }

            //Set the form view in pane 1
            pFormView = (CFormScrollView*)pFormDoc->GetView();
            pWnd = m_wndFSplitter.GetPane(1,0);
            if(pWnd && !pWnd->IsKindOf(RUNTIME_CLASS(CFormScrollView))){
                pWnd->ShowWindow(SW_HIDE);          //hide the old second pane view
                pFormView = (CFormScrollView*)pFormDoc->GetView();

                int iTempID = pFormView->GetDlgCtrlID();
                int iID = m_wndFSplitter.IdFromRowCol(1,0);

                //Set the controlId of the qsfview3 to this pane
                pFormView->SetDlgCtrlID(iID);
                pFormView->ShowWindow(SW_SHOW);
                pWnd->SetDlgCtrlID(iTempID);
            }
            SetActiveView(pFormView);
            m_wndFSplitter.SetRowInfo(2, 0, 10);
            m_wndFSplitter.GetRowInfo(0, m_iQuestPaneSz, iMin);
            m_wndFSplitter.SetRowInfo(1, rect.Height() - m_iQuestPaneSz, 10);
        }
        else if(!bFormView){
            m_wndFSplitter.SetRowInfo(0, 0, 10);
            m_wndFSplitter.SetRowInfo(2, 0, 10);
            m_wndFSplitter.SetRowInfo(1, rect.Height(), 10);
        }

        m_wndFSplitter.SetActivePane(1,0);

        m_wndFSplitter.RecalcLayout();

        GetClientRect(&rect);
        COXMDIChildWndSizeDock::OnSize(SIZE_RESTORED, rect.Width(), rect.Height());

        GetWindowRect(&rect);
        RedrawWindow(rect);
    }
}

void CFormChildWnd::DisplayQuestionnaireViewMode()
{
    CFormDoc* pFormDoc = (CFormDoc*)GetActiveDocument();

    if (!pFormDoc)
        return;
    if (m_wndFSplitter.GetSafeHwnd() && m_wndFSplitter.IsSplitterReady()) {
        //Set the QuestionnaireView
        CWnd* pWnd = m_wndFSplitter.GetPane(1, 0);
        if (pWnd && !pWnd->IsKindOf(RUNTIME_CLASS(QuestionnaireView))) {
            if (!m_pQuestionnaireView) {
                m_pQuestionnaireView = new QuestionnaireView(*pFormDoc);
                CCreateContext context = CCreateContext();
                context.m_pCurrentDoc = pFormDoc;

                CRect rect(0, 0, 100, 100);
                GetClientRect(&rect);

                int iID = m_wndFSplitter.IdFromRowCol(1, 0);
                pWnd->SetDlgCtrlID(2226);
                if (!m_pQuestionnaireView->Create(NULL, _T(""), WS_CHILD | WS_VISIBLE, rect, &m_wndFSplitter, iID, &context)) {
                    TRACE0("Failed to create questionnaire view\n");
                    ASSERT(false);
                    return;
                }
                GetActiveView()->ShowWindow(SW_HIDE);
                m_pQuestionnaireView->SendMessage(WM_INITIALUPDATE);
            }
            else {
                int iTempID = m_pQuestionnaireView->GetDlgCtrlID();
                int iID = m_wndFSplitter.IdFromRowCol(1, 0);
                //Set the controlId of the Questionnaire view to this pane
                m_pQuestionnaireView->SetDlgCtrlID(iID);
                GetActiveView()->ShowWindow(SW_HIDE);
                m_pQuestionnaireView->ShowWindow(SW_SHOW);
                pWnd->SetDlgCtrlID(iTempID);
                m_pQuestionnaireView->RefreshContent(false);
            }
        }
        m_logicDlgBar.ShowWindow(SW_HIDE);
        m_logicReferenceWnd.ShowWindow(SW_HIDE);

        CRect rect;
        GetClientRect(&rect);
        COXMDIChildWndSizeDock::OnSize(SIZE_RESTORED, rect.Width(), rect.Height());
        //reset the pane heights
        m_wndFSplitter.SetActivePane(1, 0);
        m_wndFSplitter.SetRowInfo(0, 0, 10);
        m_wndFSplitter.SetRowInfo(2, 0, 10);
        m_wndFSplitter.SetRowInfo(1, rect.Height(), 10);
        m_wndFSplitter.RecalcLayout();
        GetWindowRect(&rect);
        RedrawWindow(rect);
    }
}

/////////////////////////////////////////////////////////////////////////////////
//
//  void CFormChildWnd::DisplayEditorMode()
//
/////////////////////////////////////////////////////////////////////////////////
void CFormChildWnd::DisplayEditorMode()
{
    CFormDoc* pFormDoc = (CFormDoc*) GetActiveDocument();

    if(!pFormDoc)
        return;

    // check if there are multiple lanuages
    m_bMultiLangMode = false;

	Application* pApplication = nullptr;
	AfxGetMainWnd()->SendMessage(UWM::Designer::GetApplication, (WPARAM)&pApplication, (LPARAM)pFormDoc);

    if( pApplication != nullptr && pApplication->GetUseQuestionText() )
    {
        CArray<CLangInfo, CLangInfo&> aLangInfo;
	    AfxGetMainWnd()->SendMessage(UWM::Form::GetCapiLanguages, (WPARAM)&aLangInfo, (LPARAM)pFormDoc);

        if( aLangInfo.GetSize() > 1 )
            m_bMultiLangMode = true;
    }


    if(m_bMultiLangMode && !m_bHideSecondLang) {
        DisplayMultiLangMode();
    }
    else {
        DisplaySingleLangMode();
    }

    if(m_pQSFEditView1 && m_pQSFEditView2) {
        if(m_bMultiLangMode && bInterfaceRequest){
            bInterfaceRequest = false;
            if(m_pQSFEditView2->GetNumLanguages()  > 1) {
                m_pQSFEditView2->SetLanguage(1);
            }
        }
    }

    return;
}

void CFormChildWnd::DisplayActiveMode()
{
    if(m_bUseQuestionText) {
        switch(m_eViewMode){
        case FormViewMode:
            DisplayQuestionTextMode(true);
            break;
        case LogicViewMode:
            DisplayQuestionTextMode(false);
            break;
        case QSFEditorViewMode:
            DisplayEditorMode();
            break;
        case QuestionnaireViewMode:
            DisplayQuestionnaireViewMode();
            break;
        default:
            ASSERT(FALSE);
            break;
        }
    }
    else {
        switch(m_eViewMode){
        case FormViewMode:
            DisplayNoQuestionTextMode(true);
            break;
        case LogicViewMode:
            DisplayNoQuestionTextMode(false);
            break;
        case QSFEditorViewMode:
            DisplayEditorMode();
            break;
        case QuestionnaireViewMode:
            DisplayQuestionnaireViewMode();
            break;
        default:
            ASSERT(FALSE);
            break;
        }
    }
}



void CFormChildWnd::OnQsfEditor()
{
    if(m_eViewMode==QSFEditorViewMode)
        return;

    if(m_eViewMode == LogicViewMode || m_eViewMode == QuestionnaireViewMode){
        OnViewForm();
    }
    CFormDoc* pFormDoc = (CFormDoc*) GetActiveDocument();

    bInterfaceRequest = true;
    m_eViewMode=QSFEditorViewMode;
    DisplayActiveMode();
    //Update text
    CFormTreeCtrl* pTreeCtrl = pFormDoc->GetFormTreeCtrl();
    HTREEITEM hItem = pTreeCtrl->GetSelectedItem();
    CFormID* pFormID = (CFormID*)pTreeCtrl->GetItemData(hItem);
    pFormDoc->SetSelectedCapiQuestion(pFormID);
    pFormDoc->UpdateAllViews(nullptr, Hint::CapiEditorUpdateQuestion);

    AfxGetMainWnd()->SendMessage(UWM::Designer::ShowToolbar, (WPARAM)FrameType::Form);
}

void CFormChildWnd::OnUpdateQsfEditor(CCmdUI* pCmdUI)
{
    if(m_bUseQuestionText){
        pCmdUI->Enable(TRUE);
        if(m_eViewMode == QSFEditorViewMode){
            pCmdUI->SetCheck(TRUE);
        }
        else {
            pCmdUI->SetCheck(FALSE);
        }
    }
    else {
        pCmdUI->Enable(FALSE);
    }

}

void CFormChildWnd::ShowCapiLanguage(wstring_view language_label)
{
    if( ( m_pQSFEditView1->GetCurrentLanguage().GetLabel() != language_label ) &&
        ( m_pQSFEditView2->GetCurrentLanguage().GetLabel() != language_label || !m_bMultiLangMode || m_bHideSecondLang ) )
    {
        m_pQSFEditView1->SetLanguage(language_label);
    }
}

/////////////////////////////////////////////////////////////////////////////////
//
//  void CFormChildWnd::DisplayMultiLangMode()
//
/////////////////////////////////////////////////////////////////////////////////
void CFormChildWnd::DisplayMultiLangMode()
{
    CFormDoc* pFormDoc = (CFormDoc*) GetActiveDocument();

    if(!pFormDoc)
        return;

    if(m_wndFSplitter.GetSafeHwnd() && m_wndFSplitter.IsSplitterReady()){

        //Set the CQSFEView
        CWnd* pWnd = m_wndFSplitter.GetPane(0,0);
        if(pWnd && !pWnd->IsKindOf(RUNTIME_CLASS(CQSFEView))){
            pWnd->ShowWindow(SW_HIDE);          //hide the old second pane view
            CQSFEView* pView1 = m_pQSFEditView1;

            if (!pView1) {

                pView1=m_pQSFEditView1 =  new CQSFEView(PortableFunctions::PathGetDirectory<CString>(pFormDoc->GetPathName()));
                CCreateContext context =  CCreateContext();
                context.m_pCurrentDoc = pFormDoc;


                CRect rect(0,0,100,100);
                pWnd->GetClientRect(&rect);


                int iID = m_wndFSplitter.IdFromRowCol(0,0);
                pWnd->SetDlgCtrlID(2224);

                if (!m_pQSFEditView1->Create(NULL,_T(""),WS_CHILD|WS_VISIBLE,rect,&m_wndFSplitter,iID,&context)){
                    TRACE0("Failed to create view\n");
                    ASSERT (false);
                    return ;
                }
                m_pQSFEditView1->SendMessage(WM_INITIALUPDATE);   // csc 1/14/2004
            }
            else {

                int iTempID = pView1->GetDlgCtrlID();
                int iID = m_wndFSplitter.IdFromRowCol(0,0);

                //Set the controlId of the qsfview2 to this pane
                pView1->SetDlgCtrlID(iID);

                pView1->ShowWindow(SW_SHOW);
                pWnd->SetDlgCtrlID(iTempID);
                pView1->SetLanguage(0);
            }
        }

        //Set the CQSFEView
        pWnd = m_wndFSplitter.GetPane(1,0);
        if(pWnd && !pWnd->IsKindOf(RUNTIME_CLASS(CQSFEView))){
            pWnd->ShowWindow(SW_HIDE);          //hide the old second pane view
            CQSFEView* pView2 = m_pQSFEditView2;

            if (!pView2) {

                pView2=m_pQSFEditView2 = new CQSFEView(PortableFunctions::PathGetDirectory<CString>(pFormDoc->GetPathName()));
                CCreateContext context =  CCreateContext();

                context.m_pCurrentDoc = pFormDoc;


                CRect rect(0,0,100,100);
                pWnd->GetClientRect(&rect);

                int iID = m_wndFSplitter.IdFromRowCol(1,0);
                pWnd->SetDlgCtrlID(2225);

                if (!m_pQSFEditView2->Create(NULL,_T(""),WS_CHILD|WS_VISIBLE,rect,&m_wndFSplitter,iID,&context)){
                    TRACE0("Failed to create view\n");
                    ASSERT (false);
                    return ;
                }

                pView2->SendMessage(WM_INITIALUPDATE);   // csc 1/14/2004
            }
            else {

                int iTempID = pView2->GetDlgCtrlID();
                int iID = m_wndFSplitter.IdFromRowCol(1,0);

                //Set the controlId of the qsfview2 to this pane
                pView2->SetDlgCtrlID(iID);

                pView2->ShowWindow(SW_SHOW);
                pWnd->SetDlgCtrlID(iTempID);
                pView2->SetLanguage(0);

            //  m_pQSFEditView2->SendMessage(WM_INITIALUPDATE);   // csc 1/14/2004
            }
        }

        //Set the CQSFCndView
        pWnd = m_wndFSplitter.GetPane(2,0);
        if(pWnd && !pWnd->IsKindOf(RUNTIME_CLASS(CQSFCndView))){
            pWnd->ShowWindow(SW_HIDE);          //hide the old second pane view
            CQSFCndView* pView3 = (CQSFCndView*)pFormDoc->GetView(FormViewType::Condition);
            {
                int iTempID = pView3->GetDlgCtrlID();
                int iID = m_wndFSplitter.IdFromRowCol(2,0);

                //Set the controlId of the qsfview2 to this pane
                pView3->SetDlgCtrlID(iID);

                pView3->ShowWindow(SW_SHOW);
                pWnd->SetDlgCtrlID(iTempID);
                SetActiveView(pView3);
                m_wndFSplitter.SetActivePane(2,0);
            }
        }


        m_logicDlgBar.ShowWindow(SW_HIDE);
        m_logicReferenceWnd.ShowWindow(SW_HIDE);

        CRect rect;
        GetClientRect(&rect);
        COXMDIChildWndSizeDock::OnSize(SIZE_RESTORED, rect.Width(), rect.Height());

        int iCur ,iMin;
        iCur=iMin=0;
        if(m_iQsfVSz1 <= 0) {
            m_iQsfVSz1 =rect.Height()/3;
        }
        if(m_iQsfVSz2 <= 0) {
            m_iQsfVSz2 =rect.Height()/3;
        }



        int iLastPane = rect.Height()- m_iQsfVSz1 -m_iQsfVSz2;
        if(iLastPane < 60) {
            iLastPane = 60 ;
            if(m_iQsfVSz2 -60 < 30 ) {
                m_iQsfVSz2 = 30;
            }
            else {
                m_iQsfVSz2 = m_iQsfVSz2 - iLastPane ;
            }
            m_iQsfVSz1 = rect.Height()- iLastPane -m_iQsfVSz2;
        }

        m_wndFSplitter.SetRowInfo(0, m_iQsfVSz1,10);
        m_wndFSplitter.SetRowInfo(1, m_iQsfVSz2,10);
        m_wndFSplitter.SetRowInfo(2, iLastPane,10);


        m_wndFSplitter.RecalcLayout();
    }
    return;

}

/////////////////////////////////////////////////////////////////////////////////
//
//  void CFormChildWnd::DisplaySingleLangMode()
//
/////////////////////////////////////////////////////////////////////////////////
void CFormChildWnd::DisplaySingleLangMode()
{
    CFormDoc* pFormDoc = (CFormDoc*) GetActiveDocument();

    if(!pFormDoc)
        return;

    if(m_wndFSplitter.GetSafeHwnd() && m_wndFSplitter.IsSplitterReady()){
        //Set the CQSFEView
        CWnd* pWnd = m_wndFSplitter.GetPane(0,0);
        if(pWnd && !pWnd->IsKindOf(RUNTIME_CLASS(CQSFEView))){
            pWnd->ShowWindow(SW_HIDE);          //hide the old second pane view
            CQSFEView* pView2 = m_pQSFEditView2;

            if (!pView2) {

                pView2=m_pQSFEditView2 = new CQSFEView(PortableFunctions::PathGetDirectory<CString>(pFormDoc->GetPathName()));
                CCreateContext context =  CCreateContext();

                context.m_pCurrentDoc = pFormDoc;


                CRect rect(0,0,100,100);
                pWnd->GetClientRect(&rect);

                int iID = m_wndFSplitter.IdFromRowCol(0,0);
                pWnd->SetDlgCtrlID(2225);

                if (!m_pQSFEditView2->Create(NULL,_T(""),WS_CHILD|WS_VISIBLE,rect,&m_wndFSplitter,iID,&context)){
                    TRACE0("Failed to create view\n");
                    ASSERT (false);
                    return ;
                }

                pView2->SendMessage(WM_INITIALUPDATE);   // csc 1/14/2004


            }
            else {

                int iTempID = pView2->GetDlgCtrlID();
                int iID = m_wndFSplitter.IdFromRowCol(0,0);

                //Set the controlId of the qsfview2 to this pane
                pView2->SetDlgCtrlID(iID);

                pView2->ShowWindow(SW_SHOW);
                pWnd->SetDlgCtrlID(iTempID);
                pView2->SetLanguage(0);

            }
        }

        //Set the CQSFEView
        pWnd = m_wndFSplitter.GetPane(1,0);
        if(pWnd && !pWnd->IsKindOf(RUNTIME_CLASS(CQSFEView))){
            pWnd->ShowWindow(SW_HIDE);          //hide the old second pane view
            CQSFEView* pView1 = m_pQSFEditView1;

            if (!pView1) {

                pView1=m_pQSFEditView1 = new CQSFEView(PortableFunctions::PathGetDirectory<CString>(pFormDoc->GetPathName()));
                CCreateContext context =  CCreateContext();
                context.m_pCurrentDoc = pFormDoc;


                CRect rect(0,0,100,100);
                pWnd->GetClientRect(&rect);


                int iID = m_wndFSplitter.IdFromRowCol(1,0);
                pWnd->SetDlgCtrlID(2224);

                if (!m_pQSFEditView1->Create(NULL,_T(""),WS_CHILD|WS_VISIBLE,rect,&m_wndFSplitter,iID,&context)){
                    TRACE0("Failed to create view\n");
                    ASSERT (false);
                    return ;
                }
               m_pQSFEditView1->SendMessage(WM_INITIALUPDATE);   // csc 1/14/2004
            }
            else {

                int iTempID = pView1->GetDlgCtrlID();
                int iID = m_wndFSplitter.IdFromRowCol(1,0);

                //Set the controlId of the qsfview2 to this pane
                pView1->SetDlgCtrlID(iID);

                pView1->ShowWindow(SW_SHOW);
                pWnd->SetDlgCtrlID(iTempID);
                pView1->SetLanguage(0);

            }

        }

        //Set the CQSFCndView
        pWnd = m_wndFSplitter.GetPane(2,0);
        if(pWnd && !pWnd->IsKindOf(RUNTIME_CLASS(CQSFCndView))){
            pWnd->ShowWindow(SW_HIDE);          //hide the old second pane view
            CQSFCndView* pView3 = (CQSFCndView*)pFormDoc->GetView(FormViewType::Condition);
            {
                int iTempID = pView3->GetDlgCtrlID();
                int iID = m_wndFSplitter.IdFromRowCol(2,0);

                //Set the controlId of the qsfview2 to this pane
                pView3->SetDlgCtrlID(iID);

                pView3->ShowWindow(SW_SHOW);
                pWnd->SetDlgCtrlID(iTempID);
                SetActiveView(pView3);
                m_wndFSplitter.SetActivePane(2,0);
            }
        }


        m_logicDlgBar.ShowWindow(SW_HIDE);
        m_logicReferenceWnd.ShowWindow(SW_HIDE);

        CRect rect;
        GetClientRect(&rect);
        COXMDIChildWndSizeDock::OnSize(SIZE_RESTORED, rect.Width(), rect.Height());

        int iCur ,iMin;
        iCur=iMin=0;
        if(m_iQsfVSz1 <= 0) {
            m_iQsfVSz1 =rect.Height()-rect.Height()/3;
        }

        m_wndFSplitter.SetRowInfo(0, 0, 10);
        int iLastPane = rect.Height()- m_iQsfVSz1;
        if(iLastPane < 60) {
            iLastPane = 60 ;
            m_iQsfVSz1 = rect.Height()- iLastPane;
        }
        m_wndFSplitter.SetRowInfo(1, m_iQsfVSz1,10);
        m_wndFSplitter.SetRowInfo(2, iLastPane,10);

        m_wndFSplitter.RecalcLayout();
    }
    return;

}


/////////////////////////////////////////////////////////////////////////////////
//
//  void CFormChildWnd::OnClose()
//
/////////////////////////////////////////////////////////////////////////////////
void CFormChildWnd::OnClose()
{
    // TODO: Add your message handler code here and/or call default
    SaveHeightSettings();
    COXMDIChildWndSizeDock::OnClose();
}


/////////////////////////////////////////////////////////////////////////////////
//
//  void CFormChildWnd::SaveHeightSettings()
//
/////////////////////////////////////////////////////////////////////////////////
void CFormChildWnd::SaveHeightSettings()
{
    if(this->m_bUseQuestionText) {
        AfxGetApp()->WriteProfileInt(_T("Settings"), _T("QuestPaneSz1"), m_iQsfVSz1);
        AfxGetApp()->WriteProfileInt(_T("Settings"), _T("QuestPaneSz2"), m_iQsfVSz2);
    }
}

/////////////////////////////////////////////////////////////////////////////////
//
//  void CFormChildWnd::GetHeightSettings()
//
/////////////////////////////////////////////////////////////////////////////////
void CFormChildWnd::GetHeightSettings()
{
    if(this->m_bUseQuestionText && m_bFirstTime) {
        m_bFirstTime = false;
        m_iQsfVSz1 = AfxGetApp()->GetProfileInt(_T("Settings"), _T("QuestPaneSz1"), 0);
        m_iQsfVSz2 = AfxGetApp()->GetProfileInt(_T("Settings"), _T("QuestPaneSz2"), 0);
    }
}

BOOL CFormChildWnd::DestroyWindow()
{
    // TODO: Add your specialized code here and/or call the base class
    SaveHeightSettings();
    return COXMDIChildWndSizeDock::DestroyWindow();
}


void CFormChildWnd::OnAddcapiLang()
{
	//Get the lang info from the mainframe
	//Now instantiate the dialog
	if(m_eViewMode == QSFEditorViewMode) {
		m_eViewMode = FormViewMode;
		DisplayActiveMode();
	}
	CFormDoc* pFormDoc = (CFormDoc*) GetActiveDocument();
	CCapilangDlg dlg;

	AfxGetMainWnd()->SendMessage(UWM::Form::GetCapiLanguages, (WPARAM)&dlg.m_Langgrid.m_aLangInfo, (LPARAM)pFormDoc);

	if(dlg.DoModal() == IDOK && dlg.m_Langgrid.m_bChanged){
		//process the onok
		AfxGetMainWnd()->SendMessage(UWM::Form::UpdateCapiLanguages, (WPARAM)&dlg.m_Langgrid.m_aLangInfo, (LPARAM)pFormDoc);
        pFormDoc->UpdateAllViews(nullptr, Hint::CapiEditorUpdateLanguages);

		if(this->m_eViewMode == QSFEditorViewMode){
			DisplayEditorMode();
		}
	}
}

void CFormChildWnd::OnUpdateIfUsingQuestionText(CCmdUI* pCmdUI)
{
    pCmdUI->Enable(m_bUseQuestionText ? TRUE : FALSE);
}

void CFormChildWnd::OnCapiMacros()
{
    AfxGetMainWnd()->SendMessage(UWM::Form::CapiMacros);
}


/////////////////////////////////////////////////////////////////////////////////
//
//	void CFormChildWnd::OnRunasBch()
//
/////////////////////////////////////////////////////////////////////////////////
void CFormChildWnd::OnRunasBch()
{
	CRunBOpDlg optDlg;
	if(optDlg.DoModal() != IDOK) {
		return;
	}
	CFormDoc* pFormDoc = PreRunPublish();
	AfxGetMainWnd()->SendMessage(UWM::Form::RunActiveApplicationAsBatch, 0, (LPARAM)pFormDoc);
}

/////////////////////////////////////////////////////////////////////////////////
//
//	void CFormChildWnd::OnUpdateRunasBch(CCmdUI* pCmdUI)
//
/////////////////////////////////////////////////////////////////////////////////
void CFormChildWnd::OnUpdateRunasBch(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(TRUE);
}


/////////////////////////////////////////////////////////////////////////////////
//
//	void CFormChildWnd::OnGenerateBinary()
//
/////////////////////////////////////////////////////////////////////////////////
void CFormChildWnd::OnGenerateBinary()
{
	CFormDoc* pFormDoc = PreRunPublish();
	AfxGetMainWnd()->SendMessage(UWM::Form::PublishApplication, 0, (LPARAM)pFormDoc);
}

/////////////////////////////////////////////////////////////////////////////////
//
//	void CFormChildWnd::OnPublishAndDeploy()
//
/////////////////////////////////////////////////////////////////////////////////
void CFormChildWnd::OnPublishAndDeploy()
{
	CFormDoc* pFormDoc = PreRunPublish();
	AfxGetMainWnd()->SendMessage(UWM::Form::PublishAndDeployApplication, 0, (LPARAM)pFormDoc);
}

void CFormChildWnd::OnEditStyles()
{
    CFormDoc* pDoc = assert_cast<CFormDoc*>(GetActiveDocument());
    QSFEditStyleDlg dlg(pDoc->GetCapiQuestionManager()->GetStyles(), this);
    if (dlg.DoModal() == IDOK) {
        pDoc->GetCapiQuestionManager()->SetStyles(std::move(dlg.m_styles));
        pDoc->UpdateAllViews(nullptr, Hint::CapiEditorUpdateStyles);
        auto qsf_form_view = DYNAMIC_DOWNCAST(QSFView, pDoc->GetView(FormViewType::QuestionText));
        qsf_form_view->SetStyleCss(pDoc->GetCapiQuestionManager()->GetStylesCss());
    }
}

void CFormChildWnd::OnUpdateIfApplicationIsAvailable(CCmdUI* pCmdUI)
{
	CFormDoc* pDoc = assert_cast<CFormDoc*>(GetActiveDocument());
	Application* pApplication = nullptr;
	AfxGetMainWnd()->SendMessage(UWM::Designer::GetApplication, (WPARAM)&pApplication, (LPARAM)pDoc);
	pCmdUI->Enable(( pApplication == nullptr ) ? FALSE : TRUE);
}


void CFormChildWnd::OnOptionsSynchronization()
{
	CFormDoc* pDoc = assert_cast<CFormDoc*>(GetActiveDocument());
	Application* pApplication = nullptr;
	AfxGetMainWnd()->SendMessage(UWM::Designer::GetApplication, (WPARAM)&pApplication, (LPARAM)pDoc);

	if( pApplication == nullptr )
		return;

	AppSyncParameters syncParams = pApplication->GetSyncParameters();

	CSyncParamsDlg dlg;
	bool bNewSync = syncParams.server.empty();
	if (bNewSync) {
        dlg.m_csServerUrl = GetServerUrlFromRegistry();
        if (dlg.m_csServerUrl.IsEmpty())
            dlg.m_csServerUrl = _T("http://www.myserver.com/api");
		dlg.m_iSyncDirection = 0; // put
        dlg.m_bEnabled = FALSE;
	} else {
		dlg.m_csServerUrl = WS2CS(syncParams.server);
		dlg.m_iSyncDirection = (int)syncParams.sync_direction - 1;
        dlg.m_bEnabled = TRUE;
    }

    if( dlg.DoModal() != IDOK )
        return;

    syncParams.server = dlg.m_bEnabled ? CS2WS(dlg.m_csServerUrl) : std::wstring();
	syncParams.sync_direction = (SyncDirection) (dlg.m_iSyncDirection + 1);
	pApplication->SetSyncParameters(std::move(syncParams));
	pDoc->SetModifiedFlag();
    if (dlg.m_bEnabled)
        SaveServerUrlToRegistry(dlg.m_csServerUrl);
}


void CFormChildWnd::OnOptionsMapping()
{
    CFormDoc* form_doc = assert_cast<CFormDoc*>(GetActiveDocument());
    const CDataDict* dictionary = form_doc->GetFormFile().GetDictionary();
    ASSERT(dictionary != nullptr);

    Application* application;

    if( WindowsDesktopMessage::Send(UWM::Designer::GetApplication, &application, form_doc) != 1 )
        return;

    AppMappingOptions mapping_options = application->GetMappingOptions();

    MappingOptionsDlg dlg(mapping_options, *dictionary);

    if( dlg.DoModal() != IDOK || mapping_options == application->GetMappingOptions() )
        return;

    application->SetMappingOptions(std::move(mapping_options));
    form_doc->SetModifiedFlag();
}


void CFormChildWnd::OnOptionsDataSources()
{
	CFormDoc* pDoc = assert_cast<CFormDoc*>(GetActiveDocument());
	Application* pApplication = nullptr;
	AfxGetMainWnd()->SendMessage(UWM::Designer::GetApplication, (WPARAM)&pApplication, (LPARAM)pDoc);

	if( pApplication == nullptr )
		return;

	CDataSourceOptionsDlg dlg(pApplication);

	if( dlg.DoModal() == IDOK )
	{
		pApplication->SetAutoPartialSaveMinutes(dlg.GetAutoPartialSaveMinutes());
		pApplication->SetCreateListingFile(dlg.GetCreateListingFileFlag());
		pApplication->SetCreateLogFile(dlg.GetCreateLogFileFlag());
		pApplication->SetEditNotePermissions(EditNotePermissions::DeleteOtherOperators,dlg.GetNoteDeleteOtherOperatorsFlag());
		pApplication->SetEditNotePermissions(EditNotePermissions::EditOtherOperators,dlg.GetNoteEditOtherOperatorsFlag());
		pDoc->SetModifiedFlag();
	}
}


CFormDoc* CFormChildWnd::PreRunPublish()
{
	CFormDoc* pFormDoc = assert_cast<CFormDoc*>(GetActiveDocument());
	if (!pFormDoc)
		return NULL;

    pFormDoc->GetFormFile().RenumberAllForms();

	CView* pView = GetActiveView();
	if (!pView)
		return NULL;

	if (pView->IsKindOf(RUNTIME_CLASS(CFormScrollView))) {
		return pFormDoc;
	}

	else if (pView->IsKindOf(RUNTIME_CLASS(CFSourceEditView))) {
		HTREEITEM hItem = pFormDoc->GetFormTreeCtrl()->GetSelectedItem();
		CFormID* pID = (CFormID*)pFormDoc->GetFormTreeCtrl()->GetItemData(hItem);

		if (pID && pID->GetFormDoc() == pFormDoc) {
			if (AfxGetMainWnd()->SendMessage(UWM::Form::PutSourceCode, 0, (LPARAM)pID) == -1)
				return NULL;
		}

		return pFormDoc;
	}

	else if (m_eViewMode == QSFEditorViewMode || m_eViewMode == QuestionnaireViewMode) {
		return pFormDoc;
	}
	else {
		return NULL;
	}
}


void CFormChildWnd::RunMultipleFieldPropertiesDialog(std::vector<CDEField*>* selected_fields, CDEGroup* pCurGroup, CString form_name)
{
    CFormDoc* pDoc = assert_cast<CFormDoc*>(GetActiveDocument());
    CDEFormFile* pFF = &pDoc->GetFormFile();

    std::function<std::vector<CDEField*>()> all_fields_getter = [pFF]()
    {
        return pFF->GetAllFields();
    };

    bool apply_to_all_fields_only = ( selected_fields == nullptr );

	CMultipleFieldPropertiesDlg dlg(apply_to_all_fields_only ? all_fields_getter() : *selected_fields,
                                    apply_to_all_fields_only ? nullptr : &all_fields_getter);

	if( dlg.DoModal() != IDOK )
        return;

    std::optional<FormUndoStack> form_undo_stack = ( pCurGroup != nullptr ) ? std::make_optional<FormUndoStack>() : std::nullopt;

	for( CDEField* pField : dlg.GetFields() )
	{
		// add to undo stack
        if( form_undo_stack.has_value() )
        {
			int iFieldIndex = pCurGroup->FindItem(pField->GetName());

			if( iFieldIndex >= 0 ) // it's on the form
            {
				form_undo_stack->PushUndoObj(CFormUndoObj::Action::UR_modify, pField, iFieldIndex, form_name);
            }

			else // it's on a roster
			{
				// the undo process can't handle undos for now
				form_undo_stack.reset();
            }
		}

		// make the changes, if applicable
        CONTENT_TYPE_REFACTOR::LOOK_AT("what to do about non-numeric + non-alpha fields?");
		bool bIsAlpha = pField->GetDictItem()->GetContentType() == ContentType::Alpha;

		if( !pField->IsMirror() )
		{
			if( dlg.ApplyProtected() )
				pField->IsProtected(dlg.GetProtected());

			if( dlg.ApplyUpperCase() && bIsAlpha )
				pField->IsUpperCase(dlg.GetUpperCase());

			if( dlg.ApplyEnterKey() )
				pField->IsEnterKeyRequired(dlg.GetEnterKey());

            if( dlg.ApplyVerify() )
				pField->SetVerifyFlag(dlg.GetVerify());

            if( dlg.ApplyValidationMethod() )
                pField->SetValidationMethod(dlg.GetValidationMethod());

			if( dlg.ApplyKLID() )
				pField->SetKLID(dlg.GetKLID());
		}

		if( dlg.ApplyHideInCaseTree() )
			pField->IsHiddenInCaseTree(dlg.GetHideInCaseTree());

		if( dlg.ApplyAlwaysVisualValue() )
			pField->SetAlwaysVisualValue(dlg.GetAlwaysVisualValue());

		if( dlg.ApplyCaptureType() && !pField->IsMirror() )
		{
            if( dlg.LinkToDictionaryCaptureTypeWhenPossible() )
            {
                const auto& dictionary_capture_info = pField->GetDictItem()->GetCaptureInfo();

                if( dictionary_capture_info.IsSpecified() )
                    pField->SetCaptureInfo(CaptureType::Unspecified);
            }

            else
            {
                if( dlg.GetUseUnicodeTextBox() || dlg.GetMultiLineOption() )
				{
    				if( bIsAlpha )
					{
                        pField->SetCaptureInfo(CaptureType::TextBox);
						pField->SetUseUnicodeTextBox(true);
						pField->SetMultiLineOption(dlg.GetMultiLineOption());
					}
                }

                else
                {
                    CaptureInfo capture_info = dlg.UseDefaultCaptureType() ? CaptureInfo::GetDefaultCaptureInfo(*pField->GetDictItem()) :
                                                                                dlg.GetCaptureType();

                    if( !capture_info.IsSpecified() ||
                        CaptureInfo::IsCaptureTypePossible(*pField->GetDictItem(), capture_info.GetCaptureType()) )
                    {
                        // only modify the capture info when it changed (so that any secondary settings like
                        // date formats are not overwritten)
                        if( pField->GetCaptureInfo().GetCaptureType() != capture_info.GetCaptureType() )
                            pField->SetCaptureInfo(capture_info);

    					pField->SetUseUnicodeTextBox(false);
	    				pField->SetMultiLineOption(false);
					}
				}
			}
		}

		if( dlg.ApplyFieldLabelType() )
			pField->SetFieldLabelType(dlg.GetFieldLabelType());
	}

	if( form_undo_stack.has_value() )
		pDoc->PushUndo(std::move(*form_undo_stack));

	if( dlg.ApplyFieldLabelType() )
		pFF->RefreshAssociatedFieldText();

	RedrawWindow();
	pDoc->SetModifiedFlag(true);
}


void CFormChildWnd::OnOptionsFieldProperties()
{
    RunMultipleFieldPropertiesDialog(nullptr, nullptr, _T(""));
    if (m_eViewMode == QuestionnaireViewMode)
        DisplayActiveMode();
}


namespace
{
    LPCTSTR KEY_NAME_INTERAPP = _T("Software\\U.S. Census Bureau\\InterApp");
    LPCTSTR KEY_LAST_SYNC_URL = _T("Last Sync URL");    
}

void CFormChildWnd::SaveServerUrlToRegistry(const CString& server_url) const
{
    WinRegistry registry;
    registry.Open(HKEY_CURRENT_USER, KEY_NAME_INTERAPP, true);
    registry.WriteString(KEY_LAST_SYNC_URL, server_url);
}

CString CFormChildWnd::GetServerUrlFromRegistry() const
{
    WinRegistry registry;
    registry.Open(HKEY_CURRENT_USER, KEY_NAME_INTERAPP, false);
    CString val;
    registry.ReadString(KEY_LAST_SYNC_URL, &val);
    return val;
}



LRESULT CFormChildWnd::OnSwitchView(WPARAM wParam, LPARAM /*lParam*/)
{
    ViewType view_type = (ViewType)wParam;

    if( view_type == ViewType::Form )
        OnViewForm();

    else if( view_type == ViewType::Logic )
        OnViewLogic();

    else 
        ASSERT(false);

    return 0;
}


void CFormChildWnd::OnViewQuestionnaire()
{
    if (m_eViewMode == QuestionnaireViewMode)
        return;

    if (m_eViewMode == LogicViewMode || m_eViewMode == QSFEditorViewMode) {
        OnViewForm();
    }
    bInterfaceRequest = true;
    m_eViewMode = QuestionnaireViewMode;
    DisplayActiveMode();
    AfxGetMainWnd()->SendMessage(UWM::Designer::ShowToolbar, (WPARAM)FrameType::Form);
}


void CFormChildWnd::OnUpdateViewQuestionnaire(CCmdUI* pCmdUI)
{
    pCmdUI->Enable(TRUE);
    if (m_eViewMode == QuestionnaireViewMode) {
        pCmdUI->SetCheck(TRUE);
    }
    else {
        pCmdUI->SetCheck(FALSE);
    }
}
