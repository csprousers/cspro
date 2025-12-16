#include "StdAfx.h"
#include "MainFrame.h"


BEGIN_MESSAGE_MAP(MainFrame, CMDIFrameWnd)
    ON_WM_CREATE()
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
