#include "StdAfx.h"
#include "MainFrame.h"
#include "CodePurifierView.h"
#include <zUtilF/DocViewIterators.h>


BEGIN_MESSAGE_MAP(MainFrame, CMDIFrameWnd)
    ON_WM_CREATE()
    ON_WM_ACTIVATEAPP()
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


void MainFrame::OnActivateApp(const BOOL bActive, const DWORD dwThreadID)
{
    __super::OnActivateApp(bActive, dwThreadID);

    if( bActive )
    {
        // refresh the Code Purifier windows
        ForeachViewOfType<CodePurifierView>(
            [&](CodePurifierView& cp_view)
            {
                cp_view.PostMessage(UWM::Stygitan::AppActivated);
                return true;
            });
    }
}
