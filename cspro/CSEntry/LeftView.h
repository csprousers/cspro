#pragma once
// LeftView.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CLeftView form view

#ifndef __AFXEXT_H__
#include <afxext.h>
#endif
class CLeftPropSheet;
class CCaseView;
class CMainFrame;
class CCaseTree;
class CLeftView : public CFormView
{
protected:
    CLeftView();           // protected constructor used by dynamic creation
    DECLARE_DYNCREATE(CLeftView)

// Form Data
public:
    //{{AFX_DATA(CLeftView)
    enum { IDD = IDD_LEFTVIEW };
        // NOTE: the ClassWizard will add data members here
    //}}AFX_DATA

// Attributes
public:

// Operations
public:

    bool        CreatePropSheet(CCreateContext* pContext, CMainFrame* pMainFrame);
    void        SetMainFrame(CMainFrame* pMainFrame);
    bool        CreateCaseTree();
    void        DeleteCaseTree();

    CCaseView*      GetCaseView();
    CCaseTree*      GetCaseTree();
    CLeftPropSheet* GetPropSheet();

    void        DummyMove();


// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CLeftView)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:
    virtual ~CLeftView();
#ifdef _DEBUG
    virtual void AssertValid() const;
    virtual void Dump(CDumpContext& dc) const;
#endif

    // Generated message map functions
    //{{AFX_MSG(CLeftView)
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg void OnDestroy();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

private:

    CLeftPropSheet* m_pPropSheet;
    int m_icx, m_icy;
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.
