#include "StdAfx.h"
#include "DictDialogBar.h"


BEGIN_MESSAGE_MAP(CDictDialogBar, UnfloatableDialogBar)
    ON_WM_CREATE()
    ON_WM_SIZE()
    ON_WM_LBUTTONUP()
END_MESSAGE_MAP()


CDictDialogBar::CDictDialogBar()
    :   UnfloatableDialogBar(SZBARF_NOCAPTION | SZBARF_NORESIZEBTN)
{
}


bool CDictDialogBar::CreateAndDock(CFrameWnd* const frame_wnd)
{
    if( !Create(frame_wnd, IDD_DICT_PROP_DLGBAR, CBRS_RIGHT, static_cast<UINT>(-1) ) )
        return false;

    // dock the property panel to the right
    EnableDocking(CBRS_ALIGN_RIGHT);

    // the property pane is only shown on demand
    ShowWindow(SW_HIDE);

    frame_wnd->DockControlBar(this, CBRS_ALIGN_RIGHT);

    return true;
}


int CDictDialogBar::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if( __super::OnCreate(lpCreateStruct) == -1 ||
        !m_wndPropGridCtrl.Create(WS_VISIBLE | WS_CHILD | WS_HSCROLL | WS_VSCROLL, CRect(), this, IDD_DICT_PROP_GRID) )
    {
        return -1;
    }

    return 0;
}


void CDictDialogBar::OnSize(const UINT nType, const int cx, const int cy)
{
    ASSERT(( m_Style & SZBARF_DLGAUTOSIZE ) == 0 );

    __super::OnSize(nType, cx, cy);

    // resize the property panel to match this window
    if( m_wndPropGridCtrl.GetSafeHwnd() != nullptr )
    {
        CRect rect;
        GetClientRect(&rect);
        m_wndPropGridCtrl.MoveWindow(rect);
    }
}


BOOL CDictDialogBar::OnInitDialog()
{
    BOOL init_dialog_result = __super::OnInitDialog();

    //  replace the dialog's edit control with the logic control
    CWnd* placeHolderCtrl = GetDlgItem(IDC_DICT_PROP_PLACE_HOLDER);

    CRect rect;
    placeHolderCtrl->GetWindowRect(&rect);
    ScreenToClient(&rect);

    placeHolderCtrl->DestroyWindow();

    m_wndPropGridCtrl.SubclassDlgItem(IDC_DICT_PROP_PLACE_HOLDER, this);
    m_wndPropGridCtrl.MoveWindow(&rect);

    return init_dialog_result;
}


void CDictDialogBar::OnLButtonUp(UINT nFlags, CPoint point)
{
    //Code from COXSizeControlBar::OnLButtonUp(UINT nFlags, CPoint point) to set the parent window flag
    SIZEBARBTN pressedBtn = m_pressedBtn;

    __super::OnLButtonUp(nFlags, point);
    CPoint ptTest = point;

    if (pressedBtn == CLOSEBTN && m_rectCloseBtn.PtInRect(ptTest))
    {
        //bar is closed. set the flag for parent
        ((CDictChildWnd*)GetParentFrame())->m_bViewPropertiesPanel = FALSE;
    }

}
