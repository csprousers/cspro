#pragma once

/***********************************************
    Ultimate Grid 97
    Copyright 1994 - 1998 Dundas Software Ltd.


    class
        CUGTopHdg
    Purpose
        This class handles the drawing of the top
        heading, plus handles and forwards events
        that took place inside of the top heading.
        NOTE: The whole grid is made up of several
        parts, top heading, side heading, corner button,
        grid area, scroll bars and a tab area.
    Details
        -This class handles the drawing of the
         top heading and is responsibile for drawing
         the heading in an optomized manner.
        -This class handles all mouse and keyboard
         events and forwards the events to the
         CUGCtrl class and other appropriate classes
************************************************/

class CLASS_DECL_ZGRIDO CUGTopHdg : public CWnd
{
// Construction
public:
    CUGTopHdg();

// Attributes
public:

// Operations
public:

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CUGTopHdg)
    protected:
    virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
    //}}AFX_VIRTUAL

// Implementation
public:
    virtual ~CUGTopHdg();

    // Generated message map functions
protected:
    //{{AFX_MSG(CUGTopHdg)
    afx_msg void OnPaint();
    afx_msg void OnMouseMove(UINT nFlags, CPoint point);
    afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
    afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
    afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
    afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
    afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
    afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()


protected:

    friend CUGCtrl;
    CUGCtrl *       m_ctrl;
    CUGGridInfo *   m_GI;       //pointer to the grid information

    CUGCell         m_cell;     //general purpose cell class


    BOOL            m_isSizing;         //sizing flag
    BOOL            m_canSize;          //sizing flag
    BOOL            m_colOrRowSizing;   // 0-col 1-row
    int             m_sizingColRow;     //column/row being sized
    int             m_sizingStartSize;  //original size
    int             m_sizingStartPos;   //original start pos
    int             m_sizingStartHeight;//original top heading total height

    RECT            m_focusRect;        //focus rect for column sizing option

    CUGDrawHint     m_drawHint;     //cell drawing hints

    int             m_swapStartCol;
    int             m_swapEndCol;

    int GetCellRect(int *col,int *row,RECT *rect);
    int GetCellRect(int col,int row,RECT *rect);
    int GetCellFromPoint(CPoint *point,int *col,int *row,RECT *rect);
    int GetJoinRange(int *col,int *row,int *endCol,int *endRow);

    void DrawCellsIntern(CDC *dc);

    void CheckForUserResize(CPoint *point);

public:

    int GetTHRowHeight(int row);

    void Update();
    void Moved();
};

/////////////////////////////////////////////////////////////////////////////
