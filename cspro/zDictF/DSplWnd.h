#pragma once
// DSplwnd.h : header file
//

#include <zDictF/zDictF.h>
#include <zUToolO/OxFWndDk.h>
/////////////////////////////////////////////////////////////////////////////
// CDictSplitterWnd window

class CDictSplitterWnd : public CSplitterWnd
{
// Construction
public:
    CDictSplitterWnd();

private:
// Attributes
public:
// Operations
public:

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CDictSplitterWnd)
    //}}AFX_VIRTUAL

// Implementation
public:
    virtual ~CDictSplitterWnd();

    // Generated message map functions
protected:
    //{{AFX_MSG(CDictSplitterWnd)
    afx_msg LRESULT OnNcHitTest(CPoint point);
    afx_msg void OnNcLButtonUp(UINT nHitTest, CPoint point);
    afx_msg void OnLButtonUp(UINT nFlags, CPoint pt);
    afx_msg void OnLButtonDown(UINT nFlags, CPoint pt);
    afx_msg void OnMouseMove(UINT nFlags, CPoint point);
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
public:
    virtual void OnDrawSplitter(CDC* pDC, ESplitType nType, const CRect& rect);
    void SetSplitterBarSizes();
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.
