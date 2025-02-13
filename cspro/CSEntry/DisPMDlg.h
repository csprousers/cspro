#pragma once
// DisPMDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CDisPMDlg dialog
class CDisPMDlg : public CDialog
{
// Construction
public:
    END_MODE m_eMode;
    bool m_bEnablePartialButton;
    CDisPMDlg(CWnd* pParent = NULL);   // standard constructor

    void SetFinishText(CString csFinishText) { m_csFinishText = csFinishText; }

// Dialog Data
    //{{AFX_DATA(CDisPMDlg)
    enum { IDD = IDD_DLG_PARTIAL_MOD };
        // NOTE: the ClassWizard will add data members here
    //}}AFX_DATA


// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CDisPMDlg)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:

    // Generated message map functions
    //{{AFX_MSG(CDisPMDlg)
    virtual BOOL OnInitDialog();
    afx_msg void OnPartialSave();
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
