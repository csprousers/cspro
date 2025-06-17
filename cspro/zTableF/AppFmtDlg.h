#pragma once

// CAppFmtDlg dialog

class CAppFmtDlg : public CDialog
{
    DECLARE_DYNAMIC(CAppFmtDlg)

public:
    CAppFmtDlg(CWnd* pParent = NULL);   // standard constructor
    virtual ~CAppFmtDlg();

// Dialog Data
    enum { IDD = IDD_FORMAT_APP };

protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

    DECLARE_MESSAGE_MAP()
public:
    CTabSetFmt* m_pFmt;
    CComboBox m_DigitGrp;
    CComboBox m_LeadingZeroes;
    CComboBox m_Units;
    CString m_sTitleTemplate;
    CString m_sContinueTxt;
    CComboBox m_DefTxt;
    CString m_sAlternateTxt;
    int m_iCurSel;
    FOREIGN_KEYS m_altKeys;
    virtual BOOL OnInitDialog();
protected:
    virtual void OnOK();
public:
    CComboBox m_DecSymbl;
    CComboBox m_DigitGrpSymbl;
    CComboBox m_zRound;
    CComboBox m_ZeroMask;
    afx_msg void OnCbnSelchangeDefaulTxt();
    afx_msg void OnCbnSelendokDefaulTxt();
    CString m_sDigitGrpSymbol;
    CString m_sDecimalSymbol;
};
