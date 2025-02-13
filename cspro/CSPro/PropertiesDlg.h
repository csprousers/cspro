#pragma once

#include <CSPro/PropertiesDlgPage.h>
#include <zUToolO/TreePrps.h>

class PropertiesDlgApplicationPropertiesFilePage;
class PropertiesDlgLogicSettingsPage;


class PropertiesDlg : public CTreePropertiesDlg
{
public:
    PropertiesDlg(const Application& application, CWnd* pParent = nullptr);

    ApplicationProperties ReleaseApplicationProperties() { return std::move(m_applicationProperties); }

    const std::wstring& GetApplicationPropertiesFilename() const;

    const LogicSettings& GetLogicSettings() const;

protected:
    DECLARE_MESSAGE_MAP()

    void ResizeDlg(const CRect& new_page_rect) override;

    void OnPageChange(CDialog* old_page, CDialog* new_page) override;

    void OnOK() override;

    void OnResetProperties();

private:
    template<typename T>
    std::shared_ptr<T> AddPage(const TCHAR* caption, std::shared_ptr<T> page, CDialog* parent_page = nullptr);

private:
    ApplicationProperties m_applicationProperties;

    std::map<CDialog*, std::shared_ptr<PropertiesDlgPage>> m_propertiesDlgPagesMap;
    std::shared_ptr<PropertiesDlgApplicationPropertiesFilePage> m_propertiesDlgApplicationPropertiesFilePage;
    std::shared_ptr<PropertiesDlgLogicSettingsPage> m_propertiesDlgLogicSettingsPage;
};
