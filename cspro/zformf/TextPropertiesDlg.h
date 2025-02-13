#pragma once


/////////////////////////////////////////////////////////////////////////////
// CTxtPropDlg dialog

class CTxtPropDlg : public CDialog
{
// Construction
public:
    CTxtPropDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
    //{{AFX_DATA(CTxtPropDlg)
    enum { IDD = IDD_TEXT_PROPDLG };
    int     m_iFont;
    CString m_sText;
    //}}AFX_DATA
    CFormScrollView* m_pView;
    LOGFONT m_lfDefault;
    LOGFONT m_lfCustom;
    CString m_csDlgTitle;
    bool    m_bEnableText;
    COLORREF        m_color;
    bool                m_applytoall;
// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CTxtPropDlg)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:

    // Generated message map functions
    //{{AFX_MSG(CTxtPropDlg)
    virtual BOOL OnInitDialog();
    afx_msg void OnRadiofont();
    afx_msg void OnRadiofont2();
    afx_msg void OnFont();
    afx_msg void OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct);
    afx_msg void OnColor();
    afx_msg void OnApply();
    afx_msg void OnReset();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.
