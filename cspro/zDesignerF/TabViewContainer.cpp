#include "StdAfx.h"
#include "TabViewContainer.h"


BOOL TabViewContainer::SetActivePageIndex(int nIndex)
{
    BOOL return_value = COXTabViewContainer::SetActivePageIndex(nIndex);

    if( return_value )
        GetParent()->SendMessage(UWM::Designer::TabViewContainerTabChange, (WPARAM)nIndex, 0);

    return return_value;
}


extern  BOOL g_bUpdatingScrollInfo =  FALSE;
extern  BOOL g_bUpdatingScrollState = FALSE;
void TabViewContainer::UpdateScrollState()
{
    if (g_bUpdatingScrollState || g_bUpdatingScrollInfo)
    {
        return;
    }

    g_bUpdatingScrollState = TRUE;

    CWnd* pWnd = GetActivePage();
    if (pWnd == NULL || !pWnd->IsWindowVisible())
    {
        g_bUpdatingScrollState = FALSE;
        return;
    }

    PAGEINFO& pi = m_arrPages[GetActivePageIndex()];

    BOOL bHasScrollHorz = ((pWnd->GetStyle() & WS_HSCROLL) == WS_HSCROLL);
    BOOL bHasScrollVert = ((pWnd->GetStyle() & WS_VSCROLL) == WS_VSCROLL);
    if (bHasScrollHorz || bHasScrollVert)
    {
        if (bHasScrollHorz)
        {
            pi.bHasScrollHorz = bHasScrollHorz;
            //pWnd->ModifyStyle(WS_HSCROLL, NULL, SWP_DRAWFRAME);
            //pWnd->ShowScrollBar(SB_HORZ, FALSE);
        }
        if (bHasScrollVert)
        {
            pi.bHasScrollVert = bHasScrollVert;
            //pWnd->ModifyStyle(WS_VSCROLL, NULL, SWP_DRAWFRAME);
            //pWnd->ShowScrollBar(SB_VERT, FALSE);
        }
    }

    pWnd->ModifyStyle(WS_VISIBLE, NULL);
    pWnd->ShowScrollBar(SB_BOTH, TRUE);
    pWnd->ModifyStyle(NULL, WS_HSCROLL| WS_VSCROLL, SWP_DRAWFRAME);
    //pWnd->ShowScrollBar(SB_VERT, TRUE);
    //pWnd->ModifyStyle(NULL, WS_VSCROLL, SWP_DRAWFRAME);
    pi.GetScrollInfo(pWnd, TRUE);
    if (pi.bHasScrollHorz)
    {
        if (pi.scrlInfoHorz.nMax == 0 || pi.scrlInfoHorz.nPage == 0 ||
            pi.scrlInfoHorz.nMax < (int)pi.scrlInfoHorz.nPage)
        {
            pi.bHasScrollHorz = FALSE;
        }
    }
    else
    {
        if (pi.scrlInfoHorz.nMax > 0 && pi.scrlInfoHorz.nPage > 0 &&
            pi.scrlInfoHorz.nMax >= (int)pi.scrlInfoHorz.nPage)
        {
            pi.bHasScrollHorz = TRUE;
        }
    }

    pi.GetScrollInfo(pWnd, FALSE);
    if (pi.bHasScrollVert)
    {
        if (pi.scrlInfoVert.nMax == 0 || pi.scrlInfoVert.nPage == 0 ||
            pi.scrlInfoVert.nMax < (int)pi.scrlInfoVert.nPage)
        {
            pi.bHasScrollVert = FALSE;
        }
    }
    else
    {
        if (pi.scrlInfoVert.nMax > 0 && pi.scrlInfoVert.nPage > 0 &&
            pi.scrlInfoVert.nMax >= (int)pi.scrlInfoVert.nPage)
        {
            pi.bHasScrollVert = TRUE;
        }
    }

    pWnd->ShowScrollBar(SB_BOTH, FALSE);
    pWnd->ModifyStyle(WS_HSCROLL | WS_VSCROLL, NULL, SWP_DRAWFRAME);
    pWnd->ModifyStyle(NULL, WS_VISIBLE);

    DWORD dwStyle = GetScrollStyle();
    dwStyle = (pi.bHasScrollHorz ? WS_HSCROLL : 0) |
        (pi.bHasScrollVert ? WS_VSCROLL : 0);
    SetScrollStyle(dwStyle, FALSE);

    g_bUpdatingScrollState = FALSE;
}

void TabViewContainer::UpdateScrollInfo()
{
    if (g_bUpdatingScrollInfo || g_bUpdatingScrollState)
    {
        return;
    }

    g_bUpdatingScrollInfo = TRUE;

    CWnd* pWnd = GetActivePage();
    if (pWnd == NULL || !pWnd->IsWindowVisible())
    {
        g_bUpdatingScrollInfo = FALSE;
        return;
    }

    PAGEINFO& pi = m_arrPages[GetActivePageIndex()];

    if (pi.bHasScrollHorz)
    {
        pWnd->ModifyStyle(WS_VISIBLE, NULL);
        pWnd->ShowScrollBar(SB_HORZ, TRUE);
        pWnd->ModifyStyle(NULL, WS_HSCROLL, SWP_DRAWFRAME);

        pi.GetScrollInfo(pWnd, TRUE);

        ASSERT(::IsWindow(m_scrlBarHorz.GetSafeHwnd()));
        m_scrlBarHorz.SetScrollInfo(&pi.scrlInfoHorz);

        pWnd->ShowScrollBar(SB_HORZ, FALSE);
        pWnd->ModifyStyle(WS_HSCROLL, NULL, SWP_DRAWFRAME);
        //pWnd->ModifyStyle(NULL, WS_VISIBLE);

    }
    else
    {
        pi.scrlInfoHorz.cbSize = sizeof(SCROLLINFO);
        pWnd->GetScrollInfo(SB_HORZ, &pi.scrlInfoHorz);
    }

    if (pi.bHasScrollVert)
    {
        pWnd->ModifyStyle(WS_VISIBLE, NULL);
        pWnd->ShowScrollBar(SB_VERT, TRUE);
        pWnd->ModifyStyle(NULL, WS_VSCROLL, SWP_DRAWFRAME);

        pi.GetScrollInfo(pWnd, FALSE);

        ASSERT(::IsWindow(m_scrlBarVert.GetSafeHwnd()));
        m_scrlBarVert.SetScrollInfo(&pi.scrlInfoVert);

        pWnd->ShowScrollBar(SB_VERT, FALSE);
        pWnd->ModifyStyle(WS_VSCROLL, NULL, SWP_DRAWFRAME);
    //    pWnd->ModifyStyle(NULL, WS_VISIBLE);
    }
    else
    {
        pi.scrlInfoVert.cbSize = sizeof(SCROLLINFO);
        pWnd->GetScrollInfo(SB_VERT, &pi.scrlInfoVert);
    }

    if (pi.bHasScrollHorz || pi.bHasScrollVert) {
        pWnd->ShowScrollBar(SB_BOTH, FALSE);
        pWnd->ModifyStyle(WS_HSCROLL | WS_VSCROLL, NULL, SWP_DRAWFRAME);

        //pWnd->ShowScrollBar(SB_VERT, FALSE);
        //pWnd->ModifyStyle(WS_VSCROLL, NULL, SWP_DRAWFRAME);
        pWnd->ModifyStyle(NULL, WS_VISIBLE);
    }


    g_bUpdatingScrollInfo = FALSE;
}
