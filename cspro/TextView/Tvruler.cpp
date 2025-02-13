//***************************************************************************
//  File name: IVRuler.cpp
//
//  Description:
//       Implementation of CRulerView and other ruler functionality
//
//  History:    Date       Author     Comment
//              -----------------------------
//              1995        csc       created
//              13 Jan 98   bmd       Fixed bug in over use of CFont::Create
//              12 Feb 98   bmd       Make characters smaller on screen
//              4  Jan 03   csc       Support for font size changes
//              4  Jan 03   csc       Add left-click support (sel current line)
//
//***************************************************************************

#include "StdAfx.h"
#include <math.h>

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif
static CLPoint ptlOffset;        // relative pixel coordinates, global for speed

/////////////////////////////////////////////////////////////////////////////
// CRulerView

IMPLEMENT_DYNCREATE(CRulerView, CView)


/******************************************************************************
*
*
*                                      CRulerView::CRulerView()
*
*   Function:       CRulerView constructor
*
*   Prototype:      CRulerView::CRulerView (void)
*
*   Scope:          protected
*
*   To call:        CRulerView x
*
*   Return codes:   n/a
*
*   Notes:
*
*
*******************************************************************************
*   Revision history:
*
*
******************************************************************************/
CRulerView::CRulerView()  {
    m_pRulerMgr = NULL;
}


void CRulerView::OnInitialUpdate()
{
    CClientDC dc (this);
    CFont* pOldFont = dc.SelectObject(&((CMainFrame*) AfxGetMainWnd())->m_Font);
    m_iTextWidth = dc.GetTextExtent (_T("A"),1).cx;
    m_iTextHgt = dc.GetTextExtent (_T("A"),1).cy;
    dc.SelectObject(pOldFont);
}


/******************************************************************************
*
*
*                                      CRulerView::~CRulerView()
*
*   Function:       CRulerView destructor
*
*   Prototype:      CRulerView::~CRulerView (void)
*
*   Scope:          public
*
*   To call:        n/a
*
*   Return codes:   n/a
*
*   Notes:
*
*
*******************************************************************************
*   Revision history:
*
*
******************************************************************************/
CRulerView::~CRulerView()  {
    // stub
}


BEGIN_MESSAGE_MAP(CRulerView, CView)
    //{{AFX_MSG_MAP(CRulerView)
    ON_WM_SETFOCUS()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CRulerView drawing

/******************************************************************************
*
*
*                                      CRulerView::OnPrepareDC
*
*   Function:       CRulerView device context initialization
*
*   Prototype:      void CRulerView::OnPrepareDC (CDC*, CPrintInfo*)
*
*   Scope:          protected
*
*   To call:        n/a -- called from within MFC
*
*   Return codes:   n/a
*
*   Notes:
*
*
*******************************************************************************
*   Revision history:
*
*
******************************************************************************/
void CRulerView::OnPrepareDC (CDC *pDC, CPrintInfo *pInfo)  {

    CView::OnPrepareDC(pDC, pInfo);
    pDC->SelectObject(&((CMainFrame*) AfxGetMainWnd())->m_Font);
//    ASSERT (((CTextViewApp*)AfxGetApp())->m_iNumColors >= 16);
    pDC->SetTextColor (RULER_FCOLOR);         // RGB(255,255,255));
    pDC->SetBkColor (RULER_BCOLOR);
}

/******************************************************************************
*
*
*                                      CRulerView::OnDraw
*
*   Function:       CRulerView screen drawing function
*
*   Prototype:      void CRulerView::OnDraw (CDC*)
*
*   Scope:          protected
*
*   To call:        n/a -- called from within MFC
*
*   Return codes:   n/a
*
*   Notes:
*
*
*******************************************************************************
*   Revision history:
*
*
******************************************************************************/
void CRulerView::OnDraw(CDC* /* pDC  */)  {
    // stub
}


/******************************************************************************
*
*
*                                      CRulerView::OnSetFocus
*
*   Function:       Called after the CRulerView object gains the focus.
*
*   Prototype:      void CRulerView::OnSetFocus (CWnd*)
*
*   Scope:          protected
*
*   To call:        n/a -- called from within MFC
*
*   Return codes:   n/a
*
*   Notes:
*
*
*******************************************************************************
*   Revision history:
*
*
******************************************************************************/
void CRulerView::OnSetFocus(CWnd* /*pOldWnd*/ )  {

    POSITION pos = ((CTVDoc*) GetDocument())->GetFirstViewPosition();     // the first view position is the "main" view -- the one created first!
    CView* pMainView = ((CTVDoc*) GetDocument())->GetNextView(pos);
    ASSERT(pMainView->IsKindOf(RUNTIME_CLASS(CTVView)));
    GetParentFrame()->SetActiveView (pMainView);
    pMainView->SetFocus ();
}


/////////////////////////////////////////////////////////////////////////////
// CHRulerView

IMPLEMENT_DYNCREATE(CHRulerView, CView)

/******************************************************************************
*
*
*                                      CHRulerView::CHRulerView
*
*   Function:       CHRulerView constructor
*
*   Prototype:      CHRulerView::CHRulerView(void)
*
*   Scope:          public
*
*   To call:        CHRulerView* pHRuler = new (CHRulerView)
*
*   Return codes:   n/a
*
*   Notes:
*
*
*******************************************************************************
*   Revision history:
*
*
******************************************************************************/

CHRulerView::CHRulerView()
{
    m_bBlockActive = NO;
    m_rclOldBlock = CLRect (0L,0L,0L,0L);
    m_bCaptured = FALSE;
    m_iTimer = NONE;                            // Inactive
}

/******************************************************************************
*
*
*                                      CHRulerView::~CHRulerView
*
*   Function:       CHRulerView destructor
*
*   Prototype:      CHRulerView::CHRulerView()
*
*   Scope:          public
*
*   To call:        n/a -- called automatically
*
*   Return codes:   n/a
*
*   Notes:
*
*
*******************************************************************************
*   Revision history:
*
*
******************************************************************************/
CHRulerView::~CHRulerView()  {
    // stub
}

BEGIN_MESSAGE_MAP(CHRulerView, CRulerView)
    //{{AFX_MSG_MAP(CHRulerView)
    ON_WM_LBUTTONUP()
    ON_WM_LBUTTONDOWN()
    ON_WM_MOUSEMOVE()
    ON_WM_TIMER()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CHRulerView drawing

/******************************************************************************
*
*
*                                      CHRulerView::OnDraw
*
*   Function:       CHRulerView screen drawing function
*
*   Prototype:      void CHRulerView::OnDraw (CDC*)
*
*   Scope:          protected
*
*   To call:        n/a -- called automatically by MFC
*
*   Return codes:   n/a
*
*   Notes:
*
*
*******************************************************************************
*   Revision history:
*
*
******************************************************************************/
void CHRulerView::OnDraw(CDC* pDC)  {

    ASSERT (m_pRulerMgr);
    BuildRuler ((int) m_pRulerMgr->GetCurrPos().x);
    pDC->TextOut ( 0, 0, m_csRuler );
}


/******************************************************************************
*
*
*                                      CHRulerView::BuildRuler
*
*   Function:       Determines and constructs the horizontal ruler contents
*
*   Prototype:      void CHRulerView::BuildRuler (int)
*
*   Scope:          private
*
*   To call:        BuildRuler ((int) m_ptlCurrPos.x)
*
*   Return codes:   n/a
*
*   Notes:
*
*
*******************************************************************************
*   Revision history:
*
*
******************************************************************************/
void CHRulerView::BuildRuler (int iCurrXPos)  {
    int i;
    int iColSpacing, iWindowWidth, iStart;
    TCHAR pszTemp [11];
    CRect rcCurrClient;

//    ASSERT (iCurrXPos >= 0);
    GetClientRect ( &rcCurrClient );
    iWindowWidth = rcCurrClient.Size().cx / m_iTextWidth;
    if ( iWindowWidth + iCurrXPos > 1000 )  {
        iColSpacing = 10;
    }
    else  {
        iColSpacing = 5;
    }
    pszTemp [iColSpacing] = EOS;
    m_csRuler.Empty ();
    iStart = iCurrXPos - (iCurrXPos % iColSpacing);
    for ( i = 0 ; i < iWindowWidth +iColSpacing ; i+= iColSpacing )  {
        itoc ( pszTemp, i+iStart+iColSpacing, iColSpacing, '.');
        m_csRuler += pszTemp;
    }
    m_csRuler += _T("..........");
    m_csRuler = m_csRuler.Right ( m_csRuler.GetLength() - (iCurrXPos % iColSpacing));
}


/******************************************************************************
*
*
*                                      CHRulerView::UpdateRulerPosition
*
*   Function:       Scrolls the ruler window in response to horizontal movement
*                   within the file being viewed, and calls UpdateWindow() to
*                   cause the ruler to be redrawn.
*
*   Prototype:      void CHRulerView::UpdateRulerPosition (int, int)
*
*   Scope:          public
*
*   To call:        m_pHRulerView->UpdateRulerPosition (iX, (int) ptlPrevPos.x);
*
*   Return codes:   n/a
*
*   Notes:
*
*
*******************************************************************************
*   Revision history:
*
*
******************************************************************************/
void CHRulerView::UpdateRulerPosition (int iNewX, int iPrevX )  {
    ASSERT (iNewX >= 0);
    ASSERT (iPrevX >= 0 || iPrevX == NONE);
    if ( iNewX != iPrevX )  {
        // ruler needs to be shifted, or completely redrawn ...
            if ( (long) (iPrevX - iNewX) * (long) m_iTextWidth >= 32767L )  {
                Invalidate();
            }
            else  {
                ScrollWindow ( (iPrevX - iNewX) * m_iTextWidth , 0);
            }
        UpdateWindow();
    }
}


/******************************************************************************
*
*
*                                      CHRulerView::Resize
*
*   Function:       Called to signal the horizontal ruler that it needs to
*                   resize itself.  This happens when the rulers are
*                   activated/deactivated, or when the vertical ruler needs
*                   to be made wider or narrower, or when the user resizes
*                   the CTVView window. within the file being viewed,
*                   and calls UpdateWindow() to cause the ruler to be
*                   redrawn.
*
*   Prototype:      void CHRulerView::Resize (CRect, int)
*
*   Scope:          public
*
*   To call:        m_pHRulerView->Resize (rcPageSize, GetOffset(rcPageSize.Size().cy));
*
*   Return codes:   n/a
*
*   Notes:
*
*
*******************************************************************************
*   Revision history:
*
*
******************************************************************************/
void CHRulerView::Resize (CRect rcPageSize, int iOffset)  {   //long lCurrYPos)  {
    WINDOWPLACEMENT wndpl;

    wndpl.length = sizeof (WINDOWPLACEMENT);
    VERIFY (GetWindowPlacement (&wndpl));
    wndpl.rcNormalPosition = rcPageSize;
    wndpl.rcNormalPosition.bottom = m_iTextHgt;
    wndpl.rcNormalPosition.left += iOffset + 1;
    wndpl.rcNormalPosition.right += iOffset + 1;
    VERIFY (SetWindowPlacement (&wndpl));
}


/////////////////////////////////////////////////////////////////////////////
// CVRulerView

IMPLEMENT_DYNCREATE(CVRulerView, CView)

/******************************************************************************
*
*
*                                      CVRulerView::CVRulerView()
*
*   Function:       CVRulerView constructor
*
*   Prototype:      CVRulerView::CVRulerView (void)
*
*   Scope:          public
*
*   To call:        CVRulerView* pVRuler = new (CVRulerView)
*
*   Return codes:   n/a
*
*   Notes:
*
*
*******************************************************************************
*   Revision history:
*
*
******************************************************************************/
CVRulerView::CVRulerView()
{
    m_bBlockActive = NO;
    m_rclOldBlock = CLRect (0L,0L,0L,0L);
    m_bCaptured = FALSE;
    m_iTimer = NONE;                            // Inactive
}

/******************************************************************************
*
*
*                                      CVRulerView::~CVRulerView
*
*   Function:       CVRulerView destructor
*
*   Prototype:      CVRulerView::CVRulerView()
*
*   Scope:          public
*
*   To call:        n/a -- called automatically
*
*   Return codes:   n/a
*
*   Notes:
*
*
*******************************************************************************
*   Revision history:
*
*
******************************************************************************/
CVRulerView::~CVRulerView()  {
    // stub
}

BEGIN_MESSAGE_MAP(CVRulerView, CRulerView)
    //{{AFX_MSG_MAP(CVRulerView)
    ON_WM_LBUTTONUP()
    ON_WM_LBUTTONDOWN()
    ON_WM_MOUSEMOVE()
    ON_WM_TIMER()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/******************************************************************************
*
*
*                                      CVRulerView::BuildRuler
*
*   Function:       Determines and constructs the vertical ruler contents
*
*   Prototype:      void CVRulerView::BuildRuler (long)
*
*   Scope:          private
*
*   To call:        BuildRuler (m_ptlCurrPos.y)
*
*   Return codes:   n/a
*
*   Notes:
*
*
*******************************************************************************
*   Revision history:
*
*
******************************************************************************/
void CVRulerView::BuildRuler (long lCurrYPos)  {
    int i;
    int iWidth, iNumLines;
    TCHAR pszTemp [11];
    CRect rcCurrClient;

//    ASSERT (lCurrYPos >= 0L);
    GetClientRect ( &rcCurrClient );
    iNumLines = rcCurrClient.Size().cy / m_iTextHgt + 2;
    iWidth = rcCurrClient.Size().cx / m_iTextWidth;

    m_csRuler.Empty ();
    pszTemp[iWidth] = EOS;
    for ( i = 0 ; i < iNumLines ; i++ )  {
        ltoc ( pszTemp, (long) i + lCurrYPos + 1, iWidth, ' ');
        m_csRuler += pszTemp;
    }
}

/******************************************************************************
*
*
*                                      CVRulerView::UpdateRulerPosition
*
*   Function:       Scrolls the ruler window in response to vertical movement
*                   within the file being viewed, and calls UpdateWindow() to
*                   cause the ruler to be redrawn.
*
*   Prototype:      void CVRulerView::UpdateRulerPosition (long, long)
*
*   Scope:          public
*
*   To call:        m_pVRulerView->UpdateRulerPosition (lY, ptlPrevPos.y);
*
*   Return codes:   n/a
*
*   Notes:
*
*
*******************************************************************************
*   Revision history:
*
*
******************************************************************************/
void CVRulerView::UpdateRulerPosition (long lNewY, long lPrevY)  {
    ASSERT (lNewY >= 0L);
    ASSERT (lPrevY >= 0L || lPrevY == (long) NONE);
    if ( lPrevY != lNewY )  {
        // ruler has moved, need to update ...
        if (  (long) m_iTextHgt * labs (lPrevY-lNewY) >= 32767L )  {
            // int calculations overflow, just redraw the whole thing
            Invalidate();
        }
        else  {
            // just move the ruler a bit (or more, depends on the movement operation)
            ASSERT ( (long) m_iTextHgt * (lPrevY - lNewY) <= 32767L);
            ASSERT ( (long) m_iTextHgt * (lPrevY - lNewY) >= -32767L);
            ScrollWindow ( 0, (int) (lPrevY - lNewY) * m_iTextHgt);
        }
        UpdateWindow();
    }
}

/******************************************************************************
*
*
*                                      CVRulerView::OnDraw
*
*   Function:       CVRulerView screen drawing function
*
*   Prototype:      void CVRulerView::OnDraw (CDC*)
*
*   Scope:          protected
*
*   To call:        n/a -- called automatically by MFC
*
*   Return codes:   n/a
*
*   Notes:
*
*
*******************************************************************************
*   Revision history:
*
*
******************************************************************************/
void CVRulerView::OnDraw(CDC* pDC)  {
    CRect rcCurrClient;
    int iNumLines, iWidth;

    ASSERT (m_pRulerMgr);
    BuildRuler (m_pRulerMgr->GetCurrPos().y);
    GetClientRect ( &rcCurrClient );
    iNumLines = rcCurrClient.Size().cy / m_iTextHgt + 2;
    iWidth = rcCurrClient.Size().cx / m_iTextWidth;
    for ( int i = 0 ; i < iNumLines ; i++ )  {
        pDC->TextOut ( 0, i*m_iTextHgt, m_csRuler.Mid (i*iWidth, iWidth), iWidth );
    }
}

/******************************************************************************
*
*
*                                      CVRulerView::Resize
*
*   Function:       Called to signal the vertical ruler that it needs to
*                   resize itself.  This happens when the rulers are
*                   activated/deactivated, or when the vertical ruler needs
*                   to be made wider or narrower, or when the user resizes
*                   the CTVView window.
*
*   Prototype:      void CVRulerView::Resize (CRect, int)
*
*   Scope:          public
*
*   To call:        m_pVRulerView->Resize (rcPageSize, GetOffset (rcPageSize.Size().cy));
*
*   Return codes:   n/a
*
*   Notes:
*
*
*******************************************************************************
*   Revision history:
*
*
******************************************************************************/
void CVRulerView::Resize (CRect rcPageSize, int iOffset)  {
    WINDOWPLACEMENT wndpl;

    wndpl.length = sizeof (WINDOWPLACEMENT);
    GetWindowPlacement(&wndpl);
    wndpl.rcNormalPosition = rcPageSize;
    wndpl.rcNormalPosition.right = iOffset;
    wndpl.rcNormalPosition.top += m_iTextHgt + 1;
    wndpl.rcNormalPosition.bottom += m_iTextHgt + 1;
    SetWindowPlacement (&wndpl);
}


/////////////////////////////////////////////////////////////////////////////
// CRulerFillerView

IMPLEMENT_DYNCREATE(CRulerFillerView, CView)

/******************************************************************************
*
*
*                                      CRulerFillerView::CRulerFillerView()
*
*   Function:       CRulerFillerView constructor
*
*   Prototype:      CRulerFillerView::CRulerFillerView (void)
*
*   Scope:          public
*
*   To call:        CRulerFillerView* pRulerFillerView = new (CRulerFillerView)
*
*   Return codes:   n/a
*
*   Notes:
*
*
*******************************************************************************
*   Revision history:
*
*
******************************************************************************/
CRulerFillerView::CRulerFillerView()  {
    // stub
}

/******************************************************************************
*
*
*                                      CRulerFillerView::~CRulerFillerView
*
*   Function:       CRulerFillerView destructor
*
*   Prototype:      CRulerFillerView::CRulerFillerView()
*
*   Scope:          public
*
*   To call:        n/a -- called automatically
*
*   Return codes:   n/a
*
*   Notes:
*
*
*******************************************************************************
*   Revision history:
*
*
******************************************************************************/
CRulerFillerView::~CRulerFillerView()  {
    // stub
}

BEGIN_MESSAGE_MAP(CRulerFillerView, CView)
    //{{AFX_MSG_MAP(CRulerFillerView)
    ON_WM_SETFOCUS()
    ON_WM_ERASEBKGND()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/******************************************************************************
*
*
*                                      CRulerFillerView::OnDraw
*
*   Function:       CRulerFillerView screen drawing function
*
*   Prototype:      void CRulerFillerView::OnDraw (CDC*)
*
*   Scope:          protected
*
*   To call:        n/a -- called automatically by MFC
*
*   Return codes:   n/a
*
*   Notes:
*
*
*******************************************************************************
*   Revision history:
*
*
******************************************************************************/
void CRulerFillerView::OnDraw(CDC* /*pDC*/)  {
    // stub
}

/******************************************************************************
*
*
*                                      CRulerFillerView::OnEraseBkgnd
*
*   Function:       CRulerFillerView screen drawing function; shades the
*                   filler view area using the ruler color.
*
*   Prototype:      void CRulerFillerView::OnEraseBkgnd (CDC*)
*
*   Scope:          protected
*
*   To call:        n/a -- called automatically by MFC
*
*   Return codes:   n/a
*
*   Notes:
*
*
*******************************************************************************
*   Revision history:
*
*
******************************************************************************/
BOOL CRulerFillerView::OnEraseBkgnd(CDC* pDC)  {
//    ASSERT (((CTextViewApp*)AfxGetApp())->m_iNumColors >= 16);
    CBrush backBrush(RULER_BCOLOR);
    CBrush* pOldBrush = pDC->SelectObject(&backBrush);
    CRect rect;
    GetClientRect (&rect);
    pDC->PatBlt(rect.left, rect.top, rect.Width(), rect.Height(), PATCOPY);
    pDC->SelectObject(pOldBrush);
    return TRUE;
}

/******************************************************************************
*
*
*                                      CRulerFillerView::Resize
*
*   Function:       Called to signal the filler ruler that it needs to
*                   resize itself.  This happens when the rulers are
*                   activated/deactivated, when the vertical ruler needs to
*                   be made wider or narrower, or when the user resizes the
*                   CTVView window.
*
*   Prototype:      void CRulerFillerView::Resize (CRect)
*
*   Scope:          public
*
*   To call:        m_pRulerFillerView->Resize (rcPageSize);
*
*   Return codes:   n/a
*
*   Notes:
*
*
*******************************************************************************
*   Revision history:
*
*
******************************************************************************/
void CRulerFillerView::Resize (CRect rcNewSize)  {
    WINDOWPLACEMENT wndpl;

    wndpl.length = sizeof (WINDOWPLACEMENT);
    VERIFY (GetWindowPlacement (&wndpl));
    wndpl.rcNormalPosition = rcNewSize;
    VERIFY (SetWindowPlacement (&wndpl));
    UpdateWindow();
}


/******************************************************************************
*
*
*                                      CRulerFillerView::OnSetFocus
*
*   Function:       Called after the CRulerFillerView object gains the focus.
*
*   Prototype:      void CRulerFillerView::OnSetFocus (CWnd*)
*
*   Scope:          protected
*
*   To call:        n/a -- called from within MFC
*
*   Return codes:   n/a
*
*   Notes:
*
*
*******************************************************************************
*   Revision history:
*
*
******************************************************************************/
void CRulerFillerView::OnSetFocus(CWnd* /*pOldWnd*/ )  {
    POSITION pos = ((CTVDoc*) GetDocument())->GetFirstViewPosition();     // the first view position is the "main" view -- the one created first!
    CView* pMainView = ((CTVDoc*) GetDocument())->GetNextView(pos);
    ASSERT(pMainView->IsKindOf(RUNTIME_CLASS(CTVView)));
    GetParentFrame()->SetActiveView (pMainView);
    pMainView->SetFocus ();
}

/////////////////////////////////////////////////////////////////////////////
// CRulerViewMgr

/******************************************************************************
*
*
*                                      CRulerViewMgr::CRulerViewMgr
*
*   Function:       CRulerViewMgr constructor
*
*   Prototype:      CRulerViewMgr::CRulerViewMgr (void)
*
*   Scope:          public
*
*   To call:        CRulerViewMgr rulerMgr;
*
*   Return codes:   n/a
*
*   Notes:
*
*
*******************************************************************************
*   Revision history:
*
*
******************************************************************************/
CRulerViewMgr::CRulerViewMgr (void)  {
    m_ptlCurrPos = CLPoint ((long) NONE, (long) NONE);   // NONE's will cause the rulers to be updated the first time they are activated
}


/******************************************************************************
*
*
*                                      CRulerViewMgr::Init
*
*   Function:       Creates and intializes the five ruler components owned
*                   by the ruler manager.
*
*   Prototype:      void CRulerViewMgr::Init (CDocument*, CFrameWnd*)
*
*   Scope:          public
*
*   To call:        rulerMgr.Init (pDoc, pFrame)
*             (in)  pDoc    - document associated with the view that creates the ruler manager
*             (in)  pFrame  - frame window associated with the view that creates the ruler manager
*
*   Return codes:   n/a
*
*   Notes:
*
*
*******************************************************************************
*   Revision history:
*
*
******************************************************************************/
void CRulerViewMgr::Init (CDocument* pDoc, CFrameWnd* pFrame)  {
    m_pHRulerView = new CHRulerView ();
    m_pVRulerView = new CVRulerView ();
    m_pRulerFillerView[0] = new CRulerFillerView;
    m_pRulerFillerView[1] = new CRulerFillerView;
    m_pRulerFillerView[2] = new CRulerFillerView;

    VERIFY (m_pHRulerView);
    VERIFY (m_pVRulerView);
    VERIFY (m_pRulerFillerView[0]);
    VERIFY (m_pRulerFillerView[1]);
    VERIFY (m_pRulerFillerView[2]);

    m_pHRulerView->SetCurrRulerMgr (this);
    m_pVRulerView->SetCurrRulerMgr (this);

    CCreateContext newContext;
    newContext.m_pNewViewClass = NULL;
    newContext.m_pNewDocTemplate = NULL;
    newContext.m_pLastView = NULL;
    newContext.m_pCurrentFrame = NULL;
    newContext.m_pCurrentDoc = pDoc;

    VERIFY ( m_pHRulerView->Create (NULL, _T("foo"), WS_CHILD, CRect (0,0,0,0), pFrame, AFX_IDW_PANE_FIRST+1, &newContext) );
    VERIFY ( m_pVRulerView->Create (NULL, _T("foo"), WS_CHILD, CRect (0,0,0,0), pFrame, AFX_IDW_PANE_FIRST+2, &newContext) );
    VERIFY ( m_pRulerFillerView[0]->Create (NULL, _T("foo"), WS_CHILD, CRect (0,0,0,0), pFrame, AFX_IDW_PANE_FIRST+3, &newContext) );
    VERIFY ( m_pRulerFillerView[1]->Create (NULL, _T("foo"), WS_CHILD, CRect (0,0,0,0), pFrame, AFX_IDW_PANE_FIRST+4, &newContext) );
    VERIFY ( m_pRulerFillerView[2]->Create (NULL, _T("foo"), WS_CHILD, CRect (0,0,0,0), pFrame, AFX_IDW_PANE_FIRST+5, &newContext) );
}

/******************************************************************************
*
*
*                                      CRulerViewMgr::IsOffsetChanged
*
*   Function:       Signals whether or not the vertical ruler bar's width
*                   needs to change (for example, when scrolling from line
*                   9999 to line 10000).
*
*   Prototype:      void CRulerViewMgr::Init (CDocument*, CFrameWnd*)
*
*   Scope:          public
*
*   To call:        bChanged = rulerMgr.IsOffsetChanged (iScrHgt)
*             (in)  iScrHgt   - number of lines displable in the main view
*                               window
*             (out) bChanged  - true if the width of the vertical ruler bar
*                               needs to be changed
*
*   Return codes:   n/a
*
*   Notes:
*
*
*******************************************************************************
*   Revision history:
*
*
******************************************************************************/
BOOL CRulerViewMgr::IsOffsetChanged (int iScrHgt)  {   //long lNewY)  {
    CRect rcCurr;
    m_pVRulerView->GetClientRect (&rcCurr);
    //SAVY 05/03/2004 vc7 compatibility
    return ( rcCurr.Size().cx/m_iTextWidth != std::max( DEFAULT_RULER_WIDTH, (int) log10 ((long double) iScrHgt + m_ptlCurrPos.y)+1) );
}


/******************************************************************************
*
*
*                                      CRulerViewMgr::Resize
*
*   Function:       Causes the five member rulers to resize themselves.
*
*   Prototype:      void CRulerViewMgr::Resize (int, CRect = CRect(0,0,0,0))
*
*   Scope:          public
*
*   To call:        rulerMgr.Resize (iShowCmd, rcPageSize)
*             (in)  iShowCmd    - SW_SHOW if rulers should be displayed (are active)
*                                 SW_HIDE if the rulers are hidden (not active)
*             (in)  rcPageSize  - CRect object representing the main view window's
*                                 client rectangle size
*
*   Return codes:   n/a
*
*   Notes:
*
*
*******************************************************************************
*   Revision history:
*
*
******************************************************************************/
void CRulerViewMgr::Resize (int iShowCmd, CRect rcPageSize /*=CRect(0,0,0,0)*/ )  {
    ASSERT (iShowCmd==SW_SHOW || iShowCmd==SW_HIDE);
    if (iShowCmd == SW_SHOW)  {
        m_pHRulerView->Resize(rcPageSize, GetOffset(rcPageSize.Size().cy));
        m_pVRulerView->Resize(rcPageSize, GetOffset(rcPageSize.Size().cy));
        m_pRulerFillerView[0]->Resize(CRect(0,0, GetOffset(rcPageSize.Size().cy)+1, m_iTextHgt+1));
        m_pRulerFillerView[1]->Resize(CRect(rcPageSize.right+GetOffset(rcPageSize.Size().cy), 0, rcPageSize.right+GetSystemMetrics (SM_CXVSCROLL)+GetOffset (rcPageSize.Size().cy)+1, m_iTextHgt+2));
        m_pRulerFillerView[2]->Resize(CRect(0, rcPageSize.bottom+m_iTextHgt+1, GetOffset (rcPageSize.Size().cy)+1, rcPageSize.bottom+GetSystemMetrics (SM_CYHSCROLL)+m_iTextHgt+3));
    }
    else  {
        m_pHRulerView->Resize(rcPageSize, GetOffset(rcPageSize.Size().cy));
        m_pVRulerView->Resize(rcPageSize, GetOffset(rcPageSize.Size().cy));
        m_pRulerFillerView[0]->Resize(rcPageSize);
        m_pRulerFillerView[1]->Resize(rcPageSize);
        m_pRulerFillerView[2]->Resize(rcPageSize);
    }
}

/******************************************************************************
*
*
*                                      CRulerViewMgr::GetOffset
*
*   Function:       Gets the width of the vertical ruler bar, in pixels
*
*   Prototype:      int CRulerViewMgr::GetOffset (int)
*
*   Scope:          public
*
*   To call:        iOffset = rulerMgr.GetOffset (iYSize)
*             (in)  iYSize   - height of the main view window, in pixels;
*                              (usually accomplished via   rcPageSize().Size().cy  )
*             (out) iOffset  - offset size (width), in pixels
*
*   Return codes:   n/a
*
*   Notes:
*
*
*******************************************************************************
*   Revision history:
*
*
******************************************************************************/
int CRulerViewMgr::GetOffset (int iY)  {
    //SAVY 05/03/2004 vc7 compatibility
    return std::max ( DEFAULT_RULER_WIDTH, 1+((int) log10((long double) m_ptlCurrPos.y + (long) ( iY / m_iTextHgt + 2 )) )) * m_iTextWidth;
}


/******************************************************************************
*
*
*                                      CRulerViewMgr::UpdateRulerPosition
*
*   Function:       Updates the current position (ptlCurrPos) and signals
*                   the horizontal and vertical rulers to scroll and redraw
*                   themselves.
*
*   Prototype:      void CRulerViewMgr::UpdateRulerPosition (int, long)
*
*   Scope:          public
*
*   To call:        rulerMgr.UpdateRulerPosition (iCurrCol, lCurrLine)
*             (in)  iCurrCol   - current column number of the file being viewed
*             (in)  lCurrLine  - current line number of the file being viewed
*
*   Return codes:   n/a
*
*   Notes:
*
*
*******************************************************************************
*   Revision history:
*
*
******************************************************************************/
void CRulerViewMgr::UpdateRulerPosition (int iX, long lY)  {
    CLPoint ptlPrevPos = m_ptlCurrPos;
    m_ptlCurrPos.x = (long)iX;
    m_ptlCurrPos.y = lY;
    m_pHRulerView->UpdateRulerPosition (iX, (int) ptlPrevPos.x);
    m_pVRulerView->UpdateRulerPosition (lY, ptlPrevPos.y);
}


///////////////////////////////////////////////////////////////////////////////
//
//                      CHRulerView::OnLButtonDown()
//
///////////////////////////////////////////////////////////////////////////////

void CHRulerView::OnLButtonDown(UINT nFlags, CPoint point)
{
//  Begin selecting entire columns   // BMD 24 Mar 2003
    if (IsProgressDlgActive())  {
        // yikes!  the user is searching or doing something that makes our state transient;
        // buffers aren't lined up correctly, so we can't render ourselves right now
        return;
    }
    SetCapture ();
    m_bCaptured = TRUE;
    ASSERT ( m_iTimer == NONE );
    ASSERT_KINDOF(CMDIChildWnd, ((CMDIFrameWnd*) AfxGetMainWnd())->MDIGetActive());
    CBlockScrollView* pView = (CBlockScrollView*) ((CMDIFrameWnd*) AfxGetMainWnd())->MDIGetActive()->GetActiveView();
    ASSERT_VALID(pView);
    pView->OnSelLine(point, HRFIRST);
    CRulerView::OnLButtonDown(nFlags, point);
}


///////////////////////////////////////////////////////////////////////////////
//
//                      CHRulerView::OnMouseMove()
//
///////////////////////////////////////////////////////////////////////////////

void CHRulerView::OnMouseMove(UINT nFlags, CPoint point)
{
//  Keep selecting entire columns   // BMD 24 Mar 2003
    if ((nFlags & MK_LBUTTON) && GetCapture() == this)  {
        // Update block
        ASSERT_KINDOF(CMDIChildWnd, ((CMDIFrameWnd*) AfxGetMainWnd())->MDIGetActive());
        CBlockScrollView* pView = (CBlockScrollView*) ((CMDIFrameWnd*) AfxGetMainWnd())->MDIGetActive()->GetActiveView();
        ASSERT_VALID(pView);
        pView->OnSelLine(point, HRNEXT);
        CRect rcClient;

        // Scroll
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
                CBufferMgr* pBuffMgr = ((CTVDoc *) GetDocument())->GetBufferMgr();
                ptlOffset = CLPoint ( (long) pBuffMgr->GetCurrCol() * m_iTextWidth, pBuffMgr->GetCurrLine() * m_iTextHgt);
                ASSERT (m_iTimer==NONE);
                m_iTimer = SetTimer (SCROLL_TIMER, uScrollSpeed*3, NULL);
            }
        }
    }
    CRulerView::OnMouseMove(nFlags, point);
}


///////////////////////////////////////////////////////////////////////////////
//
//                      CHRulerView::OnLButtonUp()
//
///////////////////////////////////////////////////////////////////////////////

void CHRulerView::OnLButtonUp(UINT nFlags, CPoint point)
{
//  Finish selecting entire columns   // BMD 24 Mar 2003
    if (GetCapture() == this)  {
        ReleaseCapture ();
        m_bCaptured = FALSE;
        ASSERT_KINDOF(CMDIChildWnd, ((CMDIFrameWnd*) AfxGetMainWnd())->MDIGetActive());
        CBlockScrollView* pView = (CBlockScrollView*) ((CMDIFrameWnd*) AfxGetMainWnd())->MDIGetActive()->GetActiveView();
        ASSERT_VALID(pView);
        pView->OnSelLine(point, HRNEXT);
    }
    if ( m_iTimer != NONE )  {
        VERIFY ( KillTimer ( m_iTimer ) );
        m_iTimer = NONE;
    }
    CRulerView::OnLButtonUp(nFlags, point);
}


///////////////////////////////////////////////////////////////////////////////
//
//                      CVRulerView::OnLButtonDown()
//
///////////////////////////////////////////////////////////////////////////////

void CVRulerView::OnLButtonDown(UINT nFlags, CPoint point)
{
//  Begin selecting entire rows   // BMD 24 Mar 2003
    if (IsProgressDlgActive())  {
        // yikes!  the user is searching or doing something that makes our state transient;
        // buffers aren't lined up correctly, so we can't render ourselves right now
        return;
    }
    SetCapture ();
    m_bCaptured = TRUE;
    ASSERT ( m_iTimer == NONE );
    ASSERT_KINDOF(CMDIChildWnd, ((CMDIFrameWnd*) AfxGetMainWnd())->MDIGetActive());
    CBlockScrollView* pView = (CBlockScrollView*) ((CMDIFrameWnd*) AfxGetMainWnd())->MDIGetActive()->GetActiveView();
    ASSERT_VALID(pView);
    pView->OnSelLine(point, VRFIRST);
    CRulerView::OnLButtonDown(nFlags, point);
}


///////////////////////////////////////////////////////////////////////////////
//
//                      CVRulerView::OnMouseMove()
//
///////////////////////////////////////////////////////////////////////////////

void CVRulerView::OnMouseMove(UINT nFlags, CPoint point)
{
//  Keep selecting entire rows   // BMD 24 Mar 2003
    if ((nFlags & MK_LBUTTON) && GetCapture() == this) {
        // Update block
        ASSERT_KINDOF(CMDIChildWnd, ((CMDIFrameWnd*) AfxGetMainWnd())->MDIGetActive());
        CBlockScrollView* pView = (CBlockScrollView*) ((CMDIFrameWnd*) AfxGetMainWnd())->MDIGetActive()->GetActiveView();
        ASSERT_VALID(pView);
        pView->OnSelLine(point, VRNEXT);
        CRect rcClient;

        // Scroll
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
                CBufferMgr* pBuffMgr = ((CTVDoc *) GetDocument())->GetBufferMgr();
                ptlOffset = CLPoint ( (long) pBuffMgr->GetCurrCol() * m_iTextWidth, pBuffMgr->GetCurrLine() * m_iTextHgt);
                ASSERT (m_iTimer==NONE);
                m_iTimer = SetTimer (SCROLL_TIMER, uScrollSpeed*3, NULL);
            }
        }
    }
    CRulerView::OnMouseMove(nFlags, point);
}


///////////////////////////////////////////////////////////////////////////////
//
//                     CVRulerView::OnLButtonUp()
//
///////////////////////////////////////////////////////////////////////////////

void CVRulerView::OnLButtonUp(UINT nFlags, CPoint point)
{
//  Finish selecting entire rows   // BMD 24 Mar 2003
    if (GetCapture() == this)  {
        ReleaseCapture ();
        m_bCaptured = FALSE;
        ASSERT_KINDOF(CMDIChildWnd, ((CMDIFrameWnd*) AfxGetMainWnd())->MDIGetActive());
        CBlockScrollView* pView = (CBlockScrollView*) ((CMDIFrameWnd*) AfxGetMainWnd())->MDIGetActive()->GetActiveView();
        ASSERT_VALID(pView);
        pView->OnSelLine(point, VRNEXT);
    }
    if ( m_iTimer != NONE )  {
        VERIFY ( KillTimer ( m_iTimer ) );
        m_iTimer = NONE;
    }
    CRulerView::OnLButtonUp(nFlags, point);
}

void CVRulerView::OnTimer(UINT nIDEvent)
{
    ASSERT ( m_iTimer != NONE );
    ASSERT_KINDOF(CMDIChildWnd, ((CMDIFrameWnd*) AfxGetMainWnd())->MDIGetActive());
    CBlockScrollView* pView = (CBlockScrollView*) ((CMDIFrameWnd*) AfxGetMainWnd())->MDIGetActive()->GetActiveView();
    ASSERT_VALID(pView);
    CBufferMgr* pBuffMgr = ((CTVDoc *) GetDocument())->GetBufferMgr();
    ptlOffset = CLPoint ( (long) pBuffMgr->GetCurrCol() * m_iTextWidth, pBuffMgr->GetCurrLine() * m_iTextHgt);
    CPoint point;
    CRect   rcClient;

    GetCursorPos (&point);
    ScreenToClient (&point);
    GetClientRect (&rcClient);
    rcClient.InflateRect (-::GetSystemMetrics(SM_CXBORDER), -::GetSystemMetrics(SM_CYBORDER));     // account for Win 3.1 bug; see KB Q43596

    if ( point.y < rcClient.top )  {
        if ( ptlOffset.y > 0)  {
            pView->PostMessage (WM_VSCROLL, SB_LINEUP);
            pView->OnSelLine(point, VRNEXT);
        }
    }
    if ( point.y >= rcClient.bottom )  {
        pView->PostMessage (WM_VSCROLL, SB_LINEDOWN);
        pView->OnSelLine(point, VRNEXT);
    }
}

void CHRulerView::OnTimer(UINT nIDEvent)
{
    ASSERT ( m_iTimer != NONE );
    ASSERT_KINDOF(CMDIChildWnd, ((CMDIFrameWnd*) AfxGetMainWnd())->MDIGetActive());
    CBlockScrollView* pView = (CBlockScrollView*) ((CMDIFrameWnd*) AfxGetMainWnd())->MDIGetActive()->GetActiveView();
    ASSERT_VALID(pView);
    CBufferMgr* pBuffMgr = ((CTVDoc *) GetDocument())->GetBufferMgr();
    ptlOffset = CLPoint ( (long) pBuffMgr->GetCurrCol() * m_iTextWidth, pBuffMgr->GetCurrLine() * m_iTextHgt);
    CPoint point;
    CRect   rcClient;

    GetCursorPos (&point);
    ScreenToClient (&point);
    GetClientRect (&rcClient);
    rcClient.InflateRect (-::GetSystemMetrics(SM_CXBORDER), -::GetSystemMetrics(SM_CYBORDER));     // account for Win 3.1 bug; see KB Q43596

    if ( point.x < rcClient.left )  {
        if ( ptlOffset.x > 0)  {
            pView->PostMessage (WM_HSCROLL, SB_PAGELEFT);
            pView->OnSelLine(point, HRNEXT);
        }
    }
    if ( point.x >= rcClient.right )  {
        pView->PostMessage (WM_HSCROLL, SB_PAGERIGHT);
        pView->OnSelLine(point, HRNEXT);
    }
}
