#pragma once

#include <CSPro/PropertiesDlgPage.h>
#include <zInterfaceF/LogicSettingsDlg.h>


class PropertiesDlgLogicSettingsPage : public LogicSettingsDlg, public PropertiesDlgPage
{
public:
    PropertiesDlgLogicSettingsPage(const Application& application, CWnd* pParent = nullptr);

    void FormToProperties() override;
    void ResetProperties() override;

protected:
    BOOL OnInitDialog() override;

    void OnOK() override;
};
