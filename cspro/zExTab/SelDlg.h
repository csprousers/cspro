#pragma once
// SelDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CSelDlg dialog


class CLASS_DECL_ZEXTAB CSelDlg : public CDialog
{
// Construction
public:
    CSelDlg(CWnd* pParent = NULL);   // standard constructor
    bool    m_bDisableCon;
    bool    m_bHideAll;

// Dialog Data
    //{{AFX_DATA(CSelDlg)
    int     m_iProcess;
    //}}AFX_DATA

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CSelDlg)
protected:
    void DoDataExchange(CDataExchange* pDX) override;    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:

    // Generated message map functions
    //{{AFX_MSG(CSelDlg)
    virtual void OnOK();
    //}}AFX_MSG
public:
    BOOL OnInitDialog() override;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.
