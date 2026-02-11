#include "StdAfx.h"
#include "MainFrame.h"
#include "LogFrameAndView.h"
#include "OpenSourceSyncer.h"
#include <zToolsO/UWM.h>
#include <zUtilF/DocViewIterators.h>


BEGIN_MESSAGE_MAP(MainFrame, CMDIFrameWnd)
    ON_WM_CREATE()
    ON_WM_CLOSE()
    ON_MESSAGE(UWM::OpenSourceSyncer::UpdateStatusBar, OnUpdateStatusBar)
    ON_MESSAGE(UWM::OpenSourceSyncer::OperationInitialize, OnOperationInitialize)
    ON_MESSAGE(UWM::OpenSourceSyncer::OperationComplete, OnOperationComplete)
    ON_MESSAGE(UWM::ToolsO::DisplayErrorMessage, OnDisplayErrorMessage)
END_MESSAGE_MAP()


MainFrame::MainFrame()
    :   m_controller(Controller::GetInstance())
{
}


int MainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if( __super::OnCreate(lpCreateStruct) == -1 )
        return -1;

    constexpr UINT indicators[] = { ID_SEPARATOR };

    if( !m_wndStatusBar.Create(this) ||
        !m_wndStatusBar.SetIndicators(indicators, _countof(indicators) ) )
    {
        return -1;
    }

    // Switch the order of document name and application name on the window title bar. This
    // improves the usability of the taskbar because the document name is visible with the thumbnail.
    ModifyStyle(0, FWS_PREFIXTITLE);

    return 0;
}


void MainFrame::OnClose()
{
    if( m_controller.IsOperationRunning() )
    {
        ErrorMessage::Display("You cannot close the program while an operation is in progress.");
        return;
    }

    __super::OnClose();
}


LRESULT MainFrame::OnUpdateStatusBar(const WPARAM wParam, LPARAM /*lParam*/)
{
    const SharableString text = WindowsDesktopMessage::GetPostedObject<SharableString>(wParam);
    m_wndStatusBar.SetPaneText(0, TC::ToWide(*text).c_str());
    return 1;
}


LRESULT MainFrame::OnOperationInitialize(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    m_wndStatusBar.SetPaneText(0, L"Running operation...");

    // make sure that a log is showing, and when not, open one and tile the
    // windows vertically so that the log is visible alongside the active view
    bool log_showing = false;

    ForeachViewOfType<LogView>(
        [&](const LogView& /*log_view*/)
        {
            log_showing = true;
            return false;
        });

    if( !log_showing )
    {
        assert_cast<OpenSourceSyncerApp*>(AfxGetApp())->OpenLog();
        SendMessage(WM_COMMAND, ID_WINDOW_TILE_VERT);
    }

    return 1;
}


LRESULT MainFrame::OnOperationComplete(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    m_controller.MarkOperationComplete();
    return 1;
}


LRESULT MainFrame::OnDisplayErrorMessage(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    ErrorMessage::DisplayPostedMessages();
    return 1;
}
