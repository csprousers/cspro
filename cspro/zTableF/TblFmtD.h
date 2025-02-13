#pragma once


// CTblFmtDlg dialog

class CTblFmtDlg : public CDialog
{
    DECLARE_DYNAMIC(CTblFmtDlg)

public:
    CTblFmtDlg(CWnd* pParent = NULL);   // standard constructor
    virtual ~CTblFmtDlg();

// Dialog Data
    enum { IDD = IDD_FORMAT_TABLE };
    CTblFmt*    m_pFmt;
    CTblFmt*    m_pDefFmt;
    BOOL m_bShowDefaults;
    bool    m_bViewer;
protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

    DECLARE_MESSAGE_MAP()
public:
    virtual BOOL OnInitDialog();
    BOOL m_bIncludeSubTitle;
    BOOL m_bIncludePageNote;
    BOOL m_bIncludeEndNote;
    CComboBox m_LStubLeadering;
    CComboBox m_RStubLeadering;
    CComboBox m_FrqRdrBreaks;
    CComboBox m_BorderL;
    CComboBox m_BorderR;
    CComboBox m_BorderT;
    CComboBox m_BorderB;
    afx_msg void OnBnClickedReset();
    void PrepareBordersControls(LINE eLineDef,LINE eLine ,CComboBox* pComboBox);
    void PrepareLeadering(LEADERING eLeaderingDef, LEADERING eLeadering , CComboBox* pComboBox);
    void PrepareRdrBrkCtl();

protected:
    virtual void OnOK();
};
