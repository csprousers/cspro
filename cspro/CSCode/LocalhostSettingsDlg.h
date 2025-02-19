#pragma once


class LocalhostSettingsDlg : public CDialog
{
public:
    LocalhostSettingsDlg(CWnd* pParent = nullptr);

protected:
    DECLARE_MESSAGE_MAP()

    void DoDataExchange(CDataExchange* pDX) override;
    BOOL OnInitDialog() override;

    void OnOK() override;

    void OnCreateMapping();

private:
    struct Settings
    {
        int start_automatically;
        std::string preferred_port;
        std::string automatically_mapped_drives;
    };

    Settings m_initialSettings;
    Settings m_settings;

    const std::vector<std::string> m_logicalDrives;
    CCheckListBox m_automaticallyMappedDrivesCtrl;
};
