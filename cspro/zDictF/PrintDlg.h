#pragma once
// PrintDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CPrintDlg dialog

class CPrintDlg : public CDialog
{
// Construction
public:
    CPrintDlg(CWnd* pParent = NULL);   // standard constructor
    bool    m_bPreview;

// Dialog Data
    //{{AFX_DATA(CPrintDlg)
    enum { IDD = IDD_PRINT_DLG };
    int     m_iPrintBrief;
    int     m_iPrintNameFirst;
    int     m_iPrintToFile;
    //}}AFX_DATA


// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CPrintDlg)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:

    // Generated message map functions
    //{{AFX_MSG(CPrintDlg)
    virtual BOOL OnInitDialog();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.
