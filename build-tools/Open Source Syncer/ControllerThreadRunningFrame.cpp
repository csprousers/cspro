#include "StdAfx.h"
#include "ControllerThreadRunningFrame.h"


IMPLEMENT_DYNCREATE(ControllerThreadRunningFrame, CMDIChildWnd)


BEGIN_MESSAGE_MAP(ControllerThreadRunningFrame, CMDIChildWnd)
    ON_WM_CLOSE()
END_MESSAGE_MAP()


ControllerThreadRunningFrame::ControllerThreadRunningFrame()
    :   m_controller(Controller::GetInstance())
{
}


void ControllerThreadRunningFrame::OnClose()
{
    if( m_controller.IsOperationRunning(this) )
    {
        ErrorMessage::Display("Wait until the operation finishes.");
        return;
    }

    __super::OnClose();
}
