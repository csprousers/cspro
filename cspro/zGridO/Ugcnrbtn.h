#pragma once

/***********************************************
    Ultimate Grid 97
    Copyright 1994 - 1997 Dundas Software Ltd.

    Class
        CUGnrBtn
    Purpose
        This class is used to draw the cells corner
        button. The corner button is the region
        at the top left of the grid between
        the top and side headings. The width and
        height of this object are determined by
        the width and height of the side and
        top headings.
    Details
        -This class uses a cell located at col -1
         by row -1 as its source of information
        -This object is drawn using the celltype
         that the above cell object points to
        -This object gives the user the ability
         to size the width and/or height of the
         side and top headings. This is done
         when the mouse is moved to the edge and
         depressed, then moved. Sizing nofications
         are make, which lets the programmer allow
         or dis-allow the sizing, or even set sizing
         limits.
************************************************/


class CLASS_DECL_ZGRIDO CUGCnrBtn : public CWnd
{
// Construction
public:
    CUGCnrBtn();

// Attributes
public:

// Operations
public:

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CUGCnrBtn)
    protected:
    //}}AFX_VIRTUAL

// Implementation
public:
    virtual ~CUGCnrBtn();

    // Generated message map functions
protected:
    //{{AFX_MSG(CUGCnrBtn)
    afx_msg void OnPaint();
    afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
    afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
    afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
    afx_msg void OnMouseMove(UINT nFlags, CPoint point);
    afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
    afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
    afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

public:


    // internal information
    friend CUGCtrl;
    CUGCtrl     *   m_ctrl;     //pointer to the main class
    CUGGridInfo *   m_GI;       //pointer to the grid information

    int             m_isSizing;     //sizing flag
    int             m_canSize;      //sizing flag
    int             m_sizingHeight; //sizing flag
    int             m_sizingWidth;  //sizing flag

    //internal functions
    void Update();
    void Moved();
};

/////////////////////////////////////////////////////////////////////////////
