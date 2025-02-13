#pragma once


// CFontPrefDlg dialog

class CFontPrefDlg : public CDialog
{
    DECLARE_DYNAMIC(CFontPrefDlg)

public:
    CFontPrefDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
    enum { IDD = IDD_FONTPREFDLG };

protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

    DECLARE_MESSAGE_MAP()
public:
    afx_msg void OnBnClickedOk();
    afx_msg void OnBnClickedResetBtn();
    CComboBox m_fontCmb;
    CComboBox m_zoomCmb;
    virtual BOOL OnInitDialog();
};
