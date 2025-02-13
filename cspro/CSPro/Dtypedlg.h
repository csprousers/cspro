#pragma once
// DTypeDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CDictTypeDlg dialog

class CDictTypeDlg : public CDialog
{
// Construction
public:
    CDictTypeDlg(CWnd* pParent = NULL);   // standard constructor
    BOOL    m_bMain;
    BOOL    m_bSpecialOutPut;

// Dialog Data
    //{{AFX_DATA(CDictTypeDlg)
    enum { IDD = IDD_DICTTYPE };
    CButton m_DictType;
    int     m_iDType;
    //}}AFX_DATA


// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CDictTypeDlg)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:

    // Generated message map functions
    //{{AFX_MSG(CDictTypeDlg)
    virtual BOOL OnInitDialog();
    virtual void OnOK();
    //}}AFX_MSG
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.
