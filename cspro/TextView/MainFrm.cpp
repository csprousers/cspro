//***************************************************************************
//  File name: MainFrm.cpp
//
//  Description:
//       MFC main frame class
//
//  History:    Date       Author     Comment
//              -----------------------------
//              1995        csc       created
//              4  Jan 03   csc       support for font size changing
//
//***************************************************************************
// MainFrm.cpp : implementation of the CMainFrame class
//

#include "StdAfx.h"
#include <zUtilO/CSProExecutables.h>

// for OnDDEExecute below
#include <Dde.h>
//#include <afxisapi.h> -SAVY VS2010 upgrade


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

const CRect NEAR CMainFrame::rectDefault(10, 10, 500, 400);  // static

/////////////////////////////////////////////////////////////////////////////
// CMainFrame

IMPLEMENT_DYNAMIC(CMainFrame, CMDIFrameWnd)

BEGIN_MESSAGE_MAP(CMainFrame, CMDIFrameWnd)
    //{{AFX_MSG_MAP(CMainFrame)
    ON_WM_CREATE()
    ON_COMMAND(ID_VIEW_RULER, OnViewRuler)
    ON_UPDATE_COMMAND_UI(ID_FILE_OPEN_IN_DATA_VIEWER, OnUpdateOpenInDataViewer)
    ON_COMMAND(ID_FILE_OPEN_IN_DATA_VIEWER, OnOpenInDataViewer)
    ON_UPDATE_COMMAND_UI(ID_OPTIONS_COMMAS, OnUpdateOptionsCommas)
    ON_COMMAND(ID_OPTIONS_COMMAS, OnOptionsCommas)
    ON_WM_DESTROY()
    ON_COMMAND(ID_QUICK_QUIT, OnQuickQuit)
    ON_WM_MENUCHAR()
    ON_COMMAND(ID_OPTIONS_LINEDRAW, OnOptionsLinedraw)
    ON_UPDATE_COMMAND_UI(ID_OPTIONS_LINEDRAW, OnUpdateOptionsLinedraw)
    ON_COMMAND(ID_FORMAT_FONT, OnFormatFont)
    ON_COMMAND(ID_WINDOW_NEXT, OnWindowNext)
    ON_UPDATE_COMMAND_UI(ID_WINDOW_NEXT, OnUpdateWindowNext)
    ON_COMMAND(ID_WINDOW_PREV, OnWindowPrev)
    ON_UPDATE_COMMAND_UI(ID_WINDOW_PREV, OnUpdateWindowPrev)
    ON_UPDATE_COMMAND_UI(ID_INDICATOR_OVR, OnUpdateKeyOvr)
    ON_WM_ACTIVATE()
    //}}AFX_MSG_MAP
    ON_MESSAGE(WM_IMSA_FILEOPEN, OnIMSAFileOpen)
    ON_MESSAGE(WM_IMSA_FILECLOSE, OnIMSAFileClose)
    ON_MESSAGE(WM_IMSA_SETFOCUS, OnIMSASetFocus)
    // Global help commands
    ON_COMMAND(ID_HELP_FINDER, OnHelpFinder)
    ON_COMMAND(ID_HELP, OnHelp)
    ON_COMMAND(ID_CONTEXT_HELP, OnContextHelp)
    ON_COMMAND(ID_DEFAULT_HELP, OnHelpFinder)
    ON_MESSAGE(WM_DDE_EXECUTE, OnDDEExecute)
END_MESSAGE_MAP()

static UINT indicators[] =
{
    ID_SEPARATOR,           // menu helps
    ID_SEPARATOR,           // blocking coordinates
    ID_INDICATOR_NUM,       // num lock status
    ID_INDICATOR_CAPS,      // caps lock status
    ID_INDICATOR_OVR,       // insert status (for consistency)
    ID_SEPARATOR,           // file encoding
    ID_SEPARATOR,           // file size
    ID_SEPARATOR            // current position in document
};

int CMainFrame::GetFontHeight(int iPointSize) const
{
    CClientDC dc((CWnd*)this);
    return -MulDiv (iPointSize, dc.GetDeviceCaps(LOGPIXELSY), 72);
}


/////////////////////////////////////////////////////////////////////////////
// CMainFrame construction/destruction

CMainFrame::CMainFrame()
{
    m_bFirstTime = TRUE;
    m_pszClassName = NULL;
}

CMainFrame::~CMainFrame()
{
}


/////////////////////////////////////////////////////////////////////////////
//
//                             CMainFrame::OnCreate
//
/////////////////////////////////////////////////////////////////////////////

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct) {

    if (CMDIFrameWnd::OnCreate(lpCreateStruct) == -1)
        return -1;

    if (!m_wndToolBar.CreateEx(this) ||
        !m_wndToolBar.LoadToolBar(IDR_MAINFRAME))
    {
        TRACE0("Failed to create toolbar\n");
        return -1;      // fail to create
    }

    if (!m_wndReBar.Create(this) ||
        !m_wndReBar.AddBar(&m_wndToolBar))
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

    m_wndToolBar.SetBarStyle(m_wndToolBar.GetBarStyle() | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC);
    m_wndToolBar.EnableDocking(CBRS_ALIGN_ANY);

    EnableDocking(CBRS_ALIGN_ANY);

    if (!((CTextViewApp*) AfxGetApp())->m_bCalledAsChild)  {
        // we are *not* dependent ... hide the quick quit toolbar button
        m_wndToolBar.GetToolBarCtrl().HideButton(ID_QUICK_QUIT);
    }
     m_bLineDraw = AfxGetApp()->GetProfileInt(_T("Options"), _T("LineDraw"), FALSE);

    // csc 4 jan 03 start section ...
    m_iPointSize = AfxGetApp()->GetProfileInt(_T("Options"),_T("FontSize"), 8);
    int iFontHeight = GetFontHeight(m_iPointSize);
    AfxGetApp()->WriteProfileInt(_T("Options"), _T("LineDraw"), m_bLineDraw); //Init the linedraw font

    if (m_bLineDraw) {
        //do not create the line draw font. now the line draw characters are all unicode
        m_Font.CreateFont (iFontHeight, 0, 0, 0, 400, FALSE, FALSE, 0, DEFAULT_CHARSET,
                            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                            DEFAULT_QUALITY, FF_DONTCARE,
                            _T("Courier New"));
    }
    else {
        m_Font.CreateFont (iFontHeight, 0, 0, 0, 400, FALSE, FALSE, 0, DEFAULT_CHARSET,
                            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                            DEFAULT_QUALITY, FF_DONTCARE,
                            _T("Courier New"));
    }
        // csc 4 jan 03 ... end section
    return 0;
}


/////////////////////////////////////////////////////////////////////////////
//
//                             CMainFrame::PreCreateWindow
//
/////////////////////////////////////////////////////////////////////////////

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs) {

    if(!CMDIFrameWnd::PreCreateWindow(cs)) {
        return FALSE;
    }
    if (m_pszClassName == NULL)    {
        WNDCLASS wndcls;
        ::GetClassInfo(AfxGetInstanceHandle(), cs.lpszClass, &wndcls);
        wndcls.hIcon = ((CTextViewApp*)AfxGetApp())->m_hIcon;
        CString csClassName = ((CTextViewApp*)AfxGetApp())->m_csWndClassName;
        wndcls.lpszClassName = csClassName;
        VERIFY(AfxRegisterClass(&wndcls));
        m_pszClassName = csClassName;
    }
    cs.lpszClass = m_pszClassName;
    return TRUE;
}



/////////////////////////////////////////////////////////////////////////////
//
//                             CMainFrame::diagnostics
//
/////////////////////////////////////////////////////////////////////////////

#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
    CMDIFrameWnd::AssertValid();
}

void CMainFrame::Dump(CDumpContext& dc) const
{
    CMDIFrameWnd::Dump(dc);
}

#endif //_DEBUG


/////////////////////////////////////////////////////////////////////////////
//
//                             CMainFrame::UpdateStatusBarScr
//
/////////////////////////////////////////////////////////////////////////////

void CMainFrame::UpdateStatusBarScr (CLPoint ptlCurrPos) {

    CStatusBar* pStatus = (CStatusBar*) GetDescendantWindow (AFX_IDW_STATUS_BAR);
    TCHAR pszStr[40];
    if (pStatus)  {
        CDC* pDC = pStatus->GetDC();
        pDC->SelectObject(pStatus->GetFont()); // GHM 20120207 the text extent isn't correct without this statement
        CString csTitle;
        csTitle.LoadString (IDS_MSG01);
        wsprintf (pszStr, _T("%s: (%ld,%ld)"), (const TCHAR*) csTitle, ptlCurrPos.y, ptlCurrPos.x);
        pStatus->SetPaneInfo (7, indicators[7], SBPS_NORMAL, pDC->GetTextExtent (pszStr, _tcslen(pszStr)).cx+5);
        pStatus->ReleaseDC (pDC);
        pStatus->SetPaneText (7, pszStr);
        pStatus->UpdateWindow ();
    }
}


/////////////////////////////////////////////////////////////////////////////
//
//                             CMainFrame::UpdateStatusBarBlock
//
/////////////////////////////////////////////////////////////////////////////

void CMainFrame::UpdateStatusBarBlock (CLPoint ptlOrigin, BOOL bActive) {

    CStatusBar* pStatus = (CStatusBar*) GetDescendantWindow (AFX_IDW_STATUS_BAR);
    TCHAR pszStr[40];
    if (pStatus)  {
        CString csTitle;
        csTitle.LoadString (IDS_MSG02);
        if ( bActive )  {
            wsprintf (pszStr, _T("%s: (%ld,%ld)"), (const TCHAR*) csTitle, ptlOrigin.y, ptlOrigin.x);
        }
        else  {
            wsprintf (pszStr, _T("%s: (none)"), (const TCHAR*) csTitle);
        }
        CDC* pDC = pStatus->GetDC();
        pDC->SelectObject(pStatus->GetFont()); // GHM 20120207 the text extent isn't correct without this statement
        pStatus->SetPaneInfo (1, indicators[1], SBPS_NORMAL, pDC->GetTextExtent (pszStr, _tcslen(pszStr)).cx+5);
        pStatus->SetPaneText (1,pszStr);
        pStatus->ReleaseDC (pDC);
    }
}


/////////////////////////////////////////////////////////////////////////////
//
//                             CMainFrame::UpdateStatusBarSize
//
/////////////////////////////////////////////////////////////////////////////

void CMainFrame::UpdateStatusBarSize (const TCHAR* pszStr) {

    CStatusBar* pStatus = (CStatusBar*) GetDescendantWindow (AFX_IDW_STATUS_BAR);
    if (pStatus)  {
        CDC* pDC = pStatus->GetDC();
        pDC->SelectObject(pStatus->GetFont()); // GHM 20120207 the text extent isn't correct without this statement
        pStatus->SetPaneInfo (6, indicators[6], SBPS_NORMAL, pDC->GetTextExtent (pszStr, _tcslen(pszStr)).cx+5);
        pStatus->SetPaneText (6, pszStr);
        pStatus->ReleaseDC (pDC);
    }
}


/////////////////////////////////////////////////////////////////////////////
//
//                             CMainFrame::UpdateStatusBarEncoding
//
/////////////////////////////////////////////////////////////////////////////


void CMainFrame::UpdateStatusBarEncoding(const TCHAR* pszStr) // GHM 20111222
{
    CStatusBar* pStatus = (CStatusBar*) GetDescendantWindow (AFX_IDW_STATUS_BAR);
    if( pStatus )
    {
        CDC * pDC = pStatus->GetDC();
        pDC->SelectObject(pStatus->GetFont()); // GHM 20120207 the text extent isn't correct without this statement
        pStatus->SetPaneInfo(5,indicators[5],SBPS_NORMAL,pDC->GetTextExtent(pszStr,_tcslen(pszStr)).cx+5);
        pStatus->SetPaneText(5,pszStr);
        pStatus->ReleaseDC(pDC);
    }
}


/////////////////////////////////////////////////////////////////////////////
//
//                             CMainFrame::OnUpdateKeyOvr
//
/////////////////////////////////////////////////////////////////////////////

void CMainFrame::OnUpdateKeyOvr (CCmdUI* pCmdUI) {

    pCmdUI->Enable (::GetKeyState (VK_INSERT) % 2 == 0);
}


/////////////////////////////////////////////////////////////////////////////
//
//                             CMainFrame::OnViewRuler
//
/////////////////////////////////////////////////////////////////////////////

void CMainFrame::OnViewRuler()  {

    BOOL bRulers = AfxGetApp()->GetProfileInt(_T("Ruler"), _T("Toggle"), 0);
    CTextViewApp* pApp = (CTextViewApp*)AfxGetApp();
    POSITION pos = pApp->GetFirstDocTemplatePosition();
    while (pos)  {
        CDocTemplate* pTemplate = pApp->GetNextDocTemplate(pos);
        POSITION pos2 = pTemplate->GetFirstDocPosition();
        while (pos2)  {
            CDocument* pDocument;
            if ((pDocument=pTemplate->GetNextDoc(pos2)) != NULL)  {
                POSITION posView = pDocument->GetFirstViewPosition();
                while (posView != NULL)  {
                    CView* pView = pDocument->GetNextView(posView);
                    pView->SendMessage(WM_COMMAND, UWM::TextView::ToggleRuler);
                }
            }
        }
    }
    AfxGetApp()->WriteProfileInt(_T("Ruler"), _T("Toggle"), !bRulers);
}


/////////////////////////////////////////////////////////////////////////////
//
//                             CMainFrame::OnUpdateOptions
//
/////////////////////////////////////////////////////////////////////////////

void CMainFrame::OnUpdateOptionsCommas(CCmdUI* pCmdUI) {

    m_bKeepCommas = AfxGetApp()->GetProfileInt(_T("Options"), _T("Commas"), FALSE);
    pCmdUI->SetCheck(m_bKeepCommas);
}


/////////////////////////////////////////////////////////////////////////////
//
//                             CMainFrame::OnOptionsCommas
//
/////////////////////////////////////////////////////////////////////////////

void CMainFrame::OnOptionsCommas() {

    if (m_bKeepCommas) {
        m_bKeepCommas = FALSE;
    }
    else {
        m_bKeepCommas = TRUE;
    }
    AfxGetApp()->WriteProfileInt(_T("Options"), _T("Commas"), m_bKeepCommas);
}


/////////////////////////////////////////////////////////////////////////////
//
//                             CMainFrame::LoadFrame
//
/////////////////////////////////////////////////////////////////////////////

BOOL CMainFrame::LoadFrame(UINT nIDResource, DWORD dwDefaultStyle, CWnd* pParentWnd, CCreateContext* pContext) {

    CWnd*           pBar;
    CString         csText;
    BOOL            bIconic = FALSE, bMaximized = FALSE, bTool, bStatus, bRetVal;
    UINT            flags = 0;
    WINDOWPLACEMENT wndpl;
    CRect           rect;
    int             nCmdShow;

    bRetVal = CMDIFrameWnd::LoadFrame(nIDResource, dwDefaultStyle, pParentWnd, pContext);
    ASSERT (bRetVal);

    if (m_bFirstTime) {
        m_bFirstTime = FALSE;

        /////////////////////////////////////////////////////////////////////
        // window placement from .ini file
        csText = AfxGetApp()->GetProfileString(INI_SECTION_WINDOWSIZE, INI_KEY_RECT);
        if (!csText.IsEmpty()) {
            // can't use sscanf in a DLL
            rect.left = _ttoi((const TCHAR*) csText);
            rect.top = _ttoi((const TCHAR*) csText + 5);
            rect.right = _ttoi((const TCHAR*) csText + 10);
            rect.bottom = _ttoi((const TCHAR*) csText + 15);
        }
        else {
            rect = rectDefault;
        }

        bIconic = AfxGetApp()->GetProfileInt(INI_SECTION_WINDOWSIZE, INI_KEY_ICON, 0);
        bMaximized = AfxGetApp()->GetProfileInt(INI_SECTION_WINDOWSIZE, INI_KEY_MAX, 0);

        if (bIconic) {
            nCmdShow = SW_SHOWMINNOACTIVE;
            if (bMaximized) {
                flags = WPF_RESTORETOMAXIMIZED;
            }
        }
        else {
          if (bMaximized) {
              nCmdShow = SW_SHOWMAXIMIZED;
              flags = WPF_RESTORETOMAXIMIZED;
          }
          else {
              nCmdShow = SW_NORMAL;
              flags = 0;
          }
        }
        wndpl.length = sizeof(WINDOWPLACEMENT);
        wndpl.showCmd = nCmdShow;
        wndpl.flags = flags;
        wndpl.ptMinPosition = CPoint(0, 0);
        wndpl.ptMaxPosition = CPoint(-::GetSystemMetrics(SM_CXBORDER), -::GetSystemMetrics(SM_CYBORDER));
        wndpl.rcNormalPosition = rect;

        bTool = AfxGetApp()->GetProfileInt(INI_SECTION_WINDOWSIZE, INI_KEY_TOOL, 1);
        if ((pBar = GetDescendantWindow(AFX_IDW_TOOLBAR)) != NULL) {
            pBar->ShowWindow(bTool);
        }
        bStatus = AfxGetApp()->GetProfileInt(INI_SECTION_WINDOWSIZE, INI_KEY_STATUS, 1);
        if ((pBar = GetDescendantWindow(AFX_IDW_STATUS_BAR)) != NULL) {
            pBar->ShowWindow(bStatus);
        }
        // sets window's position and iconized/maximized status
        SetWindowPlacement(&wndpl);
    }
    return bRetVal;
}


/////////////////////////////////////////////////////////////////////////////
//
//                             CMainFrame::OnDestroy
//
/////////////////////////////////////////////////////////////////////////////

void CMainFrame::OnDestroy() {

    CString csText, csTemp;
    CWnd*   pBar;
    BOOL    bIconic = FALSE, bMaximized = FALSE;

    /////////////////////////////////////////////////////////////////////
    // window placement to .ini file
    WINDOWPLACEMENT wndpl;
    wndpl.length = sizeof(WINDOWPLACEMENT);
    // gets current window position and iconized/maximized status
    GetWindowPlacement(&wndpl);
    if (wndpl.showCmd == SW_SHOWNORMAL) {
        bIconic = FALSE;
        bMaximized = FALSE;
    }
    else if (wndpl.showCmd == SW_SHOWMAXIMIZED) {
        bIconic = FALSE;
        bMaximized = TRUE;
    }
    else if (wndpl.showCmd == SW_SHOWMINIMIZED) {
        bIconic = TRUE;
        if (wndpl.flags) {
            bMaximized = TRUE;
        }
        else {
            bMaximized = FALSE;
        }
    }
    wsprintf(csText.GetBuffer(20), _T("%04d %04d %04d %04d"), wndpl.rcNormalPosition.left,
             wndpl.rcNormalPosition.top, wndpl.rcNormalPosition.right, wndpl.rcNormalPosition.bottom);

    csText.ReleaseBuffer();
    AfxGetApp()->WriteProfileString(INI_SECTION_WINDOWSIZE, INI_KEY_RECT, csText);
    AfxGetApp()->WriteProfileInt(INI_SECTION_WINDOWSIZE, INI_KEY_ICON, bIconic);
    AfxGetApp()->WriteProfileInt(INI_SECTION_WINDOWSIZE, INI_KEY_MAX, bMaximized);
    if ( (pBar = GetDescendantWindow(AFX_IDW_TOOLBAR)) != NULL) {
        AfxGetApp()->WriteProfileInt(INI_SECTION_WINDOWSIZE, INI_KEY_TOOL, (pBar->GetStyle() & WS_VISIBLE) != 0L);
    }
    if ((pBar = GetDescendantWindow(AFX_IDW_STATUS_BAR)) != NULL) {
        AfxGetApp()->WriteProfileInt(INI_SECTION_WINDOWSIZE, INI_KEY_STATUS, (pBar->GetStyle() & WS_VISIBLE) != 0L);
    }

    CMDIFrameWnd::OnDestroy();
}


/////////////////////////////////////////////////////////////////////////////
//
//                             CMainFrame::OnQuickQuit
//
/////////////////////////////////////////////////////////////////////////////

void CMainFrame::OnQuickQuit() {

    if (((CTextViewApp*) AfxGetApp())->m_bCalledAsChild) {
        SendMessage(WM_COMMAND, ID_APP_EXIT);
    }
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

    CString csFileName;
    CString csTemp;
    CString csWndClass = ((CTextViewApp*)AfxGetApp())->m_csWndClassName;

    if (!IMSAOpenSharedFile(csFileName))  {
        csTemp = csWndClass + _T(": internal error receiving inter-app file open message");
        AfxMessageBox(csTemp, MB_OK|MB_ICONSTOP);
        return 0L;
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
        CTextViewApp* pApp = (CTextViewApp*) AfxGetApp();

        // see if the document is already open; if so, then close and reload it
        CTVDoc* pDoubleDoc = pApp->FindFile(csFileName);
        if (pDoubleDoc && !pDoubleDoc->IsReloadingOrClosing())  {
            ASSERT_VALID(pDoubleDoc);
            TRACE(_T("CIMPSViewerMainFrame::OnIMPS40FileOpen - Closing duplicate document %p, %s %s\n"), pDoubleDoc, (const TCHAR*) pDoubleDoc->GetTitle(), (const TCHAR*) pDoubleDoc->GetPathName());
            pDoubleDoc->OnCloseDocument();
        }

        // open the document
        if( OpenFilesOnStartupManager.open_file_directly ) {
            pApp->OpenDocumentFile(csFileName);
        }

        else {
            std::lock_guard<std::mutex> lock(OpenFilesOnStartupManager.filenames_mutex);
            OpenFilesOnStartupManager.filenames.emplace_back(csFileName);
        }
    }

    /*--- enable Esc key for quitting, activate the "quick quit" button, and redraw the tool bar  ---*/
    ((CTextViewApp*) AfxGetApp())->m_bCalledAsChild = TRUE;
    m_wndToolBar.GetToolBarCtrl().HideButton(ID_QUICK_QUIT, FALSE);  // show the button
    ShowControlBar(&m_wndToolBar, TRUE, FALSE);

    return 0L;
}


/////////////////////////////////////////////////////////////////////////////
//
//                             CMainFrame::OnIMSAFileClose
//
/////////////////////////////////////////////////////////////////////////////

LONG CMainFrame::OnIMSAFileClose (UINT, LPARAM)  {

//    This function responds to the message WM_IMPS40_FILECLOSE which is sent by other
//    IMPS 40 modules to cause us to close a file (if we've got it opened!)


    CString csWndClass = ((CTextViewApp*)AfxGetApp())->m_csWndClassName;
    CString csFileName;
    CString csTemp;

    if (!IMSAOpenSharedFile(csFileName))  {
        csTemp = csWndClass + _T(": internal error receiving inter-app file open message");
        AfxMessageBox(csTemp, MB_OK|MB_ICONSTOP);
        return 0L;
    }

    csTemp = csWndClass + _T(" -- CIMPSViewerMainFrame::OnIMPS40FileClose x%sx\n");
    TRACE(csTemp, (const TCHAR*) csFileName);

    // close the document
    CTVDoc* pDoubleDoc = ((CTextViewApp*) AfxGetApp())->FindFile(csFileName);

    // savy/gsf 8/28/00:  if file not open in textview, don't crash!
    if (pDoubleDoc == NULL) {
        return 0L;
    }

    ASSERT_VALID(pDoubleDoc);
    if (!pDoubleDoc->IsReloadingOrClosing()) {
        pDoubleDoc->OnCloseDocument();
    }
    return 0L;
}


/////////////////////////////////////////////////////////////////////////////
//
//                             CMainFrame::OnIMSASetFocus
//
/////////////////////////////////////////////////////////////////////////////

LONG CMainFrame::OnIMSASetFocus (UINT, LPARAM)  {

    CString csWndClass = ((CTextViewApp*)AfxGetApp())->m_csWndClassName;
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
        ASSERT(pView != NULL);
        pView->SetFocus();
    }
    return 0L;
}


/////////////////////////////////////////////////////////////////////////////
//
//                             CMainFrame::TextViewMenu
//
/////////////////////////////////////////////////////////////////////////////

HMENU CMainFrame::TextViewMenu()
{
    static UINT toolbars[]={
        IDR_MAINFRAME,
        IDR_VIEWTYPE
    };
    m_TextViewMenu.LoadMenu(IDR_VIEWTYPE);
    m_TextViewMenu.LoadToolbars(toolbars, 2);

    return(m_TextViewMenu.Detach());
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
//                             CMainFrame::OnOptionsLinedraw
//
//  Revision history;
//     4 Jan 03     csc       created
//
/////////////////////////////////////////////////////////////////////////////
void CMainFrame::OnOptionsLinedraw()
{
    // Update the switch
    if (m_bLineDraw) {
        m_bLineDraw = FALSE;
    }
    else {
        m_bLineDraw = TRUE;
    }
    AfxGetApp()->WriteProfileInt(_T("Options"), _T("LineDraw"), m_bLineDraw);

    // nuke the current font...
    m_Font.DeleteObject();

    // recreate the font ... csc 4 jan 03
    int iFontHeight = GetFontHeight(m_iPointSize);
    if (m_bLineDraw)  {
        //do not create the line draw font. now the line draw characters are all unicode
           m_Font.CreateFont (iFontHeight, 0, 0, 0, 400, FALSE, FALSE, 0, DEFAULT_CHARSET,
                        OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                        DEFAULT_QUALITY, FF_DONTCARE,
                        _T("Courier New"));
    }
    else {
        m_Font.CreateFont (iFontHeight, 0, 0, 0, 400, FALSE, FALSE, 0, DEFAULT_CHARSET,
                        OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                        DEFAULT_QUALITY, FF_DONTCARE,
                        _T("Courier New"));
    }

    // Redo all views
    ((CTextViewApp*) AfxGetApp())->UpdateAllAppViews(HINT_CHANGEFONT);
}


/////////////////////////////////////////////////////////////////////////////
//
//                             CMainFrame::OnUpdateOptionsLinedraw
//
/////////////////////////////////////////////////////////////////////////////
void CMainFrame::OnUpdateOptionsLinedraw(CCmdUI* pCmdUI)
{
    CMDIChildWnd* pChild = MDIGetActive();
    if (pChild != NULL)  {
        ASSERT_KINDOF(CMDIChildWnd, pChild);
        CView* pView = pChild->GetActiveView();
        if(pView){
            CTVDoc* pDoc = DYNAMIC_DOWNCAST(CTVDoc,pView->GetDocument());
            if(pDoc){
                Encoding currEncoding = pDoc->GetBufferMgr()->GetFileIO()->GetEncoding();
                if(currEncoding != Encoding::Ansi){
                    pCmdUI->Enable(FALSE);
                    return;
                }
            }

        }
    }

    m_bLineDraw = AfxGetApp()->GetProfileInt(_T("Options"), _T("LineDraw"), FALSE);
    pCmdUI->SetCheck(m_bLineDraw);
}


/////////////////////////////////////////////////////////////////////////////
//
//                             CMainFrame::OnFormatFont
//
//  Revision history;
//     4 Jan 03     csc       created
//
/////////////////////////////////////////////////////////////////////////////
void CMainFrame::OnFormatFont()
{
    // request the user's preferred point size ...
    CFontSizeDlg dlg;

    dlg.m_csSize.Str(m_iPointSize);

    if (dlg.DoModal()==IDOK)  {
        // convert the user's point size to LOGFONT height, so that we can use it
        LOGFONT lf;
        m_Font.GetLogFont(&lf);
        CClientDC dc(this);
        lf.lfHeight = -MulDiv ((int)dlg.m_csSize.Val(), dc.GetDeviceCaps(LOGPIXELSY), 72);
        m_Font.DeleteObject();
        m_Font.CreateFontIndirect(&lf);
        m_iPointSize = (int)dlg.m_csSize.Val();
        AfxGetApp()->WriteProfileInt(_T("Options"),_T("FontSize"), m_iPointSize);

        // Redo all views
        ((CTextViewApp*) AfxGetApp())->UpdateAllAppViews(HINT_CHANGEFONT);
    }

// Note to developers:
//   in the future, you might want to allow other font types to be allowed.  Try using this:
//       LOGFONT lf;
//       m_Font.GetLogFont(&lf);
//       CFontDialog dlg(&lf, CF_FIXEDPITCHONLY | CF_SCREENFONTS);
//       dlg.DoModal();
//

}

void CMainFrame::OnWindowNext()
{
    MDIGetActive()->SendMessage(WM_SYSCOMMAND,SC_NEXTWINDOW);
}

void CMainFrame::OnUpdateWindowNext(CCmdUI* pCmdUI)
{
    if (MDIGetActive() != NULL) {
        pCmdUI->Enable(TRUE);
    }
    else {
        pCmdUI->Enable(FALSE);
    }
}

void CMainFrame::OnWindowPrev()
{
    MDIGetActive()->SendMessage(WM_SYSCOMMAND,SC_PREVWINDOW);
}

void CMainFrame::OnUpdateWindowPrev(CCmdUI* pCmdUI)
{
    if (MDIGetActive() != NULL) {
        pCmdUI->Enable(TRUE);
    }
    else {
        pCmdUI->Enable(FALSE);
    }
}


////////////////////////////////////////////////////////////////
//
//     CMainFrame::OnActivate()
//
//  Loop through all docs, and see if any require closing or
//  reloading.
//
//  csc 5/17/04
//
////////////////////////////////////////////////////////////////
void CMainFrame::OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized)
{
    static bool bIsActivating=false;

    if (!bIsActivating && (nState==WA_ACTIVE || nState==WA_CLICKACTIVE)) {
        bIsActivating = true;
        CTextViewApp* pApp = (CTextViewApp*)AfxGetApp();
        POSITION pos = pApp->GetFirstDocTemplatePosition();
        while (pos)  {
            CDocTemplate* pTemplate = pApp->GetNextDocTemplate(pos);
            POSITION pos2 = pTemplate->GetFirstDocPosition();
            while (pos2)  {
                CDocument* pDocument;
                if ((pDocument=pTemplate->GetNextDoc(pos2)) != NULL)  {
                    CTVDoc* pTVDoc = DYNAMIC_DOWNCAST(CTVDoc, pDocument);
                    if (pTVDoc->GetBufferMgr()->RequiresClose()) {
                        pTVDoc->CloseDeletedFile();
                    }
                    else if (pTVDoc->GetBufferMgr()->RequiresReload()) {
                        pTVDoc->ReloadFile();
                    }
                }
            }
        }
        bIsActivating = false;
    }
    CMDIFrameWnd::OnActivate(nState, pWndOther, bMinimized);
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

    return 0L;
}


void CMainFrame::OnUpdateOpenInDataViewer(CCmdUI* pCmdUI)
{
    pCmdUI->Enable(MDIGetActive() != nullptr);
}

void CMainFrame::OnOpenInDataViewer()
{
    CMDIChildWnd* pChild = MDIGetActive();

    if( pChild == nullptr )
        return;

    CTVDoc* pDoc = (CTVDoc*)pChild->GetActiveDocument();
    OpenInDataViewer(pDoc->GetPathName());
}

void CMainFrame::OpenInDataViewer(const CString& filename)
{
    CSProExecutables::RunProgramOpeningFile(CSProExecutables::Program::DataViewer, CS2WS(filename));
}
