#include "StdAfx.h"
#include "SettingsDlg.h"


SettingsDlg::SettingsDlg(CWnd* const pParent/* = nullptr*/)
    :   ResizableDlg(IDD_SETTINGS, pParent),
        m_controller(Controller::GetInstance()),
        m_openSourceCodeDirectory(m_controller.GetOpenSourceCodeDirectory()),
        m_openSourceLibrariesDirectory(m_controller.GetOpenSourceLibrariesDirectory()),
        m_githubPAT(m_controller.GetGitHubPAT())
{
    SerializeDialogSize("OpenSourceSyncer-SettingsDlg");
}


void SettingsDlg::DoDataExchange(CDataExchange* const pDX)
{
    __super::DoDataExchange(pDX);

    DDX_Text(pDX, IDC_OPEN_SOURCE_CODE_DIRECTORY, m_openSourceCodeDirectory, true);
    DDX_Text(pDX, IDC_OPEN_SOURCE_LIBRARIES_DIRECTORY, m_openSourceLibrariesDirectory, true);
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
    m_controller.SetOpenSourceLibrariesDirectory(m_openSourceLibrariesDirectory);
    m_controller.SetGitHubPAT(m_githubPAT);

    __super::OnOK();
}
