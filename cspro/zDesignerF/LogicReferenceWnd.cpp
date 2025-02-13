#include "StdAfx.h"
#include "LogicReferenceWnd.h"


BEGIN_MESSAGE_MAP(LogicReferenceWnd, UnfloatableDialogBar)
    ON_WM_CREATE()
END_MESSAGE_MAP()


LogicReferenceWnd::LogicReferenceWnd()
    :   UnfloatableDialogBar(SZBARF_DLGAUTOSIZE | SZBARF_NOCAPTION | SZBARF_NORESIZEBTN)
{
}


bool LogicReferenceWnd::CreateAndDock(CFrameWnd* pFrameWnd)
{
    if( !Create(pFrameWnd, IDD_REFERENCE_WINDOW, CBRS_RIGHT, (UINT)-1) )
        return false;

    // dock the reference window on the right
    EnableDocking(CBRS_ALIGN_RIGHT);
    ShowWindow(SW_HIDE);

    pFrameWnd->DockControlBar(this, CBRS_ALIGN_RIGHT);

    return true;
}


int LogicReferenceWnd::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if( UnfloatableDialogBar::OnCreate(lpCreateStruct) == -1 ||
        !m_editCtrl.Create(WS_VISIBLE | WS_CHILD | WS_HSCROLL | WS_VSCROLL, this) )
    {
        return -1;
    }

    return 0;
}


BOOL LogicReferenceWnd::OnInitDialog()
{
    BOOL init_dialog_result = UnfloatableDialogBar::OnInitDialog();

    // replace the dialog's edit control with the logic control
    CWnd* reference_window_edit = GetDlgItem(IDC_REFERENCE_WINDOW_EDIT);

    CRect rect;
    reference_window_edit->GetWindowRect(&rect);
    ScreenToClient(&rect);

    reference_window_edit->DestroyWindow();
    m_editCtrl.SubclassDlgItem(IDC_REFERENCE_WINDOW_EDIT, this);
    m_editCtrl.MoveWindow(&rect);

    return init_dialog_result;
}
