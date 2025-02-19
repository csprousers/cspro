#pragma once


class OpenInDataManagerDlg : public CDialog
{
public:
    OpenInDataManagerDlg(CWnd* pParent = nullptr);

    bool OpenInDataManager() const { return m_openInDataManager; }
    bool RememberSetting() const  { return m_rememberSetting; }

protected:
    BOOL OnInitDialog() override;
    void OnOK() override;

private:
    bool m_openInDataManager;
    bool m_rememberSetting;
};
