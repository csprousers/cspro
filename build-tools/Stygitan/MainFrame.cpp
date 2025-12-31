#include "StdAfx.h"
#include "MainFrame.h"
#include "CodePurifierView.h"
#include <zToolsO/UWM.h>


BEGIN_MESSAGE_MAP(MainFrame, CMDIFrameWnd)
    ON_WM_CREATE()
    ON_WM_CLOSE()
    ON_MESSAGE(UWM::Stygitan::OpenContainingFolder, OnOpenContainingFolder)
    ON_MESSAGE(UWM::ToolsO::DisplayErrorMessage, OnDisplayErrorMessage)
    ON_MESSAGE(UWM::UtilF::RunOnUIThread, OnRunOnUIThread)
END_MESSAGE_MAP()


MainFrame::MainFrame()
{
}


int MainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if( __super::OnCreate(lpCreateStruct) == -1 )
        return -1;

    // Switch the order of document name and application name on the window title bar. This
    // improves the usability of the taskbar because the document name is visible with the thumbnail.
    ModifyStyle(0, FWS_PREFIXTITLE);

    return 0;
}


void MainFrame::OnClose()
{
    // make sure that no threads are active
    bool thread_is_active = false;

    ForeachParentFrame(
        [&](CFrameWnd& frame_wnd)
        {
            if( frame_wnd.SendMessage(UWM::Stygitan::ThreadIsActive) == 1 )
            {
                ErrorMessage::PostMessageForDisplay("You must cancel any running tasks before closing Stygitan.");
                thread_is_active = true;
                return false;
            }

            return true;
        });

    if( !thread_is_active )
        __super::OnClose();
}


LRESULT MainFrame::OnOpenContainingFolder(const WPARAM wParam, LPARAM /*lParam*/)
{
    // OpenContainingFolder does not appear to work when called from a thread, so it is processed here
    OpenContainingFolder(WindowsDesktopMessage::GetPostedObject<SharableString>(wParam).GetString());
    return 1;
}


LRESULT MainFrame::OnDisplayErrorMessage(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    ErrorMessage::DisplayPostedMessages();
    return 1;
}


LRESULT MainFrame::OnRunOnUIThread(const WPARAM wParam, LPARAM /*lParam*/)
{
    UIThreadRunner* const ui_thread_runner = reinterpret_cast<UIThreadRunner*>(wParam);
    ui_thread_runner->Execute();
    return 1;
}
