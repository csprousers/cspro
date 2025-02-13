#pragma once


/////////////////////////////////////////////////////////////////////////////
// CColumnProp dialog

class CColumnProp : public CDialog
{
// Construction
public:
    CColumnProp(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
    //{{AFX_DATA(CColumnProp)
    enum { IDD = IDD_COLUMNPROP };
    int     m_iHorzAlign;
    int     m_iVertAlign;
    int     m_iFont;
    CString m_csTxt;
    CString m_csMsg;
    //}}AFX_DATA
    LOGFONT m_lfDefault;
    LOGFONT m_lfCustom;
    CString m_csDlgTitle;
    bool    m_bDisableTxt;
    COLORREF m_color;
    bool    m_applytoall;


// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CColumnProp)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:

    // Generated message map functions
    //{{AFX_MSG(CColumnProp)
    afx_msg void OnHelp();
    virtual BOOL OnInitDialog();
    afx_msg void OnFont();
    afx_msg void OnRadiofont();
    afx_msg void OnRadiofont2();
    afx_msg void OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct);
    afx_msg void OnColor();
    afx_msg void OnApply();
    afx_msg void OnReset();
    afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};
