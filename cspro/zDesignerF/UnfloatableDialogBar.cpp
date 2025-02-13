#include "StdAfx.h"
#include "UnfloatableDialogBar.h"


BEGIN_MESSAGE_MAP(UnfloatableDialogBar, COXSizeDialogBar)
    ON_WM_LBUTTONDOWN()
    ON_WM_LBUTTONDBLCLK()
    ON_WM_RBUTTONUP()
END_MESSAGE_MAP()


UnfloatableDialogBar::UnfloatableDialogBar(int nStyle)
    :   COXSizeDialogBar(nStyle)
{
}


void UnfloatableDialogBar::OnLButtonDblClk(UINT /*nFlags*/, CPoint /*point*/)
{
    // do not process left-button double clicks (to avoid creating a floating toobar)
    return;
}


void UnfloatableDialogBar::OnRButtonUp(UINT /*nFlags*/, CPoint /*point*/)
{
    // do not process right-button clicks (to avoid dragging the dialog bar away)
    return;
}


void UnfloatableDialogBar::OnLButtonDown(UINT /*nFlags*/, CPoint point)
{
    // only handle sizing and closing (do not allow drags)
    if( !IsFloating() )
    {
        CPoint ptTest = point;

        // handle mouse click over close, restore buttons
        m_pressedBtn = TB_NONE;

        if( m_rectCloseBtn.PtInRect(ptTest) )
        {
            SetCapture();
            m_pressedBtn = CLOSEBTN;
            RedrawCloseBtn();
        }

        else if( m_rectResizeBtn.PtInRect(ptTest) && CanResize() )
        {
            SetCapture();
            m_pressedBtn = RESIZEBTN;
            RedrawResizeBtn();
        }
    }
}
