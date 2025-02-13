#pragma once
// DiscMDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CDiscMDlg dialog
class CDiscMDlg : public CDialog
{
// Construction
public:
    END_MODE m_eMode;
    CDiscMDlg(CWnd* pParent = NULL);   // standard constructor

    void SetFinishText(CString csFinishText) { m_csFinishText = csFinishText; }

// Dialog Data
    //{{AFX_DATA(CDiscMDlg)
    enum { IDD = IDD_DLG_DISCARD_MOD };
        // NOTE: the ClassWizard will add data members here
    //}}AFX_DATA


// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CDiscMDlg)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL


// Implementation
protected:

    // Generated message map functions
    //{{AFX_MSG(CDiscMDlg)
    virtual BOOL OnInitDialog();
    afx_msg void OnFinish();
    afx_msg void OnDiscard();
    virtual void OnCancel();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

private:
    CString m_csFinishText;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.
