#pragma once


/////////////////////////////////////////////////////////////////////////////
// CRunBOpDlg dialog

class CRunBOpDlg : public CDialog
{
// Construction
public:
    CRunBOpDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
    //{{AFX_DATA(CRunBOpDlg)
    enum { IDD = IDD_RUNASB_DLG };
    BOOL    m_bSkipStruc;
    BOOL    m_bCheckRanges;
    //}}AFX_DATA


// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CRunBOpDlg)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:

    // Generated message map functions
    //{{AFX_MSG(CRunBOpDlg)
    virtual BOOL OnInitDialog();
    virtual void OnOK();
    //}}AFX_MSG
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.
