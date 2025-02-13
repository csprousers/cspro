#include "StdAfx.h"
#include "DictDialogBar.h"


BEGIN_MESSAGE_MAP(CDictDialogBar, UnfloatableDialogBar)
    ON_WM_CREATE()
    ON_WM_LBUTTONUP()
END_MESSAGE_MAP()


CDictDialogBar::CDictDialogBar()
    :   UnfloatableDialogBar(SZBARF_DLGAUTOSIZE | SZBARF_NOCAPTION | SZBARF_NORESIZEBTN)
{
}

bool CDictDialogBar::CreateAndDock(CFrameWnd* pFrameWnd)
{
    if (!Create(pFrameWnd, IDD_DICT_PROP_DLGBAR, CBRS_RIGHT, (UINT)-1))
        return false;

    // dock the reference window on the right
    EnableDocking(CBRS_ALIGN_RIGHT);
    ShowWindow(SW_HIDE);

    pFrameWnd->DockControlBar(this, CBRS_ALIGN_RIGHT);

    return true;
}

int CDictDialogBar::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (UnfloatableDialogBar::OnCreate(lpCreateStruct) == -1 ||
        !m_wndPropGridCtrl.Create(WS_VISIBLE | WS_CHILD | WS_HSCROLL | WS_VSCROLL,
            CRect(0, 0, 0, 0), this, IDD_DICT_PROP_GRID))
    {
        return -1;
    }

    return 0;
}

BOOL CDictDialogBar::OnInitDialog()
{
    BOOL init_dialog_result = UnfloatableDialogBar::OnInitDialog();

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

    UnfloatableDialogBar::OnLButtonUp(nFlags, point);
    CPoint ptTest = point;

    if (pressedBtn == CLOSEBTN && m_rectCloseBtn.PtInRect(ptTest))
    {
        //bar is closed. set the flag for parent
        ((CDictChildWnd*)GetParentFrame())->m_bViewPropertiesPanel = FALSE;
    }

}
