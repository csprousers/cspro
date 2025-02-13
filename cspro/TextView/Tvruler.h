#pragma once

//***************************************************************************
//  File name: IVRuler.h
//
//  Description:
//       Interface for the CRulerView and related classes
//
//  History:    Date       Author     Comment
//              -----------------------------
//              1995        csc       created
//              4  Jan 03   csc       Add support for changing font size
//
//***************************************************************************

#define DEFAULT_RULER_WIDTH  4        // ruler won't resize itself until line number becomes > 9999
class CRulerViewMgr;                  // forward declaration


///////////////////////////////////////////////////////////////////////////////
/*-----------------------------------------------------------------------------

class CRulerView : public CView
-------------------------------

The CRulerView class is the base class used to derive horizontal and
vertical ruler bars, which display the current line and column position of
the file being viewed.


Data members -- protected members
---------------------------------
m_csRuler               A CString that contains the contents (either line or
                        column numbers) of the ruler.  This is what is
                        displayed on the screen.

m_iTextHgt              The height and width of a (monospaced, OEM font)
m_iTextWidth            character in a ruler.

m_pRulerMgr             A pointer to the CRulerViewMgr object that acts as
                        the ruler's parent and owns the CRulerView-derived
                        ruler.

Operations -- public members
----------------------------
SetCurrRulerMgr         Declares the CRulerViewMgr object that owns this
                        ruler.

Implementation -- public members
--------------------------------
OnDraw                  Stub, the base CRulerView version does nothing.

OnPrepareDC             Sets colors, loads monospaced font

GetDocument             Returns the document associated with this ruler.

Contstruction/Destruction members
---------------------------------
~CRulerView             Constructs and destructs CRulerView objects.
CRulerView

General message handlers -- public members
-----------------------------------------
OnSetFocus              Called after the CRulerView gains the input focus.

-----------------------------------------------------------------------------*/


class CRulerView : public CView
{
    DECLARE_DYNCREATE(CRulerView)

protected:
    CRulerView();

// Attributes
protected:
    CString         m_csRuler;      // holds ruler contents, both for horizontal and vertical
    int             m_iTextHgt;
    int             m_iTextWidth;   // passed from CBlockScrollView
    CRulerViewMgr*  m_pRulerMgr;

    CFont           m_Font;         // display font

// Operations
public:
    void SetCurrRulerMgr (CRulerViewMgr* pRVM)  { m_pRulerMgr = pRVM;  }

// Implementation
protected:
    virtual void OnDraw(CDC* pDC);         // stub, does nothing
    virtual void OnInitialUpdate();
    void OnPrepareDC (CDC* pDC, CPrintInfo* pInfo = NULL);

public:
    virtual ~CRulerView();
    CTVDoc* GetDocument ()  {
        ASSERT ( m_pDocument->IsKindOf(RUNTIME_CLASS(CTVDoc)));
        return (CTVDoc*) m_pDocument;
    }

    // following function added csc 4 Jan 03
    void SetText (int iH, int iW)  { m_iTextHgt = iH;  m_iTextWidth = iW; }

    // Generated message map functions
protected:
    //{{AFX_MSG(CRulerView)
    afx_msg void OnSetFocus(CWnd* pOldWnd);
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

};


///////////////////////////////////////////////////////////////////////////////
/*-----------------------------------------------------------------------------

class CHRulerView : public CRulerView
-------------------------------------

The CHRulerView class represents a horizontal ruler (optionally activated)
displayed along the top of a CTVView object.  The horizontal ruler
shows the column numbers for the file being viewed.


Operations -- public members
----------------------------
UpdateRulerPosition     Scrolls the ruler window in response to horizontal
                        movement within the file being viewed, and calls
                        UpdateWindow() to cause the ruler to be redrawn.

Implementation -- private members
---------------------------------
BuildRuler              Determines and constructs the ruler contents (a
                        CString object).

Implementation -- protected members
--------------------------------
OnDraw                  Calls BuildRuler() and draws the horizontal ruler.

Implementation -- public members
--------------------------------
Resize                  Called to signal the horizontal ruler that it needs
                        to resize itself.  This happens when the rulers are
                        activated/deactivated, or when the vertical ruler
                        needs to be made wider or narrower, or when the user
                        resizes the CTVView window.

Contstruction/Destruction members
---------------------------------
~CHRulerView            Constructs and destructs CHRulerView objects.
CGRulerView


-----------------------------------------------------------------------------*/

class CHRulerView : public CRulerView
{
    DECLARE_DYNCREATE(CHRulerView)

public:
    CHRulerView();

// Attributes
private:
    CLRect      m_rclOldBlock;
    BOOL        m_bBlockActive;
    BOOL        m_bCaptured;
    int         m_iTimer;           // timer number, used for scrolling while outside of the view

// Operations
public:
    void UpdateRulerPosition (int, int);

// Implementation
private:
    void BuildRuler (int);

public:
    void Resize (CRect, int);
    virtual ~CHRulerView();

protected:
    virtual void OnDraw(CDC* pDC);

    // Generated message map functions
protected:
    //{{AFX_MSG(CHRulerView)
    afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
    afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
    afx_msg void OnMouseMove(UINT nFlags, CPoint point);
    afx_msg void OnTimer(UINT nIDEvent);
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};


///////////////////////////////////////////////////////////////////////////////
/*-----------------------------------------------------------------------------

class CVRulerView : public CRulerView
-------------------------------------

The CVRulerView class represents a vertical ruler (optionally activated)
displayed along the lefthand side of a CTVView object.  The vertical
ruler shows the line numbers of the file being viewed.


Operations -- public members
----------------------------
UpdateRulerPosition     Scrolls the ruler window in response to vertical
                        movement within the file being viewed, and calls
                        UpdateWindow() to cause the ruler to be redrawn.

Implementation -- private members
---------------------------------
BuildRuler              Determines and constructs the ruler contents (a
                        CString object).

Implementation -- protected members
--------------------------------
OnDraw                  Calls BuildRuler() and draws the vertical ruler.

Implementation -- public members
--------------------------------
Resize                  Called to signal the vertical ruler that it needs to
                        resize itself.  This happens when the rulers are
                        activated/deactivated, or when the vertical ruler
                        needs to be made wider or narrower, or when the user
                        resizes the CTVView window.

Contstruction/Destruction members
---------------------------------
~CVRulerView            Constructs and destructs CVRulerView objects.
CVRulerView


-----------------------------------------------------------------------------*/

class CVRulerView : public CRulerView
{
    DECLARE_DYNCREATE(CVRulerView)
//protected:
public:
    CVRulerView();

private:
    CLRect      m_rclOldBlock;
    BOOL        m_bBlockActive;
    BOOL        m_bCaptured;
    int         m_iTimer;           // timer number, used for scrolling while outside of the view

// Operations
public:
    void UpdateRulerPosition (long, long);

// Implementation
private:
    void BuildRuler (long);

public:
    void Resize (CRect, int);
    virtual ~CVRulerView();

protected:
    virtual void OnDraw(CDC* pDC);      // overridden to draw this view

    // Generated message map functions
protected:
    //{{AFX_MSG(CVRulerView)
    afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
    afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
    afx_msg void OnMouseMove(UINT nFlags, CPoint point);
    afx_msg void OnTimer(UINT nIDEvent);
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};


///////////////////////////////////////////////////////////////////////////////
/*-----------------------------------------------------------------------------

class CRulerFillerView : public CView
-------------------------------------

The CRulerFillerView class fills in an empty area of a CTVView object.
When horizontal and vertical rulers are active, there are three such areas:
above the vertical scroll bar in the upper righthand corner of the view,
between the horizontal and vertical rulers in the upper lefthand corner of
the view, and to the left of the horizontal scroll bar in the lower lefthand
corner of the view.  These empty areas need to be drawn in the same color as
the horizontal and vertical rulers; thus a CRulerViewMgr object will own
and manage three CRulerFillerViews.

Implementation -- protected members
--------------------------------
OnDraw                  Not used, include because required; stub function.

Implementation -- public members
--------------------------------
Resize                  Called to signal the filler ruler that it needs to
                        resize itself.  This happens when the rulers are
                        activated/deactivated, when the vertical ruler needs
                        to be made wider or narrower, or when the user
                        resizes the CTVView window.

Contstruction/Destruction members
---------------------------------
~CRulerFillerView       Constructs and destructs CHRulerView objects.
CRulerFillerView

General message handlers -- public members
-----------------------------------------
OnSetFocus              Called after the CRulerView gains the input focus.

OnEraseBkgnd            Shades in the client rectangle owned by this filler
                        view.

-----------------------------------------------------------------------------*/

class CRulerFillerView : public CView
{
    DECLARE_DYNCREATE(CRulerFillerView)
//protected:
public:
    CRulerFillerView();

// Implementation
protected:
    virtual void OnDraw(CDC* pDC);

public:
    virtual ~CRulerFillerView();
    void Resize (CRect);

    // Generated message map functions
protected:
    //{{AFX_MSG(CRulerFillerView)
    afx_msg void OnSetFocus(CWnd* pOldWnd);
    afx_msg BOOL OnEraseBkgnd(CDC* pDC);
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};



///////////////////////////////////////////////////////////////////////////////
/*-----------------------------------------------------------------------------

class CRulerViewMgr
-------------------

The CRulerViewMgr class owns and manages the five different ruler views
necessary:  one horizontal (a CHRulerView), one vertical (a CVRulerView),
and three fillers (CRulerFillerViews).  A CRulerViewMgr acts as a liason
between the main CTVView and the different ruler view components.

Data members -- private members
---------------------------------
m_pHRulerView           The horizontal ruler bar
m_pVRulerView           The vertical ruler bar
m_pRulerFillerView[3]   The three filler views

m_ptlCurrPos            The coordinates (0-based) of the column and line
                        number of the upper lefthand corner of the file
                        being viewed.  After the ruler views have been
                        updated, this will correspond to their column and
                        line numbers.

m_iTextHgt              The height and width of a (monospaced, OEM font)
m_iTextWidth            character in a ruler.  (Needed to calculate ruler
                        widths).

Implementation -- public members
--------------------------------
Resize                  Causes the five member rulers to resize themselves.

SetText                 Sets the m_iTextHgt and m_iTextWidth private members.

UpdateRulerPosition     Updates the current position (ptlCurrPos) and
                        signals the horizontal and vertical rulers to scroll
                        and redraw themselves.

IsOffsetChanged         Signals whether or not the vertical ruler bar's
                        width needs to change (for example, when scrolling
                        from line 9999 to line 10000).

GetOffset               Gets the width of the vertical ruler bar (in pixels)

GetCurrPos              Returns the current ruler positions (ptlCurrPos).


Initialization -- public members
--------------------------------
Init                    Creates the five member rulers, registers them with
                        the document/view MDI MFC hierarchy, and initializes
                        them.

Contstruction/Destruction members
---------------------------------
CRulerViewMgr           Constructs and destructs CHRulerView objects.

-----------------------------------------------------------------------------*/

class CRulerViewMgr {
private:
    CVRulerView*       m_pVRulerView;
    CHRulerView*       m_pHRulerView;
    CRulerFillerView*  m_pRulerFillerView [3];  // [0]=top left, [1]=top right, [2]=bottom left
    CLPoint            m_ptlCurrPos;
    int                m_iTextHgt;
    int                m_iTextWidth;    // mirrored from the main view

public:
    CRulerViewMgr(void);
    void Init (CDocument*, CFrameWnd*);
    void SetText (int iH, int iW)  {
        m_iTextHgt = iH;  m_iTextWidth = iW;
        m_pVRulerView->SetText(iH, iW);        // added csc 4 Jan 03
        m_pHRulerView->SetText(iH, iW);        // added csc 4 Jan 03
    }
    void UpdateRulerPosition (int, long);
    BOOL IsOffsetChanged (int);
    void Resize (int, CRect rcPageSize = CRect(0,0,0,0) );
    int GetOffset (int);
    CLPoint GetCurrPos(void)  { return m_ptlCurrPos;  }
};
