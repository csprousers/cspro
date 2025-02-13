// XSplitter.cpp : implementation file
//

#include "StdAfx.h"
#include "FSplWnd.h"
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
// CFSplitterWnd

int iCurRow =-1;
CFSplitterWnd::CFSplitterWnd()
{
  m_cxSplitter=PANE_BORDER;     // = 1
  m_cySplitter=PANE_BORDER;     // = 1
  m_cxBorderShare=PANE_SHARE;   // = 0
  m_cyBorderShare=PANE_SHARE;   // = 0
  m_cxSplitterGap=PANE_GAP;     // = 1
  m_cySplitterGap=PANE_GAP;     // = 1

  m_pHiddenWnd = NULL;

}

CFSplitterWnd::~CFSplitterWnd()
{
}


BEGIN_MESSAGE_MAP(CFSplitterWnd, CSplitterWnd)
    //{{AFX_MSG_MAP(CFSplitterWnd)
    ON_WM_NCHITTEST()
    ON_WM_MOUSEMOVE()
    ON_WM_SETCURSOR()
    ON_WM_LBUTTONDOWN()
    ON_WM_NCLBUTTONUP()
    ON_WM_LBUTTONUP()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CFSplitterWnd message handlers

LRESULT CFSplitterWnd::OnNcHitTest(CPoint point)
{
    // TODO: Add your message handler code here and/or call default
    CFormChildWnd* pParentFrame = (CFormChildWnd*)GetParentFrame();
    if( pParentFrame && pParentFrame->GetUseQuestionText() ) {
        return CSplitterWnd::OnNcHitTest(point);
    }
    else {
      return 0;
    }
}

void CFSplitterWnd::OnMouseMove(UINT nFlags, CPoint point)
{
    // TODO: Add your message handler code here and/or call default
    CFormChildWnd* pParentFrame = (CFormChildWnd*)GetParentFrame();

    if( pParentFrame && pParentFrame->GetUseQuestionText() && pParentFrame->GetViewMode() != QSFEditorViewMode ) {
        int     vSplitterBar1           = 101;

        int iRow = HitTest(point) -vSplitterBar1;
        if(iRow ==0){//if it is first splitterbar do normal stuff
            CSplitterWnd::OnMouseMove(nFlags, point);
            return;
        }
        else if(iRow ==1){
            return ;

        }
    }
    CSplitterWnd::OnMouseMove(nFlags, point);
}

BOOL CFSplitterWnd::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
    // TODO: Add your message handler code here and/or call default
    return CSplitterWnd::OnSetCursor(pWnd, nHitTest, message);
}

void CFSplitterWnd::OnLButtonDown(UINT nFlags, CPoint point)
{
    CFormChildWnd* pParentFrame = (CFormChildWnd*)GetParentFrame();

    if( pParentFrame && pParentFrame->GetUseQuestionText() && pParentFrame->GetViewMode() != QSFEditorViewMode ) {
        int     vSplitterBar1           = 101;
        int iRow = HitTest(point) -vSplitterBar1;
        if(iRow ==0){//if it is first splitterbar do normal stuff
            CSplitterWnd::OnLButtonDown(nFlags, point);
            return;
        }
        else if(iRow ==1) {
            return ;

        }
    }
    CSplitterWnd::OnLButtonDown(nFlags, point);
}


void CFSplitterWnd::OnDrawSplitter(CDC *pDC, ESplitType nType,
      const CRect &rectArg)
{
    // if pDC == NULL, then just invalidate
    if (pDC == NULL)
    {
        RedrawWindow(rectArg, NULL, RDW_INVALIDATE|RDW_NOCHILDREN);
        return;
    }
    ASSERT_VALID(pDC);

    // otherwise, actually draw
    CRect rect = rectArg;
    switch (nType)
    {
    case splitBorder:
        if(iCurRow != -1) {
            if(iCurRow == 0 || iCurRow == 1) {
                pDC->Draw3dRect(rect, afxData.clrBtnShadow, afxData.clrBtnHilite);
            }
            if(iCurRow == 1) {
            /*  rect.top -= -CX_BORDER;
                rect.left -= -CY_BORDER;

                rect.bottom -= -CX_BORDER;
                rect.right -= -CY_BORDER;*/
                rect.InflateRect(-CX_BORDER, -CY_BORDER);

            }
            else {
                rect.InflateRect(-CX_BORDER, -CY_BORDER);
            }
            if(iCurRow == 0  || iCurRow == 1) {
                pDC->Draw3dRect(rect, afxData.clrWindowFrame, afxData.clrBtnFace);
            }
        }
        else {
            pDC->Draw3dRect(rect, afxData.clrBtnShadow, afxData.clrBtnHilite);
            rect.InflateRect(-CX_BORDER, -CY_BORDER);
            pDC->Draw3dRect(rect, afxData.clrWindowFrame, afxData.clrBtnFace);
        }
        return;

    case splitIntersection:
        ASSERT(0);
        break;

    case splitBox:
        pDC->Draw3dRect(rect, afxData.clrBtnFace, afxData.clrWindowFrame);
        rect.InflateRect(-CX_BORDER, -CY_BORDER);
        pDC->Draw3dRect(rect, afxData.clrBtnHilite, afxData.clrBtnShadow);
        rect.InflateRect(-CX_BORDER, -CY_BORDER);
        break;
        // fall through...
    case splitBar:
        /*
        {//it does not hit this for newer Windows versions  I am not worrying about this now
            pDC->Draw3dRect(rect, afxData.clrBtnHilite, afxData.clrBtnShadow);
            rect.InflateRect(-CX_BORDER, -CY_BORDER);
        }
        */
        break;

    default:
        ASSERT(FALSE);  // unknown splitter type
    }

    // fill the middle
    COLORREF clr = afxData.clrBtnFace;
    pDC->FillSolidRect(rect, clr);
    /*ASSERT_VALID(pDC);
    CRect rect = rectArg;
    CPen WhitePen;
    WhitePen.CreatePen( PS_SOLID, 2, ::GetSysColor(COLOR_BTNFACE)); //Get this color later from runview
    pDC->SelectObject(&WhitePen);
    pDC->Rectangle(rect);*/
}



/////////////////////////////////////////////////////////////////////////////////
//
//  void CFSplitterWnd::OnNcLButtonUp(UINT nHitTest, CPoint point)
//
/////////////////////////////////////////////////////////////////////////////////
void CFSplitterWnd::OnNcLButtonUp(UINT nHitTest, CPoint point)
{
    // TODO: Add your message handler code here and/or call default
    if (m_bTracking) {

        CFormChildWnd* pParentFrame = (CFormChildWnd*)GetParentFrame();
        CSplitterWnd::OnNcLButtonUp(nHitTest, point);
        pParentFrame->DisplayActiveMode();

        return;

    }
    CSplitterWnd::OnNcLButtonUp(nHitTest, point);
}

void CFSplitterWnd::OnLButtonUp(UINT nFlags, CPoint point)
{
    if (m_bTracking) {

        CFormChildWnd* pParentFrame = (CFormChildWnd*)GetParentFrame();
        CSplitterWnd::OnLButtonUp(nFlags, point);
        int iMin=0;
        if( pParentFrame && pParentFrame->GetUseQuestionText() ) {
            bool bMultiMode = pParentFrame->m_bMultiLangMode && !pParentFrame->m_bHideSecondLang;
            if(pParentFrame->GetViewMode() == FormViewMode) {
                GetRowInfo(0, pParentFrame->m_iQuestPaneSz, iMin);

                CFormDoc* pFormDoc = (CFormDoc*)pParentFrame->GetActiveDocument();
                CFormScrollView* pFormView = (CFormScrollView*)pFormDoc->GetView();
                if(pParentFrame->m_iQuestPaneSz != (int)pFormView->GetCurForm()->GetQuestionTextHeight()) {
                    pFormView->GetCurForm()->SetQuestionTextHeight(pParentFrame->m_iQuestPaneSz);
                    pFormDoc->SetModifiedFlag(true);
                }
            }
            else if(pParentFrame->GetViewMode() == QSFEditorViewMode && !bMultiMode) {
                GetRowInfo(1,pParentFrame->m_iQsfVSz1,iMin);
                pParentFrame->m_iQsfVSz2 = 0;
            }
            else if(pParentFrame->GetViewMode() == QSFEditorViewMode && bMultiMode) {
                GetRowInfo(0,pParentFrame->m_iQsfVSz1,iMin);
                GetRowInfo(1,pParentFrame->m_iQsfVSz2,iMin);
            }
        }
        pParentFrame->DisplayActiveMode();

        if ( m_pRowInfo != NULL ){
            CRect rect;
            GetParentFrame()->GetWindowRect(&rect);
            GetParentFrame()->RedrawWindow(rect);

        }
        return;

    }
    CSplitterWnd::OnLButtonUp(nFlags, point);
}

void CFSplitterWnd::DrawAllSplitBars(CDC* pDC, int cxInside, int cyInside)
{
    ASSERT_VALID(this);

    // draw column split bars
    CRect rect;
    GetClientRect(rect);
    rect.left += m_cxBorder;
    for (int col = 0; col < m_nCols - 1; col++)
    {
        rect.left += m_pColInfo[col].nCurSize + m_cxBorderShare;
        rect.right = rect.left + m_cxSplitter;
        if (rect.left > cxInside)
            break;      // stop if not fully visible
        OnDrawSplitter(pDC, splitBar, rect);
        rect.left = rect.right + m_cxBorderShare;
    }

    // draw row split bars
    GetClientRect(rect);
    rect.top += m_cyBorder;
    for (int row = 0; row < m_nRows - 1; row++)
    {
        SetSplitterBarSizes(row);

        rect.top += m_pRowInfo[row].nCurSize + m_cyBorderShare;
        rect.bottom = rect.top + m_cySplitter;
        if (rect.top > cyInside)
            break;      // stop if not fully visible
        OnDrawSplitter(pDC, splitBar, rect);
        rect.top = rect.bottom + m_cyBorderShare;
    }

    // draw pane borders
    GetClientRect(rect);
    int x = rect.left;
    for (int col = 0; col < m_nCols; col++)
    {
        int cx = m_pColInfo[col].nCurSize + 2*m_cxBorder;
        if (col == m_nCols-1 && m_bHasVScroll)
            cx += afxData.cxVScroll - CX_BORDER;
        int y = rect.top;
        for (int row = 0; row < m_nRows; row++)
        {
            SetSplitterBarSizes(row);

            int cy = m_pRowInfo[row].nCurSize + 2*m_cyBorder;
            if (row == m_nRows-1 && m_bHasHScroll)
                cy += afxData.cyHScroll - CX_BORDER;
            OnDrawSplitter(pDC, splitBorder, CRect(x, y, x+cx, y+cy));
            y += cy + m_cySplitterGap - 2*m_cyBorder;
        }
        x += cx + m_cxSplitterGap - 2*m_cxBorder;
    }
}


/////////////////////////////////////////////////////////////////////////////////
//
//  void CFSplitterWnd::SetSplitterBarSizes()
//
/////////////////////////////////////////////////////////////////////////////////
void CFSplitterWnd::SetSplitterBarSizes(int iRow /*=-1*/)
{
    CFormChildWnd* pParentFrame = (CFormChildWnd*)GetParentFrame();

    if( pParentFrame && pParentFrame->GetUseQuestionText() && pParentFrame->GetViewMode() != LogicViewMode ) { //later on check for logic view / editor mode
    /*  if(iRow == 1 || iCurRow == 2) {
            iCurRow = iRow;
        }
        else {
            iCurRow = -1;
        }*/
        if(pParentFrame->GetViewMode() != QSFEditorViewMode){
            iCurRow = iRow;
        }
        else {
            iCurRow = -1;
        }
        m_cxSplitter = m_cySplitter = 3 + 2 + 2;
        m_cxBorderShare = m_cyBorderShare = 0;
        m_cxSplitterGap = m_cySplitterGap = 3 + 2 + 2;
        m_cxBorder = m_cyBorder = 2;
        return;

    }
    else {
        iCurRow = -1;
        m_cxSplitter=PANE_BORDER;     // = 1
        m_cySplitter=PANE_BORDER;     // = 1
        m_cxBorderShare=PANE_SHARE;   // = 0
        m_cyBorderShare=PANE_SHARE;   // = 0
        m_cxSplitterGap=PANE_GAP;     // = 1
        m_cySplitterGap=PANE_GAP;     // = 1
    }
    return;
}
