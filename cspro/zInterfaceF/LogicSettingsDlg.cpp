#include "StdAfx.h"
#include "LogicSettingsDlg.h"
#include <zToolsO/WinRegistry.h>
#include <zJson/Json.h>


BEGIN_MESSAGE_MAP(LogicSettingsDlg, CDialog)
    ON_BN_CLICKED(IDC_SET_AS_DEFAULT, OnSetAsDefault)
END_MESSAGE_MAP()


LogicSettingsDlg::LogicSettingsDlg(LogicSettings logic_settings, CWnd* pParent/* = nullptr*/)
    :   CDialog(LogicSettingsDlg::IDD, pParent),
        m_logicSettings(std::move(logic_settings)),
        m_versionRadioEnumHelper({ LogicSettings::Version::V0,
                                   LogicSettings::Version::V8_0 }),
        m_actionInvokerAccessFromExternalCallerRadioEnumHelper({ LogicSettings::ActionInvokerAccessFromExternalCaller::AlwaysAllow,
                                                                 LogicSettings::ActionInvokerAccessFromExternalCaller::PromptIfNoValidAccessToken,
                                                                 LogicSettings::ActionInvokerAccessFromExternalCaller::RequireAccessToken })
{
    PropertiesToForm(m_logicSettings);
}


void LogicSettingsDlg::DoDataExchange(CDataExchange* pDX)
{
    __super::DoDataExchange(pDX);

    DDX_Radio(pDX, IDC_LOGIC_VERSION_0, m_version);
    DDX_Check(pDX, IDC_CASE_SYMBOLS, m_caseSymbols);
    DDX_Check(pDX, IDC_ACTION_INVOKER_CONVERT_RESULTS, m_actionInvokerConvertResults);
    DDX_CBIndex(pDX, IDC_ACTION_INVOKER_ACCESS_FROM_EXTERNAL_CALLER, m_actionInvokerAccessFromExternalCaller);
    DDX_Text(pDX, IDC_ACTION_INVOKER_ACCESS_TOKENS, m_actionInvokerAccessTokens);
}


void LogicSettingsDlg::PropertiesToForm(const LogicSettings& logic_settings)
{
    m_version = m_versionRadioEnumHelper.ToForm(logic_settings.GetVersion());

    m_caseSymbols = logic_settings.CaseSensitiveSymbols() ? BST_CHECKED : BST_UNCHECKED;

    m_actionInvokerConvertResults = logic_settings.GetActionInvokerConvertResults() ? BST_CHECKED : BST_UNCHECKED;
    m_actionInvokerAccessFromExternalCaller = m_actionInvokerAccessFromExternalCallerRadioEnumHelper.ToForm(logic_settings.GetActionInvokerAccessFromExternalCaller());
    m_actionInvokerAccessTokens = SO::CreateSingleString(logic_settings.GetActionInvokerAccessTokens(), _T("\r\n"));
}


void LogicSettingsDlg::FormToProperties()
{
    UpdateData(TRUE);

    m_logicSettings.SetVersion(m_versionRadioEnumHelper.FromForm(m_version));

    m_logicSettings.SetCaseSensitiveSymbols(m_caseSymbols == BST_CHECKED);

    m_logicSettings.SetActionInvokerConvertResults(m_actionInvokerConvertResults == BST_CHECKED);
    m_logicSettings.SetActionInvokerAccessFromExternalCaller(m_actionInvokerAccessFromExternalCallerRadioEnumHelper.FromForm(m_actionInvokerAccessFromExternalCaller));

    std::vector<std::wstring> access_tokens;

    SO::ForeachLine(m_actionInvokerAccessTokens, false,
        [&](std::wstring access_token)
        {
            SO::MakeTrim(access_token);
            ASSERT(!access_token.empty());

            if( std::find(access_tokens.cbegin(), access_tokens.cend(), access_token) != access_tokens.cend() )
                throw CSProException(_T("The access token '%s' has already been specified."), access_token.c_str());

            access_tokens.emplace_back(std::move(access_token));

            return true;
        });

    m_logicSettings.SetActionInvokerAccessTokens(std::move(access_tokens));
}


void LogicSettingsDlg::OnOK()
{
    try
    {
        FormToProperties();

        __super::OnOK();
    }

    catch( const CSProException& exception )
    {
        ErrorMessage::Display(exception);
    }
}


void LogicSettingsDlg::OnSetAsDefault()
{
    FormToProperties();

    // same the settings to the registry in JSON format
    auto json_writer = Json::CreateStringWriter();
    json_writer->Write(m_logicSettings);

    WinSettings::Write(WinSettings::Type::LogicSettings, json_writer->GetString());

    AfxMessageBox(_T("The logic settings have been saved as the default for future applications."));
}
