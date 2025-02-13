#pragma once
// DiscPDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CDiscPDlg dialog
class CDiscPDlg : public CDialog
{
// Construction
public:
    CDiscPDlg(CWnd* pParent = NULL);   // standard constructor
    END_MODE    m_eMode;
    APP_MODE    m_eAppMode; // Add / Verify
    bool    m_bEnablePartialButton;

// Dialog Data
    //{{AFX_DATA(CDiscPDlg)
    enum { IDD = IDD_DLG_PARTIAL };
        // NOTE: the ClassWizard will add data members here
    //}}AFX_DATA


// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CDiscPDlg)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:

    // Generated message map functions
    //{{AFX_MSG(CDiscPDlg)
    virtual BOOL OnInitDialog();
    afx_msg void OnPartialSave();
    virtual void OnOK();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.
