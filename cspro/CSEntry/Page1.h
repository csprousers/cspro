#pragma once
// Page1.h : header file
//

#include <CSEntry/CaseView.h>
/////////////////////////////////////////////////////////////////////////////
// CPage1 dialog

class CPage1 : public CPropertyPage
{
    DECLARE_DYNCREATE(CPage1)

// Construction
public:

    bool    CreateCaseView(CCreateContext* pContext);

    CCaseView*  GetCaseView();

    CPage1();
    ~CPage1();

// Dialog Data
    //{{AFX_DATA(CPage1)
    enum { IDD = IDD_PROPPAGE1 };
        // NOTE - ClassWizard will add data members here.
        //    DO NOT EDIT what you see in these blocks of generated code !
    //}}AFX_DATA


// Overrides
    // ClassWizard generate virtual function overrides
    //{{AFX_VIRTUAL(CPage1)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:
    // Generated message map functions
    //{{AFX_MSG(CPage1)
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

private:
    CCaseView*  m_pCaseView;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.
