//
//  GridEnt.cpp
//  Data Entry interface for CGridWnd
//

#include "StdAfx.h"


///////////////////////////////////////////////////////////////////////////////////////////////////
//
//                          CGridWnd::StartEdit()
//
// NOTE: Derived-class versions *must* call base-class StartEdit() !!!
///////////////////////////////////////////////////////////////////////////////////////////////////
/*V*/ void CGridWnd::StartEdit(CDEField* pFld, int iOcc)
{
    int iRow, iCol;
    FindField(pFld, iOcc, &iRow, &iCol);

    CGridCell& cell = GetCell(iRow, iCol);
    if (m_pEdit!=NULL)  {
        CRect rc(pFld->GetDims());
        CRect rcCell(cell.GetRect());
        rcCell.DeflateRect(GRIDCELL_BORDER, GRIDCELL_BORDER);
        rc.OffsetRect(rcCell.TopLeft());
        rc.DeflateRect(1,1); // remove extra pixel for border
        rc.OffsetRect(-GetScrollPos().x + GetRightJustifyOffset(), -GetScrollPos().y);
        ClientToScreen(&rc.TopLeft());
        GetParent()->ScreenToClient(&rc.TopLeft());
        ClientToScreen(&rc.BottomRight());
        GetParent()->ScreenToClient(&rc.BottomRight());

        CRect rcClient, rcTest;
        GetClientRect(&rcClient);
        rcTest = CRect(m_szHeader.cx, m_szHeader.cy, rcClient.right, rcClient.bottom);
        ClientToScreen(&rcTest);
        GetParent()->ScreenToClient(&rcTest);

        // hide the rect if outside of viewport area
        CRect rcIntersect;
        rcIntersect.IntersectRect(rc, rcTest);
        if (rcIntersect != rc)  {
            m_pEdit->ShowWindow(SW_HIDE);
        }
        else  {
            m_pEdit->ShowWindow(SW_SHOWNORMAL);
        }
        rc.top = rc.top+1;
        m_pEdit->MoveWindow(&rc);
        m_pEdit->SetFocus();
    }
}


///////////////////////////////////////////////////////////////////////////////////////////////////
//
//                          CGridWnd::SetFieldData()
//
///////////////////////////////////////////////////////////////////////////////////////////////////
void CGridWnd::SetFieldData(CDEField* pFld, int iOcc, const CString& csVal, bool bRedraw /*=false*/)
{
    int iRow, iCol;
    CCellField& fld = FindField(pFld, iOcc, &iRow, &iCol);
    bool bChanged=false;
    if (fld.GetVal().Compare(csVal)!=0)  {
        fld.SetVal(csVal);
        bChanged=true;
    }
    if (bRedraw)  {
        RedrawCell(iRow, iCol);
    }
    if (IsQueuing() && bChanged)  {
        CRect rc = pFld->GetDims() + GetCell(iRow, iCol).GetRect().TopLeft() + CSize(GRIDCELL_BORDER, GRIDCELL_BORDER);
        Queue(rc);
    }
}



///////////////////////////////////////////////////////////////////////////////////////////////////
//
//                          CGridWnd::GetFieldData()
//
///////////////////////////////////////////////////////////////////////////////////////////////////
const CString& CGridWnd::GetFieldData(CDEField* pFld, int iOcc)
{
    return FindField(pFld,iOcc).GetVal();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//
//                          CGridWnd::GoToField()
//
///////////////////////////////////////////////////////////////////////////////////////////////////
void CGridWnd::GoToField(CDEField* pFld, int iOcc)
{
    int iRow, iCol;
    iRow = -1;
    iCol = -1;
    FindField(pFld,iOcc, &iRow, &iCol);
    if(iRow == -1 || iCol == -1){
        return;
    }
    EnsureVisible(iRow, iCol);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//
//                          CGridWnd::SetFieldBColor()
//
///////////////////////////////////////////////////////////////////////////////////////////////////
void CGridWnd::SetFieldBColor(CDEField* pFld, int iOcc, const COLORREF& c, bool bRedraw /*=false*/)
{
    int iRow, iCol;
    CCellField& fld = FindField(pFld, iOcc, &iRow, &iCol);
    bool bChanged=false;
    if (fld.GetBColor()!=c)  {
        fld.SetBColor(c);
        bChanged=true;
    }
    if (bRedraw)  {
        RedrawCell(iRow, iCol);
    }
    if (IsQueuing() && bChanged)  {
        CRect rc = pFld->GetDims() + GetCell(iRow, iCol).GetRect().TopLeft() + CSize(GRIDCELL_BORDER, GRIDCELL_BORDER);
        Queue(rc);
//        Queue(GetCell(iRow,iCol).GetRect());
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//
//                          CGridWnd::GetFieldBColor()
//
///////////////////////////////////////////////////////////////////////////////////////////////////
COLORREF CGridWnd::GetFieldBColor(CDEField* pFld, int iOcc)
{
    return FindField(pFld,iOcc).GetBColor();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//
//                          CGridWnd::FindField
//
// returns a reference to the cell field
// you can optionally call with int *'s to retrieve row and col for pFld
///////////////////////////////////////////////////////////////////////////////////////////////////
CCellField& CGridWnd::FindField(CDEField* pFld, int iOcc, int* piRow /*=NULL*/, int* piCol /*=NULL*/)
{
    bool bFound=false;
    int iCol, iRow, iFld, iNumRows, iNumCols;
    iNumCols = GetNumCols();
    iNumRows = GetNumRows();
    iFld = NONE;

    if (m_pRoster->GetOrientation() == RosterOrientation::Horizontal)  {
        for (iCol=1 ; iCol<iNumCols && !bFound; iCol++)  {
            CGridCell& cell = GetCell(1,iCol);
            for (iFld=0 ; iFld<cell.GetNumFields() ; iFld++)  {
                if (cell.GetField(iFld).GetDEField()==pFld)  {
                    bFound=true;
                    break;
                }
            }
        }
        ASSERT(bFound);
        iCol--;
        iRow = iOcc;
    }
    else  {
        // VERTICAL orientation
        for (iRow=1 ; iRow<iNumRows && !bFound ; iRow++)  {
            CGridCell& cell = GetCell(iRow,1);
            for (iFld=0 ; iFld<cell.GetNumFields() ; iFld++)  {
                if (cell.GetField(iFld).GetDEField()==pFld)  {
                    bFound=true;
                    break;
                }
            }
        }
        ASSERT(bFound);
        iRow--;
        iCol = iOcc;

        // first occ is on right for right to left roster
        if (m_pRoster->GetRightToLeft()) {
            iCol = iNumCols - iCol;
        }
    }
    ASSERT(iRow>0 && iRow<iNumRows);
    ASSERT(iCol>0 && iCol<iNumCols);
    ASSERT(iFld>=0 && iFld<GetCell(iRow,iCol).GetNumFields());
    if (piRow!=NULL)  {
        *piRow = iRow;
    }
    if (piCol!=NULL)  {
        *piCol = iCol;
    }
    return GetCell(iRow, iCol).GetField(iFld);
}


void CGridWnd::StartQueue(void)
{
    ASSERT(!m_bQueue);
    m_aQueue.RemoveAll();
    m_bQueue = true;
}


void CGridWnd::Queue(CRect& rc)
{
    ASSERT(IsQueuing());
    m_aQueue.Add(rc);
}

void CGridWnd::StopQueue(void)
{
    ASSERT(IsQueuing());
    //CPoint pt(GetScrollPos());
    //Savy to fix the right to left rosters bug with paint.  If there is empty area on the left
    //for RTL rosters. The "smart render" does not redraw all the cells. 'Cos it does not take
    //into consideration the offset
    CPoint pt(GetScrollPos() - CPoint(GetRightJustifyOffset(),0));
    for (int i=0 ; i<m_aQueue.GetSize() ; i++)  {
        CRect& rc = m_aQueue[i];
        rc.OffsetRect(-pt);
        InvalidateRect(&rc);
    }
    UpdateWindow();
    m_aQueue.RemoveAll();
    m_bQueue = false;
}
