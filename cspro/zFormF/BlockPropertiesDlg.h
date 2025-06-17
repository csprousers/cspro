#pragma once


class CBlockPropDlg : public CDialog
{
// Construction
public:
    CBlockPropDlg (CWnd* pParent = NULL);   // standard constructor
    CBlockPropDlg (CDEBlock* pBlock, CFormScrollView* pParent);


    CDEBlock*        m_pBlock;
    CFormScrollView* m_pView;
// Dialog Data
    //{{AFX_DATA(CBlockPropDlg)
    enum { IDD = IDD_BLOCKPROP };
    CIMSAString m_sBlockLabel;
    CIMSAString m_sBlockName;
    BOOL m_bDisplayOnSameScreen;
    //}}AFX_DATA


// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CBlockPropDlg)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:

    // Generated message map functions
    //{{AFX_MSG(CBlockPropDlg)
    virtual void OnOK();
    virtual BOOL OnInitDialog();
    //}}AFX_MSG
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.
