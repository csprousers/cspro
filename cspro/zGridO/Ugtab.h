#pragma once

/***********************************************
    Ultimate Grid 97
    Copyright 1994 - 1997 Dundas Software Ltd.


    class
        CUGTab
    Purpose
        This is a general purose tab control
        that is built into the grid. It is
        most commonly used to switch grid sheets,
        but can be used for other purposes.
    Details
        -This control can contain upto 64 tabs
        -A notification is sent when a tab is
         selected, it does not change grid sheets
         automatically. Although grid sheets can
         be changed during this notification.
         This method gives the tab control greater
         abilities.
        -Fonts and colors can be setup for the
         tab control
***********************************************/

#define UTABSCROLLID 100

#define UTMAXTABS   64

#define IMSAMAXTABLN    20 //Max TAB LENGTH

class CLASS_DECL_ZGRIDO CUGTab : public CWnd
{
// Construction
public:
    CUGTab();

// Attributes
public:
    bool    m_bCanClick;        // gsf 09/13/99

// Operations
public:

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CUGTab)
    protected:
    virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
    virtual int OnToolHitTest( CPoint point, TOOLINFO* pTI ) const;

    //}}AFX_VIRTUAL

// Implementation
public:
    virtual ~CUGTab();

    // Generated message map functions
protected:
    //{{AFX_MSG(CUGTab)
    afx_msg void OnPaint();
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
    afx_msg void OnMouseMove(UINT nFlags, CPoint point);
    afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
    afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
    afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

protected:

    CString     m_tabStrings[UTMAXTABS];
    int         m_tabWidths[UTMAXTABS];
    int         m_tabIDs[UTMAXTABS];
    COLORREF    m_tabTextColors[UTMAXTABS];
    COLORREF    m_tabBackColors[UTMAXTABS];

    int     m_tabCount;
    int     m_tabOffset;
    int     m_maxTabOffset;

    int     m_currentTab;

    int     m_scrollWidth;

    CScrollBar m_scroll;
    CFont   m_defFont;
    CFont   *m_font;

    int     m_resizeReady;
    int     m_resizeInProgress;

    friend CUGCtrl;
    CUGCtrl* m_ctrl;
    CUGGridInfo *   m_GI;           //pointer to the grid information

    int     m_bestWidth;

    int GetTabItemWidth(LPCTSTR string);

public:

    int AddTab(LPCTSTR string,long ID);
    int InsertTab(int pos,LPCTSTR string,long ID);
    int DeleteTab(long ID);

    int SetCurrentTab(long ID);
    int GetCurrentTab();
    //Savy 10/21/98
    int GetCurrentTabID();
    int GetTabID(int iIndex);

    int GetTabCount();

    int AdjustScrollBars();

    int SetTabBackColor(long ID,COLORREF color);
    int SetTabTextColor(long ID,COLORREF color);
    int SetTabFont(CFont *font);
    //Savy 10/23/98
    int SetTabText(long ID,CString sString);

public:

    void OnTabLClicked(long ID);
    void OnTabRClicked(long ID);
    void OnTabDClicked(long ID);
    void OnTabSizing(int width);

    void Update();
};

/////////////////////////////////////////////////////////////////////////////
