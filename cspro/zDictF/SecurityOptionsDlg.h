#pragma once


class SecurityOptionsDlg : public CDialog
{
    DECLARE_DYNAMIC(SecurityOptionsDlg)

public:
    SecurityOptionsDlg(const CDataDict& dictionary, CWnd* pParent = NULL); // standard constructor

    bool GetAllowDataViewerModifications() const { return ( m_allowDataViewerModifications != 0 ); }
    bool GetAllowExport() const                  { return ( m_allowExport != 0 ); }
    int GetCachedPasswordMinutes() const         { return m_cachedPasswordMinutes; }

// Dialog Data
#ifdef AFX_DESIGN_TIME
    enum { IDD = IDD_SECURITY_OPTIONS };
#endif

protected:
    DECLARE_MESSAGE_MAP()

    BOOL OnInitDialog() override;
    void DoDataExchange(CDataExchange* pDX) override;
    void OnOK() override;

    afx_msg void OnMinutesComboChange();
    afx_msg void OnMinutesTextChange();


protected:
    CComboBox m_minutesCombo;
    CIMSAString m_minutesText;

private:
    BOOL m_allowDataViewerModifications;
    BOOL m_allowExport;
    int m_cachedPasswordMinutes;
};
