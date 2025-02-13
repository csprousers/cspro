#pragma once
// GoToDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CNumEdit window

class CNumEdit : public CEdit
{
// Construction
public:
    CNumEdit();

// Attributes
public:

// Operations
public:

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CNumEdit)
    //}}AFX_VIRTUAL

// Implementation
public:
    virtual ~CNumEdit();

    // Generated message map functions
protected:
    //{{AFX_MSG(CNumEdit)
    afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
    afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
    //}}AFX_MSG

    DECLARE_MESSAGE_MAP()
};



/////////////////////////////////////////////////////////////////////////////
// CGoToDlg dialog

class CGoToDlg : public CDialog
{
// Construction
public:
    CGoToDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
    //{{AFX_DATA(CGoToDlg)
    enum { IDD = IDD_GOTO };
    CString m_sName;
    CNumEdit    m_numEdit;
    //}}AFX_DATA
    CDEItemBase* m_pBase;

    int m_iOcc;
// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CGoToDlg)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:

    // Generated message map functions
    //{{AFX_MSG(CGoToDlg)
    virtual void OnOK();
    virtual BOOL OnInitDialog();
    //}}AFX_MSG
};


/////////////////////////////////////////////////////////////////////////////
//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.
