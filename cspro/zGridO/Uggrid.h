#pragma once

/***********************************************
    Ultimate Grid 97
    Copyright 1994 - 1997 Dundas Software Ltd.

    Class
        CUGGrid
    Purpose
        This class handles the drawing of the main
        grid area, plus handles and forwards events
        that took place inside of the main grid area.
        NOTE: The whole grid is made up of several
        parts, top heading, side heading, corner button,
        grid area, scroll bars and a tab area.
    Details
        -This class handles the drawing of the main
         grid area and is responsibile for drawing
         the grid in an optomized manner.
        -This class handles all mouse and keyboard
         events and forwards the events to the
         CUGCtrl class and other appropriate classes
************************************************/


#ifndef WM_MOUSEWHEEL
#define ON_WM_MOUSEWHEEL
#endif


class CLASS_DECL_ZGRIDO CUGGrid : public CWnd
{
// Construction
public:
    CUGGrid();

// Attributes
public:

// Operations
public:

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CUGGrid)
    protected:
    virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
    //}}AFX_VIRTUAL

// Implementation
public:
    virtual ~CUGGrid();

    // Generated message map functions
protected:
    //{{AFX_MSG(CUGGrid)
    afx_msg void OnPaint();
    afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
    afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
    afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
    afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
    afx_msg void OnMouseMove(UINT nFlags, CPoint point);
    afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
    afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
    afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
    afx_msg void OnKillFocus(CWnd* pNewWnd);
    afx_msg void OnSetFocus(CWnd* pOldWnd);
    afx_msg int  OnMouseActivate(CWnd* pDesktopWnd, UINT nHitTest, UINT message);
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
    afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
    afx_msg void OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags);
    afx_msg UINT OnGetDlgCode();
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnDestroy();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()


    BOOL ToolTipNeedText( UINT id, NMHDR * pTTTStruct, LRESULT * pResult );
    virtual int OnToolHitTest( CPoint point, TOOLINFO* pTI ) const;

public:

    //internal data
    CUGCtrl     *   m_ctrl;         //pointer to the main class
    CUGGridInfo *   m_GI;           //pointer to the grid information

    CUGDrawHint     m_drawHint;     //cell drawing hints


    CBitmap *       m_bitmap;       //double buffering

    RECT            m_clientRect;
    BOOL            m_paintOverRide;


    int             m_doubleBufferMode;

    CUGCell         m_cell;         //general purpose cell object

    long            m_keyRepeatCount; //key ballistics repeat counter

    int             m_hasFocus;

    BOOL            m_cellTypeCapture;

public:

    void Update();
    void Moved();
    void DrawCellsIntern(CDC *dc,CDC *db_dc);

    void RedrawCell(int col,long row);
    void RedrawRow(long row);
    void RedrawCol(int col);
    void RedrawRange(int startCol,long startRow,int endCol,long endRow);

    int SetDoubleBufferMode(int mode);
};

/////////////////////////////////////////////////////////////////////////////
