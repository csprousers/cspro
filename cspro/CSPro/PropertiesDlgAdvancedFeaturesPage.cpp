#include "StdAfx.h"
#include "PropertiesDlgAdvancedFeaturesPage.h"


// COMPONENTS_TODO_RESTORE_FOR_CSPRO81: in the resource editor, modify IDC_USE_COMPONENTS_INSTEAD_OF_NATIVE_VERSIONS to Visible = True


PropertiesDlgAdvancedFeaturesPage::PropertiesDlgAdvancedFeaturesPage(ApplicationProperties& application_properties, CWnd* pParent/* = nullptr*/)
    :   CDialog(PropertiesDlgAdvancedFeaturesPage::IDD, pParent),
        m_applicationProperties(application_properties)
{
    PropertiesToForm(m_applicationProperties);
}


void PropertiesDlgAdvancedFeaturesPage::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);

    DDX_Check(pDX, IDC_USE_HTML_DIALOGS, m_useHtmlDialogs);
    DDX_Check(pDX, IDC_USE_COMPONENTS_INSTEAD_OF_NATIVE_VERSIONS, m_useHtmlComponentsInsteadOfNativeVersions);
}


void PropertiesDlgAdvancedFeaturesPage::PropertiesToForm(const ApplicationProperties& application_properties)
{
    m_useHtmlDialogs = ToForm::Check(application_properties.UseHtmlDialogs);
    m_useHtmlComponentsInsteadOfNativeVersions = ToForm::Check(application_properties.GetUseHtmlComponentsInsteadOfNativeVersions());
}


void PropertiesDlgAdvancedFeaturesPage::FormToProperties()
{
    UpdateData(TRUE);

    m_applicationProperties.UseHtmlDialogs = FromForm::Check(m_useHtmlDialogs);
    m_applicationProperties.SetUseHtmlComponentsInsteadOfNativeVersions(FromForm::Check(m_useHtmlComponentsInsteadOfNativeVersions));
}


void PropertiesDlgAdvancedFeaturesPage::ResetProperties()
{
    const ApplicationProperties default_application_properties;

    PropertiesToForm(default_application_properties);

    UpdateData(FALSE);
}


void PropertiesDlgAdvancedFeaturesPage::OnOK()
{
    FormToProperties();

    CDialog::OnOK();
}
