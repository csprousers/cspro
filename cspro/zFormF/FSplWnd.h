#pragma once
// FSplwnd.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CFSplitterWnd window

class CFSplitterWnd : public CSplitterWnd
{
// Construction
public:
    CFSplitterWnd();

private:
    CWnd*   m_pHiddenWnd;
// Attributes
public:
// Operations
public:

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CFSplitterWnd)
    //}}AFX_VIRTUAL
    void OnDrawSplitter(CDC *pDC, ESplitType nType,const CRect &rectArg);
    void DrawAllSplitBars(CDC* pDC, int cxInside, int cyInside);
    bool IsSplitterReady() {
        bool bRet = m_pRowInfo ?  true: false;
        return bRet;
    }

    void SetSplitterBarSizes(int iRow =-1);

// Implementation
public:
    virtual ~CFSplitterWnd();

    // Generated message map functions
protected:
    //{{AFX_MSG(CFSplitterWnd)
    afx_msg LRESULT OnNcHitTest(CPoint point);
    afx_msg void OnMouseMove(UINT nFlags, CPoint point);
    afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
    afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
    afx_msg void OnNcLButtonUp(UINT nHitTest, CPoint point);
    afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.
