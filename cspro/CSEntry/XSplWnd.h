#pragma once
// XSplitter.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CXSplitterWnd window

class CXSplitterWnd : public CSplitterWnd
{
// Construction
public:
    CXSplitterWnd();

// Attributes
public:
    bool        m_bUseQuestionText;
// Operations
public:

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CXSplitterWnd)
    //}}AFX_VIRTUAL
    void OnDrawSplitter(CDC *pDC, ESplitType nType,const CRect &rectArg);

// Implementation
public:
    virtual ~CXSplitterWnd();

    // Generated message map functions
protected:
    //{{AFX_MSG(CXSplitterWnd)
    afx_msg LRESULT OnNcHitTest(CPoint point);

    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg void OnMouseMove(UINT nFlags, CPoint point);
    afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
    afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.
