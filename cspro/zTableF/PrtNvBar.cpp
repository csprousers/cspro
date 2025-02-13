//***************************************************************************
//  File name: PrtNvBar.h
//
//  Description:
//       Print view navigation dialog bar.
//
//***************************************************************************

#include "StdAfx.h"
#include "PrtNvBar.h"
#include <afxdhtml.h>

/////////////////////////////////////////////////////////////////////////////
//
//                              CPrtViewNavigationBar
//
/////////////////////////////////////////////////////////////////////////////
IMPLEMENT_DYNAMIC(CPrtViewNavigationBar, CDialogBar)

static UINT BASED_CODE nav_buttons[] =
{
    // same order as in the navigation toolbar bitmap
    ID_VIEW_FIRST_PAGE,
    ID_VIEW_PREV_PAGE,
    ID_SEPARATOR,
    ID_SEPARATOR,           // for static text box (placeholder)
    ID_SEPARATOR,
    ID_VIEW_NEXT_PAGE,
    ID_VIEW_LAST_PAGE,
};

static const int iStaticTextWidth = 175;  // width of static member ("page X of Y")
static const int iSeparatorWidth = 12;    // width of separator
static const int iStaticTextID=3;         // button #3 in the toolbar
static const int iSeparator1ID=2;         // separator is in position #2
static const int iSeparator2ID=4;         // separator is in position #4


CPrtViewNavigationBar::CPrtViewNavigationBar()
{
}


CPrtViewNavigationBar::~CPrtViewNavigationBar()
{
    m_fontPageNumber.DeleteObject();
}


////////////////////////////////////////////////////////////////////////////////////
//
//                              CPrtViewNavigation::DoDataExchange
//
////////////////////////////////////////////////////////////////////////////////////
void CPrtViewNavigationBar::DoDataExchange(CDataExchange* pDX)
{
    CDialogBar::DoDataExchange(pDX);
}


////////////////////////////////////////////////////////////////////////////////////
//
//                              CPrtViewNavigation::CenterToolBar
//
// Positions the navigation tool bar in the bottom center of the prt view
//
////////////////////////////////////////////////////////////////////////////////////
void CPrtViewNavigationBar::CenterToolBar(void)
{
    CRect rcDlgBar, rcToolBar;

    // get dlg bar dimensions
    GetClientRect(rcDlgBar);

    // get toolbar dimensions
    m_wndToolBar.GetItemRect(0, &rcToolBar);
    rcToolBar.bottom += 6;
    rcToolBar.right = rcToolBar.Width()*5 + iSeparatorWidth*2 + iStaticTextWidth;
    rcToolBar.TopLeft()=CPoint(0,0);
    m_wndToolBar.MoveWindow((rcDlgBar.Width()-rcToolBar.Width())/2, 0, rcToolBar.Width(), rcToolBar.Height());

}


////////////////////////////////////////////////////////////////////////////////////
//
//                              CPrtViewNavigation::OnInitDialog
//
// Initializes navigation bar and the toolbar inside it.
//
////////////////////////////////////////////////////////////////////////////////////
LONG CPrtViewNavigationBar::OnInitDialog (UINT wParam, LONG lParam)
{
    BOOL bRet = HandleInitDialog(wParam, lParam);
    if (!UpdateData(FALSE))  {
        TRACE0("Warning: UpdateData failed during dialog init.\n");
    }

    CRect rcDlgBar, rcToolBar;
    GetClientRect(rcDlgBar);

    //add the tool bar to the dialog
    if (!m_wndToolBar.Create(this)) {
        TRACE(_T("Failed to create toolbar inside the navigation dlgbar\n"));
        ASSERT(FALSE);
    }
    m_wndToolBar.LoadToolBar(IDR_PRTVIEW_NAVIGATION_BAR);
    m_wndToolBar.SetButtons(nav_buttons, sizeof(nav_buttons)/sizeof(UINT));
    m_wndToolBar.ShowWindow(SW_SHOW);
    m_wndToolBar.SetBarStyle(CBRS_ALIGN_TOP|CBRS_TOOLTIPS|CBRS_FLYBY);

    // Create the static text
    m_wndToolBar.SetButtonInfo(iStaticTextID, ID_PRTVIEW_PAGENUM, TBBS_SEPARATOR, iStaticTextWidth);

    // Design guide advises 12 pixel gap between static text and buttons
    m_wndToolBar.SetButtonInfo(iSeparator1ID, ID_SEPARATOR, TBBS_SEPARATOR, iSeparatorWidth);
    m_wndToolBar.SetButtonInfo(iSeparator2ID, ID_SEPARATOR, TBBS_SEPARATOR, iSeparatorWidth);

    // load the static text box into the middle button (ID_PRTVIEW_PAGENUM)
    CRect rcStatic;
    m_wndToolBar.GetToolBarCtrl().GetItemRect(iStaticTextID, &rcStatic);
    rcStatic.top = 4;
    if (!m_staticPageNumber.Create(_T("Go Skins!"), WS_VISIBLE | WS_CHILD | SS_CENTER, rcStatic, &m_wndToolBar, ID_PRTVIEW_PAGENUM)) {
        TRACE(_T("Failed to create pagenum control inside the navigation toolbar\n"));
        return FALSE;
    }

    // load a font into the static text box
    LOGFONT lf;
    lf.lfHeight = -13;
    lf.lfWidth = 0;
    lf.lfEscapement = 0;
    lf.lfOrientation = 0;
    lf.lfWeight = 400;
    lf.lfItalic = 0;
    lf.lfUnderline = 0;
    lf.lfStrikeOut = 0;
    lf.lfCharSet = 0;
    lf.lfOutPrecision = 3;
    lf.lfClipPrecision = 2;
    lf.lfQuality = 1;
    lf.lfPitchAndFamily = 34;
    _tcscpy(lf.lfFaceName, _T("Arial"));
    m_fontPageNumber.CreateFontIndirect(&lf);
    m_staticPageNumber.SetFont(&m_fontPageNumber, TRUE);

    // resize and redraw ...
    RepositionBars(AFX_IDW_CONTROLBAR_FIRST, AFX_IDW_CONTROLBAR_LAST, 0);
    UpdateWindow();

    return bRet;
}


BEGIN_MESSAGE_MAP(CPrtViewNavigationBar, CDialogBar)
    ON_MESSAGE(WM_INITDIALOG, OnInitDialog)
    ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTW, 0, 0xFFFF, OnToolTipText)
    ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTA, 0, 0xFFFF, OnToolTipText)
END_MESSAGE_MAP()

////////////////////////////////////////////////////////////////////////////////////
//
//                              CPrtViewNavigation::SetPageInfo
//
// Sets the page number message in the toolbar.
//
////////////////////////////////////////////////////////////////////////////////////
void CPrtViewNavigationBar::SetPageInfo(int iCurrentPage, int iNumPages)
{
    CString s;
    s.Format (_T("Page %d of %d"), iCurrentPage, iNumPages);
    m_staticPageNumber.SetWindowText(s);
    UpdateWindow();
}


////////////////////////////////////////////////////////////////////////////////////
//
//                              CPrtViewNavigation::OnToolTipText
//
// Puts out tool tip text.
//
////////////////////////////////////////////////////////////////////////////////////
BOOL CPrtViewNavigationBar::OnToolTipText(UINT, NMHDR* pNMHDR, LRESULT* pResult)
{
    ASSERT(pNMHDR->code == TTN_NEEDTEXTA || pNMHDR->code == TTN_NEEDTEXTW);

    // if there is a top level routing frame then let it handle the message
    if (GetRoutingFrame() != NULL)  {
        return FALSE;
    }

    // to be thorough we will need to handle UNICODE versions of the message also !!
    TOOLTIPTEXTA* pTTTA = (TOOLTIPTEXTA*)pNMHDR;
    TOOLTIPTEXTW* pTTTW = (TOOLTIPTEXTW*)pNMHDR;
    TCHAR szFullText[512];
    CString strTipText;
    UINT nID = pNMHDR->idFrom;

    if (pNMHDR->code == TTN_NEEDTEXTA && (pTTTA->uFlags & TTF_IDISHWND) || pNMHDR->code == TTN_NEEDTEXTW && (pTTTW->uFlags & TTF_IDISHWND)) {
        // idFrom is actually the HWND of the tool
        nID = ::GetDlgCtrlID((HWND)nID);
    }

    if (nID != 0) {   // && nID!=ID_VIEW_TOOLBAR) {
        // will be zero on a separator
        AfxLoadString(nID, szFullText);
        strTipText=szFullText;

        if (pNMHDR->code == TTN_NEEDTEXTA) {
            _wcstombsz(pTTTA->szText, strTipText,sizeof(pTTTA->szText));
        }
        else {
            lstrcpyn(pTTTW->szText, strTipText, sizeof(pTTTW->szText));
        }

        *pResult = 0;

        // bring the tooltip window above other popup windows
        ::SetWindowPos(pNMHDR->hwndFrom, HWND_TOP, 0, 0, 0, 0,
        SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOMOVE|SWP_NOOWNERZORDER);

        return TRUE;
    }

    return FALSE;
}
