#pragma once

/***********************************************
    Ultimate Grid 97
    Copyright 1994 - 1997 Dundas Software Ltd.

    Class
        CUGHScroll
    Purpose
        This class controls the grid's horizontal
        scrollbar and is responsible for updating
        it and making sure that it is using the
        specified scroll mode
************************************************/
class CLASS_DECL_ZGRIDO CUGHScroll : public CScrollBar
{
// Construction
public:
    CUGHScroll();

// Attributes
public:

// Operations
public:

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CUGHScroll)
    //}}AFX_VIRTUAL

// Implementation
public:
    virtual ~CUGHScroll();

    // Generated message map functions
protected:
    //{{AFX_MSG(CUGHScroll)
    afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    //}}AFX_MSG

    DECLARE_MESSAGE_MAP()


protected:

    friend CUGCtrl;
    CUGCtrl     *   m_ctrl;     //pointer to the main class
    CUGGridInfo *   m_GI;           //pointer to the grid information

    int m_lastMaxLeftCol;
    int m_lastNumLockCols;

    int m_trackColPos;

public:



    //internal functions
    void Update();
    void Moved();

    void HScroll(UINT nSBCode, UINT nPos);
};

/////////////////////////////////////////////////////////////////////////////
