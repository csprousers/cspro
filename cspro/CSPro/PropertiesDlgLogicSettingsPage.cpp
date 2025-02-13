#include "StdAfx.h"
#include "PropertiesDlgLogicSettingsPage.h"


PropertiesDlgLogicSettingsPage::PropertiesDlgLogicSettingsPage(const Application& application, CWnd* pParent/* = nullptr*/)
    :   LogicSettingsDlg(application.GetLogicSettings(), pParent)
{
}


BOOL PropertiesDlgLogicSettingsPage::OnInitDialog()
{
    BOOL result = LogicSettingsDlg::OnInitDialog();
    ShiftNonPropertyPageDialogChildWindows(this);
    return result;
}


void PropertiesDlgLogicSettingsPage::FormToProperties()
{
    LogicSettingsDlg::FormToProperties();
}


void PropertiesDlgLogicSettingsPage::ResetProperties()
{
    LogicSettingsDlg::PropertiesToForm(LogicSettings::GetUserDefaultSettings());

    UpdateData(FALSE);
}


void PropertiesDlgLogicSettingsPage::OnOK()
{
    FormToProperties();

    // here we call CDialog::OnOK, not LogicSettingsDlg::OnOK, because LogicSettingsDlg::OnOK catches exceptions,
    // which we do not want to catch because they should be handled by PropertiesDlg::OnOK()
    CDialog::OnOK();
}
