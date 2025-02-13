#include "StdAfx.h"
#include "MainFrm.h"


IMPLEMENT_DYNCREATE(CMainFrame, CFrameWnd)

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWnd)
    ON_WM_CREATE()
    ON_WM_MENUCHAR()
    ON_COMMAND(ID_HELP_FINDER, OnHelpFinder)
    ON_COMMAND(ID_HELP, OnHelp)
    ON_COMMAND(ID_CONTEXT_HELP, OnContextHelp)
    ON_COMMAND(ID_DEFAULT_HELP, OnHelpFinder)
END_MESSAGE_MAP()


constexpr UINT indicators[] =
{
    ID_SEPARATOR,           // status line indicator
    ID_INDICATOR_CAPS,
    ID_INDICATOR_NUM,
    ID_INDICATOR_SCRL,
};


BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
    cs.style = cs.style ^ FWS_ADDTOTITLE;
    return CFrameWnd::PreCreateWindow(cs);
}


int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if( CFrameWnd::OnCreate(lpCreateStruct) == -1 )
        return -1;

    if( !m_wndToolBar.CreateEx(this, TBSTYLE_FLAT, WS_CHILD | WS_VISIBLE | CBRS_TOP |
                                     CBRS_GRIPPER | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC) ||
        !m_wndToolBar.LoadToolBar(IDR_MAINFRAME) )
    {
        TRACE0("Failed to create toolbar\n");
        return -1;      // fail to create
    }

    if( !m_wndStatusBar.Create(this) ||
        !m_wndStatusBar.SetIndicators(indicators, _countof(indicators) ) )
    {
        TRACE0("Failed to create status bar\n");
        return -1;      // fail to create
    }

    // TODO: Delete these three lines if you don't want the toolbar to
    //  be dockable
    //  m_wndToolBar.EnableDocking(CBRS_ALIGN_ANY);
    //  EnableDocking(CBRS_ALIGN_ANY);
    //  DockControlBar(&m_wndToolBar);

    return 0;
}


LRESULT CMainFrame::OnMenuChar(UINT nChar, UINT nFlags, CMenu* pMenu)
{
    return BCMenu::IsMenu(pMenu) ? BCMenu::FindKeyboardShortcut(nChar, nFlags, pMenu) :
                                   CFrameWnd::OnMenuChar(nChar, nFlags, pMenu);
}
