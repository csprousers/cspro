#pragma once
// StatDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CStatDlg dialog
#include "StatGrid.h"

class CStatDlg : public CDialog
{
// Construction
public:
    CStatDlg(CWnd* pParent = NULL);   // standard constructor

public:
    CStatGrid       m_StatGrid;
// Dialog Data
    //{{AFX_DATA(CStatDlg)
    enum { IDD = IDD_STATDLG };
    CString m_sVerifiedCases;
    CString m_sNumCases;
    //}}AFX_DATA

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CStatDlg)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:
    // Generated message map functions
    //{{AFX_MSG(CStatDlg)
    virtual BOOL OnInitDialog();
    //}}AFX_MSG
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.
