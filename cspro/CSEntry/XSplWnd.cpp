// XSplitter.cpp : implementation file
//

#include "StdAfx.h"
#include "XSplWnd.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[]= __FILE__;
#endif

#define PANE_BORDER 1
#define PANE_SHARE  0
#define PANE_GAP    1
/////////////////////////////////////////////////////////////////////////////
// CXSplitterWnd

CXSplitterWnd::CXSplitterWnd()
{
  m_bUseQuestionText = false;
  m_cxSplitter=PANE_BORDER;     // = 1
  m_cySplitter=PANE_BORDER;     // = 1
  m_cxBorderShare=PANE_SHARE;   // = 0
  m_cyBorderShare=PANE_SHARE;   // = 0
  m_cxSplitterGap=PANE_GAP;     // = 1
  m_cySplitterGap=PANE_GAP;     // = 1

}

CXSplitterWnd::~CXSplitterWnd()
{
}


BEGIN_MESSAGE_MAP(CXSplitterWnd, CSplitterWnd)
    //{{AFX_MSG_MAP(CXSplitterWnd)
    ON_WM_NCHITTEST()
    ON_WM_SIZE()
    ON_WM_MOUSEMOVE()
    ON_WM_SETCURSOR()
    ON_WM_LBUTTONDOWN()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CXSplitterWnd message handlers
LRESULT CXSplitterWnd::OnNcHitTest(CPoint point)
{
    // TODO: Add your message handler code here and/or call default
    if(m_bUseQuestionText) {
        return CSplitterWnd::OnNcHitTest(point);
    }
    else {
      return 0;
    }
}

void CXSplitterWnd::OnSize(UINT nType, int cx, int cy)
{
    RECT        rect;
    int         iHeight =0;
    GetClientRect( &rect );

    if(!m_bUseQuestionText){
        iHeight =0;
    }
    else { //Do we want to  set a minimum then do here later
        //iHeight = rect.bottom - rect.top ;
        //iHeight = iHeight /4;
    }

    if ( m_pRowInfo != NULL && !m_bUseQuestionText){ //if not capi set the row info here
        if ( iHeight < 0 )
            SetRowInfo( 0, 1, 1);
        else
            SetRowInfo( 0, iHeight, iHeight );
    }

    CSplitterWnd::OnSize(nType, cx, cy);
}


void CXSplitterWnd::OnMouseMove(UINT nFlags, CPoint point)
{
    // TODO: Add your message handler code here and/or call default
        //lock the splitter
    bool bLockSplitter =true;
    if(m_bUseQuestionText && bLockSplitter) {
        return;
    }
    CSplitterWnd::OnMouseMove(nFlags, point);
}

BOOL CXSplitterWnd::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
    // TODO: Add your message handler code here and/or call default
    return CSplitterWnd::OnSetCursor(pWnd, nHitTest, message);
}

void CXSplitterWnd::OnLButtonDown(UINT nFlags, CPoint point)
{
    // TODO: Add your message handler code here and/or call default
    bool bLockSplitter =true;
    if(m_bUseQuestionText && bLockSplitter) {
        return;
    }
    CSplitterWnd::OnLButtonDown(nFlags, point);
}


void CXSplitterWnd::OnDrawSplitter(CDC *pDC, ESplitType nType,
      const CRect &rectArg)
{
  if (pDC == NULL)
  {
    RedrawWindow(rectArg, NULL, RDW_INVALIDATE|RDW_NOCHILDREN);
    return;
  }
  /**
  * If you want to see the splitter window,
  * Use the base class's drawing funcion;
  * otherwise, fill the Splitter with the View's color.
  */
    if(m_bUseQuestionText)
    {
        if(m_cxSplitter ==1) {// reset splitter size
            m_cxSplitter = m_cySplitter = 3 + 2 + 2;
            m_cxBorderShare = m_cyBorderShare = 0;
            m_cxSplitterGap = m_cySplitterGap = 3 + 2 + 2;
            m_cxBorder = m_cyBorder = 2;
        }
        CSplitterWnd::OnDrawSplitter(pDC,nType,rectArg);
        return;
    }
    else {
        m_cxSplitter=PANE_BORDER;     // = 1
        m_cySplitter=PANE_BORDER;     // = 1
        m_cxBorderShare=PANE_SHARE;   // = 0
        m_cyBorderShare=PANE_SHARE;   // = 0
        m_cxSplitterGap=PANE_GAP;     // = 1
        m_cySplitterGap=PANE_GAP;     // = 1
    }
   CSplitterWnd::OnDrawSplitter(pDC,nType,rectArg);
   return;
  /*ASSERT_VALID(pDC);
  CRect rect = rectArg;
  CPen WhitePen;
  WhitePen.CreatePen( PS_SOLID, 2, ::GetSysColor(COLOR_BTNFACE)); //Get this color later from runview
  pDC->SelectObject(&WhitePen);
  pDC->Rectangle(rect);*/
}
