#pragma once

#include <CSPro/PropertiesDlgPage.h>


class PropertiesDlgApplicationPropertiesFilePage : public CDialog, public PropertiesDlgPage
{
public:
    enum { IDD = IDD_PROPERTIES_CSPROPS_FILE };

    PropertiesDlgApplicationPropertiesFilePage(const Application& application, CWnd* pParent = nullptr);

    const std::string& GetApplicationPropertiesFilePath() const;

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

    std::string m_applicationPropertiesFilePath;
    std::string m_applicationFilePath;
    int m_useApplicationPropertiesFile;
};
