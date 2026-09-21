#include "StdAfx.h"
#include "SettingsDlg.h"


SettingsDlg::SettingsDlg(CWnd* const pParent/* = nullptr*/)
    :   ResizableDlg(IDD_SETTINGS, pParent),
        m_controller(Controller::GetInstance()),
        m_openSourceCodeDirectory(m_controller.GetOpenSourceCodeDirectory()),
        m_thirdPartyLibrariesDirectory(m_controller.GetThirdPartyLibrariesDirectory()),
        m_githubPAT(m_controller.GetGitHubPAT())
{
    SerializeDialogSize("OpenSourceSyncer-SettingsDlg");
}


void SettingsDlg::DoDataExchange(CDataExchange* const pDX)
{
    __super::DoDataExchange(pDX);

    DDX_Text(pDX, IDC_OPEN_SOURCE_CODE_DIRECTORY, m_openSourceCodeDirectory, true);
    DDX_Text(pDX, IDC_THIRD_PARTY_LIBRARIES_DIRECTORY, m_thirdPartyLibrariesDirectory, true);
    DDX_Text(pDX, IDC_GITHUB_PAT, m_githubPAT, true);
}


BOOL SettingsDlg::OnInitDialog()
{
    __super::OnInitDialog();

    WindowHelpers::RemoveDialogSystemIcon(*this);

    return TRUE;
}


void SettingsDlg::OnOK()
{
    UpdateData(TRUE);

    m_controller.SetOpenSourceCodeDirectory(m_openSourceCodeDirectory);
    m_controller.SetThirdPartyLibrariesDirectory(m_thirdPartyLibrariesDirectory);
    m_controller.SetGitHubPAT(m_githubPAT);

    __super::OnOK();
}
