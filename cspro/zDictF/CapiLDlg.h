#pragma once
// CapiLDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CCapilangDlg dialog

#include <zDictF/zDictF.h>
#include <zDictF/langgrid.h>

class CLASS_DECL_ZDICTF CCapilangDlg : public CDialog
{
public:
    CLangGrid m_Langgrid;

// Construction
public:
    CCapilangDlg(CWnd* pParent = NULL);   // standard constructor
    LOGFONT     m_lf;
// Dialog Data

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CCapilangDlg)
    public:
    virtual BOOL PreTranslateMessage(MSG* pMsg);
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

    //}}AFX_VIRTUAL

// Implementation
protected:

    // Generated message map functions
    //{{AFX_MSG(CCapilangDlg)
    virtual BOOL OnInitDialog();
    afx_msg void OnClose();
    afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
    virtual void OnOK();
    afx_msg void OnEditAdd();
    afx_msg void OnAdd();
    afx_msg void OnDelete();
    afx_msg void OnModify();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.
