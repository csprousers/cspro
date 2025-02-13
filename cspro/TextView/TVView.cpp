//
//  History:    Date       Author     Comment
//              -----------------------------
//              1995        csc       created
//              9  Sep 96   csc       bug fix when sending WM_HSCROLL + thumb positioning
//              28 feb 97   csc       bug fix when search needs to hscroll on unscaled files ... "butterfly bug"
//              28 feb 97   csc       big fix to properly close down search dialog when cancelled from progress dlg
//              28 feb 97   csc       auto-toggle rulers in OnInitialUpdate if indicated by .INI entry
//                                    (added WM_TOGGLE_RULER to accommodate this)
//              28 Apr 97   bmd       fix bug in scroll limits
//              13 Fan 98   bmd       fix bug in overuse of CFont::Create
//              12 Feb 98   bmd       Make characters smaller on screen
//              01 Oct 98   bmd       Keep ruler out of print preview window
//              22 Dec 99   bmd       Fix to prevent assert failure in CString::Mid (does not effect release)  OnDraw
//              4  Jan 03   csc       Add support for changing font sizes
//              4  Jan 03   csc       Add support for find next/prev
//              4  Jan 03   csc       Uses currently selected block as find string if on one line only
//             16  Jan 05   csc       Mouse wheel panning support added
//
//***************************************************************************

#include "StdAfx.h"
#include <zUtilO/BCMenu.h>
#include <zUtilO/Interapp.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CFindDlg CTVView::m_dlgFind;

//////////////////////////////////////////////////////////////////////////////
// global variables
CString csLine;                 // static and global here for speed
CString csPadding = _T("          ");
int nLastPage;
BOOL bFirstPage;
long lPrevLine;
long lTopPrevPage;
CCriticalErrorMgr errorMgr;

static BOOL g_bIsProgressDlgActive = FALSE;   // signals when a progress bar is alive

//////////////////////////////////////////////////////////////////////////////
// prototypes

/////////////////////////////////////////////////////////////////////////////
// CTVView

IMPLEMENT_DYNCREATE(CTVView, CBlockScrollView)

BEGIN_MESSAGE_MAP(CTVView, CBlockScrollView)
    //{{AFX_MSG_MAP(CTVView)
    ON_COMMAND(ID_EDIT_COPY, OnEditCopy)
    ON_WM_KEYDOWN()
    ON_WM_SIZE()
    ON_WM_VSCROLL()
    ON_COMMAND(ID_EDIT_FIND, OnEditFind)
    ON_WM_SETFOCUS()
    ON_WM_LBUTTONDOWN()
    ON_WM_HSCROLL()
    ON_COMMAND(ID_EDIT_COPY_SS, OnEditCopySS)
    ON_UPDATE_COMMAND_UI(ID_VIEW_RULER, OnUpdateViewRuler)
    ON_COMMAND(ID_VIEW_GOTOLINE, OnViewGotoline)
    ON_UPDATE_COMMAND_UI(ID_EDIT_COPY, OnUpdateEditCopy)
    ON_UPDATE_COMMAND_UI(ID_EDIT_COPY_SS, OnUpdateEditCopySs)
    ON_WM_TIMER()
    ON_UPDATE_COMMAND_UI(ID_EDIT_FIND, OnUpdateEditFind)
    ON_UPDATE_COMMAND_UI(ID_VIEW_GOTOLINE, OnUpdateViewGotoline)
    ON_WM_MOUSEWHEEL()
    ON_WM_RBUTTONUP()
    ON_COMMAND(ID_EDIT_FIND_NEXT, OnEditFindNext)
    ON_UPDATE_COMMAND_UI(ID_EDIT_FIND_NEXT, OnUpdateEditFindNext)
    ON_COMMAND(ID_EDIT_FIND_PREV, OnEditFindPrev)
    ON_UPDATE_COMMAND_UI(ID_EDIT_FIND_PREV, OnUpdateEditFindPrev)
    //}}AFX_MSG_MAP

    // Standard printing commands
    ON_COMMAND(ID_FILE_PRINT, CView::OnFilePrint)
    ON_COMMAND(ID_FILE_PRINT_DIRECT, CView::OnFilePrint)
    ON_COMMAND(ID_FILE_PRINT_PREVIEW, OnFilePrintPreview)

    // "find dialog"
    ON_MESSAGE(UWM::TextView::Search, OnSearch)
    ON_MESSAGE(UWM::TextView::SearchClose, OnSearchClose)

    // ruler toggle
    ON_COMMAND(UWM::TextView::ToggleRuler, OnToggleRuler)

    ON_COMMAND(ID_CLOSE_WINDOW, OnFileClose) // GHM 20110802 three new shortcuts
    ON_COMMAND(ID_FONT_BIGGER, OnFontBigger)
    ON_COMMAND(ID_FONT_BIGGER2, OnFontBigger)
    ON_COMMAND(ID_FONT_SMALLER, OnFontSmaller)
    ON_COMMAND(ID_FONT_SMALLER2, OnFontSmaller)


END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTVView construction/destruction

CTVView::CTVView()  {
    m_bForceVScrollBarOverride = FALSE;
    m_bForceHScrollBarOverride = FALSE;
    m_bRulersActivated = FALSE;
    m_bRulersInitialized = FALSE;
    m_bRulerTempOff = FALSE;
    m_iTimer = NONE;
    m_ptlLastFind = CLPoint ( (long) NONE, (long) NONE );
    m_bShowLastFind = FALSE;
}

CTVView::~CTVView()  {
    ASSERT(!IsProgressDlgActive());
}

void CTVView::OnInitialUpdate(void)  {
    WINDOWPLACEMENT wndpl;
    BOOL bInitOK = TRUE;
    BOOL bBigFile;

    CClientDC dc (this);

    CFont* pOldFont = dc.SelectObject(&((CMainFrame*) AfxGetMainWnd())->m_Font);

    m_iTextWidth = dc.GetTextExtent (_T("A"),1).cx;
    m_iTextHgt = dc.GetTextExtent (_T("A"),1).cy;
    // start chris' change 2/5/02, put in by gsf
    if (dc.GetDeviceCaps (BITSPIXEL)>=24)  {
         ((CTextViewApp*)AfxGetApp())->m_iNumColors = 256;
     }
    else  {
        ((CTextViewApp*)AfxGetApp())->m_iNumColors = 1 << (dc.GetDeviceCaps (BITSPIXEL) * dc.GetDeviceCaps (PLANES));
     }
//    ((CTblViewApp*)AfxGetApp())->m_iNumColors = 1 << (dc.GetDeviceCaps (BITSPIXEL) * dc.GetDeviceCaps (PLANES));
    // end chris' change 2/5/02, put in by gsf

    errorMgr.SetCurrView (this);       // used to communicate views back up from the bowels of the Buffer, BufferMgr etc to the Doc/View level
    m_bErrorState = FALSE;
    bBigFile = (((CTVDoc*) GetDocument())->GetBufferMgr()->GetFileSize() > BIG_FILE);
//    if (bBigFile)  {
//        dlgWait = new CWaitDialog (this, IDS_WAIT01);
//        dlgWait->Create();     // positioning and message-loading are handled by CCoundDialog::OnInitDialog()
//    }

    ((CTVDoc*) GetDocument())->UpdateAllViews(NULL);
    GetParentFrame()->UpdateWindow();
    ((CTVDoc*) GetDocument())->GetBufferMgr()->Init ();

    m_bErrorState = errorMgr.IsError();
    errorMgr.Reset();

    if (bBigFile)  {
        bInitOK = (((CTVDoc*) GetDocument())->GetBufferMgr()->GetLineCount() > 0);
    }

    if (! bInitOK || m_bErrorState)  {
        // user pressed "Cancel" while the initial scan was going on ... simulate a ctrl-F4
        PostMessage (WM_COMMAND, ID_FILE_CLOSE);
        return;
    }

    m_szLineSize =  CSize (m_iTextWidth, m_iTextHgt);
    SetScrollParameters ();

    CBlockScrollView::OnInitialUpdate ();
    UpdateStatusBar ();

    wndpl.length = sizeof (WINDOWPLACEMENT);

    // gsf:  got rid of m_iNumDocs 5/97
//    if (((CTextViewApp*) AfxGetApp())->m_iNumDocs == 1) {
        // maximize the frame
        VERIFY (GetParentFrame()->GetWindowPlacement (&wndpl));
        wndpl.showCmd = SW_SHOWMAXIMIZED;
        VERIFY (GetParentFrame()->SetWindowPlacement (&wndpl));
//    }

    // changed order of following 2 lines ... csc 4 Jan 03
    m_rulerMgr.Init (GetDocument(), GetParentFrame());
    m_rulerMgr.SetText (m_iTextHgt, m_iTextWidth);
    m_bRulersInitialized = TRUE;

    // following section added csc 2/28/97
    // toggle rulers if indicated by the .INI
    UpdateWindow();
    BOOL bRulers = AfxGetApp()->GetProfileInt(_T("Ruler"), _T("Toggle"), 0);
    if (bRulers)  {
        PostMessage(WM_COMMAND, UWM::TextView::ToggleRuler);
    }
    dc.SelectObject(pOldFont);
}


/////////////////////////////////////////////////////////////////////////////
// CTVView drawing

void CTVView::OnDraw(CDC* pDC)  {
    if (IsProgressDlgActive())  {
        // yikes!  the user is searching or doing something that makes our state transient;
        // buffers aren't lined up correctly, so we can't render ourselves right now
        return;
    }

    int iOldMode = pDC->SetBkMode (TRANSPARENT);
    CPen pen(PS_DASH, 1, RGB(0,0,192) );
    CFont* pOldFont = (CFont*) pDC->SelectObject(&((CMainFrame*) AfxGetMainWnd())->m_Font);

    CPen* pOldPen = pDC->SelectObject (&pen);
    CBufferMgr *pBuffMgr = ((CTVDoc*) GetDocument())->GetBufferMgr();
    CPoint ptOffset = GetScrollPosition ();

    StatusType stStatus;
    int i, iOffset, iExtraAmount = 0;

    CRect rcPageSize;
    GetClientRect (&rcPageSize);
    if (rcPageSize.Size().cy >= GetTotalSize().cy && pBuffMgr->GetCurrLine() != 0L)  {
        // vertical scroll bars have disappeared because entire file can be displayed in 1 pageful;
        // we need to signal this so that we scroll to BOF ...
        SendMessage (WM_VSCROLL, SB_TOP);
    }
    if (rcPageSize.Size().cx >= GetTotalSize().cx && pBuffMgr->GetCurrCol() != 0)  {
        // horizontal scroll bars have disappeared because entire file can be displayed in 1 pageful;
        // we need to signal this so that we scroll to BOF ...
        SendMessage (WM_HSCROLL, SB_LEFT);
    }

    if (ShowLastFind())  {
        // make sure that the "found" highlighted text can be seen on the current screen...
        if ((int) m_ptlLastFind.x < pBuffMgr->GetCurrCol() || (int) m_ptlLastFind.x + (int) m_dlgFind.GetFindLen() > pBuffMgr->GetCurrCol() + m_iScrWidth)  {
            // at least some of the "found" highlighted text is not in the current view!  Scroll so that it is ...
            int iWidth = m_dlgFind.GetFindLen() * m_iTextWidth;  //(int) (m_ptlLastFind.x + m_dlgFind.GetFindLen()) * m_iTextWidth;
            int iScrollRange = GetScrollLimit(SB_HORZ);        // bmd  28 apr 97
            int iScaledPos;  // scaled horizontal position of recently "found" text
            iScaledPos = (int) ( (long) (m_ptlLastFind.x * m_iTextWidth) * ((float) (iScrollRange)/((long) m_iTextWidth * (pBuffMgr->GetFileWidth()-m_iScrWidth))));
            if (iScaledPos != ptOffset.x)  {

                // csc 9/9/96 ... we have to make the THUMBPOSITION and iScaledPos into a single param.  This is different from Win16 ... see MSDN Q147684
                WPARAM wParam = MAKELONG (SB_THUMBPOSITION, iScaledPos);
                SendMessage (WM_HSCROLL, wParam);
                //SendMessage (WM_HSCROLL, SB_THUMBPOSITION, iScaledPos);  changed csc 9/9/96

                // following section added ... csc 2/28/97 ... "butterfly bug"
                // sometimes scaling or not causes us to misalign horiz scrolling; we'll go char by char until it's OK

                if (iWidth <= rcPageSize.Size().cx || (iWidth > rcPageSize.Size().cx && iScaledPos != ptOffset.x))  {
                    SetRedraw(FALSE);
                    while ((int) m_ptlLastFind.x < pBuffMgr->GetCurrCol())  {
                        ASSERT (FALSE);           //  butterfly bug fix is needed
                        SendMessage(WM_HSCROLL, SB_LINELEFT);
                    }
                    while ((int) m_ptlLastFind.x+(int) m_dlgFind.GetFindLen() > pBuffMgr->GetCurrCol()+m_iScrWidth)  {
                        ASSERT (FALSE);           //  butterfly bug fix is needed
                        SendMessage(WM_HSCROLL, SB_LINERIGHT);
                    }
                    SetRedraw(TRUE);
                }

                SetShowLastFind (TRUE);            // the call to OnHScroll() due to the SendMessage() will also invoke ClearFindSelection(), so we need
                                                   // to re-activate the "found" status.
                ptOffset = GetScrollPosition();    // might have just been updated!
            }

            if ( iWidth <= rcPageSize.Size().cx || ( iWidth > rcPageSize.Size().cx && iScaledPos != ptOffset.x) )  {// && (int) m_ptlLastFind.x * m_iTextWidth != GetScrollPos(SB_HORZ) ) )  {
                // the whole highlighted text can either 1) fit completely, or 2) can't fit but isn't left aligned ...
//                ASSERT ( m_ptlLastFind.x * (long) m_iTextWidth >= 0L );
//                ASSERT ( m_ptlLastFind.x * (long) m_iTextWidth <= 32767L );
                SetScrollPos (SB_HORZ, iScaledPos, TRUE);
                Invalidate ();   // redraw it all...
            }
        }
    }

    if ( pBuffMgr->Status () == BEGFILE )  {
        // ensure that we're at the first displayable line ...
        pBuffMgr->GetNextLine ();
    }

    for ( i = 0 ; i < m_iScrHgt ; i++ )  {
        stStatus = pBuffMgr->Status();

        csLine = pBuffMgr->GetNextLine ();

        if ( stStatus != ENDFILE )  {
            while ( (iOffset = csLine.Find ((TCHAR) 9)) != NONE )  {
                // convert tabs to spaces in the line ...
                csLine = csLine.Left (iOffset) + csPadding.Left (TAB_SPACES-(iOffset % TAB_SPACES)) + csLine.Mid (iOffset+1);
            }
            if (pBuffMgr->GetCurrCol() < csLine.GetLength()) {       // BMD 22 Dec 99
                pDC->TextOut ( ptOffset.x, i*m_iTextHgt+ptOffset.y, csLine.Mid(pBuffMgr->GetCurrCol(), m_iScrWidth) );
            }
            if ( pBuffMgr->IsFormFeed() )  {
                // hard form feed found ('\f'); indicate it by drawing a horizontal line across the bottom pixel of the line just displayed
                pDC->MoveTo (0, (i+1)*m_iTextHgt-1+ptOffset.y);
                pDC->LineTo (std::max (GetTotalSize().cx, rcPageSize.Size().cx), (i+1)*m_iTextHgt+ptOffset.y-1);
            }
        }
        else  {
            iExtraAmount++;
        }
    }
    pDC->SetBkMode(iOldMode);

    for ( i = 0 ; i <= m_iScrHgt-1-iExtraAmount ; i++ )  {
        // restore the buffer manager to the top of the window...
        pBuffMgr->GetPrevLine ();
    }

    //------------------------------------------------------------
    // display recently "found" text in a different color ...
    //------------------------------------------------------------
    if ( ShowLastFind() )  {
        if ( m_ptlLastFind.y >= pBuffMgr->GetCurrLine() && m_ptlLastFind.y <= pBuffMgr->GetCurrLine() + (long) m_iScrHgt)  {
            COLORREF colorOldBk = pDC->GetBkColor (),
                     colorOldText = pDC->GetTextColor ();

            int iDelta = 0;

            pDC->SetBkColor ( RGB(0,0,128) );
            pDC->SetTextColor ( RGB(255,255,255) );
            while ( m_ptlLastFind.y >= pBuffMgr->GetCurrLine() )  {
                ASSERT ( pBuffMgr->Status () != ENDFILE );
                csLine = pBuffMgr->GetNextLine();
                iDelta++;
            }

            // convert tabs to spaces in the line ...
            while ( (iOffset = csLine.Find ((TCHAR) 9)) != NONE )  {
                csLine = csLine.Left (iOffset) + csPadding.Left (TAB_SPACES-(iOffset % TAB_SPACES)) + csLine.Mid (iOffset+1);
            }

//            ASSERT ( m_dlgFind.GetFindLen () > 0 );  not needed csc 4 jan 03
            ASSERT ( m_ptlLastFind.x <= 32767L );
            ASSERT ( m_ptlLastFind.x >= 0L );
            pDC->TextOut ( ptOffset.x + m_iTextWidth*((int)m_ptlLastFind.x-pBuffMgr->GetCurrCol()), (iDelta-1)*m_iTextHgt+ptOffset.y, csLine.Mid( (int) m_ptlLastFind.x), (int) m_dlgFind.GetFindLen() );
            for ( i = 0 ; i < iDelta ; i++ )  {
                ASSERT ( pBuffMgr->Status () != BEGFILE );
                pBuffMgr->GetPrevLine();
            }
            pDC->SetBkColor (colorOldBk);
            pDC->SetTextColor (colorOldText);
        }
    }
    CBlockScrollView::OnDraw (pDC);
    pDC->SelectObject(pOldPen);   // frees "pen" so that it can be automatically destroyed on exit
    pDC->SelectObject(pOldFont);
    csLine.Empty();
}

/////////////////////////////////////////////////////////////////////////////
// CTVView printing

void CTVView::OnFilePrintPreview(void) {

    if (m_bRulersActivated) {
        m_bRulerTempOff = TRUE;
        SendMessage(WM_COMMAND, UWM::TextView::ToggleRuler);
    }
    CView::OnFilePrintPreview();
}



BOOL CTVView::OnPreparePrinting(CPrintInfo* pInfo)
{
    // default preparation
    ASSERT(pInfo->m_pPD != NULL);
    pInfo->m_pPD->m_pd.Flags &= 0x00FFFF00;
    pInfo->m_pPD->m_pd.Flags |= PD_USEDEVMODECOPIES;
    if (IsBlockActive()) {
        pInfo->m_pPD->m_pd.Flags |= PD_SELECTION;
    }
    else {
        pInfo->m_pPD->m_pd.Flags |= PD_NOSELECTION;
    }
    m_bContinuePrinting = TRUE;
    nLastPage = 0;
    return DoPreparePrinting(pInfo);
}

void CTVView::OnBeginPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)  {
    CBufferMgr *pBuffMgr = ((CTVDoc *) GetDocument())->GetBufferMgr();
    lPrevLine = pBuffMgr->GetCurrLine();
    bFirstPage = TRUE;
    lTopPrevPage = 0L;
//    m_folio.Create(GetDocument()->GetTitle());
}

void CTVView::OnEndPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)  {
    CBufferMgr *pBuffMgr = ((CTVDoc *) GetDocument())->GetBufferMgr();
    pBuffMgr->GotoLineNumber (lPrevLine);
    Invalidate();                                     // bmd 11 Oct 95
    if (m_bRulerTempOff) {
        m_bRulerTempOff = FALSE;
        SendMessage(WM_COMMAND, UWM::TextView::ToggleRuler);
    }
}

void CTVView::OnPrepareDC (CDC *pDC, CPrintInfo *pInfo)  {
    CBlockScrollView::OnPrepareDC(pDC, pInfo);
    if (pInfo != NULL) {
        pInfo->m_bContinuePrinting = m_bContinuePrinting;
    }
}

void CTVView::OnPrint(CDC *pDC, CPrintInfo* pInfo) {

    CBufferMgr *pBuffMgr = ((CTVDoc*) GetDocument())->GetBufferMgr();
    pDC->SetMapMode(MM_TWIPS);

    long i;
    int iPrevFileWidth = pBuffMgr->GetFileWidth();
    int j, nCount, nHeight, nWidth, nOffset;
    CFont font;
    TEXTMETRIC  tm;

    int iSize = (int) AfxGetApp()->GetProfileInt(_T("Options"), _T("FontSize"), 8);
    iSize = -iSize * 20;

    bool bLineDraw = !AfxGetApp()->GetProfileInt(_T("Options"), _T("LineDraw"), FALSE)?false:true;

    if (bLineDraw) {
        //do not create the line draw font. now the line draw characters are all unicode
        font.CreateFont (iSize, 0, 0, 0, 700, FALSE, FALSE, 0, DEFAULT_CHARSET,
                        OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                        DEFAULT_QUALITY, FF_DONTCARE,
                        _T("MS LineDraw"));
    }
    else {
        font.CreateFont (iSize, 0, 0, 0, 700, FALSE, FALSE, 0, DEFAULT_CHARSET,
                        OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                        DEFAULT_QUALITY, FF_DONTCARE,
                        _T("Courier New"));
    }
    CFont* pOldFont = (CFont*) pDC->SelectObject(&font);
    pDC->GetTextMetrics(&tm);
    nHeight = tm.tmHeight + tm.tmExternalLeading;
    nWidth = tm.tmAveCharWidth;

    m_folio.Create(GetDocument()->GetTitle());
    CRect rectTextArea = m_folio.GetUserArea();
    CRect rectHeadArea = rectTextArea;
    CRect rectFootArea = rectTextArea;
    int nSize = -rectTextArea.Height();
    if (m_folio.HasHeader()) {
        nSize -= 2 * nHeight;
        rectHeadArea.bottom = rectHeadArea.top - nHeight;
        rectTextArea.top -= 2 * nHeight;
    }
    if (m_folio.HasFooter()) {
        nSize -= 2 * nHeight;
        rectFootArea.top = rectFootArea.bottom + nHeight;
        rectTextArea.bottom += 2 * nHeight;
    }

    int nLinesPerPage = nSize / nHeight;
    int nLineLen = rectTextArea.Width() / nWidth;
    if (bFirstPage) {
        pBuffMgr->GotoLineNumber (0L);
        bFirstPage = FALSE;
    }
    CPoint pt;
    CSize sizeText;
    CString csText;
    if (m_folio.HasHeader()) {
        int iWidth = rectHeadArea.Width();
        pDC->LPtoDP(rectHeadArea);
        pt = pDC->SetViewportOrg(rectHeadArea.TopLeft());
        pDC->TextOut(0, 0, m_folio.GetHeaderLeft(pInfo->m_nCurPage));
        csText = m_folio.GetHeaderCenter(pInfo->m_nCurPage);
        sizeText = pDC->GetTextExtent(csText);
        pDC->TextOut((iWidth - sizeText.cx)/2, 0, csText);
        csText = m_folio.GetHeaderRight(pInfo->m_nCurPage);
        sizeText = pDC->GetTextExtent(csText);
        pDC->TextOut(iWidth - sizeText.cx, 0, csText);
        pDC->SetViewportOrg(pt);
    }
    pDC->LPtoDP(rectTextArea);
    pt = pDC->SetViewportOrg(rectTextArea.TopLeft());
    if (pInfo->m_pPD->PrintSelection ()) {
        // Print selected text
        CLRect rclBlockedRect = GetBlockedRectChar ();
        ASSERT(rclBlockedRect.top >= 0);
        ASSERT(rclBlockedRect.left >= 0);
        int iLen = (int) (rclBlockedRect.right - rclBlockedRect.left + 1);
        int iLeft = (int) rclBlockedRect.left;
        if (pInfo->m_nCurPage == 1) {
            pBuffMgr->GotoLineNumber(rclBlockedRect.top);
        }
        else {
            nCount = (pInfo->m_nCurPage - nLastPage) - 1;
            if (nCount < 0) {
                pBuffMgr->GotoLineNumber(lTopPrevPage);
                for (j = 0; j < (abs(nCount)-1); j++) {
                    for (i = 0; i < nLinesPerPage; i++) {
                        pBuffMgr->GetPrevLine();
                        if (pBuffMgr->IsFormFeed()) {
                            break;
                        }
                    }
                }
            }
            else if (nCount > 0) {
                for (j = 0; j < nCount; j++) {
                    for (i = 0; i < nLinesPerPage; i++) {
                        pBuffMgr->GetNextLine();
                        if (pBuffMgr->IsFormFeed() || pBuffMgr->GetCurrLine() >rclBlockedRect.bottom ||
                                                      pBuffMgr->GetCurrLine() >= pBuffMgr->GetLineCount()) {
                            break;
                        }
                    }
                }
            }
        }
        lTopPrevPage = pBuffMgr->GetCurrLine();
        for (j = 0; j < nLinesPerPage; j++) {
            csLine = pBuffMgr->GetNextLine();
            while ( (nOffset = csLine.Find ((TCHAR) 9)) != NONE )  {
                csLine = csLine.Left (nOffset) + csPadding.Left (TAB_SPACES-(nOffset % TAB_SPACES)) + csLine.Mid (nOffset+1);
            }
            if (csLine.Mid(iLeft, iLen).GetLength() > nLineLen) {
                pDC->TextOut(0, - (j * nHeight), csLine.Mid(iLeft, iLen), nLineLen);
            }
            else {
                pDC->TextOut(0, - (j * nHeight), csLine.Mid(iLeft, iLen));
            }
//          pDC->TextOut(0, -j * nHeight, csLine.Mid(iLeft, iLen));
            if (pBuffMgr->IsFormFeed() || pBuffMgr->GetCurrLine() > rclBlockedRect.bottom ||
                                          pBuffMgr->GetCurrLine() >= pBuffMgr->GetLineCount()) {
                break;
            }
        }
        if (pBuffMgr->GetCurrLine() > rclBlockedRect.bottom || pBuffMgr->GetCurrLine() >= pBuffMgr->GetLineCount()) {
            if (pInfo->m_bPreview) {
                pInfo->SetMaxPage(pInfo->m_nCurPage);
            }
            else {
                m_bContinuePrinting = FALSE;
            }
        }
    }
    else {
        if (pInfo->m_nCurPage == 1) {
            pBuffMgr->GotoLineNumber(0);
        }
        else {
            nCount = (pInfo->m_nCurPage - nLastPage) - 1;
            if (nCount < 0) {
                pBuffMgr->GotoLineNumber(lTopPrevPage);
                for (j = 0; j < (abs(nCount)-1); j++) {
                    for (i = 0; i < nLinesPerPage; i++) {
                        ASSERT(pBuffMgr->GetCurrLine() > 0);
                        pBuffMgr->GetPrevLine();
                        if (pBuffMgr->IsFormFeed() || pBuffMgr->GetCurrLine() <= 0) {
                            break;
                        }
                    }
                }
            }
            else if (nCount > 0) {
                for (j = 0; j < nCount; j++) {
                    for (i = 0; i < nLinesPerPage; i++) {
                        pBuffMgr->GetNextLine();
                        if (pBuffMgr->IsFormFeed() || pBuffMgr->GetCurrLine() >= pBuffMgr->GetLineCount()) {
                            break;
                        }
                    }
                }
            }
        }
        lTopPrevPage = pBuffMgr->GetCurrLine();
        for (j = 0; j < nLinesPerPage; j++) {
            csLine = pBuffMgr->GetNextLine();
            while ( (nOffset = csLine.Find ((TCHAR) 9)) != NONE )  {
                csLine = csLine.Left (nOffset) + csPadding.Left (TAB_SPACES-(nOffset % TAB_SPACES)) + csLine.Mid (nOffset+1);
            }
            if (csLine.GetLength() > nLineLen) {
                pDC->TextOut(0, - (j * nHeight), csLine, nLineLen);
            }
            else {
                pDC->TextOut(0, - (j * nHeight), csLine);
            }
//          pDC->TextOut(0, -j * nHeight, csLine, csLine.GetLength());
            if (pBuffMgr->IsFormFeed() || pBuffMgr->GetCurrLine() >= pBuffMgr->GetLineCount()) {
                break;
            }
        }
        if (pBuffMgr->GetCurrLine() >= pBuffMgr->GetLineCount()) {
            if (pInfo->m_bPreview) {
                pInfo->SetMaxPage(pInfo->m_nCurPage);
            }
            else {
                m_bContinuePrinting = FALSE;
            }
        }
    }
    pDC->SetViewportOrg(pt);
    if (m_folio.HasFooter()) {
        int iWidth = rectFootArea.Width();
        pDC->LPtoDP(rectFootArea);
        pDC->SetViewportOrg(rectFootArea.TopLeft());
        pDC->TextOut(0, 0, m_folio.GetFooterLeft(pInfo->m_nCurPage));
        csText = m_folio.GetFooterCenter(pInfo->m_nCurPage);
        sizeText = pDC->GetTextExtent(csText);
        pDC->TextOut((iWidth - sizeText.cx)/2, 0, csText);
        csText = m_folio.GetFooterRight(pInfo->m_nCurPage);
        sizeText = pDC->GetTextExtent(csText);
        pDC->TextOut(iWidth - sizeText.cx, 0, csText);
        pDC->SetViewportOrg(pt);
    }
    pDC->SelectObject(pOldFont);
    nLastPage = pInfo->m_nCurPage;

    if ( iPrevFileWidth != pBuffMgr->GetFileWidth() )  {
        // file width might have changed due to estimating number of lines in the file (see BufferMgr::CountLines() etc.)
        // and incorrectly guessing the maximum file width when the file was initially (partially) scanned
        SetScrollParameters();
        ResizeRulers();
    }
}


/////////////////////////////////////////////////////////////////////////////
// CTVView diagnostics

#ifdef _DEBUG
void CTVView::AssertValid() const
{
    CBlockScrollView::AssertValid();
}

void CTVView::Dump(CDumpContext& dc) const
{
    CBlockScrollView::Dump(dc);
    dc << _T("\nCTVView::Dump\n");
}

CTVDoc* CTVView::GetDocument() // non-debug version is inline
{
        ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CTVDoc)));
        return (CTVDoc*)m_pDocument;
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CTVView message handlers
/******************************************************************************
*
*                     CTVView::OnEditCopy()
*
*******************************************************************************
*   Revision history:
*   - error if user somehow gets a blocked area greater than 64K; CSC 8/1/95
*
******************************************************************************/
void CTVView::OnEditCopy()  {
    BOOL bRetCode;
    long lSize = GetBlockedBufferSize()+1;
    lSize = lSize* sizeof(TCHAR);

    if (lSize > MAXCLIP95)  {
        AfxMessageBox (_T("Block is too large to copy to the clipboard"), MB_ICONINFORMATION);
        return;
    }
    if (lSize > 0)  {
        HGLOBAL hGlobalMemory;
        VERIFY ( (hGlobalMemory = GlobalAlloc (GHND, lSize )) != NULL );
        TCHAR FAR *lpGlobalMemory = (TCHAR FAR *) GlobalLock (hGlobalMemory);
        GetBlockedBuffer ( lpGlobalMemory );
        bRetCode = ! GlobalUnlock (hGlobalMemory);
        ASSERT ( bRetCode );
        bRetCode = OpenClipboard ();
        ASSERT ( bRetCode );
        bRetCode = EmptyClipboard ();
        ASSERT ( bRetCode );
        if (!((CMainFrame*) AfxGetMainWnd())->m_bLineDraw) {
            bRetCode = (SetClipboardData (CF_UNICODETEXT, hGlobalMemory) != NULL);
        }
        else {
            bRetCode = (SetClipboardData (CF_UNICODETEXT, hGlobalMemory) != NULL);
        }
        ASSERT ( bRetCode );
        bRetCode = CloseClipboard ();
        ASSERT ( bRetCode );
    }
}

/******************************************************************************
*
*                     CTVView::OnEditCopySS()
*
*******************************************************************************
*   Revision history:
*   - error if user somehow gets a blocked area greater than 64K; CSC 8/1/95
*
******************************************************************************/
void CTVView::OnEditCopySS()  {
    BOOL bRetCode;
    long lSize = GetBlockedBufferSize()+1;
    lSize = lSize * sizeof(TCHAR); // GHM 20130212

    if (lSize > MAXCLIP95)  {
        AfxMessageBox (_T("Block is too large to copy to the clipboard"), MB_ICONINFORMATION);
        return;
    }
    if (lSize > 0)  {
        HGLOBAL hGlobalMemory;
        VERIFY ( (hGlobalMemory = GlobalAlloc (GHND, lSize )) != NULL );
        TCHAR FAR *lpGlobalMemory = (TCHAR FAR *) GlobalLock (hGlobalMemory);
        GetBlockForSS ( lpGlobalMemory );
        bRetCode = ! GlobalUnlock (hGlobalMemory);
        ASSERT ( bRetCode );
        bRetCode = OpenClipboard ();
        ASSERT ( bRetCode );
        bRetCode = EmptyClipboard ();
        ASSERT ( bRetCode );
        if (!((CMainFrame*) AfxGetMainWnd())->m_bLineDraw) {
            bRetCode = (SetClipboardData (CF_UNICODETEXT, hGlobalMemory) != NULL);
        }
        else {
            bRetCode = (SetClipboardData (CF_UNICODETEXT, hGlobalMemory) != NULL);
        }
        ASSERT ( bRetCode );
        bRetCode = CloseClipboard ();
        ASSERT ( bRetCode );
    }
}

void CTVView::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)  {
    CBlockScrollView::OnKeyDown(nChar, nRepCnt, nFlags);

    switch (this, nChar)  {

//case VK_INSERT:
//    AfxGetMainWnd()->SendMessage(WM_IMPS40_LOGOPEN);
//    break;

    case VK_ESCAPE:
        ClearFindSelection ();
        break;
    case VK_UP:
        SendMessage (WM_VSCROLL, SB_LINEUP);
        break;
    case VK_DOWN:
        SendMessage (WM_VSCROLL, SB_LINEDOWN);
        break;
    case VK_HOME:
        if ( GetKeyState (VK_CONTROL) < 0 )  {
            SendMessage (WM_VSCROLL, SB_TOP);
        }
        SendMessage (WM_HSCROLL, SB_LEFT);
        break;
    case VK_END:
        if ( GetKeyState (VK_CONTROL) < 0 )  {
            SendMessage (WM_VSCROLL, SB_BOTTOM);
            SendMessage (WM_HSCROLL, SB_LEFT);
        }
        else  {
            SendMessage (WM_HSCROLL, SB_RIGHT);
        }
        break;
    case VK_PRIOR:
        if ( GetKeyState (VK_CONTROL) < 0 )  {
            SendMessage (WM_VSCROLL, SB_PHYSICAL_PAGEUP);
        }
        else {
            SendMessage (WM_VSCROLL, SB_PAGEUP);
        }
        break;
    case VK_NEXT:
        if ( GetKeyState (VK_CONTROL) < 0 )  {
            SendMessage (WM_VSCROLL, SB_PHYSICAL_PAGEDOWN);
        }
        else {
            SendMessage (WM_VSCROLL, SB_PAGEDOWN);
        }
        break;
    case VK_LEFT:
        SendMessage (WM_HSCROLL, SB_LINELEFT);
        break;
    case VK_RIGHT:
        SendMessage (WM_HSCROLL, SB_LINERIGHT);
        break;
    case VK_TAB:
        if ( GetKeyState (VK_SHIFT) < 0 )  {
            // shift is pressed!
            SendMessage (WM_HSCROLL, SB_PAGELEFT);
        }
        else  {
            // shift is not pressed
            SendMessage (WM_HSCROLL, SB_PAGERIGHT);
        }
        break;
    }
}

void CTVView::SetScrollParameters (void)  {
    CRect rcPageSize;
    CBufferMgr *pBuffMgr = ((CTVDoc *) GetDocument())->GetBufferMgr();

    // this function will be called several times before the document has been fully created (and
    // the file initialized, thus setting up lNumLines, etc.).  We don't want to do anything here
    // before the BufferMgr has been initialized!
    if ( pBuffMgr->IsInitialized () && ! m_bErrorState )  {
        GetClientRect (&rcPageSize);
        ASSERT ( pBuffMgr->GetLineCount() > 0L );
        ASSERT ( m_iTextWidth > 0 && m_iTextHgt > 0 );

        if ( (long) pBuffMgr->GetFileWidth() * (long) m_iTextWidth + rcPageSize.right % m_szLineSize.cx > 32767L )  {
            m_bForceHScrollBarOverride = TRUE;
            m_szTotalSize.cx = 32767 - rcPageSize.right % m_szLineSize.cx;
        }
        else  {
            m_szTotalSize.cx = pBuffMgr->GetFileWidth() * m_iTextWidth;
            m_bForceVScrollBarOverride = FALSE;
        }
        if ( pBuffMgr->GetLineCount() * m_iTextHgt + rcPageSize.bottom % m_szLineSize.cy > 32767L )  {
            m_bForceVScrollBarOverride = TRUE;
            m_szTotalSize.cy = 32767 - rcPageSize.bottom % m_szLineSize.cy;
        }
        else  {
            m_szTotalSize.cy = (int) pBuffMgr->GetLineCount() * m_iTextHgt;
            m_bForceVScrollBarOverride = FALSE;
        }
        m_szTotalSize += CSize (rcPageSize.right % m_szLineSize.cx, rcPageSize.bottom % m_szLineSize.cy);
        ASSERT ( m_szLineSize.cy != 0 );
        m_iScrHgt = (rcPageSize.bottom/m_szLineSize.cy) + 1;
        m_iScrWidth = (rcPageSize.right/m_szLineSize.cx) + 1;
        rcPageSize.bottom = m_iTextHgt * (m_iScrHgt - 1);
        rcPageSize.right = TAB_SCROLL_SIZE * m_iTextWidth;
        ASSERT ( m_iScrHgt > 0 );
        ASSERT ( rcPageSize.Size().cy >= 0 );   // can happen if the app is very tiny
        ASSERT ( rcPageSize.Size().cx > 0 );
        ASSERT ( m_szLineSize.cy > 0 && m_szLineSize.cx > 0 );
//        ASSERT ( m_szTotalSize.cy > 0 && m_szTotalSize.cx > 0 );
        SetScrollSizes (MM_TEXT, m_szTotalSize, rcPageSize.Size(), m_szLineSize);
    }
}

void CTVView::OnSize(UINT nType, int cx, int cy)  {
    CBlockScrollView::OnSize(nType, cx, cy);
    ResizeRulers ();
    SetScrollParameters ();
    ClearFindSelection ();
}

void CTVView::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)  {
    CTVDoc* pDoc = DYNAMIC_DOWNCAST(CTVDoc, GetDocument());
    CBufferMgr *pBuffMgr = pDoc->GetBufferMgr();
    long lTotalLines = pBuffMgr->GetLineCount() + 1;
    long lCurrLine = pBuffMgr->GetCurrLine();
    int iPrevFileWidth = pBuffMgr->GetFileWidth();
    long lMoveLines;
    int iScrollPos = GetScrollPos(SB_VERT);
    int iScrollRange = GetScrollLimit(SB_VERT);    // csc 2/16/95 Win32

    lMoveLines = lCurrLine;
    switch ( nSBCode )  {
    case SB_LINEUP:
        pBuffMgr->Up (m_iScrHgt);
        break;
    case SB_LINEDOWN:
        pBuffMgr->Down (m_iScrHgt);
        break;
    case SB_PAGEUP:
        pBuffMgr->PgUp (m_iScrHgt);
        break;
    case SB_PAGEDOWN:
        pBuffMgr->PgDown (m_iScrHgt);
        break;
    case SB_PHYSICAL_PAGEUP:
        pBuffMgr->PhysicalPgUp (m_iScrHgt);
        if ( lCurrLine != pBuffMgr->GetCurrLine() )  {
            Invalidate();
        }
        break;
    case SB_PHYSICAL_PAGEDOWN:
        pBuffMgr->PhysicalPgDown (m_iScrHgt);
        if ( lCurrLine != pBuffMgr->GetCurrLine() )  {
            Invalidate();
        }
        break;
    case SB_TOP:
        pBuffMgr->BeginOfFile (m_iScrHgt);
        if ( lMoveLines - pBuffMgr->GetCurrLine() > (long) m_iScrHgt )  {
            Invalidate ();                                      // cause total redraw
            SetScrollPos (SB_VERT, m_iScrHgt * m_iTextHgt, FALSE);  // ensure that we have enough scroll space to move and redraw
            lMoveLines = pBuffMgr->GetCurrLine ();              // don't need forced repositioning of scroll bars below (potential overflow problems)
        }
        break;
    case SB_BOTTOM:
        pBuffMgr->EndOfFile (m_iScrHgt);
        if ( pBuffMgr->GetCurrLine() - lMoveLines > (long) m_iScrHgt )  {
            Invalidate ();                                      // cause total redraw
            SetScrollPos (SB_VERT, m_iScrHgt * m_iTextHgt, FALSE);  // ensure that we have enough scroll space to move and redraw
            lMoveLines = pBuffMgr->GetCurrLine ();              // don't need forced repositioning of scroll bars below (potential overflow problems)
        }
        break;
    case SB_THUMBTRACK:
        // we ignore these messages; they mean that the user is in the middle of
        // sliding the thumbtrack.  Instead, wait for a thumbposition msg, indicating
        // that the sliding operation is finished (user let up on the lbutton)
        return;
    case SB_THUMBPOSITION:
        long lUnscaledNPos;

        ASSERT (iScrollRange != 0);
        lUnscaledNPos = (long) ( (float) nPos * ( (float) m_iTextHgt * (lTotalLines-m_iScrHgt))/iScrollRange);
        ASSERT (m_iTextHgt != 0);
        lMoveLines = (long) (lUnscaledNPos/m_iTextHgt-lCurrLine);
        pBuffMgr->GotoLineNumber ((long) lUnscaledNPos/m_iTextHgt);
        if (!pBuffMgr->IsEstimatingNumLines() && (lUnscaledNPos/(long)m_iTextHgt)!=pBuffMgr->GetCurrLine() && pBuffMgr->Status()==ENDFILE)  {
            // we were estimating and overshot the end of file!
            for ( int i = 0 ; i < m_iScrHgt-1 ; i++ )  {
                // back us up so that a whole page is displayed
                pBuffMgr->GetPrevLine ();
            }
        }
        Invalidate ();                                          // cause redraw of the whole client rect...
        if ( m_bForceVScrollBarOverride )  {
            SetScrollPos (SB_VERT, m_iScrHgt * m_iTextHgt, FALSE);  // ensure that we have enough scroll space to move and redraw
            lMoveLines = pBuffMgr->GetCurrLine ();              // don't need forced repositioning of scroll bars below (potential overflow problems)
        }
        else  {
            nPos = (int) pBuffMgr->GetCurrLine() * m_iTextHgt;
            ASSERT ( nPos >= 0 );
            ASSERT ( (long) pBuffMgr->GetCurrLine() * m_iTextHgt <= 32767L );
            SetScrollPos (SB_VERT, nPos, TRUE);
        }
        break;
    case SB_ENDSCROLL:
        // no action
        break;
    }

    // csc 5/3/2004 ... process close file and reload signals ...
    if (pBuffMgr->Status()==CLOSEFILE) {
        SetRedraw(FALSE);
        pDoc->CloseDeletedFile();
        return;
    }
    if (pBuffMgr->Status()==RELOADFILE) {
        SetRedraw(FALSE);
        pDoc->ReloadFile();
        return;
    }

    if (m_bForceVScrollBarOverride)  {
        lMoveLines -= pBuffMgr->GetCurrLine ();   // < 0  --> downward movement, > 0 --> upward movement
        if (lMoveLines > 0 && (long)iScrollPos < lMoveLines * m_iTextHgt)  {
            // upward motion and not enough room to handle the scroll action ...
            ASSERT (m_iTextHgt * lMoveLines <= 32767L);
            SetScrollPos (SB_VERT, (int) lMoveLines * m_iTextHgt, FALSE);      // assure that we have enough room to perform the scrolling
        }
        if (lMoveLines < 0 && (long) (iScrollRange - iScrollPos) < -lMoveLines * m_iTextHgt) {
            // downward motion and not enough room to handle the scroll action ...
            ASSERT ( (long) iScrollRange + lMoveLines * m_iTextHgt >= 0L );
            ASSERT ( (long) iScrollRange + lMoveLines * m_iTextHgt <= 32767L );
            SetScrollPos (SB_VERT, iScrollRange + (int) (lMoveLines * m_iTextHgt), FALSE);
        }
    }
    CBlockScrollView::OnVScroll(nSBCode, nPos, pScrollBar);
    if (iPrevFileWidth != pBuffMgr->GetFileWidth())  {
        // file width might have changed due to estimating number of lines in the file (see BufferMgr::CountLines() etc.)
        // and incorrectly guessing the maximum file width when the file was initially (partially) scanned
        SetScrollParameters();
        ResizeRulers();
    }
    UpdateVScrollPos (TRUE);
}

void CTVView::UpdateVScrollPos (BOOL bRedrawFlag)  {
    CBufferMgr *pBuffMgr = ((CTVDoc *) GetDocument())->GetBufferMgr();
    int iScrollPos;
    long lCurrLine = pBuffMgr->GetCurrLine();
    if (m_bForceVScrollBarOverride)  {
        long lTotalLines;
        int iScrollRange = GetScrollLimit(SB_VERT);    // bmd 28 apr 97
        lTotalLines = pBuffMgr->GetLineCount() + 1;
        ASSERT (lTotalLines - m_iScrHgt != 0);
        iScrollPos = (int) (iScrollRange * ((float) lCurrLine / (lTotalLines-m_iScrHgt)));
        ASSERT (iScrollPos >= 0);
        ASSERT ((long) m_iScrHgt < lTotalLines);
        ASSERT (lCurrLine >= 0);
    }
    else  {
        CRect rcPageSize;
        GetClientRect (&rcPageSize);
        if ( rcPageSize.Size().cy >= GetTotalSize().cy )  {
            // the whole fits on one screen ... make sure we're at the top!
            if ( lCurrLine != 0L )  {
                pBuffMgr->BeginOfFile (m_iScrHgt);
            }
            iScrollPos = 0;
            ASSERT ( pBuffMgr->GetLineCount() * (long) m_iTextHgt <= (long) rcPageSize.Size().cy );  // double check it!
        }
        else  {
            iScrollPos = m_iTextHgt * (int) lCurrLine;
            ASSERT ( (long) m_iTextHgt * lCurrLine <= 32767L);
        }
    }
    SetScrollPos (SB_VERT, iScrollPos, bRedrawFlag);
    ClearFindSelection ();                   // turn off highlighted "find" text, if any
    UpdateRulers ();
    UpdateStatusBar ();
}

void CTVView::UpdateHScrollPos (BOOL bRedrawFlag)  {
    CBufferMgr *pBuffMgr = ((CTVDoc *) GetDocument())->GetBufferMgr();
    int iScrollPos;
    int iCurrCol = pBuffMgr->GetCurrCol();
    int iTotalWidth = pBuffMgr->GetFileWidth();

    if ( m_bForceHScrollBarOverride )  {
        int iScrollRange = GetScrollLimit(SB_HORZ);    // bmd 28 apr 97
        ASSERT ( iTotalWidth - m_iScrWidth != 0 );
        iScrollPos = (int) (iScrollRange * ((float) iCurrCol / (iTotalWidth-m_iScrWidth)));
        ASSERT ( iScrollPos >= 0 );
        ASSERT ( (long) m_iScrWidth < iTotalWidth );
        ASSERT ( iCurrCol >= 0 );
    }
    else  {
        CRect rcPageSize;
        GetClientRect (&rcPageSize);
        if ( rcPageSize.Size().cx >= GetTotalSize().cx )  {
            // the whole fits on one screen ... make sure we're at the top!
            if ( iCurrCol != 0L )  {
                pBuffMgr->LeftOfFile (m_iScrWidth);
            }
            iScrollPos = 0;
            ASSERT ( pBuffMgr->GetFileWidth() * (long) m_iTextWidth <= (long) rcPageSize.Size().cx );  // double check it!
        }
        else  {
            iScrollPos = m_iTextWidth * iCurrCol;
            ASSERT ( (long) m_iTextWidth * (long) iCurrCol <= 32767L);
        }
    }
    SetScrollPos (SB_HORZ, iScrollPos, bRedrawFlag);
    ClearFindSelection ();                   // turn off highlighted "find" text, if any
    UpdateRulers ();
    UpdateStatusBar ();
}

LONG CTVView::OnSearch (UINT, LONG)  {
    // message received in response to modeless dialoak Next or Previous button
    CBufferMgr* pBuffMgr = ((CTVDoc*) GetDocument())->GetBufferMgr();
    CLPoint ptlFindPos;
    BOOL bFound = FALSE;
    int iPrevFileWidth = pBuffMgr->GetFileWidth();
    std::shared_ptr<ProgressDlg> dlgProgress = ProgressDlgFactory::Instance().Create();

    if (CTVView::m_dlgFind.GetSafeHwnd()!=NULL)  {
        CTVView::m_dlgFind.ShowWindow (SW_HIDE);
    }

    UpdateWindow();  // force redraw of the main view before going into a potentially long serach operation ...

    if (ShowLastFind())  {
        ptlFindPos = GetLastFind();
    }
    else  {
        ptlFindPos = CLPoint((long) NONE, (long) NONE);
    }

    CString msg;                                     // BMD 05 Sep 2002
    msg.Format(IDS_WAIT02, (LPCTSTR)m_dlgFind.m_csSearchText);
    dlgProgress->SetStatus(msg);
    dlgProgress->SetStep(1);
//    dlgProgress.SetRange(0, min(65535, pBuffMgr->GetCurrLine()+1));
    dlgProgress->SetRange(0, std::min(65535, (int) pBuffMgr->GetLineCount()+1));   // changed csc 8/23/96

    ASSERT(!IsProgressDlgActive());
    g_bIsProgressDlgActive = TRUE;   // for preventing redraws during search (buffer won't correspond) ... moved from IMPSUtil 9/9/96 csc

    switch (CTVView::m_dlgFind.GetDirection())  {
    case SEARCH_FORWARD:
        bFound = pBuffMgr->SearchForward((const TCHAR*)CTVView::m_dlgFind.m_csSearchText, CTVView::m_dlgFind.m_bCaseSensitive, ptlFindPos, dlgProgress);
        break;
    case SEARCH_BACKWARD:
        bFound = pBuffMgr->SearchBackward((const TCHAR*)CTVView::m_dlgFind.m_csSearchText, CTVView::m_dlgFind.m_bCaseSensitive, ptlFindPos, dlgProgress);
        break;
    default:
        ASSERT(FALSE);
        break;
    }

    g_bIsProgressDlgActive = FALSE;

    if (CTVView::m_dlgFind.GetSafeHwnd()!=NULL)  {
        CTVView::m_dlgFind.SetFocus();
    }

    if (dlgProgress->CheckCancelButton())  {
        // the user cancelled the search before it finished ...
        AfxMessageBox (IDS_ERR01, MB_OK | MB_ICONEXCLAMATION );
        UpdateVScrollPos (TRUE);
        UpdateHScrollPos (TRUE);
        Invalidate();
        CTVView::m_dlgFind.SendMessage(WM_COMMAND, IDCLOSE);       // added csc 2/28/97
        return 0L;
    }

    if (bFound)  {
        Invalidate ();        // this might not be needed...
        UpdateVScrollPos (TRUE);
        UpdateHScrollPos (TRUE);
        SetLastFind (ptlFindPos);
        SetShowLastFind (TRUE);
    }
    else  {
        CString csErrMsg;
        csErrMsg.LoadString (IDS_ERR03);
        csErrMsg += CTVView::m_dlgFind.m_csSearchText;
        AfxMessageBox (csErrMsg, MB_OK | MB_ICONEXCLAMATION );
        UpdateVScrollPos (TRUE);
        UpdateHScrollPos (TRUE);
    }

    if (iPrevFileWidth != pBuffMgr->GetFileWidth())  {
        // file width might have changed due to estimating number of lines in the file (see BufferMgr::CountLines() etc.)
        // and incorrectly guessing the maximum file width when the file was initially (partially) scanned
        SetScrollParameters();
        ResizeRulers();
    }

    if (CTVView::m_dlgFind.GetSafeHwnd()!=NULL)  {
        CTVView::m_dlgFind.ShowWindow (SW_SHOW);
        CTVView::m_dlgFind.GetDlgItem(IDC_SEARCH_TEXT)->PostMessage(WM_SETFOCUS);   // BMD 28 April 2001
    }

    Invalidate();   // force redraw in case the user moved the progress dlg around the screen
    return 0L;
}


void CTVView::OnTimer(UINT nIDEvent)  {
    // the timer was fired due to the user moving the mouse outside of the client area while
    // blocking is active ... call the base class version!
    ASSERT (nIDEvent==SCROLL_TIMER);
    CBlockScrollView::OnTimer(nIDEvent);
}

LONG CTVView::OnSearchClose (UINT, LONG)  {
    return 0L;
}

void CTVView::ClearFindSelection (void)  {
    if ( ShowLastFind() )  {
        long lCurrLine = ((CTVDoc*) GetDocument())->GetBufferMgr()->GetCurrLine();
        int iCurrCol =  ((CTVDoc*) GetDocument())->GetBufferMgr()->GetCurrCol();
//        CLPoint m_ptlLastFind = GetLastFind ();
        ASSERT ( m_ptlLastFind.x >= 0L );
        ASSERT ( m_ptlLastFind.x <= 32767L );
        if ( m_ptlLastFind.y >= lCurrLine && m_ptlLastFind.y <= lCurrLine+m_iScrHgt && ! ( iCurrCol >= (int)m_ptlLastFind.x + (int)m_dlgFind.GetFindLen() || (int)m_ptlLastFind.x >= iCurrCol+m_iScrWidth) )  {
            // the recently found text *is* currently displayed on the screen!        ... thanks to Glenn for the above algorithm!
            ASSERT ((int) (m_ptlLastFind.y - lCurrLine) * m_iTextHgt >= 0);
            ASSERT ((int) (m_ptlLastFind.y - lCurrLine + 1L) * m_iTextHgt >= 0);
            ASSERT (m_iTextWidth*((int)m_ptlLastFind.x-iCurrCol) >= -32767L);
            ASSERT (m_iTextWidth*((int)m_ptlLastFind.x-iCurrCol + (int) m_dlgFind.GetFindLen()) >= -32767L);
            InvalidateRect ( CRect (m_iTextWidth * ((int)m_ptlLastFind.x-iCurrCol),                        // left
                                    (int) (m_ptlLastFind.y - lCurrLine) * m_iTextHgt,                      // top
                                    m_iTextWidth*((int)m_ptlLastFind.x-iCurrCol + m_dlgFind.GetFindLen()),   // right
                                    (int) (m_ptlLastFind.y - lCurrLine + 1L) * m_iTextHgt) );              // bottom
        }
    }
    SetShowLastFind (FALSE);
}

///////////////////////////////////////////////////////////////////////////////
//
//    CTVView::OnEditFind()
//
//  revision history:
//      4 Jan 2003     CSC       uses currently selected block as find string if on one line only
//
///////////////////////////////////////////////////////////////////////////////
void CTVView::OnEditFind()  {

    // csc 4 jan 03 ... if something is blocked, then try to use it as the initial find string
    if (GetBlockedRectChar().Height()==0)  {
        long lSize = GetBlockedBufferSize()+1;
        lSize = lSize* sizeof(TCHAR);
        if (lSize < MAXINT && lSize>1)  {
            HGLOBAL hGlobalMemory;
            VERIFY ( (hGlobalMemory = GlobalAlloc (GHND, lSize )) != NULL );
            TCHAR FAR *lpGlobalMemory = (TCHAR FAR *) GlobalLock (hGlobalMemory);
            GetBlockedBuffer ( lpGlobalMemory );
            GlobalUnlock (hGlobalMemory);
            CTVView::m_dlgFind.m_csSearchText = lpGlobalMemory;
        }
    }

    if ( CTVView::m_dlgFind.GetSafeHwnd() == 0 )  {
        // first time through, need to create the dialog ...
        CRect rcViewClient, rcDlgRect;
        CPoint ptDlgOrigin;

        CTVView::m_dlgFind.Create();                      // won't display the dialog on the screen because
                                                              //    we have declared it not "visible" in AppStudio
        CTVView::m_dlgFind.GetWindowRect ( &rcDlgRect);   // position of the dialog, relative to client view
        GetClientRect (&rcViewClient);                        // right,bottom hold size of client view
        ptDlgOrigin = CPoint (rcViewClient.right-rcDlgRect.Size().cx, rcViewClient.bottom-rcDlgRect.Size().cy);    // left, top of dialog, relative to client view
        ClientToScreen ( &ptDlgOrigin );                      // convert to absolute (screen) coordinates
        CTVView::m_dlgFind.MoveWindow ( ptDlgOrigin.x, ptDlgOrigin.y, rcDlgRect.Size().cx, rcDlgRect.Size().cy);  // x,y,width,height
        CTVView::m_dlgFind.ShowWindow (SW_SHOW);          // display the dialog box on the screen
    }

    // set the input focus to the "Find What" combo box ...
    CTVView::m_dlgFind.GotoDlgCtrl ( (CComboBox *) CTVView::m_dlgFind.GetDlgItem (IDC_SEARCH_TEXT) );
}

void CTVView::OnSetFocus(CWnd* pOldWnd)  {
//TRACE("TextView ... CTVView::OnSetFocus()\n");
    CBlockScrollView::OnSetFocus(pOldWnd);

    // we need to tell the dialog about the current view (where it will do its searching!)
    CTVView::m_dlgFind.SetCurrView (this);
    errorMgr.SetCurrView (this);
}

void CTVView::OnLButtonDown(UINT nFlags, CPoint point)  {
    ClearFindSelection ();
    CBlockScrollView::OnLButtonDown(nFlags, point);
}

void CTVView::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)  {
    CTVDoc* pDoc = DYNAMIC_DOWNCAST(CTVDoc, GetDocument());
    CBufferMgr *pBuffMgr = pDoc->GetBufferMgr();
    int iPrevCol = pBuffMgr->GetCurrCol();
    CPoint ptScaledOffset = GetScrollPosition();
    CRect rcPageSize;
    int iScrollRange = GetScrollLimit(SB_HORZ);    // bmd 4/28/97
    GetClientRect (&rcPageSize);
    switch ( nSBCode )  {
    case SB_LEFT:
        pBuffMgr->LeftOfFile (m_iScrWidth);
        if ( m_bForceHScrollBarOverride && m_iTextWidth * (iPrevCol - pBuffMgr->GetCurrCol()) > ptScaledOffset.x )  {
            // we need to ensure that we have enough scroll space to move and redraw
            SetScrollPos (SB_HORZ, m_iScrWidth * m_iTextWidth, FALSE);
        }
        break;
    case SB_RIGHT:
        pBuffMgr->RightOfFile (m_iScrWidth);
        if ( m_bForceHScrollBarOverride && m_iTextWidth * (pBuffMgr->GetCurrCol() - iPrevCol) > m_iTextWidth*m_iScrWidth - (GetTotalSize().cx - ptScaledOffset.x) )  {
            // we need to ensure that we have enough scroll space to move and redraw
            SetScrollPos (SB_HORZ, 0, FALSE);
        }
        break;
    case SB_LINELEFT:
        pBuffMgr->Left (1, m_iScrWidth);
        if ( m_bForceHScrollBarOverride && m_iTextWidth * (iPrevCol - pBuffMgr->GetCurrCol()) > ptScaledOffset.x )  {
            // we need to ensure that we have enough scroll space to move and redraw
            SetScrollPos (SB_HORZ, m_iTextWidth * (iPrevCol - pBuffMgr->GetCurrCol()), FALSE);
        }
        break;
    case SB_LINERIGHT:
        pBuffMgr->Right (1, m_iScrWidth);
        if ( m_bForceHScrollBarOverride && m_iTextWidth * (pBuffMgr->GetCurrCol() - iPrevCol) > m_iTextWidth*m_iScrWidth - (GetTotalSize().cx - ptScaledOffset.x))  {
            // we need to ensure that we have enough scroll space to move and redraw
            SetScrollPos (SB_HORZ, GetTotalSize().cx - rcPageSize.Size().cx - m_iTextWidth*(pBuffMgr->GetCurrCol() - iPrevCol), FALSE);
        }
        break;
    case SB_PAGELEFT:
        pBuffMgr->Left (TAB_SCROLL_SIZE, m_iScrWidth);
        if ( m_bForceHScrollBarOverride && m_iTextWidth * (iPrevCol - pBuffMgr->GetCurrCol()) > ptScaledOffset.x )  {
            // we need to ensure that we have enough scroll space to move and redraw
            SetScrollPos (SB_HORZ, m_iTextWidth * (iPrevCol - pBuffMgr->GetCurrCol()), FALSE);
        }
        break;
    case SB_PAGERIGHT:
        pBuffMgr->Right (TAB_SCROLL_SIZE, m_iScrWidth);
        if ( m_bForceHScrollBarOverride && m_iTextWidth * (pBuffMgr->GetCurrCol() - iPrevCol) > m_iTextWidth*m_iScrWidth - (GetTotalSize().cx - ptScaledOffset.x) )  {
            // we need to ensure that we have enough scroll space to move and redraw
            SetScrollPos (SB_HORZ, GetTotalSize().cx - rcPageSize.Size().cx - m_iTextWidth*(pBuffMgr->GetCurrCol() - iPrevCol), FALSE);
        }
        break;
    case SB_THUMBTRACK:
        // we ignore these messages; they mean that the user is in the middle of
        // sliding the thumbtrack.  Instead, wait for a thumbposition msg, indicating
        // that the sliding operation is finished (user let up on the lbutton)
        return;
    case SB_THUMBPOSITION:
        long lUnscaledNPos;

        ASSERT ( m_iTextWidth != 0 );
        Invalidate ();                                          // cause redraw of the whole client rect...
        if ( m_bForceHScrollBarOverride )  {
            ASSERT ( iScrollRange != 0 );
            lUnscaledNPos = (long) ( (float) nPos * ( (float) m_iTextWidth * (float) (pBuffMgr->GetFileWidth()-m_iScrWidth))/iScrollRange);
            if ( (int) (lUnscaledNPos/m_iTextWidth) >= iPrevCol )  {
                // net movement to the right ...
                pBuffMgr->Right ( (int) (lUnscaledNPos / m_iTextWidth - iPrevCol), m_iScrWidth);
            }
            else  {
                // net movement to the left ...
                pBuffMgr->Right ( (int) (lUnscaledNPos / m_iTextWidth - iPrevCol), m_iScrWidth);
            }
            SetScrollPos (SB_HORZ, m_iScrHgt * m_iTextHgt, FALSE);  // ensure that we have enough scroll space to move and redraw
        }
        else  {
            if ( (int) (nPos/m_iTextWidth) >= iPrevCol )  {
                // net movement to the right ...
                pBuffMgr->Right (nPos/m_iTextWidth-iPrevCol, m_iScrWidth);
            }
            else  {
                // net movement to the left ...
                pBuffMgr->Left (iPrevCol-nPos/m_iTextWidth, m_iScrWidth);
            }
            nPos = (int) pBuffMgr->GetCurrCol() * m_iTextWidth;   // recalculate to handle any rounding errors; align along a character column
            ASSERT ( nPos >= 0 );
            ASSERT ( (long) pBuffMgr->GetCurrCol() * m_iTextWidth <= 32767L );
            SetScrollPos (SB_HORZ, nPos, TRUE);
        }
        break;
    case SB_ENDSCROLL:
        // no action on this one ...
        return;
    default:
        ASSERT (FALSE);
    }

    // csc 5/3/2004 ... process close file and reload signals ...
    if (pBuffMgr->Status()==CLOSEFILE) {
        SetRedraw(FALSE);
        pDoc->CloseDeletedFile();
        return;
    }
    if (pBuffMgr->Status()==RELOADFILE) {
        SetRedraw(FALSE);
        pDoc->ReloadFile();
        return;
    }

    ClearFindSelection ();

    CBlockScrollView::OnHScroll(nSBCode, nPos, pScrollBar);
    UpdateRulers ();
    UpdateStatusBar ();
    UpdateHScrollPos (TRUE);
/*
    UpdateRulers ();
    UpdateStatusBar ();
    UpdateHScrollPos (TRUE);
    CBlockScrollView::OnHScroll(nSBCode, nPos, pScrollBar);
*/
}

void CTVView::ResizeRulers (void)  {
    // this function is called when the ruler width (offset) needs to be changed, a WM_SIZE is being
    // processed, or the user has toggled the ruler status option.
    if (m_bRulersInitialized)  {
        // adjust positioning of horizontal and verical rulers, as well as the three "filler" leftover spots on the window...
        WINDOWPLACEMENT wndpl;
        CRect rcPageSize;

        wndpl.length = sizeof (WINDOWPLACEMENT);
        VERIFY (GetWindowPlacement (&wndpl));
        if (m_bRulersActivated)  {
            GetClientRect (&rcPageSize);
            wndpl.rcNormalPosition.top = m_iTextHgt-1;
//            wndpl.rcNormalPosition.left = pVRulerView->GetOffset (rcPageSize.Size().cy);
            wndpl.rcNormalPosition.left = m_rulerMgr.GetOffset (rcPageSize.Size().cy);
            SetWindowPlacement(&wndpl);
            GetClientRect(&rcPageSize);
            m_rulerMgr.Resize (SW_SHOW, rcPageSize);
        }
        else  {
            wndpl.rcNormalPosition.top = NONE;
            wndpl.rcNormalPosition.left = NONE;
            VERIFY (SetWindowPlacement (&wndpl));
            m_rulerMgr.Resize (SW_HIDE);
        }
    }
}

void CTVView::UpdateRulers (void)  {
    if ( m_bRulersActivated )  {
        if ( m_rulerMgr.IsOffsetChanged (m_iScrHgt) )  {
            ResizeRulers ();
        }
        CBufferMgr *pBuffMgr = ((CTVDoc *) GetDocument())->GetBufferMgr();
        m_rulerMgr.UpdateRulerPosition (pBuffMgr->GetCurrCol(), pBuffMgr->GetCurrLine());
    }
}

void CTVView::OnToggleRuler()  {
    WINDOWPLACEMENT wndpl;
    wndpl.length = sizeof (WINDOWPLACEMENT);
    VERIFY (GetWindowPlacement (&wndpl));

    if (! m_bRulersActivated)  {
        wndpl.rcNormalPosition.top = m_iTextHgt;
        wndpl.rcNormalPosition.left = m_rulerMgr.GetOffset (wndpl.rcNormalPosition.bottom - wndpl.rcNormalPosition.top);
        m_bRulersActivated = TRUE;
        UpdateRulers ();
    }
    else  {
        wndpl.rcNormalPosition.top = -1;
        wndpl.rcNormalPosition.left = -1;
        m_bRulersActivated = FALSE;
    }
    VERIFY (SetWindowPlacement (&wndpl));
    // SetWindowPlacement() will cause a WM_SIZE, which will be processed in CTVView::OnSize(),
    // where the ruler and filler views' sizes will be ajusted appropriately.
}

void CTVView::OnUpdateViewRuler(CCmdUI* pCmdUI)  {
    // toggles menu check box and toolbar button statuses
    if ( GetParentFrame()->IsIconic() )  {
        pCmdUI->Enable (FALSE);
    }
    else  {
        pCmdUI->SetCheck (m_bRulersActivated);
    }
    if (m_bRulerTempOff) {
        pCmdUI->Enable(!m_bRulerTempOff);
    }
}

void CTVView::OnViewGotoline()  {
    CBufferMgr *pBuffMgr = ((CTVDoc *) GetDocument())->GetBufferMgr();
    CGotoDialog dlgGoto;
    CRect rcViewClient, rcDlgRect;
    CPoint ptDlgOrigin;
    int iPrevFileWidth = pBuffMgr->GetFileWidth();

    dlgGoto.SetView (this);              // let it know who it's owner is (CGotoDialog::OnSize will reposition the dialog box in a moment)
    dlgGoto.m_LineNumber = pBuffMgr->GetCurrLine() + 1L;
    int iRetVal = dlgGoto.DoModal();        // remember that result here is 1-based, while BuffMgr::CurrLine is 0-based
    dlgGoto.m_LineNumber--;
    if ( iRetVal == IDOK )  {
        if ( dlgGoto.m_LineNumber > pBuffMgr->GetLineCount() )  {
            // invalid line number .. too big!
            AfxMessageBox (IDS_ERR04, MB_OK | MB_ICONEXCLAMATION );
            pBuffMgr->EndOfFile (m_iScrHgt);
        }
        else  {
            if ( dlgGoto.m_LineNumber < 0 )  {
                // invalid line number ... too small!
                AfxMessageBox (IDS_ERR05, MB_OK | MB_ICONEXCLAMATION );
                pBuffMgr->BeginOfFile (m_iScrHgt);
            }
            else  {
                // good line number!
                pBuffMgr->GotoLineNumber ( dlgGoto.m_LineNumber );
            }
        }
        if ( iPrevFileWidth != pBuffMgr->GetFileWidth() )  {
            // file width might have changed due to estimating number of lines in the file (see BufferMgr::CountLines() etc.)
            // and incorrectly guessing the maximum file width when the file was initially (partially) scanned
            SetScrollParameters();
            ResizeRulers();
        }
        Invalidate ();
        UpdateVScrollPos (TRUE);
        UpdateHScrollPos (TRUE);
    }
}

void CTVView::UpdateStatusBar (void)  {
    CBufferMgr* pBuffMgr = ((CTVDoc *) GetDocument())->GetBufferMgr();
    ((CMainFrame*) AfxGetApp()->m_pMainWnd)->UpdateStatusBarScr ( CLPoint((long) pBuffMgr->GetCurrCol()+1L, pBuffMgr->GetCurrLine()+1L) );
}

inline void CTVView::OnUpdateEditCopy(CCmdUI* pCmdUI)  {
    pCmdUI->Enable ( IsBlockActive() && ! GetParentFrame()->IsIconic() );
}

inline void CTVView::OnUpdateEditCopySs(CCmdUI* pCmdUI)  {
    pCmdUI->Enable ( IsBlockActive() && ! GetParentFrame()->IsIconic() );
}

inline void CTVView::OnUpdateEditFind(CCmdUI* pCmdUI)  {
    pCmdUI->Enable ( ! GetParentFrame()->IsIconic() );
}

inline void CTVView::OnUpdateViewGotoline(CCmdUI* pCmdUI)  {
    pCmdUI->Enable ( ! GetParentFrame()->IsIconic() );
}


void CTVView::OnActivateView(BOOL bActivate, CView* pActivateView, CView* pDeactiveView)  {
    UpdateStatusBar();
    ((CTVDoc*) GetDocument())->UpdateStatusBar();
    CBlockScrollView::UpdateStatusBar();
    CBlockScrollView::OnActivateView (bActivate, pActivateView, pDeactiveView);
}

BOOL IsProgressDlgActive(void)  {
    return g_bIsProgressDlgActive;
}


// GHM 20110802 the code here is slightly modified from what used to be in OnMouseWheel
void CTVView::ChangeFontSize(bool increase)
{
    CMainFrame* pMainFrame=DYNAMIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
    int iPointSize = pMainFrame->m_iPointSize;

    iPointSize += increase ? 1 : -1;

    // min 7 points
    if (iPointSize<7) {
        iPointSize=7;
    }
    // max 32 points
    else if (iPointSize>32) {
        iPointSize=32;
    }

    LOGFONT lf;
    pMainFrame->m_Font.GetLogFont(&lf);
    CClientDC dc(this);
    lf.lfHeight = -MulDiv (iPointSize, dc.GetDeviceCaps(LOGPIXELSY), 72);
    pMainFrame->m_Font.DeleteObject();
    pMainFrame->m_Font.CreateFontIndirect(&lf);
    pMainFrame->m_iPointSize=iPointSize;
    AfxGetApp()->WriteProfileInt(_T("Options"),_T("FontSize"), iPointSize);
    ((CTextViewApp*) AfxGetApp())->UpdateAllAppViews(HINT_CHANGEFONT);
}

BOOL CTVView::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt) {

    if (GetKeyState(VK_CONTROL) < 0)  {

        ChangeFontSize(zDelta > 0); // GHM 20110802

        /*
        CMainFrame* pMainFrame=DYNAMIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
        int iPointSize = pMainFrame->m_iPointSize;
        if (zDelta > 0) {
            iPointSize--;
        }
        else {
            iPointSize++;
        }

        // min 7 points
        if (iPointSize<7) {
            iPointSize=7;
        }
        // max 32 points
        else if (iPointSize>32) {
            iPointSize=32;
        }

        LOGFONT lf;
        pMainFrame->m_Font.GetLogFont(&lf);
        CClientDC dc(this);
        lf.lfHeight = -MulDiv (iPointSize, dc.GetDeviceCaps(LOGPIXELSY), 72);
        pMainFrame->m_Font.DeleteObject();
        pMainFrame->m_Font.CreateFontIndirect(&lf);
        pMainFrame->m_iPointSize=iPointSize;
        AfxGetApp()->WriteProfileInt("Options","FontSize", iPointSize);
        ((CTextViewApp*) AfxGetApp())->UpdateAllAppViews(HINT_CHANGEFONT);
        */
    }
    else {
        if (zDelta > 0) {
            OnVScroll(SB_LINEUP, 0, NULL);
        }
        else {
            OnVScroll(SB_LINEDOWN, 0, NULL);
        }
    }
    return FALSE;
}


///////////////////////////////////////////////////////////////////////////////
//
//                       CTVView::OnRButtonUp()
//
///////////////////////////////////////////////////////////////////////////////

void CTVView::OnRButtonUp(UINT nFlags, CPoint point)
{
    BCMenu popMenu;   // BMD 29 Sep 2003
    popMenu.CreatePopupMenu();
    if (GetBlockedBufferSize() > 0) {
        popMenu.AppendMenu(MF_STRING, ID_EDIT_COPY, _T("&Copy\tCtrl+C"));
        popMenu.AppendMenu(MF_STRING, ID_EDIT_COPY_SS, _T("Copy for a &Spreadsheet"));
    }
    else {
        popMenu.AppendMenu(MF_STRING | MF_GRAYED, ID_EDIT_COPY, _T("&Copy\tCtrl+C"));
        popMenu.AppendMenu(MF_STRING | MF_GRAYED, ID_EDIT_COPY_SS, _T("Copy for a &Spreadsheet"));
    }
    popMenu.AppendMenu(MF_SEPARATOR);
    popMenu.AppendMenu(MF_STRING, ID_FILE_SAVE_AS, _T("Save &As...\tCtrl+S"));
    popMenu.AppendMenu(MF_STRING, ID_FILE_PRINT_PREVIEW, _T("Print Pre&view"));
    popMenu.AppendMenu(MF_STRING, ID_FILE_PRINT, _T("&Print...\tCtrl+P"));

    popMenu.LoadToolbar(IDR_MAINFRAME);   // BMD 29 Sep 2003

    CRect rectWin;
    GetWindowRect(rectWin);
    popMenu.TrackPopupMenu(TPM_RIGHTBUTTON, rectWin.left + point.x, rectWin.top + point.y, this);
}


///////////////////////////////////////////////////////////////////////////////
//
//    CTVView::OnUpdate()
//
//  General MFC function, used to process font size changes here
//
//  revision history:
//      4 Jan 2003     CSC       created
//
///////////////////////////////////////////////////////////////////////////////
void CTVView::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint)
{
    if (lHint==HINT_CHANGEFONT)  {
        CClientDC dc (this);

        CFont* pOldFont = dc.SelectObject(&((CMainFrame*) AfxGetMainWnd())->m_Font);

        m_iTextWidth = dc.GetTextExtent (_T("A"),1).cx;
        m_iTextHgt = dc.GetTextExtent (_T("A"),1).cy;
        m_szLineSize =  CSize (m_iTextWidth, m_iTextHgt);

        m_rulerMgr.SetText (m_iTextHgt, m_iTextWidth);
        ResizeRulers();
        UpdateRulers();
        SetScrollParameters ();

        dc.SelectObject(pOldFont);
    }
    RedrawWindow();
}

///////////////////////////////////////////////////////////////////////////////
//
//    CTVView::OnEditFindNext()
//
//  General MFC function, used to process font size changes here
//
//  revision history:
//      4 Jan 2003     CSC       created
//
///////////////////////////////////////////////////////////////////////////////
void CTVView::OnEditFindNext()
{
    CTVView::m_dlgFind.SetDirection(SEARCH_FORWARD);
    if (CTVView::m_dlgFind.GetCurrFindSel().IsEmpty())  {
        SendMessage (WM_COMMAND, ID_EDIT_FIND);
    }
    else  {
        SendMessage(UWM::TextView::Search);
    }
}

///////////////////////////////////////////////////////////////////////////////
//
//    CTVView::OnUpdateEditFindNext()
//
//  revision history:
//      4 Jan 2003     CSC       created
//
///////////////////////////////////////////////////////////////////////////////
void CTVView::OnUpdateEditFindNext(CCmdUI* pCmdUI)
{
    pCmdUI->Enable(TRUE);
}

///////////////////////////////////////////////////////////////////////////////
//
//    CTVView::OnEditFindPrev()
//
//  revision history:
//      4 Jan 2003     CSC       created
//
///////////////////////////////////////////////////////////////////////////////
void CTVView::OnEditFindPrev()
{
    CTVView::m_dlgFind.SetDirection(SEARCH_BACKWARD);
    if (CTVView::m_dlgFind.GetCurrFindSel().IsEmpty())  {
        SendMessage (WM_COMMAND, ID_EDIT_FIND);
    }
    else  {
        SendMessage(UWM::TextView::Search);
    }
}

///////////////////////////////////////////////////////////////////////////////
//
//    CTVView::OnUpdateEditFindPrev()
//
//  revision history:
//      4 Jan 2003     CSC       created
//
///////////////////////////////////////////////////////////////////////////////
void CTVView::OnUpdateEditFindPrev(CCmdUI* pCmdUI)
{
    pCmdUI->Enable(TRUE);
}


///////////////////////////////////////////////////////////////////////////////
//
//    CTVView::OnScrollBy()
//
//  Implements mouse wheel panning (drag with the mouse wheel pressed).  MFC
//  handles ON_MBUTTONDOWN, captures the mouse and handles ON_MOUSEMOVE for it,
//  and also ON_MBUTTONUP.  These eventually call OnScrollBy for the derived
//  view, which handles the actual scrolling.  This conflicts with the regular
//  scrolling mechanism, so we need to trap when it is occurring (done by the
//  2 bools).
//
//  revision history:
//      16 Jan 2005     CSC       created
//
///////////////////////////////////////////////////////////////////////////////
BOOL CTVView::OnScrollBy(CSize sizeScroll, BOOL bDoScroll) {
    bool bMouseWheelPanning=(GetKeyState(VK_MBUTTON)<0);  //=true if mbutton is pressed
    static bool bProcessingMousePanning=false;            //=true if we're in the process of sending out scroll messages

    if (bProcessingMousePanning) {
        return CBlockScrollView::OnScrollBy(sizeScroll, bDoScroll);
    }

    if (!bMouseWheelPanning) {
        // regular scrolling
        return CBlockScrollView::OnScrollBy(sizeScroll, bDoScroll);
    }
    else {
        // user is mouse wheel panning!
        bProcessingMousePanning=true;
        int iLines=0;

        // each pixel in sizeScroll represents a line, so we send a series of line up/down/left/right messages

        // scroll vertically ...
        if (sizeScroll.cy!=0) {
            bool bUp=(sizeScroll.cy<0);
            iLines=abs(sizeScroll.cy);
            for (int i=0 ; i<iLines ; i++) {
                SendMessage(WM_VSCROLL, bUp?SB_LINEUP:SB_LINEDOWN);
            }
        }

        // scroll horizontally ...
        if (sizeScroll.cx!=0) {
            bool bLeft=(sizeScroll.cx<0);
            iLines=abs(sizeScroll.cx);
            for (int i=0 ; i<iLines ; i++) {
                SendMessage(WM_HSCROLL, bLeft?SB_LINELEFT:SB_LINERIGHT);
            }
        }
        bProcessingMousePanning=false;
        return TRUE;
    }
}



// GHM 20110802 three new shortcuts
void CTVView::OnFileClose()
{
    PostMessage(WM_COMMAND,ID_FILE_CLOSE);
}

void CTVView::OnFontBigger()
{
    ChangeFontSize(true);
}

void CTVView::OnFontSmaller()
{
    ChangeFontSize(false);
}
