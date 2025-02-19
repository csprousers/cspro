#include "StdAfx.h"
#include "ApplicationPropertiesPageDlg.h"
#include "ManageFilesDlg.h"


BEGIN_MESSAGE_MAP(ApplicationPropertiesPageDlg, CDialog)
    ON_EN_CHANGE(IDC_NAME, OnNameChange)
END_MESSAGE_MAP()


unsigned int ApplicationPropertiesPageDlg::GetDialogTemplateId()
{
    return IDD_PAGE_PROPERTIES_APPLICATION;
}


ApplicationPropertiesPageDlg::ApplicationPropertiesPageDlg(ManageFilesDlg& manage_files_dlg, const Application& application,
                                                           CWnd* const pParent/* = nullptr*/)
    :   CDialog(GetDialogTemplateId(), pParent),
        m_manageFilesDlg(manage_files_dlg),
        m_filePath(application.GetApplicationFilePath()),
        m_name(TC::ToWide(application.GetName())),
        m_label(TC::ToWide(application.GetLabel()))
{
}


void ApplicationPropertiesPageDlg::DoDataExchange(CDataExchange* const pDX)
{
    __super::DoDataExchange(pDX);

    DDX_Text(pDX, IDC_NAME, m_name);
    DDX_Text(pDX, IDC_LABEL, m_label);
}


void ApplicationPropertiesPageDlg::OnNameChange()
{
    std::wstring new_name = WindowsWS::GetDlgItemText(this, IDC_NAME);

    if( m_name != new_name )
        m_manageFilesDlg.UpdateNameInTree(*this, std::move(new_name));
}


void ApplicationPropertiesPageDlg::OnValidatePage()
{
    UpdateData(TRUE);

    // validate the name and label
    m_manageFilesDlg.ValidateName(TC::ToUtf8(m_name), m_filePath);

    if( SO::IsWhitespace(m_label) )
        throw CSProException("The label for '%s' cannot be blank.", GetName().c_str());
}
