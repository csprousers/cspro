#pragma once
// DiscDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CDiscDlg dialog
class CDiscDlg : public CDialog
{
// Construction
public:
    CDiscDlg(CWnd* pParent = NULL);   // standard constructor
    END_MODE    m_eMode;
    APP_MODE    m_eAppMode; // Add / Verify
    bool        m_bEnablePartialButton;

// Dialog Data
    //{{AFX_DATA(CDiscDlg)
    enum { IDD = IDD_DLG_DISCARD };
        // NOTE: the ClassWizard will add data members here
    //}}AFX_DATA


// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CDiscDlg)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:

    // Generated message map functions
    //{{AFX_MSG(CDiscDlg)
    virtual BOOL OnInitDialog();
    virtual void OnOK();
    //}}AFX_MSG
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.
