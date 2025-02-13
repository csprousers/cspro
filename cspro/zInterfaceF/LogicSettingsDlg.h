#pragma once

#include <zInterfaceF/zInterfaceF.h>
#include <zAppO/LogicSettings.h>
#include <zUtilF/DialogValidators.h>


class CLASS_DECL_ZINTERFACEF LogicSettingsDlg : public CDialog
{
public:
    LogicSettingsDlg(LogicSettings logic_settings, CWnd* pParent = nullptr);

    enum { IDD = IDD_LOGIC_SETTINGS };

    const LogicSettings& GetLogicSettings() const { return m_logicSettings; }

protected:
    DECLARE_MESSAGE_MAP()

    void DoDataExchange(CDataExchange* pDX) override;
    void OnOK() override;

    void OnSetAsDefault();

protected:
    void PropertiesToForm(const LogicSettings& logic_settings);
    void FormToProperties();

private:
    LogicSettings m_logicSettings;

    int m_version;
    RadioEnumHelper<LogicSettings::Version> m_versionRadioEnumHelper;

    int m_caseSymbols;

    int m_actionInvokerConvertResults;
    int m_actionInvokerAccessFromExternalCaller;
    RadioEnumHelper<LogicSettings::ActionInvokerAccessFromExternalCaller> m_actionInvokerAccessFromExternalCallerRadioEnumHelper;
    std::wstring m_actionInvokerAccessTokens;
};
