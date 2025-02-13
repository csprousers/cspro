#pragma once

class CFormScrollView;

/////////////////////////////////////////////////////////////////////////////
// CGlobalFDlg dialog

class CGlobalFDlg : public CDialog
{
// Construction
public:
    CGlobalFDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
    //{{AFX_DATA(CGlobalFDlg)
    enum { IDD = IDD_GLOBALFONTDLG };
    int     m_iFont;
    CString m_sCurFontDesc;
    //}}AFX_DATA
    LOGFONT m_lfDefault; //system default
    LOGFONT m_lfCurrentFont;//current font
    LOGFONT m_lfSelectedFont;//Selected font

    CFormScrollView* m_pFormView;
// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CGlobalFDlg)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:
    DECLARE_MESSAGE_MAP()

    // Generated message map functions
    //{{AFX_MSG(CGlobalFDlg)
    afx_msg void OnRadio1();
    afx_msg void OnRadio2();
    afx_msg void OnFont();
    afx_msg void OnApply();
    virtual BOOL OnInitDialog();
    //}}AFX_MSG
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.
