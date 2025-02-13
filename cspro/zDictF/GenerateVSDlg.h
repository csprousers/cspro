#pragma once


class GenerateVSDlg : public CDialog
{
    DECLARE_DYNAMIC(GenerateVSDlg)

public:
    GenerateVSDlg(CWnd* pParent = nullptr);

protected:
    DECLARE_MESSAGE_MAP()

    void DoDataExchange(CDataExchange* pDX) override;
    BOOL OnInitDialog() override;
    void OnOK() override;

    afx_msg void OnEnKillfocusGetVsetInterval();
    afx_msg void OnEnKillfocusGenVsetName();
    afx_msg void OnEnKillfocusGenVsetFrom();
    afx_msg void OnEnKillfocusGenVsetTo();

public:
    CDDDoc*     m_pDoc;
    CDictItem*  m_pItem;

    double      m_dMinVal;
    double      m_dMaxVal;
    double      m_dMinInterval;

    CIMSAString m_sLabel;
    CIMSAString m_sName;
    double      m_dFrom;
    double      m_dTo;
    double      m_dInterval;
    CIMSAString m_sTemplate;
    BOOL        m_bUseThousandsSeparator;
};
