#include "StdAfx.h"
#include "BoxToolBar.h"


BEGIN_MESSAGE_MAP(BoxToolbar, CToolBar)
    ON_WM_WINDOWPOSCHANGING()
END_MESSAGE_MAP()


BoxToolbar::BoxToolbar(CFormChildWnd* pFormCW)
    :   m_pFormCW(pFormCW)
{    
}


void BoxToolbar::OnWindowPosChanging(LPWINDOWPOS lpWndPos)
{
    CToolBar::OnWindowPosChanging(lpWndPos);

    if( ( lpWndPos->flags & SWP_HIDEWINDOW ) != 0 )
    {
        if( !IsWindowVisible() && m_pFormCW != nullptr )
        {
            // destruction of the obj will occur in CFormChildWnd::OnUpdateSelectItems
            CRect rect;

            GetWindowRect(&rect);

            m_pFormCW->CanUserDrawBox(false);

            // update x,y coords w/last place viewed
            m_pFormCW->SetBoxToolBarLocation(rect.TopLeft());
        }
    }
}
