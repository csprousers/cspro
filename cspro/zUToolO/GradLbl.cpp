//***************************************************************************
//  File name: GradLbl.cpp
//
//  Description:
//  Custom draw CStatic that draws a gradient background under the text.
//
//***************************************************************************

#include "StdAfx.h"
#include "GradLbl.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CGradientLabel

/////////////////////////////////////////////////////////////////////////////////
//                      CGradientLabel::CGradientLabel
// Constructor
/////////////////////////////////////////////////////////////////////////////////
CGradientLabel::CGradientLabel()
{
}

/////////////////////////////////////////////////////////////////////////////////
//                      CGradientLabel::~CGradientLabel
// Destructor
/////////////////////////////////////////////////////////////////////////////////
CGradientLabel::~CGradientLabel()
{
}

/////////////////////////////////////////////////////////////////////////////////
//                      CGradientLabel::PreSubclassWindow
// Set style to owner draw
/////////////////////////////////////////////////////////////////////////////////
void CGradientLabel::PreSubclassWindow()
{
    CStatic::PreSubclassWindow();
    ModifyStyle(0, SS_OWNERDRAW);
}

BEGIN_MESSAGE_MAP(CGradientLabel, CStatic)
    ON_WM_DRAWITEM_REFLECT()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CGradientLabel message handlers


/////////////////////////////////////////////////////////////////////////////////
//                      CGradientLabel::DrawItem
// Draw gradient and text.
/////////////////////////////////////////////////////////////////////////////////
void CGradientLabel::DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct)
{
    ASSERT(lpDrawItemStruct != NULL);

    CDC dc;
    dc.Attach(lpDrawItemStruct->hDC);
    CRect r = lpDrawItemStruct->rcItem;

    // draw gradient
    COLORREF    clrLeft = GetSysColor(COLOR_INACTIVECAPTION);
    COLORREF    clrRight = GetSysColor(CTLCOLOR_DLG);
    DrawGradient(dc, r, clrLeft, clrRight);

    // draw text
    COLORREF oldClr = dc.SetTextColor(GetSysColor(COLOR_CAPTIONTEXT));
    int oldBkMode = dc.SetBkMode(TRANSPARENT);
    CFont* pOldFont = (CFont*)dc.SelectStockObject(SYSTEM_FONT);
    r.left += 3; // offset text from edge a touch
    CString sWindowText;
    GetWindowText(sWindowText);
    dc.DrawText(sWindowText, r, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
    dc.SetTextColor(oldClr);
    dc.SetBkMode(oldBkMode);
    dc.SelectObject(pOldFont);

    dc.Detach();
}

/////////////////////////////////////////////////////////////////////////////////
//                      CGradientLabel::DrawGradient
// Helper function to draw gradient rectangle.
/////////////////////////////////////////////////////////////////////////////////
void CGradientLabel::DrawGradient(CDC& dc, const CRect& r, COLORREF clrLeft, COLORREF clrRight)
{
    TRIVERTEX        vert[2] ;
    GRADIENT_RECT    gRect;
    vert [0].x      = r.left;
    vert [0].y      = r.top;
    vert [0].Red    = (COLOR16) ((clrLeft & 0x000000FF)<<8);
    vert [0].Green  = (COLOR16) (clrLeft & 0x0000FF00);
    vert [0].Blue   = (COLOR16) ((clrLeft & 0x00FF0000)>>8);
    vert [0].Alpha  = 0x0000;
    vert [1].x      = r.right;
    vert [1].y      = r.bottom;
    vert [1].Red    = 0x0000;
    vert [1].Red    = (COLOR16) ((clrRight & 0x000000FF)<<8);
    vert [1].Green  = (COLOR16) (clrRight & 0x0000FF00);
    vert [1].Blue   = (COLOR16) ((clrRight & 0x00FF0000)>>8);
    gRect.UpperLeft  = 0;
    gRect.LowerRight = 1;
    dc.GradientFill(vert,2,&gRect,1,GRADIENT_FILL_RECT_H);
}
