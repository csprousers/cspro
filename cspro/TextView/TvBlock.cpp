//***************************************************************************
//  File name: TVBlock.cpp
//
//  Description:
//       Blocking view source implementation, CBlockScrollView:public CScrollView
//
//  History:    Date       Author     Comment
//              -----------------------------
//              1995        csc       created
//              12 Feb 98   bmd       On select all, put orgin in status bar
//                                    InvalidateRect correctly when deselect all
//              4  Jan 03   csc       added double-click support (select current word)
//
//***************************************************************************

#include "StdAfx.h"
#include <io.h>

//////////////////////////////////////////////////////////////////////////////
// global variables
extern CString csLine;          // declared in ivview.cpp
//extern CWaitDialog* dlgWait;
extern CString csPadding;       // declared in ivview.cpp
static CLPoint ptlOffset;        // relative pixel coordinates, global for speed
static CLRect  rclTempNew,       // relative pixel coordinates, global for speed
               rclTempOld;       // relative pixel coordinates, global for speed

void CBlockScrollView::GetBlockedBuffer (TCHAR FAR * lpBuffer)  {
    CBufferMgr *pBuffMgr = ((CTVDoc *) GetDocument())->GetBufferMgr();
    if ( m_bBlockActive )  {
        CLRect rclBlockedRect = GetBlockedRectChar ();
        if ( rclBlockedRect.top < 0 )
            rclBlockedRect.top = 0;
        if ( rclBlockedRect.left < 0 )
            rclBlockedRect.left = 0;
        long lLen = rclBlockedRect.right - rclBlockedRect.left + 1;
        long lPrevLine = pBuffMgr->GetCurrLine ();

        StatusType stStatus;
        int iOffset;
        long i, lCurrLineExtent;

        pBuffMgr->GotoLineNumber ( rclBlockedRect.top );
        for ( i = rclBlockedRect.top ; i <= rclBlockedRect.bottom ; i++ )  {
            stStatus = pBuffMgr->Status();
            csLine = pBuffMgr->GetNextLine ();

            // convert tabs to spaces in the line ...
            while ( (iOffset = csLine.Find ((TCHAR) 9)) != NONE )  {
                csLine = csLine.Left (iOffset) + csPadding.Left (TAB_SPACES-(iOffset % TAB_SPACES)) + csLine.Mid (iOffset+1);
            }

            if ( stStatus != ENDFILE )  {
                if ( rclBlockedRect.left < csLine.GetLength () )  {
                    // we need to copy at least a portion of this line
                    lCurrLineExtent =  (long) (csLine.Mid((int) rclBlockedRect.left)).GetLength();
//                    _fmemcpy ((void FAR *) lpBuffer, csLine.Mid((int)rclBlockedRect.left), (size_t) min (lLen, lCurrLineExtent) );
                    memcpy ((void FAR *) lpBuffer, csLine.Mid((int)rclBlockedRect.left), (size_t) std::min (lLen*sizeof(TCHAR), lCurrLineExtent*sizeof(TCHAR)) );    // changed from _fmemcpy csc 2/14/96
                    lpBuffer += std::min (lLen, lCurrLineExtent );
                }
                else  {
                    // block is entirely to the right of the end of this line;
                    // its extent is thus zero, since none of it is displayed
                }

                // csc 4 Jan 03 -- remove trailing CR/LF from buffer
//                if (i<rclBlockedRect.bottom)  {
//                    *lpBuffer++ = (char) 13;
//                    *lpBuffer++ = (char) 10;
//                }
                // BMD 19 Mar 2003 -- Don't put training CR/LF if only part of one line
                if (rclBlockedRect.top != rclBlockedRect.bottom ||
                        (rclBlockedRect.left == 0 && rclBlockedRect.right + 1 >= csLine.GetLength())) {
                    *lpBuffer++ = (TCHAR) 13;
                    *lpBuffer++ = (TCHAR) 10;
                }
            }
        }

        pBuffMgr->GotoLineNumber ( lPrevLine );
    }
    csLine.Empty();
}


void CBlockScrollView::GetBlockForSS (TCHAR FAR * lpBuffer) {

    CBufferMgr *pBuffMgr = ((CTVDoc *) GetDocument())->GetBufferMgr();
    if ( m_bBlockActive )  {
        CLRect rclBlockedRect = GetBlockedRectChar ();
        if ( rclBlockedRect.top < 0 )
            rclBlockedRect.top = 0;
        if ( rclBlockedRect.left < 0 )
            rclBlockedRect.left = 0;
        long lLen = rclBlockedRect.right - rclBlockedRect.left + 1;
        long lPrevLine = pBuffMgr->GetCurrLine ();
        int  iBegin, iEnd, iLen, iSpaces, ii, j;
        CString csBox = _T(" ─│┤╡╢╖╕╣║╗╝╜╛┐└┴┬├┼╞╟╚╔╩╦╠═╬╧╨╤╥╙╘╒╓╫╪┘┌");
        CString csHoriz = _T(" ─╖╕╗╝╜╛┐└┴┬╚╔╩╦═╧╨╤╥╙╘╒╓┘┌");
        CString csVert  = _T("│┤╡╢╣║├┼╞╟╠╬╫╪");
        CString csDigits = _T("0123456789");
        int iOffset;

        TCHAR pszThousand[8];
        GetPrivateProfileString(_T("intl"), _T("sThousand"), _T(","), pszThousand, 8, _T("WIN.INI"));

        BOOL bKeepCommas = AfxGetApp()->GetProfileInt(_T("Options"), _T("Commas"), FALSE);

        pBuffMgr->GotoLineNumber ( rclBlockedRect.top );

        int iWidth = (int) lLen;
        CString csLineSS(_T(' '), iWidth);
        iBegin = (int) rclBlockedRect.left;
        for (long lLine = rclBlockedRect.top ; lLine <= rclBlockedRect.bottom ; lLine++) {
            csLine = pBuffMgr->GetNextLine();
            while ((iOffset = csLine.Find ((TCHAR) 9)) != -1) {
                csLine = csLine.Left (iOffset) + csPadding.Left (TAB_SPACES-(iOffset % TAB_SPACES)) + csLine.Mid (iOffset+1);
            }
            iLen = csLine.GetLength();
            if (iLen > iBegin) {
                BOOL bFirst = TRUE;
                iSpaces = 9;
                j = iWidth - 1;
                iEnd = std::min(iLen - 1, (int) rclBlockedRect.right);
                for (ii = iEnd ; ii >= iBegin ; ii--) {
                    if (bFirst) {
                        if (csBox.Find((TCHAR) csLine.GetAt(ii)) < 0) {
                            bFirst = FALSE;
                            csLineSS.SetAt(j, (TCHAR) csLine.GetAt(ii));
                            j--;
                            iSpaces = 0;
                        }
                    }
                    else {
                        if (csHoriz.Find((TCHAR) csLine.GetAt(ii)) >=0) {
                            if (iSpaces == 0) {
                                csLineSS.SetAt(j, ' ');
                                j--;
                                iSpaces++;
                            }
                            else if (iSpaces == 1) {
                                j++;
                                iSpaces++;
                            }
                            else if (iSpaces > 1) {
                                iSpaces++;
                            }
                        }
                        else if (csVert.Find((TCHAR) csLine.GetAt(ii)) >= 0) {
                            if (iSpaces == 1) {
                                csLineSS.SetAt(j+1, '\t');
                            }
                            else {
                                csLineSS.SetAt(j, '\t');
                                j--;
                            }
                            iSpaces = -1;
                        }
                        else {
                            if (iSpaces > 1) {
                                csLineSS.SetAt(j, '\t');
                                j--;
                            }
                            // Check for commas within numbers
                            if (bKeepCommas) {
                                csLineSS.SetAt(j, (TCHAR) csLine.GetAt(ii));
                                j--;
                            }
                            else {
                                if (csLine.GetAt(ii) == pszThousand[0] && csDigits.Find((TCHAR) csLine.GetAt(ii+1)) >= 0) {
                                }
                                else {
                                    csLineSS.SetAt(j, (TCHAR) csLine.GetAt(ii));
                                    j--;
                                }
                            }
                            iSpaces = 0;
                        }
                    }
                }
                if (iSpaces == 1) {
                    j++;
                }
                memmove(lpBuffer, csLineSS.Mid(j + 1), ( iWidth - 1 - j ) * sizeof(TCHAR));
                lpBuffer += (iWidth - 1 - j);
            }
            *lpBuffer++ = (TCHAR) 13;
            *lpBuffer++ = (TCHAR) 10;
        }
        pBuffMgr->GotoLineNumber ( lPrevLine );
    }
}

/////////////////////////////////////////////////////////////////////////////
// CBlockScrollView

IMPLEMENT_DYNCREATE(CBlockScrollView, CScrollView)

CBlockScrollView::CBlockScrollView()
{
    m_bBlockActive = NO;
    m_rclOldBlock = CLRect (0L,0L,0L,0L);
    m_ptlOrigin = CLPoint (0L, 0L);
    m_ptlDestination = CLPoint (0L, 0L);
    m_bCaptured = FALSE;
    m_iTimer = NONE;                            // inactive
}

CBlockScrollView::~CBlockScrollView()
{
    // stub
}

#ifdef _DEBUG
void CBlockScrollView::Dump(CDumpContext& dc) const
{
    CScrollView::Dump(dc);
    dc << _T("\nCBlockScrollView::Dump\n");
}
#endif

BEGIN_MESSAGE_MAP(CBlockScrollView, CScrollView)
    //{{AFX_MSG_MAP(CBlockScrollView)
    ON_WM_LBUTTONDOWN()
    ON_WM_LBUTTONUP()
    ON_WM_MOUSEMOVE()
    ON_WM_KEYDOWN()
    ON_WM_TIMER()
    ON_WM_VSCROLL()
    ON_WM_HSCROLL()
    ON_COMMAND(ID_EDIT_SELECT_ALL, OnEditSelectAll)
    ON_WM_LBUTTONDBLCLK()
    //}}AFX_MSG_MAP

END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CBlockScrollView drawing

void CBlockScrollView::OnInitialUpdate()  {
    CScrollView::OnInitialUpdate();
    SetClean ();
    UpdateStatusBar ();
}

void CBlockScrollView::OnDraw(CDC* pDC)  {
    if (IsProgressDlgActive())  {
        // yikes!  the user is searching or doing something that makes our state transient;
        // buffers aren't lined up correctly, so we can't render ourselves right now
        return;
    }

    CBufferMgr *pBuffMgr = ((CTVDoc *) GetDocument())->GetBufferMgr();
    if ( m_bBlockActive )  {
        int i, iOffset, iExtraAmount = 0;
        int iXPos;
        CLRect rclBlock = GetBlockedRectChar ();
        CRect rcPageSize;
        int iBlockWidth = (int) (rclBlock.right - rclBlock.left + 1L);
        CPoint ptScaledOffset;
        CLPoint ptlUnscaledOffset;
        ptScaledOffset = GetScrollPosition ();
        ptlUnscaledOffset = GetUnscaledScrollPosition ();  //ptScaledOffset);
        COLORREF colorOldText = pDC->GetTextColor (),
                 colorOldBk = pDC->GetBkColor ();

        if ( rclBlock.top < 0 )
            rclBlock.top = 0;
        if ( rclBlock.left < 0 )
            rclBlock.left = 0;
        pDC->SetTextColor (RGB (255,255,255));
        pDC->SetBkColor (RGB(0,0,0));
        GetClientRect (&rcPageSize);

        CString csLocalPadding;                      // local version of strPadding
        StatusType stStatus;
        int iCurrLineExtent;

        for ( i = 0 ; i < 1 + iBlockWidth/25 ; i++ )  {
            // put spaces into the padding buffer; used to block white
            // areas to the right of the lines in the blocked view
            csLocalPadding += _T("                         ");
        }

        if ( pBuffMgr->Status () == BEGFILE )  {
            // ensure that we're at the first displayable line ...
            pBuffMgr->GetNextLine ();
        }

        iXPos = (int) rclBlock.left * m_iTextWidth - (int) ptlUnscaledOffset.x + ptScaledOffset.x;  // where to draw the block when horizontal scaling is active
        for ( i = 0 ; i < m_iScrHgt ; i++ )  {
            stStatus = pBuffMgr->Status();
            csLine = pBuffMgr->GetNextLine ();

            // convert tabs to spaces in the line ...
            while ( (iOffset = csLine.Find ((TCHAR) 9)) != NONE )  {
                csLine = csLine.Left (iOffset) + csLocalPadding.Left (TAB_SPACES-(iOffset % TAB_SPACES)) + csLine.Mid (iOffset+1);
            }

            if ( stStatus != ENDFILE )  {
                if ( i+ptlUnscaledOffset.y/m_iTextHgt >= rclBlock.top && i+ptlUnscaledOffset.y/m_iTextHgt <= rclBlock.bottom )  {
                    // this line is in the block ...
                    if ( rclBlock.left < csLine.GetLength() )  {
                        // we need to display at least a portion of this line
                        iCurrLineExtent =  (csLine.Mid((int) rclBlock.left)).GetLength();
//                        pDC->TextOut ((int) rclBlock.left*m_iTextWidth,i*m_iTextHgt+ptScaledOffset.y, csLine.Mid((int)rclBlock.left), min (iBlockWidth, iCurrLineExtent) );
                        pDC->TextOut ( iXPos,i*m_iTextHgt+ptScaledOffset.y, csLine.Mid((int)rclBlock.left), std::min (iBlockWidth, iCurrLineExtent) );
                    }
                    else  {
                        // block is entirely to the right of the end of this line;
                        // its extent is thus zero, since none of it is displayed
                        iCurrLineExtent = 0;
                    }
                    if ( iBlockWidth > iCurrLineExtent)  {
                        // need to show spaces to the right of this line, to give a "columnar" effect
                        pDC->TextOut (iXPos + iCurrLineExtent*m_iTextWidth,i*m_iTextHgt+ptScaledOffset.y, csLocalPadding, iBlockWidth - iCurrLineExtent );
//                        pDC->TextOut (( (int) rclBlock.left+iCurrLineExtent)*m_iTextWidth,i*m_iTextHgt+ptScaledOffset.y, csLocalPadding, iBlockWidth - iCurrLineExtent );
                    }
                }
            }
            else  {
                iExtraAmount++;
            }
        }
        for ( i = 0 ; i <= m_iScrHgt-1-iExtraAmount ; i++ )  {
            // restore the buffer manager to the top of the window...
            pBuffMgr->GetPrevLine ();
        }

        SetClean ();
        pDC->SetTextColor (colorOldText);
        pDC->SetBkColor (colorOldBk);
    }
    csLine.Empty();
}

/////////////////////////////////////////////////////////////////////////////
// CBlockScrollView message handlers

inline CLPoint CBlockScrollView::GetUnscaledScrollPosition (void)  {
    CBufferMgr* pBuffMgr = ((CTVDoc *) GetDocument())->GetBufferMgr();
    return CLPoint ( (long) pBuffMgr->GetCurrCol() * m_iTextWidth, pBuffMgr->GetCurrLine() * m_iTextHgt);
}

inline void CBlockScrollView::InvalidateLRect (CLRect rclRect)  {
//    ASSERT ( rclRect.top <= MAXINT && rclRect.bottom <= MAXINT && rclRect.left <= MAXINT && rclRect.right <= MAXINT);
    InvalidateRect ( rclRect.CLRectToCRect() );
}

inline void CBlockScrollView::ValidateLRect (CLRect rclRect)  {
//    ASSERT ( rclRect.top <= MAXINT && rclRect.bottom <= MAXINT && rclRect.left <= MAXINT && rclRect.right <= MAXINT);
    ValidateRect ( rclRect.CLRectToCRect() );
}


void CBlockScrollView::OnLButtonDown(UINT nFlags, CPoint point)  {
    if (IsProgressDlgActive())  {
        // yikes!  the user is searching or doing something that makes our state transient;
        // buffers aren't lined up correctly, so we can't render ourselves right now
        return;
    }

    CLPoint ptlPoint = point;
    ptlOffset = GetUnscaledScrollPosition ();  //GetScrollPosition ());
    rclTempOld = m_rclOldBlock;
    rclTempOld.OffsetRect (-ptlOffset);
    ptlPoint.Offset (ptlOffset);

    /*--- we need to force rclTempOld to be w/in the bounds of a regular CRect; anything bigger than that is too far away to affect us ---*/
    if ( rclTempOld.top < -MAXINT )      rclTempOld.top = -MAXINT;
    if ( rclTempOld.top > MAXINT )       rclTempOld.top = MAXINT;
    if ( rclTempOld.bottom < -MAXINT )   rclTempOld.bottom = -MAXINT;
    if ( rclTempOld.bottom > MAXINT )    rclTempOld.bottom = MAXINT/2;        // bmd 12 Feb 98
    if ( rclTempOld.left < -MAXINT )     rclTempOld.left = -MAXINT;
    if ( rclTempOld.left > MAXINT )      rclTempOld.left = MAXINT;
    if ( rclTempOld.right < -MAXINT )    rclTempOld.right = -MAXINT;
    if ( rclTempOld.right > MAXINT )     rclTempOld.right = MAXINT;

    InvalidateLRect (rclTempOld);
    m_rclOldBlock = CLRect (0L,0L,0L,0L);
    m_bBlockActive = YES;
    SetOrigin (ptlPoint);
    SetDestination (ptlPoint);
    SetCapture ();
    UpdateStatusBar ();
    m_bCaptured = TRUE;
    ASSERT ( m_iTimer == NONE );
    CScrollView::OnLButtonDown(nFlags, point);
}

void CBlockScrollView::OnLButtonUp(UINT nFlags, CPoint point)  {
    if ( m_bBlockActive && GetCapture() == this)  {
        UpdateBlock ();
        ReleaseCapture ();
        m_bCaptured = FALSE;
    }
    if ( m_iTimer != NONE )  {
        VERIFY ( KillTimer ( m_iTimer ) );
        m_iTimer = NONE;
    }

    if ( GetBlockedRectChar().Height() == 0L && GetBlockedRectChar().Width() == 0L )  {
        // special case of a single character comprising the block ... we nuke it !
        m_bBlockActive = NO;
        ptlOffset = GetUnscaledScrollPosition ();  //GetScrollPosition());
        m_rclOldBlock.OffsetRect (-ptlOffset);
        InvalidateLRect (m_rclOldBlock);
        m_rclOldBlock = CRect (0,0,0,0);
        SetOrigin ( CPoint (0,0) );
        SetDestination ( CPoint (0,0) );
    }
    UpdateStatusBar ();
    CScrollView::OnLButtonUp(nFlags, point);
}


void CBlockScrollView::OnMouseMove(UINT nFlags, CPoint point)  {
    if ( m_bBlockActive && (nFlags & MK_LBUTTON) && GetCapture() == this )  {
        UpdateBlock ();
        CRect rcClient;

        GetClientRect (&rcClient);
        rcClient.InflateRect (-::GetSystemMetrics(SM_CXBORDER), -::GetSystemMetrics(SM_CYBORDER));  // account for Win 3.1 bug; see KB Q43596
        if ( m_iTimer != NONE )  {
            // outside scrolling is already enabled ...
            if ( rcClient.PtInRect ( point ) )  {
                // back within bounds, nuke the timer
                VERIFY ( KillTimer ( m_iTimer ) );
                m_iTimer = NONE;  // signal that it's disabled
            }
        }
        else  {   // no timer currently active; this condition prevents recursion
            // translate mouse movement outside of the current draw area into scroll commands
            if (!rcClient.PtInRect(point))  {
                UINT uScrollSpeed = GetProfileInt(_T("Windows"), _T("KeyboardSpeed"), 40);
                ptlOffset = GetUnscaledScrollPosition ();  //GetScrollPosition());
                    ASSERT (m_iTimer==NONE);
                    m_iTimer = SetTimer (SCROLL_TIMER, uScrollSpeed*3, NULL);
            }
        }
    }
    CScrollView::OnMouseMove(nFlags, point);
}


/******************************************************************************
*
*                     CBlockScrollView::UpdateBlock()
*
*******************************************************************************
*   Revision history:
*   - prevents user from blocking an area greater than 64K; CSC 8/1/95
*
******************************************************************************/

void CBlockScrollView::UpdateBlock (void)  {
    CPoint ptPoint;
    CLPoint ptlPoint;
    CLPoint ptlPrevDestination;

    GetCursorPos (&ptPoint);
    ScreenToClient (&ptPoint);
    ptlOffset = GetUnscaledScrollPosition ();  //GetScrollPosition());
    ptlPoint = ptPoint;
    ptlPoint.Offset (ptlOffset);
    if ( ptlPoint.x < 0 )  {
        ptlPoint.x = 0;
    }
    if ( ptlPoint.y < 0 )  {
        ptlPoint.y = 0;
    }

    BOOL bInwardMotion = InwardMotion (ptlPoint);
    CLRect rclNewBlock;

    ptlPrevDestination = m_ptlDestination;
    SetDestination (ptlPoint);

    if (GetBlockedBufferSize() > MAXCLIP95)  {                           // csc 8/1/95  bmd
        // this block is too big to fit onto the clipboard!
        ::MessageBeep(0);
        m_ptlDestination = ptlPrevDestination;
        ptlPoint = ptlPrevDestination;
        bInwardMotion = InwardMotion (ptlPoint);
    }

    rclNewBlock = GetBlockedRectPixel();

    rclTempNew = rclNewBlock;
    rclTempNew.OffsetRect (-ptlOffset);
    rclTempOld = m_rclOldBlock;
    rclTempOld.OffsetRect (-ptlOffset);

    if ( PassedOrigin( rclNewBlock ))  {
        InvalidateLRect ( rclTempOld );
        InvalidateLRect ( rclTempNew );
    }
    else  {
        if ( bInwardMotion )  {
            CRect rcIntersection;
            rcIntersection.IntersectRect (rclTempNew.CLRectToCRect(), rclTempOld.CLRectToCRect() );
            InvalidateLRect ( rclTempOld );
            InvalidateLRect ( rclTempNew );
            ValidateRect ( rcIntersection );
            m_rclOldBlock = rcIntersection;
            m_rclOldBlock.OffsetRect (ptlOffset);
        }
        else  {
            InvalidateLRect ( rclTempNew );
            ValidateLRect ( rclTempOld );
       }
    }
    m_bDirty = YES;
    UpdateWindow();      // csc 12/5
}


void CBlockScrollView::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)  {
    if (nChar == VK_ESCAPE)  {
        if (IsBlockActive())  {
            m_bBlockActive = NO;
            ptlOffset = GetUnscaledScrollPosition ();  //GetScrollPosition());
            m_rclOldBlock.OffsetRect (-ptlOffset);
            InvalidateLRect (m_rclOldBlock);
            m_rclOldBlock = CRect(0,0,0,0);
            SetOrigin (CPoint(0,0));
            SetDestination (CPoint(0,0));
            UpdateStatusBar();
        }
        else if (((CTextViewApp*) AfxGetApp())->m_bCalledAsChild)  {
            AfxGetMainWnd()->PostMessage(WM_COMMAND, ID_QUICK_QUIT);
            return;
        }
    }
    CScrollView::OnKeyDown(nChar, nRepCnt, nFlags);
}

void CBlockScrollView::OnTimer(UINT /*nIDEvent*/)  {
    ASSERT ( m_iTimer != NONE );
    ptlOffset = GetUnscaledScrollPosition ();  //GetScrollPosition());
    CPoint  point;
    CRect   rcClient;

    GetCursorPos (&point);
    ScreenToClient (&point);
    GetClientRect (&rcClient);
    rcClient.InflateRect (-::GetSystemMetrics(SM_CXBORDER), -::GetSystemMetrics(SM_CYBORDER));     // account for Win 3.1 bug; see KB Q43596

    ASSERT ( GetBlockedBufferSize() >= 0 );
    if ( point.x < rcClient.left )  {
        if ( ptlOffset.x > 0)  {
            SendMessage (WM_HSCROLL, SB_PAGELEFT);
        }
    }
    if ( point.x >= rcClient.right )  {
        SendMessage (WM_HSCROLL, SB_PAGERIGHT);
    }
    if ( point.y < rcClient.top )  {
        if ( ptlOffset.y > 0)  {
            SendMessage (WM_VSCROLL, SB_LINEUP);
        }
    }
    if ( point.y >= rcClient.bottom )  {
        SendMessage (WM_VSCROLL, SB_LINEDOWN);
    }
//    CScrollView::OnTimer(nIDEvent);   // just commented this back in...
}

void CBlockScrollView::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
    CScrollView::OnVScroll(nSBCode, nPos, pScrollBar);
    if (m_bCaptured)  {
        ASSERT ( m_bBlockActive == TRUE );
        UpdateBlock ();
    }
}

void CBlockScrollView::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
    CScrollView::OnHScroll(nSBCode, nPos, pScrollBar);         // moved up here, csc 2/13
    if ( m_bCaptured )  {
        ASSERT ( m_bBlockActive == TRUE );
        UpdateBlock ();
    }
//    CScrollView::OnHScroll(nSBCode, nPos, pScrollBar);   was here < 2/13 csc
}


void CBlockScrollView::UpdateStatusBar (void)  {

    CLPoint ptlTmp;
    ptlTmp.x = GetBlockedRectChar().left + 1L;
    ptlTmp.y = GetBlockedRectChar().top + 1L;
    ((CMainFrame*) AfxGetApp()->m_pMainWnd)->UpdateStatusBarBlock (ptlTmp, IsBlockActive());
}

void CBlockScrollView::OnEditSelectAll()
{
    CBufferMgr *pBuffMgr = ((CTVDoc *) GetDocument())->GetBufferMgr();
    long lCurrentLine = pBuffMgr->GetCurrLine();
    pBuffMgr->EndOfFile (m_iScrHgt);
    pBuffMgr->GotoLineNumber(lCurrentLine);
    SetOrigin (CLPoint(0L, 0L));
    SetDestination (CLPoint(ColToPixel((long) (pBuffMgr->GetFileWidth())), RowToPixel(pBuffMgr->GetLineCount())));
    m_bBlockActive = TRUE;
    UpdateStatusBar();                          // bmd 12 Feb 98
    Invalidate ();
}


///////////////////////////////////////////////////////////////////////////////
//
//    CBlockScrollView::OnLButtonDblClick()
//
//  Double clicking selects the word underneath the cursor.
//
//  revision history:
//      4 Jan 2003     CSC       created
//
///////////////////////////////////////////////////////////////////////////////
// csc 4 Jan 03
void CBlockScrollView::OnLButtonDblClk(UINT nFlags, CPoint point)
{
    CBufferMgr* pBuffMgr = ((CTVDoc*)GetDocument())->GetBufferMgr();

    if (IsProgressDlgActive())  {
        // yikes!  the user is searching or doing something that makes our state transient;
        // buffers aren't lined up correctly, so we can't render ourselves right now
        return;
    }
    ASSERT (m_iTimer == NONE);

    // clear any existing block
    if (IsBlockActive())  {
        m_bBlockActive = NO;
        ptlOffset = GetUnscaledScrollPosition ();
        m_rclOldBlock.OffsetRect (-ptlOffset);
        InvalidateLRect (m_rclOldBlock);
        m_rclOldBlock = CRect(0,0,0,0);
        SetOrigin (CPoint(0,0));
        SetDestination (CPoint(0,0));
        UpdateStatusBar();
    }

    // get char pos where click occurred, in local viewport
    CLPoint ptClick;  // character position where user 2x clicked
    ptClick = CLPoint(PixelToCol(point.x), PixelToRow(point.y));

    // isolate the line (as a string) where the double-click occurred
    StatusType status;
    int iRowOffset = 0;
    BOOL bEOF = FALSE;
    int i;
    for (i=0 ; i<=ptClick.y ; i++)  {
        status = pBuffMgr->Status();
        csLine = pBuffMgr->GetNextLine();
        if (status == ENDFILE)  {
            // user clicked below the last line; stop moving through buffer
            bEOF=TRUE;
            break;
        }
    }
    iRowOffset = i;

    // restore buffer to its previous position
    for (; i>0 ; i--)  {
        pBuffMgr->GetPrevLine();
    }

    // abort if we passed EOF while scanning downwards
    if (bEOF)  {
        return;
    }

    // convert tabs to spaces in the line ...
    int iOffset;
    while ( (iOffset = csLine.Find ((TCHAR) 9)) != NONE )  {
        csLine = csLine.Left (iOffset) + csPadding.Left (TAB_SPACES-(iOffset % TAB_SPACES)) + csLine.Mid (iOffset+1);
    }

    // if the clicked-on char is past EOL, then we don't do anything
    if (ptClick.x + pBuffMgr->GetCurrCol() >=csLine.GetLength())  {
        return;
    }

    // if the clicked-on char is not an alphanumeric or underline (ie, a "name" character), then we don't do anything
    if (!is_tokch(csLine.GetAt(ptClick.x + pBuffMgr->GetCurrCol())))  {
        return;
    }

    // determine the begin and end of the double-clicked word ...
    // this includes all contiguous alphanum characters (or underlines) on the line to the left of the double-clicked char, and
    // all contiguous alphanum characters (or underlines) on the line to the right of the double-clicked char
    // (conveniently, "name chars" are the same as (alphanum or underline)

    int iWordStart;  // column position of first character in word
    int iWordEnd;    // column position of last character in word
    BOOL bDone;      // TRUE when left/right scanning is finished

    // scan left
    bDone = FALSE;
    iWordStart = ptClick.x + pBuffMgr->GetCurrCol();
    while (iWordStart>0 && !bDone)  {
        iWordStart--;
        if (!is_tokch(csLine.GetAt(iWordStart)))  {
            bDone = TRUE;
            iWordStart++;
        }
    }

    // scan right
    bDone = FALSE;
    iWordEnd = ptClick.x + pBuffMgr->GetCurrCol();
    while (iWordEnd<csLine.GetLength()-1 && !bDone)  {
        iWordEnd++;
        if (!is_tokch(csLine.GetAt(iWordEnd)))  {
            bDone = TRUE;
            iWordEnd--;
        }
    }

    // see if area is too wide
    if (iWordEnd - iWordStart > MAXINT)  {
        AfxMessageBox(_T("Sorry, that word is too long to select."), MB_ICONSTOP);
        return;
    }

//    AfxMessageBox("x" + csLine.Mid(iWordStart, (iWordEnd - iWordStart+1)) + "x");

    // block the word
    CLRect rclBlock;

    rclBlock = CLRect(ColToPixel(iWordStart - pBuffMgr->GetCurrCol()), RowToPixel(iRowOffset-1), ColToPixel(iWordEnd - pBuffMgr->GetCurrCol()), RowToPixel(iRowOffset-1));
    ptlOffset = GetUnscaledScrollPosition ();
    SetOrigin (CLPoint(rclBlock.left + ptlOffset.x, rclBlock.top + ptlOffset.y));
    SetDestination (CLPoint(rclBlock.right + ptlOffset.x , rclBlock.bottom + ptlOffset.y));
    m_bBlockActive = YES;
    rclBlock.bottom += m_iTextHgt;
    rclBlock.right += m_iTextWidth;
    InvalidateLRect(rclBlock);
    UpdateStatusBar ();

    CScrollView::OnLButtonDblClk(nFlags, point);
}



///////////////////////////////////////////////////////////////////////////////
//
//                       CBlockScrollView::OnSelectLine()
//
///////////////////////////////////////////////////////////////////////////////
void CBlockScrollView::OnSelLine (CPoint pt, SELRULE type)        // BMD 24 Mar 2003
{
    CBufferMgr* pBuffMgr = ((CTVDoc*)GetDocument())->GetBufferMgr();

    if (IsProgressDlgActive())  {
        // yikes!  the user is searching or doing something that makes our state transient;
        // buffers aren't lined up correctly, so we can't render ourselves right now
        return;
    }
    // Clear any existing block if first
    if (IsBlockActive() && (type == HRFIRST || type == VRFIRST)) {
        m_bBlockActive = NO;
        ptlOffset = GetUnscaledScrollPosition ();
        m_rclOldBlock.OffsetRect (-ptlOffset);
        InvalidateLRect (m_rclOldBlock);
        m_rclOldBlock = CRect(0,0,0,0);
        SetOrigin (CPoint(0,0));
        SetDestination (CPoint(0,0));
        UpdateStatusBar();
    }

    // get char pos (row) where click occurred, in local viewport
    if (type == HRFIRST) {
        UINT x = pt.x;
        long lClick = PixelToCol(x);
        int iColOffset = ColToPixel(lClick);

        CLRect rclBlock;

        rclBlock = CLRect(iColOffset, 0, iColOffset, RowToPixel(pBuffMgr->GetLineCount()));
        ptlOffset = GetUnscaledScrollPosition ();
        SetOrigin (CLPoint(rclBlock.left + ptlOffset.x, rclBlock.top));
        SetDestination (CLPoint(rclBlock.right + ptlOffset.x, rclBlock.bottom));
        m_bBlockActive = YES;
        rclBlock.bottom += m_iTextHgt;
        rclBlock.right += m_iTextWidth;
        InvalidateLRect(rclBlock);
        UpdateStatusBar ();
    }
    else if (type == HRNEXT) {
        UINT x = pt.x;
        if (x < 0) {
            x = 0;
        }
        long lClick = PixelToCol(x);
        if (lClick < 0) {
            lClick = 0;
        }
        int iColOffset = ColToPixel(lClick);

        CLPoint ptl = CLPoint(x,RowToPixel(m_ptlDestination.y));

        CLRect rclBlock = CLRect(ColToPixel(m_ptlOrigin.x), 0, iColOffset, RowToPixel(pBuffMgr->GetLineCount()));
        ptlOffset = GetUnscaledScrollPosition ();
        BOOL bInwardMotion = InwardMotion (CLPoint(ptl.x + ptlOffset.x, ptl.y));
        rclTempOld = m_rclOldBlock;
        rclTempOld.OffsetRect (-ptlOffset);
        SetDestination (CLPoint(rclBlock.right + ptlOffset.x, rclBlock.bottom));
        CLRect rclNewBlock = GetBlockedRectPixel();

        rclTempNew = rclNewBlock;
        rclTempNew.OffsetRect(-ptlOffset);

        if (PassedOrigin(rclNewBlock)) {
            InvalidateLRect(rclTempOld);
            InvalidateLRect(rclTempNew);
        }
        else {
            if (bInwardMotion) {
                CRect rcIntersection;
                rcIntersection.IntersectRect (rclTempNew.CLRectToCRect(), rclTempOld.CLRectToCRect() );
                InvalidateLRect (rclTempOld);
                ValidateRect (rcIntersection);
                m_rclOldBlock = rcIntersection;
                m_rclOldBlock.OffsetRect (ptlOffset);
            }
            else {
                InvalidateLRect(rclTempNew);
                ValidateLRect(rclTempOld);
            }
        }
    }
    else if (type == VRFIRST) {
        UINT y = pt.y;
        long lClick = PixelToRow(y);
        int iRowOffset = RowToPixel(lClick);

        CLRect rclBlock;

        rclBlock = CLRect(0, iRowOffset, ColToPixel(pBuffMgr->GetFileWidth()), iRowOffset-1);
        ptlOffset = GetUnscaledScrollPosition ();
        SetOrigin (CLPoint(rclBlock.left, rclBlock.top + ptlOffset.y));
        SetDestination (CLPoint(rclBlock.right, rclBlock.bottom + ptlOffset.y));
        m_bBlockActive = YES;
        rclBlock.bottom += m_iTextHgt;
        rclBlock.right += m_iTextWidth;
        InvalidateLRect(rclBlock);
        UpdateStatusBar ();
    }

    else if (type == VRNEXT) {
        UINT y = pt.y;
        if (y < 0) {
            y = 0;
        }
        long lClick = PixelToRow(y);
        if (lClick < 0) {
            lClick = 0;
        }
        int iRowOffset = RowToPixel(lClick);
        CLPoint ptl = CLPoint(ColToPixel(m_ptlDestination.x),y);

        CLRect rclBlock = CLRect(0, RowToPixel(m_ptlOrigin.y), ColToPixel(pBuffMgr->GetFileWidth()), iRowOffset);
        ptlOffset = GetUnscaledScrollPosition ();
        BOOL bInwardMotion = InwardMotion (CLPoint(ptl.x, ptl.y + ptlOffset.y));
        rclTempOld = m_rclOldBlock;
        rclTempOld.OffsetRect (-ptlOffset);
        SetDestination (CLPoint(rclBlock.right, rclBlock.bottom + ptlOffset.y));
        CLRect rclNewBlock = GetBlockedRectPixel();

        rclTempNew = rclNewBlock;
        rclTempNew.OffsetRect(-ptlOffset);

        if (PassedOrigin(rclNewBlock)) {
            InvalidateLRect(rclTempOld);
            InvalidateLRect(rclTempNew);
        }
        else {
            if (bInwardMotion) {
                CRect rcIntersection;
                rcIntersection.IntersectRect (rclTempNew.CLRectToCRect(), rclTempOld.CLRectToCRect() );
                InvalidateLRect (rclTempOld);
                ValidateRect (rcIntersection);
                m_rclOldBlock = rcIntersection;
                m_rclOldBlock.OffsetRect (ptlOffset);
            }
            else {
                InvalidateLRect(rclTempNew);
                ValidateLRect(rclTempOld);
            }
        }
    }
    return;
}
