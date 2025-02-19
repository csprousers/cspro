#pragma once


class SecurityOptionsDlg : public CDialog
{
public:
    SecurityOptionsDlg(const CDataDict& dictionary, CWnd* pParent = nullptr);

    bool GetAllowDataManagerModifications() const { return m_allowDataManagerModifications; }
    bool GetAllowExport() const                   { return m_allowExport; }
    int GetCachedPasswordMinutes() const          { return m_cachedPasswordMinutes; }

protected:
    DECLARE_MESSAGE_MAP()

    BOOL OnInitDialog() override;
    void DoDataExchange(CDataExchange* pDX) override;

    void OnOK() override;

    void OnMinutesComboChange();
    void OnMinutesTextChange();

private:
    static std::string MinutesToText(int minutes);
    static int MinutesToComboBoxIndex(int minutes);


private:
    bool m_allowDataManagerModifications;
    bool m_allowExport;
    int m_cachedPasswordMinutes;

    std::string m_minutesText;
    CComboBox m_minutesCombo;
};
