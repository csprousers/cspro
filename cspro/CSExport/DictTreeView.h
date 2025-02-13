#pragma once
// DictTreeView.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CDictTreeView view

class CDictTreeView : public CView
{
protected:
    CDictTreeView();           // protected constructor used by dynamic creation
    DECLARE_DYNCREATE(CDictTreeView)


// Operations
public:
    void    SetCtrl( CTreeCtrl* pTreeCtrl );


// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CDictTreeView)
    public:
    virtual void OnInitialUpdate();
    protected:
    virtual void OnDraw(CDC* pDC);      // overridden to draw this view
    //}}AFX_VIRTUAL

// Implementation
protected:
    virtual ~CDictTreeView();
#ifdef _DEBUG
    virtual void AssertValid() const;
    virtual void Dump(CDumpContext& dc) const;
#endif

    // Generated message map functions
protected:
    //{{AFX_MSG(CDictTreeView)
    afx_msg void OnSize(UINT nType, int cx, int cy);
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()


    CTreeCtrl*  m_pTree;
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.
