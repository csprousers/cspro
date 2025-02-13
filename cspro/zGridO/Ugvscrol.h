#pragma once

/***********************************************
    Ultimate Grid 97
    Copyright 1994 - 1997 Dundas Software Ltd.

    Class
        CUGVScroll
    Purpose
        This class controls the grid's verticle
        scrollbar and is responsible for updating
        it and making sure that it is using the
        specified scroll mode
************************************************/
class CLASS_DECL_ZGRIDO CUGVScroll : public CScrollBar
{
// Construction
public:
    CUGVScroll();

// Attributes
public:

// Operations
public:

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CUGVScroll)
    //}}AFX_VIRTUAL

// Implementation
public:
    virtual ~CUGVScroll();

    // Generated message map functions
protected:
    //{{AFX_MSG(CUGVScroll)
    afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    //}}AFX_MSG

    DECLARE_MESSAGE_MAP()

protected:

    friend CUGCtrl;
    CUGCtrl *       m_ctrl;
    CUGGridInfo *   m_GI;           //pointer to the grid information


    double          m_multiRange;   //scroll bar multiplication factor
                                    //for setting the scroll range
    double          m_multiPos;     //multiplication factor for setting the
                                    //top row during a thumb track
    long            m_lastMaxTopRow;//last max top row

    int             m_lastScrollMode;

    int             m_lastNumLockRow;

    long            m_trackRowPos;

public:


    //internal functions
    void Update();
    void Moved();
    void VScroll(UINT nSBCode, UINT nPos);

};

/////////////////////////////////////////////////////////////////////////////
