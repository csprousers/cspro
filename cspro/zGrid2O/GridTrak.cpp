// GridTrak.cpp
// for CGridRectTracker ...

#include "StdAfx.h"

/////////////////////////////////////////////////////////////////////////////////
//
//          CGridRectTracker::AdjustRect()
//
/////////////////////////////////////////////////////////////////////////////////
void CGridRectTracker::AdjustRect(int iHandle, LPRECT lpRect)
{
//CString cs;
//cs.Format("CGridRectTracker::AdjustRect -- %d %d %d %d, handle=%d\n", lpRect->left, lpRect->top, lpRect->right, lpRect->bottom, iHandle);
//TRACE(cs);
    CRect rcOverlap, rcNew(*lpRect);
    BOOL bHorzOK, bVertOK;
    static CRect rcPrev=m_rect;

    if (IsFirstTime())  {
        rcPrev = m_rect;
        m_bFirstTime=false;
    }

    if (iHandle==8)  {
        // user is moving the tracker ...
        // make sure we're within cell's boundaries
        ::SetCursor(::LoadCursor(NULL, IDC_SIZEALL));
        if (rcNew.left<GetBoundingRect().left)  {
            rcNew.OffsetRect(GetBoundingRect().left - rcNew.left,0);
        }
        if (rcNew.right>GetBoundingRect().right)  {
            rcNew.OffsetRect(GetBoundingRect().right-rcNew.right,0);
        }
        if (rcNew.top<GetBoundingRect().top)  {
            rcNew.OffsetRect(0,GetBoundingRect().top-rcNew.top);
        }
        if (rcNew.bottom>GetBoundingRect().bottom)  {
            rcNew.OffsetRect(0,GetBoundingRect().bottom-rcNew.bottom);
        }

        if (GetTrackObject()==CGridRectTracker::trackField)  {
            // make sure we don't overlap with another field (only important when moving fields)
            for (int i=0 ; i<GetNumNoOverlapRects() ; i++)  {
                CRect& rcTest = GetNoOverlapRect(i);
                if (rcOverlap.IntersectRect(rcTest, rcNew))  {
                    // we overlap with another field; see if we can accomodate just the
                    // horizontal or vertical part of the movement the user is attempting
                    bHorzOK = rcOverlap.IntersectRect(CRect(rcPrev.left, rcNew.top, rcPrev.right, rcNew.bottom), rcTest);
                    bVertOK = rcOverlap.IntersectRect(CRect(rcNew.left, rcPrev.top, rcNew.right, rcPrev.bottom), rcTest);
                    if (!bHorzOK)  {
                        rcNew.left = rcPrev.left;
                        rcNew.right = rcPrev.right;
                    }
                    if (!bVertOK)  {
                        rcNew.top = rcPrev.top;
                        rcNew.bottom = rcPrev.bottom;
                    }
                    if (bHorzOK && bVertOK)  {
                        // trying to drag across a corner
                        rcNew = rcPrev;
                    }
                }
            }
        }
    }
    else if (iHandle>=0 && iHandle<=7)  {
        if ((GetTrackObject()==CGridRectTracker::trackField && !m_bAlwaysAllowSizing) || GetTrackObject()==CGridRectTracker::trackMultiple)  {
            // can't resize fields!
            CRect rcTest;
            if (rcTest.IntersectRect(rcPrev, m_rcBounding)==0)  {
                // special case where the user's first click is on a resize handle, and rcPrev is for a previously selected field
                rcPrev = rcNew;
            }
            rcNew = rcPrev;
        }
        else  {
            // user is resizing the tracker...
            if (rcNew.Size().cx<m_sizeMin.cx)  {
                rcNew.right = rcNew.left + m_sizeMin.cx;
            }
            if (rcNew.Size().cy<m_sizeMin.cy)  {
                rcNew.bottom = rcNew.top + m_sizeMin.cy;
            }
            rcOverlap.UnionRect(GetBoundingRect(), rcNew);
            if (rcOverlap!=GetBoundingRect())  {
                // trying to resize outside of cell boundaries
                const CRect& rcBound = GetBoundingRect();
                rcOverlap.UnionRect(CRect(rcBound.left, rcNew.top, rcBound.right, rcNew.bottom), rcBound);
                bHorzOK = (rcOverlap!=rcBound);
                rcOverlap.UnionRect(CRect(rcNew.left, rcBound.top, rcNew.right, rcBound.bottom), rcBound);
                bVertOK = (rcOverlap!=rcBound);
                if (!bHorzOK)  {
                    rcNew.left = rcPrev.left;
                    rcNew.right = rcPrev.right;
                }
                if (!bVertOK)  {
                    rcNew.top = rcPrev.top;
                    rcNew.bottom = rcPrev.bottom;
                }
                if (bHorzOK && bVertOK)  {
                    // trying to resize down and out at same time
                    rcNew = rcPrev;
                }
            }
        }
    }
    *lpRect = rcNew;
    rcPrev = rcNew;
}


BOOL CGridRectTracker::SetCursor(CWnd* pWnd, UINT nHitTest, bool bForceNoResize) const
{
    if(m_bAlwaysAllowSizing){//for new unicode textfields
        return CRectTracker::SetCursor(pWnd, nHitTest);
    }
    if (GetTrackObject()==CGridRectTracker::trackField || GetTrackObject()==CGridRectTracker::trackMultiple || bForceNoResize)  {
        // field tracking never shows resize arrows, just the 4-arrow movement
        CPoint pt;
        ::GetCursorPos(&pt);
        pWnd->ScreenToClient(&pt);
        int nHandle = HitTestHandles(pt);
        if (nHandle < 0)  {
            return FALSE;
        }
        ::SetCursor(::LoadCursor(NULL, IDC_SIZEALL));
        return TRUE;
    }
    return CRectTracker::SetCursor(pWnd, nHitTest);
}


