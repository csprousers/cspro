// XSplitter.cpp : implementation file
//

#include "StdAfx.h"
#include "DSplWnd.h"
#include <zUToolO/OxszDKB.h>


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define PANE_BORDER 1
#define PANE_SHARE  0
#define PANE_GAP    1
/////////////////////////////////////////////////////////////////////////////
// CDictSplitterWnd

CDictSplitterWnd::CDictSplitterWnd()
{
 /* m_cxSplitter = PANE_BORDER;     // = 1
  m_cySplitter=PANE_BORDER;     // = 1
  m_cxBorderShare=PANE_SHARE;   // = 0
  m_cyBorderShare=PANE_SHARE;   // = 0
  m_cxSplitterGap=PANE_GAP;     // = 1
  m_cySplitterGap=PANE_GAP;     // = */

}

CDictSplitterWnd::~CDictSplitterWnd()
{
}


BEGIN_MESSAGE_MAP(CDictSplitterWnd, CSplitterWnd)
    //{{AFX_MSG_MAP(CDictSplitterWnd)
    ON_WM_NCHITTEST()
    ON_WM_NCLBUTTONUP()
    ON_WM_LBUTTONUP()
    ON_WM_LBUTTONDOWN()
    ON_WM_MOUSEMOVE()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CDictSplitterWnd message handlers

LRESULT CDictSplitterWnd::OnNcHitTest(CPoint point)
{
    CDictChildWnd* pParentFrame = (CDictChildWnd*)GetParentFrame();
    if( pParentFrame && pParentFrame->GetQuestionnaireView() && pParentFrame->isQuestionnaireView()) {
        return 0;
    }
    else {
        return CSplitterWnd::OnNcHitTest(point);
    }
}

void CDictSplitterWnd::OnNcLButtonUp(UINT nHitTest, CPoint point)
{
    if (m_bTracking) {

        CDictChildWnd* pParentFrame = (CDictChildWnd*)GetParentFrame();
        CSplitterWnd::OnNcLButtonUp(nHitTest, point);
        pParentFrame->DisplayActiveMode();
        return;

    }
    CSplitterWnd::OnNcLButtonUp(nHitTest, point);
}

void CDictSplitterWnd::OnLButtonUp(UINT nFlags, CPoint point)
{
    if (m_bTracking) {
        CDictChildWnd* pParentFrame = (CDictChildWnd*)GetParentFrame();
        CSplitterWnd::OnLButtonUp(nFlags, point);
        int iMin = 0;
        GetRowInfo(0, pParentFrame->m_iGridViewPaneSize, iMin);
        pParentFrame->DisplayActiveMode();
        if (m_pRowInfo != NULL) {
            CRect rect;
            GetParentFrame()->GetWindowRect(&rect);
            GetParentFrame()->RedrawWindow(rect);
            return;
        }
        return;
    }
    CSplitterWnd::OnLButtonUp(nFlags, point);
}

void CDictSplitterWnd::OnLButtonDown(UINT nFlags, CPoint point)
{
    CDictChildWnd* pParentFrame = (CDictChildWnd*)GetParentFrame();

    if (pParentFrame && !pParentFrame->isQuestionnaireView()) {
        int     vSplitterBar1 = 101;
        int iRow = HitTest(point) - vSplitterBar1;
        if (iRow == 0) {//if it is first splitterbar do normal stuff
            CSplitterWnd::OnLButtonDown(nFlags, point);
            return;
        }
        else if (iRow == 1){
            return;

        }
    }
    CSplitterWnd::OnLButtonDown(nFlags, point);
}

void CDictSplitterWnd::OnMouseMove(UINT nFlags, CPoint point)
{
    // TODO: Add your message handler code here and/or call default
    CDictChildWnd* pParentFrame = (CDictChildWnd*)GetParentFrame();

    if (pParentFrame && !pParentFrame->isQuestionnaireView()) {
        int     vSplitterBar1 = 101;

        int iRow = HitTest(point) - vSplitterBar1;
        if (iRow == 0) {//if it is first splitterbar do normal stuff
            CSplitterWnd::OnMouseMove(nFlags, point);
            return;
        }
        else if (iRow == 1) {
            return;

        }
    }
    CSplitterWnd::OnMouseMove(nFlags, point);
}

void CDictSplitterWnd::SetSplitterBarSizes()
{
    CDictChildWnd* pParentFrame = (CDictChildWnd*)GetParentFrame();
    if (pParentFrame && pParentFrame->GetQuestionnaireView() && pParentFrame->isQuestionnaireView()) {
        m_cxSplitter = PANE_BORDER;     // = 1
        m_cySplitter = PANE_BORDER;     // = 1
        m_cxBorderShare = PANE_SHARE;   // = 0
        m_cyBorderShare = PANE_SHARE;   // = 0
        m_cxSplitterGap = PANE_GAP;     // = 1
        m_cySplitterGap = PANE_GAP;     // = 1
    }
    else {
        m_cxSplitter = m_cySplitter = 3 + 2 + 2;
        m_cxBorderShare = m_cyBorderShare = 0;
        m_cxSplitterGap = m_cySplitterGap = 3 + 2 + 2;
        m_cxBorder = m_cyBorder = 2;
    }
}
void CDictSplitterWnd::OnDrawSplitter(CDC* pDC, ESplitType nType, const CRect& rect)
{
    CSplitterWnd::OnDrawSplitter(pDC, nType, rect);
}
