// ==========================================================================
//              Class Implementation : COXGridHeader
// ==========================================================================

// Source file : OXGridHdr.cpp

// Copyright © Dundas Software Ltd. 1997 - 1998, All Rights Reserved

// //////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "oxghdr.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

IMPLEMENT_DYNAMIC(COXGridHeader, CHeaderCtrl)

#define new DEBUG_NEW

#define GET_X_LPARAM(lp)  ((int)(short)LOWORD(lp))
#define GET_Y_LPARAM(lp)  ((int)(short)HIWORD(lp))


/////////////////////////////////////////////////////////////////////////////
// Definition of static members
// Data members -------------------------------------------------------------
// protected:

// private:

// Member functions ---------------------------------------------------------
// public:

COXGridHeader::COXGridHeader()
    : m_bResizing(TRUE), m_nSortCol(-1), m_nSortOrder(0)
    {
    ASSERT_VALID(this);
    }


#ifdef _DEBUG
void COXGridHeader::AssertValid() const
    {
    CHeaderCtrl::AssertValid();
    }

void COXGridHeader::Dump(CDumpContext& dc) const
    {
    CHeaderCtrl::Dump(dc);
    }
#endif //_DEBUG

COXGridHeader::~COXGridHeader()
    {
    }

BEGIN_MESSAGE_MAP(COXGridHeader, CHeaderCtrl)
    //{{AFX_MSG_MAP(COXGridHeader)
    ON_WM_SETCURSOR()
    //}}AFX_MSG_MAP
    ON_NOTIFY_REFLECT_EX(HDN_BEGINTRACKW, OnHdrCtrlNotify)
    ON_NOTIFY_REFLECT_EX(HDN_BEGINTRACKA, OnHdrCtrlNotify)
    ON_NOTIFY_REFLECT_EX(HDN_DIVIDERDBLCLICKW, OnHdrCtrlNotify)
    ON_NOTIFY_REFLECT_EX(HDN_DIVIDERDBLCLICKA, OnHdrCtrlNotify)
END_MESSAGE_MAP()

BOOL COXGridHeader::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
    {
    if (!m_bResizing)
        {
        DWORD dwMessagePos=::GetMessagePos();
        CPoint point(GET_X_LPARAM(dwMessagePos),GET_Y_LPARAM(dwMessagePos));

        // point allways in screen coordinates, but HDM_HITTEST needs client coordinates
        ScreenToClient(&point);

        // query the header for the area where it is over for the moment
        HD_HITTESTINFO hdhti;
        hdhti.pt = point;
        SendMessage(HDM_HITTEST,(WPARAM)0,(LPARAM) (HD_HITTESTINFO FAR *)&hdhti);

        // If the mouse is over a divider line, halt processing.  This way a standard cursor
        // will be shown.
        if ((hdhti.flags & HHT_ONDIVIDER) == HHT_ONDIVIDER)
            return TRUE;
        }

    return CHeaderCtrl::OnSetCursor(pWnd, nHitTest, message);
    }

BOOL COXGridHeader::OnHdrCtrlNotify(NMHDR* pNMHDR, LRESULT* pResult)
    {
    *pResult = 0;

    switch (pNMHDR->code)
        {
        case HDN_BEGINTRACKW:
        case HDN_BEGINTRACKA:
        case HDN_DIVIDERDBLCLICKW:
        case HDN_DIVIDERDBLCLICKA:
            *pResult = TRUE & !m_bResizing;
            return TRUE & !m_bResizing;
        }

    return FALSE;
    }

// protected:

void COXGridHeader::DrawItem( LPDRAWITEMSTRUCT lpDrawItemStruct )
{
    CDC dc;
    dc.Attach( lpDrawItemStruct->hDC );
    // Get the column rect
    CRect rcLabel( lpDrawItemStruct->rcItem );
    // Save DC
    int nSavedDC = dc.SaveDC();
    // Set clipping region to limit drawing within column
    CRgn rgn;
    rgn.CreateRectRgnIndirect( &rcLabel );
    dc.SelectObject( &rgn );
    rgn.DeleteObject();
    // Draw the background
    CBrush brush(::GetSysColor(COLOR_3DFACE));
    dc.FillRect(rcLabel,&brush);
    // Labels are offset by a certain amount
    // This offset is related to the width of a space character
    int offset = dc.GetTextExtent(_T(" "), 1 ).cx*2;
    // Get the column text and format
    TCHAR buf[256];
    HD_ITEM hditem;
    hditem.mask = HDI_TEXT | HDI_FORMAT;
    hditem.pszText = buf;
    hditem.cchTextMax = 255;
    GetItem( lpDrawItemStruct->itemID, &hditem );
    // Determine format for drawing column label
    UINT uFormat = DT_SINGLELINE | DT_NOPREFIX | DT_NOCLIP |
        DT_VCENTER | DT_END_ELLIPSIS ;
    if( hditem.fmt & HDF_CENTER)
        uFormat |= DT_CENTER;
    else if( hditem.fmt & HDF_RIGHT)
        uFormat |= DT_RIGHT;
    else
        uFormat |= DT_LEFT;
    // Adjust the rect if the mouse button is pressed on it
    if( lpDrawItemStruct->itemState == ODS_SELECTED )
    {
        rcLabel.left++;
        rcLabel.top += 2;
        rcLabel.right++;
    }
    // Adjust the rect further if Sort arrow is to be displayed
    if( lpDrawItemStruct->itemID == (UINT)m_nSortCol )
    {
        rcLabel.right -= 3 * offset;
    }
    rcLabel.left += offset;
    rcLabel.right -= offset;
    // Draw column label
    if( rcLabel.left < rcLabel.right )
        dc.DrawText(buf,-1,rcLabel, uFormat);
    // Draw the Sort arrow
    if( m_nSortOrder!=0 && lpDrawItemStruct->itemID == (UINT)m_nSortCol )
    {
        CRect rcIcon( lpDrawItemStruct->rcItem );
        // Set up pens to use for drawing the triangle
        CPen penLight(PS_SOLID, 1, GetSysColor(COLOR_3DHILIGHT));
        CPen penShadow(PS_SOLID, 1, GetSysColor(COLOR_3DSHADOW));
        CPen *pOldPen = dc.SelectObject( &penLight );
        if( m_nSortOrder==1 )
        {
            // Draw triangle pointing upwards
            dc.MoveTo( rcIcon.right - 2*offset, offset-1);
            dc.LineTo( rcIcon.right - 3*offset/2, rcIcon.bottom - offset );
            dc.LineTo( rcIcon.right - 5*offset/2-2, rcIcon.bottom - offset );
            dc.MoveTo( rcIcon.right - 5*offset/2-1, rcIcon.bottom - offset-1 );
            dc.SelectObject( &penShadow );
            dc.LineTo( rcIcon.right - 2*offset, offset-2);
        }
        else
        {
            // Draw triangle pointing downwords
            dc.MoveTo( rcIcon.right - 3*offset/2, offset-1);
            dc.LineTo( rcIcon.right - 2*offset-1, rcIcon.bottom - offset + 1 );
            dc.MoveTo( rcIcon.right - 2*offset-1, rcIcon.bottom - offset );
            dc.SelectObject( &penShadow );
            dc.LineTo( rcIcon.right - 5*offset/2-1, offset -1 );
            dc.LineTo( rcIcon.right - 3*offset/2, offset -1);
        }
        // Restore the pen
        dc.SelectObject( pOldPen );
    }
    // Restore dc
    dc.RestoreDC( nSavedDC );
    // Detach the dc before returning
    dc.Detach();
}

BOOL COXGridHeader::SortColumn(int nCol, int nSortOrder)
{
    ASSERT(nCol>=0 && nCol<=GetItemCount());

    CWnd* pParentWnd=GetParent();
    ASSERT(pParentWnd->IsKindOf(RUNTIME_CLASS(CListCtrl)));

//  if(nCol!=m_nSortCol)
    {
        // Change the item from owner drawn
        HD_ITEM hditem;
        hditem.mask = HDI_FORMAT;
        GetItem(m_nSortCol, &hditem);
        hditem.fmt &= ~HDF_OWNERDRAW;
        hditem.fmt |= HDF_STRING;
        SetItem(m_nSortCol, &hditem);

        if(nSortOrder!=0)
        {
            // Change the item to owner drawn
            HD_ITEM hditem;
            hditem.mask = HDI_FORMAT;
            GetItem(nCol, &hditem);
            hditem.fmt |= HDF_OWNERDRAW;
            SetItem(nCol, &hditem);
        }

        // Invalidate header control so that it gets redrawn
        Invalidate();
    }

    m_nSortCol=nCol;
    m_nSortOrder=nSortOrder;

    return TRUE;
}

// private:

// ==========================================================================
