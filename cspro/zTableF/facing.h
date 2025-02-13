#pragma once
// facing.h : header file
//


/////////////////////////////////////////////////////////////////////////////
// CFacingPagesDlg dialog

class CFacingPagesDlg : public CDialog
{
// Construction
public:
    CFacingPagesDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
    //{{AFX_DATA(CFacingPagesDlg)
    enum { IDD = IDD_FACINGPAGES };
    CIMSAString m_sHorz;
    CIMSAString m_sVert;
    //}}AFX_DATA


// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CFacingPagesDlg)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:

    // Generated message map functions
    //{{AFX_MSG(CFacingPagesDlg)
        // NOTE: the ClassWizard will add member functions here
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.
