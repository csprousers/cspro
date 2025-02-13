// MainFrm.cpp : implementation of the CMainFrame class
//

#include "StdAfx.h"
#include "MainFrm.h"
#include "TblDoc.h"
#include "TblView.h"
#include <zToolsO/CommonObjectTransporter.h>
#include <zToolsO/UWM.h>
#include <zTableF/TabDoc.h>
#include <zTableF/TabView.h>
#include <zTableF/TabChWnd.h>

// for OnDDEExecute below
#include <Dde.h>


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// CMainFrame

IMPLEMENT_DYNAMIC(CMainFrame, CMDIFrameWnd)

BEGIN_MESSAGE_MAP(CMainFrame, CMDIFrameWnd)
    //{{AFX_MSG_MAP(CMainFrame)
    ON_WM_CREATE()
    ON_WM_MENUCHAR()
    //}}AFX_MSG_MAP
    // Global help commands
    ON_MESSAGE(UWM::ToolsO::GetObjectTransporter, OnGetObjectTransporter)
    ON_MESSAGE(UWM::Designer::ShowToolbar, ShowTabToolBar)
    ON_MESSAGE(UWM::Designer::HideToolbar, HideTabToolBar)
    ON_MESSAGE(UWM::Table::UpdateTree, OnUpdateTree)

    ON_COMMAND(ID_HELP_FINDER, CMDIFrameWnd::OnHelpFinder)
    ON_COMMAND(ID_HELP, CMDIFrameWnd::OnHelp)
    ON_COMMAND(ID_CONTEXT_HELP, CMDIFrameWnd::OnContextHelp)
    ON_COMMAND(ID_DEFAULT_HELP, CMDIFrameWnd::OnHelpFinder)
    ON_COMMAND(ID_VIEW_NAMES, OnViewNames)
    ON_COMMAND(ID_QUICK_QUIT, OnQuickQuit)

    ON_UPDATE_COMMAND_UI(ID_AREA_COMBO, OnUpdateAreaComboBox)
    ON_UPDATE_COMMAND_UI(ID_TAB_ZOOM_COMBO, OnUpdateZoomComboBox)
    ON_UPDATE_COMMAND_UI(ID_VIEW_NAMES, OnUpdateViewNames)
    ON_UPDATE_COMMAND_UI(ID_QUICK_QUIT, OnUpdateQuickQuit)
    ON_MESSAGE(WM_DDE_EXECUTE, OnDDEExecute)
    ON_MESSAGE(WM_IMSA_FILEOPEN, OnIMSAFileOpen)
    ON_MESSAGE(WM_IMSA_FILECLOSE, OnIMSAFileClose)
    ON_MESSAGE(WM_IMSA_SETFOCUS, OnIMSASetFocus)

END_MESSAGE_MAP()

static UINT indicators[] =
{
    ID_SEPARATOR,           // status line indicator
    ID_INDICATOR_CAPS,
    ID_INDICATOR_NUM,
    ID_INDICATOR_SCRL,
};

/////////////////////////////////////////////////////////////////////////////
// CMainFrame construction/destruction

CMainFrame::CMainFrame()
{
    // TODO: add member initialization code here
    m_pWndTabTBar = NULL;
    m_pszClassName = NULL;
}

CMainFrame::~CMainFrame()
{
    if(m_pWndTabTBar) {
        delete m_pWndTabTBar;
    }
}

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CMDIFrameWnd::OnCreate(lpCreateStruct) == -1)
        return -1;

    if (!m_wndToolBar.CreateEx(this) ||
        !m_wndToolBar.LoadToolBar(IDR_MAINFRAME))
    {
        TRACE0("Failed to create toolbar\n");
        return -1;      // fail to create
    }
    /*if (!m_wndDlgBar.Create(this, IDR_MAINFRAME,
        CBRS_ALIGN_TOP, AFX_IDW_DIALOGBAR))
    {
        TRACE0("Failed to create dialogbar\n");
        return -1;      // fail to create
    }*/
    // This is a sizeable dialog bar.. that includes gadget resizing

    // Create Tabulation tool bar
    m_pWndTabTBar = new CToolBar();

    if (!m_pWndTabTBar->CreateEx(this,TBSTYLE_FLAT,WS_CHILD | WS_VISIBLE | CBRS_ALIGN_TOP,CRect(0,0,0,0), 997)||
        !m_pWndTabTBar->LoadToolBar(IDR_TBLVWERVW_FRAME))
    {
        TRACE0("Failed to create table toolbar\n");
        return -1;      // fail to create
    }

    // hide quick quit - have to do this BEFORE adding in the combo boxes
    // otherwise the spacing gets all messed up
    if (!((CTblViewApp*) AfxGetApp())->m_bCalledAsChild)  {
        // we are *not* dependent ... hide the quick quit toolbar button
        m_pWndTabTBar->GetToolBarCtrl().HideButton(ID_QUICK_QUIT);
    }

    // add area combo box to toolbar in place of ID_AREA_COMBO placeholder button
    int nIndex = m_pWndTabTBar->GetToolBarCtrl().CommandToIndex(ID_VIEWER_AREA_COMBO);
    CRect tbButtonRect;
    m_pWndTabTBar->GetToolBarCtrl().GetItemRect(nIndex, &tbButtonRect);
    CRect rect(tbButtonRect);
    const int iAreaComboDropHeight = 250;
    const int iAreaComboWidth = 150;
    const int iAreaComboLeftSpacing = 0; // offset from button to left
    rect.top = -1;
    rect.bottom = rect.top + iAreaComboDropHeight;
    rect.left += iAreaComboLeftSpacing;
    rect.right = rect.left + iAreaComboWidth;
    if(!m_tabAreaComboBox.Create(CBS_DROPDOWNLIST | WS_VISIBLE |
        WS_TABSTOP | WS_VSCROLL, rect, this, ID_AREA_COMBO))
    {
        TRACE(_T("Failed to create combo-box\n"));
        return FALSE;
    }
    m_tabAreaComboBox.SetParent(m_pWndTabTBar); // parent is toolbar but messages get sent to CMainFrame

    // add zoom combo box to toolbar in place of ID_AREA_COMBO placeholder button
    // (this overlaps the area combo box but thats ok since we never show area
    // box in print view and never show zoom ouside printview).
    rect = tbButtonRect;
    const int iZoomComboDropHeight = 250;
    const int iZoomComboWidth = 75;
    const int iZoomComboLeftSpacing = 0; // offset from button to left
    rect.top = -1;
    rect.bottom = rect.top + iZoomComboDropHeight;
    rect.left += iZoomComboLeftSpacing;
    rect.right = rect.left + iZoomComboWidth;
    if(!m_tabZoomComboBox.Create(CBS_DROPDOWNLIST | WS_VISIBLE |
        WS_TABSTOP | WS_VSCROLL, rect, this, ID_TAB_ZOOM_COMBO))
    {
        TRACE(_T("Failed to create combo-box\n"));
        return FALSE;
    }
    m_tabZoomComboBox.SetParent(m_pWndTabTBar); // parent is toolbar but messages get sent to CMainFrame
    // turn the placeholder toolbar button into a separator so that buttons
    // to the right of the combo box will not get covered up
    m_pWndTabTBar->SetButtonInfo(nIndex, ID_AREA_COMBO, TBBS_SEPARATOR, rect.Width()+iZoomComboLeftSpacing);

    // add close button to toolbar in place of ID_PRINTVIEW_CLOSE placeholder button
    nIndex = m_pWndTabTBar->GetToolBarCtrl().CommandToIndex(ID_PRINTVIEW_CLOSE);
    m_pWndTabTBar->GetToolBarCtrl().GetItemRect(nIndex, &rect);
    const int iPrintViewCloseHeight = tbButtonRect.Height();
    const int iPrintViewCloseWidth = 100;
    const int iPrintViewCloseSpacing = 8; // offset from button to left
    rect.top = 0;
    rect.bottom = rect.top + iPrintViewCloseHeight;
    rect.left += iPrintViewCloseSpacing;
    rect.right = rect.left + iPrintViewCloseWidth;
    m_pWndTabTBar->SetButtonInfo(nIndex, ID_PRINTVIEW_CLOSE, TBBS_SEPARATOR, rect.Width()+iZoomComboLeftSpacing);
    const TCHAR* sPreviewClose = _T("Close");
    if(!m_printViewCloseButton.Create(sPreviewClose, WS_CHILD | WS_VISIBLE, rect, this, ID_PRINTVIEW_CLOSE))
    {
        TRACE("Failed to create button\n");
        return FALSE;
    }
    m_printViewCloseButton.SetParent(m_pWndTabTBar); // parent is toolbar but messages get sent to CMainFrame

    if (!m_wndReBar.Create(this) ||
        !m_wndReBar.AddBar(&m_wndToolBar) || !m_wndReBar.AddBar(m_pWndTabTBar)/* ||
        !m_wndReBar.AddBar(&m_wndDlgBar)*/)
    {
        TRACE0("Failed to create rebar\n");
        return -1;      // fail to create
    }

    if (!m_wndStatusBar.Create(this) ||
        !m_wndStatusBar.SetIndicators(indicators,
          sizeof(indicators)/sizeof(UINT)))
    {
        TRACE0("Failed to create status bar\n");
        return -1;      // fail to create
    }

    EnableDocking(CBRS_ALIGN_ANY);
    m_SizeDlgBar.SetSizeDockStyle(/*SZBARF_STDMOUSECLICKS |*/ SZBARF_DLGAUTOSIZE);
    if (!m_SizeDlgBar.Create(this, IDD_DIALOGBAR, CBRS_LEFT, ID_FIXEDDLGBAR) )
    {
        TRACE0("Failed to create dialog bar\n");
        return -1;
    }

    // TODO: Remove this if you don't want tool tips
    m_wndToolBar.SetBarStyle(m_wndToolBar.GetBarStyle() | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC);
    m_pWndTabTBar->SetBarStyle(m_pWndTabTBar->GetBarStyle() | CBRS_TOOLTIPS | CBRS_FLYBY);

    m_wndReBar.GetReBarCtrl().ShowBand(0,FALSE);

    //Only doc on the left side
    m_SizeDlgBar.EnableDocking(CBRS_ALIGN_LEFT);
    DockControlBar(&m_SizeDlgBar,AFX_IDW_DOCKBAR_LEFT);

    // start out maximized
    WINDOWPLACEMENT wndpl;
    GetWindowPlacement (&wndpl);
    wndpl.showCmd = SW_SHOWMAXIMIZED;
    SetWindowPlacement (&wndpl);
    return 0;
}

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
    if( !CMDIFrameWnd::PreCreateWindow(cs) )
        return FALSE;
    if (m_pszClassName == NULL)    {
        WNDCLASS wndcls;
        ::GetClassInfo(AfxGetInstanceHandle(), cs.lpszClass, &wndcls);
        wndcls.hIcon = ((CTblViewApp*)AfxGetApp())->m_hIcon;
        CString csClassName = ((CTblViewApp*)AfxGetApp())->m_csWndClassName;
        wndcls.lpszClassName = csClassName;
        VERIFY(AfxRegisterClass(&wndcls));
        m_pszClassName = csClassName;
    }
    cs.lpszClass = m_pszClassName;
    // TODO: Modify the Window class or styles here by modifying
    //  the CREATESTRUCT cs

    return TRUE;
}



/////////////////////////////////////////////////////////////////////////////
// CMainFrame message handlers


LRESULT CMainFrame::OnGetObjectTransporter(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    if( m_objectTransporter == nullptr )
        m_objectTransporter = std::make_unique<CommonObjectTransporter>();

    return reinterpret_cast<LRESULT>(m_objectTransporter.get());
}


/////////////////////////////////////////////////////////////////////////////
//
//                      CMainFrame::ShowTabToolBar
//
/////////////////////////////////////////////////////////////////////////////

LRESULT CMainFrame::ShowTabToolBar(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    m_wndReBar.GetReBarCtrl().ShowBand(1,TRUE);
    m_wndReBar.GetReBarCtrl().ShowBand(0,FALSE);

    return 0;
}

/////////////////////////////////////////////////////////////////////////////
//
//                      CMainFrame::HideTabToolBar
//
/////////////////////////////////////////////////////////////////////////////

LRESULT CMainFrame::HideTabToolBar(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    m_wndReBar.GetReBarCtrl().ShowBand(1,FALSE);
    m_wndReBar.GetReBarCtrl().ShowBand(0,TRUE);

    return 0;
}


/////////////////////////////////////////////////////////////////////////////
//
//                      CMainFrame::OnLaunchActiveApp
//
/////////////////////////////////////////////////////////////////////////////

LRESULT CMainFrame::OnUpdateTree(WPARAM wParam, LPARAM /*lParam*/)
{
    CTabulateDoc* pDoc = (CTabulateDoc*)wParam;
    pDoc->SetTabTreeCtrl(&m_SizeDlgBar.m_TblTree);
    m_SizeDlgBar.m_TblTree.BuildTVTree(pDoc);

    return 0;

}

/////////////////////////////////////////////////////////////////////////////////
//
//  void CMainFrame::OnUpdateAreaComboBox(CCmdUI* pCmdUI)
//
/////////////////////////////////////////////////////////////////////////////////
void CMainFrame::OnUpdateAreaComboBox(CCmdUI* pCmdUI)
{
    CComboBox* pCombo = (CComboBox*) m_pWndTabTBar->GetDlgItem(ID_AREA_COMBO);
    if (pCombo == NULL) {
        return; // haven't created toolbar yet
    }

    CTableChildWnd* pActTabFrame = DYNAMIC_DOWNCAST(CTableChildWnd, GetActiveFrame());
    if (pActTabFrame == NULL) {
        // not a tab frame so hide
        if (pCombo->IsWindowVisible()) {
            pCombo->ShowWindow(SW_HIDE);
        }
        return;
    }


    bool bTabView = pActTabFrame->IsTabViewActive();
    bool bHasArea = pActTabFrame->GetTabView()->GetGrid()->HasArea();
    bool bShowCombo = (bTabView && bHasArea);

    if (bShowCombo) {
        if (!pCombo->IsWindowVisible()) {
            pCombo->ShowWindow(SW_SHOW);
        }
        pCmdUI->Enable(true); // need to check for presence of %areaname% in columns here
    }
    else {
        if (pCombo->IsWindowVisible()) {
            pCombo->ShowWindow(SW_HIDE);
        }
        pCmdUI->Enable(false);
    }
}

/////////////////////////////////////////////////////////////////////////////////
//
//  void CMainFrame::OnUpdateZoomComboBox(CCmdUI* pCmdUI)
//
/////////////////////////////////////////////////////////////////////////////////
void CMainFrame::OnUpdateZoomComboBox(CCmdUI* pCmdUI)
{
    CComboBox* pCombo = (CComboBox*) m_pWndTabTBar->GetDlgItem(ID_TAB_ZOOM_COMBO);
    if (pCombo == NULL) {
        return; // haven't created toolbar yet
    }

    CTableChildWnd* pActTabFrame = DYNAMIC_DOWNCAST(CTableChildWnd, GetActiveFrame());
    if (pActTabFrame == NULL) {
        // not a tab frame so hide
        if (pCombo->IsWindowVisible()) {
            pCombo->ShowWindow(SW_HIDE);
            // hide zoom buttons and separator on toolbar
            m_pWndTabTBar->GetToolBarCtrl().HideButton(ID_VIEW_ZOOM_IN, TRUE);
            m_pWndTabTBar->GetToolBarCtrl().HideButton(ID_VIEW_ZOOM_OUT, TRUE);
            m_pWndTabTBar->GetToolBarCtrl().HideButton(ID_AREA_COMBO, TRUE);
            m_pWndTabTBar->GetToolBarCtrl().HideButton(ID_PRINTVIEW_CLOSE, TRUE);
            m_printViewCloseButton.ShowWindow(SW_HIDE);
        }
        return;
    }

    bool bTabView = pActTabFrame->IsTabViewActive();
    bool bShowCombo = !bTabView;

    if (bShowCombo) {
        if (!pCombo->IsWindowVisible()) {
            pCombo->ShowWindow(SW_SHOW);
            // show zoom buttons and separator on toolbar
            m_pWndTabTBar->GetToolBarCtrl().HideButton(ID_VIEW_ZOOM_IN, FALSE);
            m_pWndTabTBar->GetToolBarCtrl().HideButton(ID_VIEW_ZOOM_OUT, FALSE);
            m_pWndTabTBar->GetToolBarCtrl().HideButton(ID_AREA_COMBO, FALSE);
            m_pWndTabTBar->GetToolBarCtrl().HideButton(ID_PRINTVIEW_CLOSE, FALSE);
            m_printViewCloseButton.ShowWindow(SW_SHOW);
        }
        pCmdUI->Enable(true);
    }
    else {
        if (pCombo->IsWindowVisible()) {
            pCombo->ShowWindow(SW_HIDE);
            // hide zoom buttons and separator on toolbar
            m_pWndTabTBar->GetToolBarCtrl().HideButton(ID_VIEW_ZOOM_IN, TRUE);
            m_pWndTabTBar->GetToolBarCtrl().HideButton(ID_VIEW_ZOOM_OUT, TRUE);
            m_pWndTabTBar->GetToolBarCtrl().HideButton(ID_AREA_COMBO, TRUE);
            m_printViewCloseButton.ShowWindow(SW_HIDE);
            m_pWndTabTBar->GetToolBarCtrl().HideButton(ID_PRINTVIEW_CLOSE, TRUE);
        }
        pCmdUI->Enable(false);
    }
}
void CMainFrame::OnViewNames()
{

    if (m_SizeDlgBar.m_TblTree.GetViewNames4TblView()) {
        m_SizeDlgBar.m_TblTree.SetViewNames4TblView(false);
//      AfxGetApp()->WriteProfileInt("Settings", "View Names", 0);
    }
    else {
        m_SizeDlgBar.m_TblTree.SetViewNames4TblView(true);
  //    AfxGetApp()->WriteProfileInt("Settings", "View Names", 1);
    }
    // Update trees
    m_SizeDlgBar.m_TblTree.Invalidate();
}
void CMainFrame::OnUpdateViewNames(CCmdUI* pCmdUI)
{
    pCmdUI->SetCheck(m_SizeDlgBar.m_TblTree.GetViewNames4TblView());
    pCmdUI->Enable(TRUE);
}

void CMainFrame::OnUpdateQuickQuit(CCmdUI* pCmdUI)
{
    pCmdUI->Enable(TRUE);
}
/////////////////////////////////////////////////////////////////////////////
//
//                             CMainFrame::TextViewMenu
//
/////////////////////////////////////////////////////////////////////////////

HMENU CMainFrame::TblViewMenu()
{
    static UINT toolbars[]={
        IDR_MAINFRAME,
        IDR_TBLVWERVW_FRAME
    };
    m_TblViewMenu.LoadMenu(IDR_TBLVWERVW_FRAME);
    m_TblViewMenu.LoadToolbars(toolbars, 2);

    return(m_TblViewMenu.Detach());
}


/////////////////////////////////////////////////////////////////////////////
//
//                             CMainFrame::DefaultMenu
//
/////////////////////////////////////////////////////////////////////////////

HMENU CMainFrame::DefaultMenu()
{
    m_DefaultMenu.LoadMenu(IDR_MAINFRAME);
    m_DefaultMenu.LoadToolbar(IDR_MAINFRAME);

    return(m_DefaultMenu.Detach());
}


/////////////////////////////////////////////////////////////////////////////
//
//                             CMainFrame::OnMenuChar
//
/////////////////////////////////////////////////////////////////////////////

LRESULT CMainFrame::OnMenuChar(UINT nChar, UINT nFlags, CMenu* pMenu)
{
    LRESULT lresult;
    if(BCMenu::IsMenu(pMenu)) {
        lresult=BCMenu::FindKeyboardShortcut(nChar, nFlags, pMenu);
    }
    else {
        lresult=CMDIFrameWnd::OnMenuChar(nChar, nFlags, pMenu);
    }
    return(lresult);
}


/////////////////////////////////////////////////////////////////////////////
//
//                             CMainFrame::OnDDEExecute
// This is here to work around a known bug in MFC 7.1.
// See http://www.pcreview.co.uk/forums/thread-1428556.php
// We should remove this when the bug is fixed (Microsoft claims
// there will be a fix in VC++ 2003 SP 1).
// This is a cut n paste of CFrameWnd::OnDDEExecute from MFC source
// code plus added in one line (see below) to make it work correctly.
/////////////////////////////////////////////////////////////////////////////
LRESULT CMainFrame::OnDDEExecute(WPARAM wParam, LPARAM lParam)
{
    // unpack the DDE message
   UINT_PTR unused;
    HGLOBAL hData;
   //IA64: Assume DDE LPARAMs are still 32-bit
    VERIFY(UnpackDDElParam(WM_DDE_EXECUTE, lParam, &unused, (UINT_PTR*)&hData));

    // get the command string
    TCHAR szCommand[_MAX_PATH * 2];
    LPCTSTR lpsz = (LPCTSTR)GlobalLock(hData);
    lstrcpyn(szCommand, lpsz, _countof(szCommand));  // added this in, missing from MFC source
    int commandLength = lstrlen(lpsz);
    if (commandLength >= _countof(szCommand))
    {
        // The command would be truncated. This could be a security problem
        TRACE0("Warning: Command was ignored because it was too long.\n");
        return 0;
    }
    GlobalUnlock(hData);

    // acknowledge now - before attempting to execute
    ::PostMessage((HWND)wParam, WM_DDE_ACK, (WPARAM)m_hWnd,
      //IA64: Assume DDE LPARAMs are still 32-bit
        ReuseDDElParam(lParam, WM_DDE_EXECUTE, WM_DDE_ACK,
        (UINT)0x8000, (UINT_PTR)hData));

    // don't execute the command when the window is disabled
    if (!IsWindowEnabled())
    {
        TRACE(traceAppMsg, 0, _T("Warning: DDE command '%s' ignored because window is disabled.\n"),
            szCommand);
        return 0;
    }

    // execute the command
    if (!AfxGetApp()->OnDDECommand(szCommand))
        TRACE(traceAppMsg, 0, _T("Error: failed to execute DDE command '%s'.\n"), szCommand);

    return 0;
}

/////////////////////////////////////////////////////////////////////////////
//
//                             CMainFrame::OnIMSAFileOpen
//
/////////////////////////////////////////////////////////////////////////////

LONG CMainFrame::OnIMSAFileOpen (UINT, LPARAM)  {

//    This function responds to the message WM_IMSA_FILEOPEN, which is sent by other
//    IMPS 40 modules to invoke a file to be viewed.


      /*--- activate ourselves  ---*/
    SendMessage(WM_IMSA_SETFOCUS);

    CString csFileName, csTemp;
    CString csWndClass = ((CTblViewApp*)AfxGetApp())->m_csWndClassName;

    if (!IMSAOpenSharedFile(csFileName))  {
        csTemp = csWndClass + _T(": internal error receiving inter-app file open message");
        AfxMessageBox(csTemp, MB_OK|MB_ICONSTOP);
        return 0;
    }

    csTemp = csWndClass + _T(" -- CIMPSViewerMainFrame::OnIMPS40FileOpen x%sx\n");
    TRACE(csTemp, (const TCHAR*) csFileName);

    CFileStatus status;
    // GSF 25/08/00 file name comes in quoted, because of long file names
    csFileName.TrimLeft(_TCHAR('"'));
    csFileName.TrimRight(_TCHAR('"'));
    if (!CFile::GetStatus(csFileName, status))  {
        CString cs;
        cs = _T("File not found:  ") + csFileName;
        AfxMessageBox(cs, MB_OK|MB_ICONSTOP);
    }
    else  {
        CTblViewApp* pApp = (CTblViewApp*) AfxGetApp();

        // see if the document is already open; if so, then close and reload it
        CTblViewDoc* pDoubleDoc = pApp->FindFile(csFileName);
        if (pDoubleDoc)  {
            ASSERT_VALID(pDoubleDoc);
            TRACE(_T("CIMPSViewerMainFrame::OnIMPS40FileOpen - Closing duplicate document %p, %s %s\n"), pDoubleDoc, (const TCHAR*) pDoubleDoc->GetTitle(), (const TCHAR*) pDoubleDoc->GetPathName());
            pDoubleDoc->OnCloseDocument();
        }

        // open the document
        pApp->OpenDocumentFile(csFileName);

    }

    /*--- enable Esc key for quitting, activate the "quick quit" button, and redraw the tool bar  ---*/
    ((CTblViewApp*) AfxGetApp())->m_bCalledAsChild = TRUE;
    m_wndToolBar.GetToolBarCtrl().HideButton(ID_QUICK_QUIT, FALSE);  // show the button
    ShowControlBar(&m_wndToolBar, TRUE, FALSE);

    return 0;
}

/////////////////////////////////////////////////////////////////////////////
//
//                             CMainFrame::OnIMSAFileClose
//
/////////////////////////////////////////////////////////////////////////////

LONG CMainFrame::OnIMSAFileClose (UINT, LPARAM)  {

//    This function responds to the message WM_IMPS40_FILECLOSE which is sent by other
//    IMPS 40 modules to cause us to close a file (if we've got it opened!)


    CString csWndClass = ((CTblViewApp*)AfxGetApp())->m_csWndClassName;
    CString csFileName, csTemp;

    if (!IMSAOpenSharedFile(csFileName))  {
        csTemp = csWndClass + _T(": internal error receiving inter-app file open message");
        AfxMessageBox(csTemp, MB_OK|MB_ICONSTOP);
        return 0;
    }

    csTemp = csWndClass + _T(" -- CIMPSViewerMainFrame::OnIMPS40FileClose x%sx\n");
    TRACE(csTemp, (const TCHAR*) csFileName);

    // close the document
    CTblViewDoc* pDoubleDoc = ((CTblViewApp*) AfxGetApp())->FindFile(csFileName);

    // savy/gsf 8/28/00:  if file not open in textview, don't crash!
    if (pDoubleDoc == NULL) {
        return 0;
    }

    ASSERT_VALID(pDoubleDoc);
    pDoubleDoc->OnCloseDocument();
    return 0;
}


/////////////////////////////////////////////////////////////////////////////
//
//                             CMainFrame::OnIMSASetFocus
//
/////////////////////////////////////////////////////////////////////////////

LONG CMainFrame::OnIMSASetFocus (UINT, LPARAM)  {

    CString csWndClass = ((CTblViewApp*)AfxGetApp())->m_csWndClassName;
    CString csFileName, csTemp;

    csTemp = csWndClass + _T("WM_IMPS40_SETFOCUS message received\n");
    TRACE(csTemp, (const TCHAR*) csFileName);

    SetForegroundWindow();  // win32
    WINDOWPLACEMENT wndpl;
    wndpl.length = sizeof(WINDOWPLACEMENT);
    GetWindowPlacement(&wndpl);
    ActivateFrame(wndpl.showCmd);  // Win32s

    CMDIChildWnd* pChild = MDIGetActive();
    if (pChild != NULL)  {
        ASSERT_KINDOF(CMDIChildWnd, pChild);
        CView* pView = pChild->GetActiveView();
        //ASSERT(pView != NULL);
        if(pView){
            pView->SetFocus();
        }
    }
    return 0;
}


/////////////////////////////////////////////////////////////////////////////
//
//                             CMainFrame::OnQuickQuit
//
/////////////////////////////////////////////////////////////////////////////

void CMainFrame::OnQuickQuit() {

    if (((CTblViewApp*)(AfxGetApp()))->m_bCalledAsChild) {
        SendMessage(WM_COMMAND, ID_APP_EXIT);
    }
}

