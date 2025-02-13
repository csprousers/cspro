#pragma once

#include <CSPro/PropertiesDlgPage.h>


class PropertiesDlgAdvancedFeaturesPage : public CDialog, public PropertiesDlgPage
{
public:
    enum { IDD = IDD_PROPERTIES_ADVANCED_FEATURES };

    PropertiesDlgAdvancedFeaturesPage(ApplicationProperties& application_properties, CWnd* pParent = nullptr);

    void FormToProperties() override;
    void ResetProperties() override;

protected:
    void DoDataExchange(CDataExchange* pDX) override;

    void OnOK() override;

private:
    void PropertiesToForm(const ApplicationProperties& application_properties);

private:
    ApplicationProperties& m_applicationProperties;

    int m_useHtmlDialogs;
    int m_useHtmlComponentsInsteadOfNativeVersions;
};
