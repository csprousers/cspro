#pragma once

#include <zformf/FormView.h>


/////////////////////////////////////////////////////////////////////////////
// CFFPropDlg dialog

class CFFPropDlg : public CDialog
{
// Construction
public:
    CFFPropDlg (CWnd* pParent = NULL);   // standard constructor
    CFFPropDlg (CDEFormFile* pFormFile, CFormScrollView* pParent);

    CFormScrollView*    m_pView;
    CDEFormFile*        m_pFormFile;

// Dialog Data
    //{{AFX_DATA(CFFPropDlg)
    enum { IDD = IDD_FFPROP };
    CIMSAString m_sFFLabel;
    CIMSAString m_sFFName;
    //}}AFX_DATA


// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CFFPropDlg)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:
    // Generated message map functions
    //{{AFX_MSG(CFFPropDlg)
    virtual void OnOK();
    virtual BOOL OnInitDialog();
    //}}AFX_MSG
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.
