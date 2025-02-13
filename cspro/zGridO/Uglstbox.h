#pragma once

/***********************************************
    Ultimate Grid 97
    Copyright 1994 - 1997 Dundas Software Ltd.

    Class
        CUGLstBox
    Purpose
        This class is used by the CUGDropList class
        as the listbox that appears when the droplist
        is activated.
    Details
        This class is responsible for returning any
        selection make in the listbox, and destroying
        itself when an item is selected or when the
        selection process is canceled.
************************************************/

class CLASS_DECL_ZGRIDO CUGLstBox : public CListBox
{
// Construction
public:
    CUGLstBox();

// Attributes
public:

// Operations
public:

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CUGLstBox)
    //}}AFX_VIRTUAL

// Implementation
public:
    virtual ~CUGLstBox();

    // Generated message map functions
protected:
    //{{AFX_MSG(CUGLstBox)
    afx_msg void OnKillFocus(CWnd* pNewWnd);
    afx_msg int OnMouseActivate(CWnd* pDesktopWnd, UINT nHitTest, UINT message);
    afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
    afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
    afx_msg void OnSetFocus(CWnd* pOldWnd);
    //}}AFX_MSG

    DECLARE_MESSAGE_MAP()

public:
    //pointer to the main class
    CUGCtrl *   m_ctrl;
    int         m_HasFocus;
    int     *   m_col;
    long    *   m_row;
    BOOL        m_selected;

    int         m_cellTypeId;

private:
    void Select();

};

/////////////////////////////////////////////////////////////////////////////
