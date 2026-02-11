#include "StdAfx.h"
#include "LogFrameAndView.h"

// --------------------------------------------------------------------------
// LogFrame
// --------------------------------------------------------------------------

IMPLEMENT_DYNCREATE(LogFrame, CMDIChildWnd)


void LogFrame::ActivateFrame(int nCmdShow/* = -1*/)
{
    // prevent the window from being activated when it is being
    // shown for the first time from MainFrame::OnOperationInitialize
    if( m_firstActivation )
    {
        nCmdShow = SW_SHOWNA;
        m_firstActivation = false;
    }

    __super::ActivateFrame(nCmdShow);
}



// --------------------------------------------------------------------------
// LogView
// --------------------------------------------------------------------------

IMPLEMENT_DYNCREATE(LogView, CFormView)


BEGIN_MESSAGE_MAP(LogView, CFormView)
    ON_WM_CREATE()
    ON_WM_DESTROY()
END_MESSAGE_MAP()


LogView::LogView()
    :   CFormView(IDD_LOG),
        m_controller(Controller::GetInstance())
{
}


int LogView::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if( __super::OnCreate(lpCreateStruct) == -1 )
        return -1;

    m_controller.SetLoggingListBox(&m_loggingListBox);

    return 0;
}

void LogView::OnDestroy()
{
    m_controller.SetLoggingListBox(nullptr);
}


void LogView::DoDataExchange(CDataExchange* const pDX)
{
    __super::DoDataExchange(pDX);

    DDX_Control(pDX, IDC_LOG, m_loggingListBox);
}
