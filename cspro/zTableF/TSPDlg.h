#pragma once

class CTabTreeCtrl;

// CTabSetPropDlg dialog

class CTabSetPropDlg : public CDialog
{
    DECLARE_DYNAMIC(CTabSetPropDlg)

public:
    CTabSet*            m_pTabSet;
//    CTabTreeCtrl*     m_pTabTreeCtrl;

    CTabSetPropDlg(CWnd* pParent = NULL);   // standard constructor
    CTabSetPropDlg (CTabSet* pTabSet, CTabTreeCtrl* pParent);
    virtual ~CTabSetPropDlg();

// Dialog Data
    //{{AFX_DATA(CTabSetPropDlg)
    enum { IDD = IDD_TABSET_PROP_DLG  };
    CIMSAString m_sTSLabel;
    CIMSAString m_sTSName;
    //}}AFX_DATA

protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

    // Generated message map functions
    //{{AFX_MSG(CTabSetPropDlg)
    virtual void OnOK();
    virtual BOOL OnInitDialog();
    //}}AFX_MSG
};
