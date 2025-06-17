#pragma once

// FSizeDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CFontSizeDlg dialog

class CFontSizeDlg : public CDialog
{
// Construction
public:
    CFontSizeDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
    //{{AFX_DATA(CFontSizeDlg)
    enum { IDD = IDD_DLG_FONT_SIZE };
    CIMSAString m_csSize;
    //}}AFX_DATA


// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CFontSizeDlg)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:

    // Generated message map functions
    //{{AFX_MSG(CFontSizeDlg)
    virtual BOOL OnInitDialog();
    virtual void OnOK();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};
