#pragma once

/////////////////////////////////////////////////////////////////////////////
// CQuestionnaireSearchDlg dialog

class CQuestionnaireSearchDlg : public CDialog
{
// Construction
public:
    CQuestionnaireSearchDlg(CString csInitialKey,CWnd* pParent = NULL);   // standard constructor

// Dialog Data
    //{{AFX_DATA(CQuestionnaireSearchDlg)
    enum { IDD = IDD_QSRCHDLG };
    //}}AFX_DATA

private:
    int m_iKeyLength;
    CString m_csKey;

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CQuestionnaireSearchDlg)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:

    // Generated message map functions
    //{{AFX_MSG(CQuestionnaireSearchDlg)
    virtual void OnOK();
    //}}AFX_MSG
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.
