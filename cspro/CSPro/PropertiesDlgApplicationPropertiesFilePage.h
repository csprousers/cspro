#pragma once

#include <CSPro/PropertiesDlgPage.h>


class PropertiesDlgApplicationPropertiesFilePage : public CDialog, public PropertiesDlgPage
{
public:
    enum { IDD = IDD_PROPERTIES_CSPROPS_FILE };

    PropertiesDlgApplicationPropertiesFilePage(const Application& application, CWnd* pParent = nullptr);

    const std::wstring& GetApplicationPropertiesFilename() const
    {
        return ( m_useApplicationPropertiesFile == 0 ) ? SO::EmptyString : m_applicationPropertiesFilename;
    }

    void FormToProperties() override;
    void ResetProperties() override;

protected:
    DECLARE_MESSAGE_MAP()

    void DoDataExchange(CDataExchange* pDX) override;
    BOOL OnInitDialog() override;

    void OnOK() override;

    void OnUseFileChange(UINT nID);
    void OnSelectFile();

private:
    void EnableDisableControls();

private:
    const Application& m_application;

    std::wstring m_applicationPropertiesFilename;
    std::wstring m_applicationFilename;
    int m_useApplicationPropertiesFile;
};
