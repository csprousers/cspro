#include "StdAfx.h"
#include "LogicDialogBar.h"
#include "CompilerOutputTabViewPage.h"


namespace
{
    constexpr const TCHAR* CompilerOutputTabText = _T("Compiler Output");
    constexpr const TCHAR* MessagesTabText       = _T("Messages");
}


BEGIN_MESSAGE_MAP(LogicDialogBar, UnfloatableDialogBar)
    ON_WM_CREATE()
    ON_WM_SIZE()
    ON_MESSAGE(UWM::Designer::TabViewContainerTabChange, OnTabChange)
END_MESSAGE_MAP()


LogicDialogBar::LogicDialogBar()
    :   UnfloatableDialogBar(SZBARF_DLGAUTOSIZE | SZBARF_NOCAPTION | SZBARF_NOGRIPPER | SZBARF_NOCLOSEBTN | SZBARF_NORESIZEBTN),
        m_applicationChildWnd(nullptr)
{
}


LogicDialogBar::~LogicDialogBar()
{
}


bool LogicDialogBar::CreateAndDock(ApplicationChildWnd* application_child_wnd)
{
    m_applicationChildWnd = application_child_wnd;
    m_compilerOutputTabViewPage = std::make_unique<CompilerOutputTabViewPage>(m_applicationChildWnd);
    m_messageEditTabViewPage = std::make_unique<MessageEditTabViewPage>(m_applicationChildWnd);

    if( !Create(m_applicationChildWnd, IDD_LOGIC_BAR, CBRS_BOTTOM, (UINT)-1) )
        return false;

    // dock the window on the bottom
    EnableDocking(CBRS_ALIGN_BOTTOM);

    m_applicationChildWnd->DockControlBar(this, CBRS_ALIGN_BOTTOM);

    return true;
}


int LogicDialogBar::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if( UnfloatableDialogBar::OnCreate(lpCreateStruct) == -1 )
        return -1;

    // create the tab view container for the two tabs
    if( !m_tabViewContainer.Create(this) )
        return -1;


    // create the compiler output window
    const DWORD dwStyle = WS_VISIBLE | WS_CHILD | WS_HSCROLL | WS_VSCROLL;

    if( !m_compilerOutputTabViewPage->Create(dwStyle, &m_tabViewContainer) )
        return -1;

    m_tabViewContainer.AddPage(m_compilerOutputTabViewPage.get(), CompilerOutputTabText);


    // create the messages window
    if( !m_messageEditTabViewPage->Create(dwStyle, &m_tabViewContainer) )
        return -1;

    m_tabViewContainer.AddPage(m_messageEditTabViewPage.get(), MessagesTabText);


    // by default select the compiler output window
    m_tabViewContainer.SetActivePageIndex(0);
    
    return 0;
}


void LogicDialogBar::OnSize(UINT nType, int cx, int cy)
{
    UnfloatableDialogBar::OnSize(nType, cx, cy);

    // resize the tab view container to match this window
    if( m_tabViewContainer.GetSafeHwnd() != nullptr )
    {
        CRect rect;
        GetClientRect(&rect);

        m_tabViewContainer.MoveWindow(rect.left, rect.top, cx, cy);
        m_tabViewContainer.UpdateScrollState();
    }
}


LRESULT LogicDialogBar::OnTabChange(WPARAM wParam, LPARAM /*lParam*/)
{
    const int tab_index = static_cast<int>(wParam);

    // when selecting the messages tab, refresh the text
    if( tab_index == 1 )
        m_messageEditTabViewPage->OnTabChange();

    return 0;
}
