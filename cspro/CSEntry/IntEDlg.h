#pragma once
// IntEDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CIntEdtDlg dialog

class CIntEdtDlg : public CDialog
{
// Construction
public:
    CIntEdtDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
    //{{AFX_DATA(CIntEdtDlg)
    enum { IDD = IDD_INTEDTDLG };
    int     m_iOption;
    //}}AFX_DATA


// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CIntEdtDlg)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:

    // Generated message map functions
    //{{AFX_MSG(CIntEdtDlg)
    virtual BOOL OnInitDialog();
    virtual void OnOK();
    //}}AFX_MSG
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.
