#include "StdAfx.h"
#include "LogicReferenceWnd.h"
#include <zEditO/LogicView.h>


// --------------------------------------------------------------------------
// LogicReferenceView declaration
// --------------------------------------------------------------------------

class LogicReferenceView : public LogicView
{
    DECLARE_DYNCREATE(LogicReferenceView)

protected:
    // LogicView overrides
    void InitializeLogicControl() override;
};



// --------------------------------------------------------------------------
// LogicReferenceWnd
// --------------------------------------------------------------------------

BEGIN_MESSAGE_MAP(LogicReferenceWnd, UnfloatableDialogBar)
    ON_WM_CREATE()
END_MESSAGE_MAP()


LogicReferenceWnd::LogicReferenceWnd()
    :   UnfloatableDialogBar(SZBARF_NOCAPTION | SZBARF_NORESIZEBTN),
        m_doc(nullptr),
        m_logicReferenceView(nullptr),
        m_logicCtrl(nullptr)
{
}


bool LogicReferenceWnd::CreateAndDock(CFrameWnd* const frame_wnd, CDocument* const doc)
{
    ASSERT(doc != nullptr);
    m_doc = doc;

    if( !Create(frame_wnd, IDD_REFERENCE_WINDOW, CBRS_RIGHT, static_cast<UINT>(-1)) )
        return false;

    // dock the reference window on the right
    EnableDocking(CBRS_ALIGN_RIGHT);

    // the reference window is only shown on demand
    ShowWindow(SW_HIDE);

    frame_wnd->DockControlBar(this, CBRS_ALIGN_RIGHT);

    return true;
}


int LogicReferenceWnd::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if( __super::OnCreate(lpCreateStruct) == -1 )
        return -1;

    CCreateContext context;
    context.m_pCurrentDoc = m_doc;
    context.m_pCurrentFrame = GetParentFrame();
    context.m_pNewViewClass = RUNTIME_CLASS(LogicReferenceView);

    constexpr DWORD dwStyle = AFX_WS_DEFAULT_VIEW & ~WS_BORDER;

    m_logicReferenceView = assert_cast<LogicReferenceView*>(context.m_pNewViewClass->CreateObject());

    if( m_logicReferenceView == nullptr ||
        !m_logicReferenceView->Create(nullptr, nullptr, dwStyle, CRect(), this, static_cast<UINT>(-1), &context) )
    {
        return -1;
    }

    m_logicCtrl = m_logicReferenceView->GetLogicCtrl();

    return 0;
}


void LogicReferenceWnd::OnSizedOrDocked(const int cx, const int cy, const BOOL bFloating, const int flags)
{
    ASSERT(( m_Style & SZBARF_DLGAUTOSIZE ) == 0 );

    __super::OnSizedOrDocked(cx, cy, bFloating, flags);

    m_logicReferenceView->SetWindowPos(
        nullptr,
        0, 0,
        cx, cy,
        SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOZORDER
    );
}



// --------------------------------------------------------------------------
// LogicReferenceView
// --------------------------------------------------------------------------

IMPLEMENT_DYNCREATE(LogicReferenceView, LogicView)


void LogicReferenceView::InitializeLogicControl()
{
    m_logicCtrl->InitLogicControl(
        false, // show_line_numbers
        false, // show_error_markers
        SCLEX_NULL
    );

    m_logicCtrl->SetReadOnly(TRUE);
}
