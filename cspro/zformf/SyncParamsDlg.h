#pragma once

// CSyncParamsDlg dialog

class CSyncParamsDlg : public CDialogEx
{
    DECLARE_DYNAMIC(CSyncParamsDlg)

public:
    CSyncParamsDlg(CWnd* pParent = NULL);   // standard constructor
    virtual ~CSyncParamsDlg();

// Dialog Data
#ifdef AFX_DESIGN_TIME
    enum { IDD = IDD_SYNC_PARAMS_DIALOG };
#endif

protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    virtual BOOL OnInitDialog();
    virtual void OnOK();

    DECLARE_MESSAGE_MAP()
    afx_msg void OnBnClickedTestConnection();

    void UpdateEnabled();

public:
    CString m_csServerUrl;
    int m_iSyncDirection;
    int m_iServerType;
    BOOL m_bEnabled;
    afx_msg void OnBnClickedCsweb();
    afx_msg void OnBnClickedDropbox();
    afx_msg void OnBnClickedFtp();
    afx_msg void OnBnClickedCheckboxEnable();
};
