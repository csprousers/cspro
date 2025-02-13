// RectExt.cpp: implementation of the CRectExt class.
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "Rectext.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]= __FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CRectExt::CRectExt()
{

}

CRectExt::~CRectExt()
{

}

bool CRectExt::BestPos(const CRect* parentRect, const CRect* fieldRect, const int iPosition, CRect* bestRect) {
    CRect   chkRect, interRect;
    int     dlgWidth, dlgHeight, fldWidth, fldHeight, parentWidth, parentHeight;
    int     top, bottom, left, right;
    bool    bChk = FALSE;

    dlgWidth = Width();
    dlgHeight = Height();
    fldWidth = fieldRect->Width();
    fldHeight = fieldRect->Height();
    parentWidth = parentRect->Width();
    parentHeight = parentRect->Height();

    // The father must be greater the the son
    if( parentWidth < dlgWidth || parentHeight < dlgHeight ) {
        CRectExt auxRect;

        auxRect.CopyRect( this );
        bChk = auxRect.CenterRect( NULL ); // Full-Screen center
        if( bChk )
            bestRect->CopyRect( &auxRect );
        else
            bestRect->CopyRect( this ); // original rect
        return( bChk );
    }


    if( iPosition & CRECTEX_FIELD ) {
        //Check right field side with left dialog side
        if (!bChk) {
            for (int i = 0; i<dlgHeight+fldHeight; i++) {
                top     = fieldRect->top - dlgHeight + i;
                left    = fieldRect->right;
                bottom  = top + dlgHeight;
                right   = left + dlgWidth;
                chkRect.SetRect(left, top, right, bottom);

                interRect.IntersectRect(*parentRect, chkRect);
                if (interRect == chkRect) {
                    bChk = TRUE;
                    break;
                }
            }
        }

        //Check top field side with bottom dialog side
        if (!bChk) {
            for (int i = 0; i<dlgWidth+fldWidth; i++) {
                top     = fieldRect->top - dlgHeight;
                left    = fieldRect->right - i;
                bottom  = top + dlgHeight;
                right   = left + dlgWidth;
                chkRect.SetRect(left, top, right, bottom);

                interRect.IntersectRect(*parentRect, chkRect);
                if (interRect == chkRect) {
                    bChk = TRUE;
                    break;
                }
            }
        }

        //Check bottom field side with top dialog side
        if (!bChk) {
            for (int i = 0; i<dlgWidth+fldWidth; i++) {
                top     = fieldRect->bottom;
                left    = fieldRect->right - i;
                bottom  = top + dlgHeight;
                right   = left + dlgWidth;
                chkRect.SetRect(left, top, right, bottom);

                interRect.IntersectRect(*parentRect, chkRect);
                if (interRect == chkRect) {
                    bChk = TRUE;
                    break;
                }
            }
        }

        //Check left field side with right dialog side
        if (!bChk) {
            for (int i = 0; i<dlgHeight+fldHeight; i++) {
                top     = fieldRect->bottom - i;
                left    = fieldRect->left - dlgWidth;
                bottom  = top + dlgHeight;
                right   = left + dlgWidth;
                chkRect.SetRect(left, top, right, bottom);

                interRect.IntersectRect(*parentRect, chkRect);
                if (interRect == chkRect) {
                    bChk = TRUE;
                    break;
                }
            }
        }
    }
    else { // if( !iPosition & CRECTEX_FIELD )
        CPoint  chkPoint;

        //Mid point of parent
        chkPoint = parentRect->CenterPoint();

        //Default position Bottom-Center
        top     = parentRect->bottom - dlgHeight;
        left    = (int) chkPoint.x - dlgWidth/2;

        //Vertical Coordinate
        if (iPosition & CRECTEX_BOTTOM)
            top = parentRect->bottom - dlgHeight;
        else if (iPosition & CRECTEX_TOP)
            top = parentRect->top;
        else if (iPosition & CRECTEX_VCENTER)
            top = (int) chkPoint.y - dlgHeight/2;
        if (iPosition & CRECTEX_LEFT)
            left = parentRect->left;
        else if (iPosition & CRECTEX_RIGHT)
            left = parentRect->right - dlgWidth;
        else if (iPosition & CRECTEX_HCENTER)
            left = (int) chkPoint.x - dlgWidth/2;


        //Set rest of the coordinates
        bottom  = top + dlgHeight;
        right   = left + dlgWidth;
        bChk = TRUE;
    }

    if( bChk )
        bestRect->SetRect(left, top, right, bottom);
    else {
        CRectExt auxRect;

        auxRect.CopyRect( this );
        bChk = auxRect.CenterRect( parentRect );  // center in parent
        if( bChk )
            bestRect->CopyRect( &auxRect );
        else
            bestRect->CopyRect( this ); // original rect
        return( bChk );
    }


    return (TRUE);
}

// if parentRect is NULL center in the full-screen
bool CRectExt::CenterRect(const CRect * parentRect)
{
    int     dlgWidth, dlgHeight, parentWidth, parentHeight;
    int     parentLeft, parentTop, top, bottom, left, right;

    if( parentRect == NULL ) {
        parentLeft = 0;
        parentTop = 0;
        parentWidth = ::GetSystemMetrics(SM_CXSCREEN);
        parentHeight = ::GetSystemMetrics(SM_CYSCREEN);
    }
    else {
        parentLeft = parentRect->left;
        parentTop = parentRect->top;
        parentWidth = parentRect->Width();
        parentHeight = parentRect->Height();
    }

    dlgWidth = Width();
    dlgHeight = Height();

    if( parentWidth < dlgWidth || parentHeight < dlgHeight )
        return( FALSE );

    left = parentLeft + ( parentWidth - dlgWidth ) / 2;
    top =  parentTop +( parentHeight - dlgHeight ) / 2;
    bottom = top + dlgHeight;
    right  = left + dlgWidth;

    SetRect( left, top, right, bottom );

    return( TRUE );
}

// Move the rect below to pRect
bool CRectExt::Collapse(const CRect * pRect, const bool bCenter )
{
    int     iDiffWidth, iWidth, iHeight;

    // Save before change
    iWidth = Width();
    iHeight = Height();

    top = pRect->top + pRect->Height();

    if( bCenter ) {
        iDiffWidth = pRect->Width() - Width();
        left = pRect->left + iDiffWidth/2;        // Center
        left = (left <0) ? 0 : left;
    }
    else
        left = pRect->left;

    right = left + iWidth;
    bottom = top + iHeight;

    return( TRUE );
}


// Change cRect in order to avoid intersection with cFixedRect but fitting in maxRect. If not
// possible, try to don't overlap pRect & cFixedRect ignoring maxRect.
bool CRectExt::UnIntersect( CRect* pRect, const CRect cFixedRect, const CRect maxRect ) {
    bool    bFitTopLeft=false, bFitTopRight=false,bFitBottomRight=false,bFitBottomLeft=false;

    CPoint  BottomLeft( cFixedRect.left, cFixedRect.bottom);
    CPoint  TopRight( cFixedRect.right, cFixedRect.top);

    if( pRect->PtInRect( cFixedRect.TopLeft() ) )
        bFitTopLeft = true;
    if( pRect->PtInRect( TopRight ) )
        bFitTopRight = true;
    if( pRect->PtInRect( cFixedRect.BottomRight() ) )
        bFitBottomRight = true;
    if( pRect->PtInRect( BottomLeft ) )
        bFitBottomLeft = true;

    int     iMovePos=CRECTEX_NONE;

    // There are 9 positions
    if( !bFitTopLeft && !bFitTopRight && bFitBottomRight && !bFitBottomLeft ) {
        iMovePos = CRECTEX_RIGHT;
    }
    else if( !bFitTopLeft && !bFitTopRight && bFitBottomRight && bFitBottomLeft ) {
        iMovePos = CRECTEX_BOTTOM;

    }
    else if( !bFitTopLeft && !bFitTopRight && !bFitBottomRight && bFitBottomLeft ) {

        iMovePos = CRECTEX_LEFT;
    }
    else if( !bFitTopLeft && bFitTopRight && bFitBottomRight && !bFitBottomLeft ) {
        iMovePos = CRECTEX_RIGHT;
    }
    else if( bFitTopLeft && bFitTopRight && bFitBottomRight && bFitBottomLeft ) {
        // Totally intersected
        int     iLeftBorder=  abs( cFixedRect.left - pRect->left );
        int     iRightBorder= abs( cFixedRect.right - pRect->right );
        int     iTopBorder=   abs( cFixedRect.top - pRect->top );
        int     iBottomBorder=abs( cFixedRect.bottom - pRect->bottom );


        if(std::max( iLeftBorder, iRightBorder ) > std::max( iTopBorder, iBottomBorder ) ) {
            if( iTopBorder > iBottomBorder )
                iMovePos = CRECTEX_TOP;
            else
                iMovePos = CRECTEX_BOTTOM;
        }
        else {
            if( iLeftBorder > iRightBorder )
                iMovePos = CRECTEX_LEFT;
            else
                iMovePos = CRECTEX_RIGHT;
        }
    }
    else if( bFitTopLeft && !bFitTopRight && !bFitBottomRight && bFitBottomLeft ) {
        iMovePos = CRECTEX_LEFT;
    }
    else if( !bFitTopLeft && bFitTopRight && !bFitBottomRight && !bFitBottomLeft ) {
        iMovePos = CRECTEX_RIGHT;
    }
    else if( bFitTopLeft && bFitTopRight && !bFitBottomRight && !bFitBottomLeft ) {
        iMovePos = CRECTEX_TOP;
    }
    else if( bFitTopLeft && !bFitTopRight && !bFitBottomRight && !bFitBottomLeft ) {
        iMovePos = CRECTEX_LEFT;
    }


    bool    bChanged=false;
    if( iMovePos != CRECTEX_NONE ) {
        bChanged = true;

        int     iPos[] = {CRECTEX_NONE,CRECTEX_NONE,CRECTEX_NONE,CRECTEX_NONE};
        if( iMovePos == CRECTEX_TOP ) {
            iPos[0] = CRECTEX_TOP;
            iPos[1] = CRECTEX_RIGHT;
            iPos[2] = CRECTEX_BOTTOM;
            iPos[3] = CRECTEX_LEFT;
        }
        else if( iMovePos == CRECTEX_RIGHT ) {
            iPos[0] = CRECTEX_RIGHT;
            iPos[1] = CRECTEX_BOTTOM;
            iPos[2] = CRECTEX_LEFT;
            iPos[3] = CRECTEX_TOP;
        }
        else if( iMovePos == CRECTEX_BOTTOM ) {
            iPos[0] = CRECTEX_BOTTOM;
            iPos[1] = CRECTEX_LEFT;
            iPos[2] = CRECTEX_TOP;
            iPos[3] = CRECTEX_RIGHT;
        }
        else if( iMovePos == CRECTEX_LEFT ) {
            iPos[0] = CRECTEX_LEFT;
            iPos[1] = CRECTEX_TOP;
            iPos[2] = CRECTEX_RIGHT;
            iPos[3] = CRECTEX_BOTTOM;
        }

        // Move, check in zone, check overlap
        CRect   pOldRect=*pRect;
        bool    bMovedOk=false;

        for( int i=0; i < 4; i++ ) {
            *pRect = pOldRect;
            MoveTo( pRect, iPos[i], cFixedRect );
            FitIn( pRect, maxRect );
            CRect interRect;
            if( !interRect.IntersectRect( pRect, cFixedRect ) ) {
                bMovedOk = true;
                break;
            }
        }

        // Overlap with cFixedRect and fitin maxRect is not posible, better dont't overlap ignoring maxrect
        if( !bMovedOk ) {
            *pRect = pOldRect;
            MoveTo( pRect, iPos[0], cFixedRect );
        }
    }

    return bChanged;
}

// Move pRect to a relative position based on cFixedRect
bool CRectExt::MoveTo( CRect* pRect, int iPos, const CRect cFixedRect ) {
    bool    bChanged=true;
    ASSERT( iPos == CRECTEX_TOP || iPos == CRECTEX_BOTTOM ||
        iPos == CRECTEX_LEFT || iPos == CRECTEX_RIGHT );

    int     iHeight=pRect->Height();
    int     iWidth=pRect->Width();

    if( iPos == CRECTEX_TOP ) {
        pRect->bottom = cFixedRect.top-1;
        pRect->top = pRect->bottom - iHeight;
    }
    else if( iPos == CRECTEX_BOTTOM ) {
        pRect->top = cFixedRect.bottom+1;
        pRect->bottom = pRect->top + iHeight;
    }
    else if( iPos == CRECTEX_LEFT ) {
        pRect->right = cFixedRect.left-1;
        pRect->left = pRect->right - iWidth;
    }
    else if( iPos == CRECTEX_RIGHT ) {
        pRect->left = cFixedRect.right+1;
        pRect->right = pRect->left + iWidth;
    }
    else
        bChanged = false;

    return bChanged;
}

// Move cRect for fixing in maxRect
bool CRectExt::FitIn( CRect* pRect, const CRect maxRect ) {
    int     iDiffY=0, iDiffX=0;
    bool    bChanged=false;

    if( pRect->top < maxRect.top )
        iDiffY = maxRect.top-pRect->top;
    else if( pRect->bottom > maxRect.bottom )
        iDiffY = maxRect.bottom-pRect->bottom;

    if( pRect->left < maxRect.left )
        iDiffX = maxRect.left-pRect->left;
    else if( pRect->right > maxRect.right )
        iDiffX = maxRect.right-pRect->right;

    pRect->top += iDiffY;
    pRect->bottom += iDiffY;
    pRect->left += iDiffX;
    pRect->right += iDiffX;


    if( iDiffY != 0 || iDiffX != 0 )
        bChanged = true;

    return bChanged;
}
