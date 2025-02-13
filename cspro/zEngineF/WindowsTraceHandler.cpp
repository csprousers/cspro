#include "StdAfx.h"
#include "WindowsTraceHandler.h"
#include <zToolsO/Screen.h>
#include <zToolsO/WinSettings.h>
#include <zUtilF/UIThreadRunner.h>


// --------------------------------------------------------------------------
// WindowsTraceHandler
// --------------------------------------------------------------------------

WindowsTraceHandler::WindowsTraceHandler()
    :   m_traceWnd(nullptr)
{
}


WindowsTraceHandler::~WindowsTraceHandler()
{
    if( m_traceWnd != nullptr )
    {
        // destroy the window on the UI thread
        bool success = RunOnUIThread([&]()
        {
            m_traceWnd->DestroyWindow();
        });

        if( !success )
        {
            // because CSEntry can shut down the engine once the window is closed, it is not
            // always possible to destroy the window on the UI thread
            m_traceWnd->DestroyWindow();
        }
    }
}


void WindowsTraceHandler::DeleteTraceWindow()
{
    ASSERT(m_traceWnd != nullptr);
    m_traceWnd = nullptr;
}


bool WindowsTraceHandler::TurnOnWindowTrace()
{
    // only create a new window if one has not already been created
    if( m_traceWnd == nullptr )
    {
        // create the window on the UI thread
        RunOnUIThread([&]()
        {
            m_traceWnd = new TraceWnd(*this);

            if( !m_traceWnd->CreateTraceControl() )
                m_traceWnd = nullptr;
        });
    }

    return ( m_traceWnd != nullptr );
}


void WindowsTraceHandler::OutputLine(const std::wstring& text)
{
    TraceHandler::OutputLine(text);

    if( m_traceWnd != nullptr )
        m_traceWnd->AddText(text);
}



// --------------------------------------------------------------------------
// TraceLoggingListBox
// --------------------------------------------------------------------------

BEGIN_MESSAGE_MAP(TraceLoggingListBox, LoggingListBox)
    ON_COMMAND(ID_ALWAYS_ON_TOP, OnAlwaysOnTop)
    ON_UPDATE_COMMAND_UI(ID_ALWAYS_ON_TOP, OnUpdateAlwaysOnTop)
END_MESSAGE_MAP()


TraceLoggingListBox::TraceLoggingListBox()
    :   m_alwaysOnTop(WinSettings::Read<DWORD>(WinSettings::Type::TraceWindowAlwaysOnTop, 1) == 1)
{
}


void TraceLoggingListBox::AddAdditionalContextMenuItems(CMenu& popup_menu)
{
    popup_menu.AppendMenu(MF_SEPARATOR);
    popup_menu.AppendMenu(MF_STRING, ID_ALWAYS_ON_TOP, _T("Always on &Top"));
}


void TraceLoggingListBox::OnAlwaysOnTop()
{
    m_alwaysOnTop = !m_alwaysOnTop;

    WinSettings::Write<DWORD>(WinSettings::Type::TraceWindowAlwaysOnTop, m_alwaysOnTop ? 1 : 0);

    GetParent()->SetWindowPos(m_alwaysOnTop ? &CWnd::wndTopMost : &CWnd::wndNoTopMost,
                              0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE);
}


void TraceLoggingListBox::OnUpdateAlwaysOnTop(CCmdUI* pCmdUI)
{
    pCmdUI->SetCheck(m_alwaysOnTop);
}



// --------------------------------------------------------------------------
// TraceWnd
// --------------------------------------------------------------------------

BEGIN_MESSAGE_MAP(TraceWnd, CFrameWnd)
    ON_WM_CREATE()
    ON_WM_SIZE()
    ON_WM_NCDESTROY()
END_MESSAGE_MAP()


TraceWnd::TraceWnd(WindowsTraceHandler& trace_handler)
    :   m_traceHandler(trace_handler)
{
}


int TraceWnd::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if( __super::OnCreate(lpCreateStruct) == -1 )
        return -1;

    // create the trace logger
    if( !m_traceLoggingListBox.Create(WS_CHILD | WS_VISIBLE | WS_TABSTOP, CRect(), this, static_cast<UINT>(-1)) )
        return -1;

    return 0;
}


bool TraceWnd::CreateTraceControl()
{
    // create the trace window, sizing it to 40% of the width and 80% of the height of the screen's display size
    constexpr double ScreenWidthPercent = 0.40;
    constexpr double ScreenHeightPercent = 0.80;

    CSize window_size = Screen::GetScaledDisplaySize(ScreenHeightPercent);
    window_size.cx = static_cast<LONG>(window_size.cx * ScreenWidthPercent / ScreenHeightPercent);

    // position the window to the right of with a margin of 10% of the display width (thus 1/4 of the window width), and vertically in the middle
    CPoint window_location(Screen::GetFullWidth() - ( window_size.cx / 4 ) - window_size.cx,
                           ( Screen::GetFullHeight() - window_size.cy ) / 2);
                        
    if( Create(nullptr, _T("CSPro Trace Output"), WS_CAPTION | WS_POPUPWINDOW | WS_VISIBLE | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SIZEBOX, CRect()) == -1 )
        return false;

    // show the window, potentially always on top
    SetWindowPos(m_traceLoggingListBox.GetAlwaysOnTop() ? &wndTopMost : &wndTop,
                 window_location.x, window_location.y, window_size.cx, window_size.cy, 0);

    return true;
}


void TraceWnd::OnSize(UINT nType, int cx, int cy)
{
	__super::OnSize(nType, cx, cy);

	m_traceLoggingListBox.SetWindowPos(nullptr, 0, 0, cx, cy, SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOZORDER);
}


void TraceWnd::OnNcDestroy()
{
    m_traceHandler.DeleteTraceWindow();

    __super::OnNcDestroy();
}


void TraceWnd::AddText(std::wstring text)
{
    m_traceLoggingListBox.AddText(std::move(text));
}
