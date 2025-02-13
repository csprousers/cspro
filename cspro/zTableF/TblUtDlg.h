#pragma once


// CTblUnitDlg dialog

class CTblUnitDlg : public CDialog
{
    DECLARE_DYNAMIC(CTblUnitDlg)
public:
    CStringArray m_arrUnitNames;
public:
    CTblUnitDlg(CWnd* pParent = NULL);   // standard constructor
    virtual ~CTblUnitDlg();

// Dialog Data
    enum { IDD = IDD_TALLY_TABLE_UNITS };

protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

    DECLARE_MESSAGE_MAP()
public:
    CComboBox m_cUnitNames;
    CString m_sUnitName;
protected:
    virtual void OnOK();
public:
    afx_msg void OnBnClickedOk();
    virtual BOOL OnInitDialog();
};
