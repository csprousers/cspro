#pragma once

// LeftProp.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CLeftPropSheet
#include <CSEntry/Page1.h>
#include <CSEntry/Page2.h>

class CMainFrame;
class CCaseTree;
class CLeftPropSheet : public CPropertySheet
{
        DECLARE_DYNAMIC(CLeftPropSheet)

// Construction
public:
        CLeftPropSheet(UINT nIDCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);
        CLeftPropSheet(LPCTSTR pszCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);

// Attributes
public:

// Operations
public:

// Overrides
        // ClassWizard generated virtual function overrides
        //{{AFX_VIRTUAL(CLeftPropSheet)
        //}}AFX_VIRTUAL

// Implementation
public:

        void            SetMainFrame(CMainFrame* pMainFrame);

        bool            CreateCaseView(CCreateContext* pContext);

        bool            CreateCaseTree();
        void            DeleteCaseTree();

        CCaseView*      GetCaseView();
        CCaseTree*      GetCaseTree();

        void            DummyMove();

        virtual                 ~CLeftPropSheet();

        // Generated message map functions
protected:
        //{{AFX_MSG(CLeftPropSheet)
        afx_msg void OnSize(UINT nType, int cx, int cy);
        afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
        afx_msg void OnKillFocus(CWnd* pNewWnd);
        afx_msg void OnSetFocus(CWnd* pOldWnd);
        afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
        //}}AFX_MSG
        DECLARE_MESSAGE_MAP()

private:
        CPage1  m_page1;
        CPage2  m_page2;

        RECT    m_PageRect;

        int     m_icx, m_icy;

};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.
