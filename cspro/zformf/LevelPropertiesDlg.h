#pragma once


/////////////////////////////////////////////////////////////////////////////
// CLevPropDlg dialog

// dialog property box for the Form's level

class CLevPropDlg : public CDialog
{
// Construction
public:
    CLevPropDlg(CWnd* pParent = NULL);   // standard constructor
    CLevPropDlg (CDELevel* pGroup, CFormScrollView* pParent);

// Dialog Data
    //{{AFX_DATA(CLevPropDlg)
    enum { IDD = IDD_LEVELPROP };
    CIMSAString m_sLevelLabel;
    CIMSAString m_sLevelName;
    //}}AFX_DATA


// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CLevPropDlg)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

    CFormScrollView*    m_pView;
    CDELevel*           m_pLevel;
// Implementation
protected:

    // Generated message map functions
    //{{AFX_MSG(CLevPropDlg)
    virtual void OnOK();
    virtual BOOL OnInitDialog();
    //}}AFX_MSG
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.
