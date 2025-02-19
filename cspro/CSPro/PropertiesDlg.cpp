#include "StdAfx.h"
#include "PropertiesDlg.h"
#include "PropertiesDlgAdvancedFeaturesPage.h"
#include "PropertiesDlgApplicationPropertiesFilePage.h"
#include "PropertiesDlgJavaScriptPage.h"
#include "PropertiesDlgJsonPage.h"
#include "PropertiesDlgLogicSettingsPage.h"
#include "PropertiesDlgMappingPage.h"
#include "PropertiesDlgMappingTileProviderPage.h"
#include "PropertiesDlgParadataPage.h"


BEGIN_MESSAGE_MAP(PropertiesDlg, CTreePropertiesDlg)
    ON_BN_CLICKED(IDC_RESET_PROPERTIES, OnResetProperties)
END_MESSAGE_MAP()


PropertiesDlg::PropertiesDlg(const Application& application, CWnd* pParent/* = nullptr*/)
    :   CTreePropertiesDlg(L"Application Properties", IDD_PROPERTIES, true, pParent),
        m_applicationProperties(application.GetApplicationProperties())
{
    // paradata
    AddPage(L"Paradata", std::make_shared<PropertiesDlgParadataPage>(application, m_applicationProperties.GetParadataProperties()));

    // mapping
    auto mapping_page = AddPage(L"Mapping", std::make_shared<PropertiesDlgMappingPage>(m_applicationProperties.GetMappingProperties()));

    AddPage(L"Esri Tile Provider", std::make_shared<PropertiesDlgMappingTileProviderPage>(
        m_applicationProperties.GetMappingProperties(),
        m_applicationProperties.GetMappingProperties().GetEsriMappingTileProviderProperties()), mapping_page.get());

    AddPage(L"Mapbox Tile Provider", std::make_shared<PropertiesDlgMappingTileProviderPage>(
        m_applicationProperties.GetMappingProperties(),
        m_applicationProperties.GetMappingProperties().GetMapboxMappingTileProviderProperties()), mapping_page.get());

    // logic
    m_propertiesDlgLogicSettingsPage = AddPage(L"Logic Settings",
                                               std::make_shared<PropertiesDlgLogicSettingsPage>(application));

    // JavaScript
    AddPage(L"JavaScript", std::make_shared<PropertiesDlgJavaScriptPage>(m_applicationProperties.GetJavaScriptProperties()));

    // JSON Serialization
    AddPage(L"JSON Serialization", std::make_shared<PropertiesDlgJsonPage>(m_applicationProperties.GetJsonProperties()));

    // advanced features
    AddPage(L"Advanced Features", std::make_shared<PropertiesDlgAdvancedFeaturesPage>(m_applicationProperties));

    // application properties file
    m_propertiesDlgApplicationPropertiesFilePage = AddPage(L"Application Properties File",
                                                           std::make_shared<PropertiesDlgApplicationPropertiesFilePage>(application));
}


const std::string& PropertiesDlg::GetApplicationPropertiesFilePath() const
{
    return m_propertiesDlgApplicationPropertiesFilePage->GetApplicationPropertiesFilePath();
}


const LogicSettings& PropertiesDlg::GetLogicSettings() const
{
    return m_propertiesDlgLogicSettingsPage->GetLogicSettings();
}


void PropertiesDlg::ResizeDlg(const CRect& /*new_page_rect*/)
{
    // the properties dialog's size should be modified in the resource editor
}


template<typename T>
std::shared_ptr<T> PropertiesDlg::AddPage(const TCHAR* const caption, std::shared_ptr<T> page, CDialog* const parent_page/* = nullptr*/)
{
    CDialog* const dialog = std::dynamic_pointer_cast<T, CDialog>(page).get();
    ASSERT(dialog != nullptr);

    dialog->Create(T::IDD, this);
    CTreePropertiesDlg::AddPage(dialog, caption, parent_page);

    m_propertiesDlgPagesMap.try_emplace(dialog, page);

    return page;
}


bool PropertiesDlg::OnPageChange(CDialog* const old_page, CDialog* /*new_page*/)
{
    if( old_page == nullptr )
        return false;

    // when a new page is shown, update the application properties subobjects with any changed properties
    auto page_lookup = m_propertiesDlgPagesMap.find(old_page);
    ASSERT(page_lookup != m_propertiesDlgPagesMap.end());

    try
    {
        page_lookup->second->FormToProperties();
    }

    // ignore validation errors (since they will be handled in OnOK)
    catch( const CSProException& ) { }

    return false;
}


namespace
{
    // override CTreePropertiesDlg's typical OnOK handling to allow for validations
    bool OnOKTreeItemPage(CTreeCtrl& tree_ctrl, HTREEITEM hItem, void* const data)
    {
        CDialog* const page = reinterpret_cast<CDialog*>(tree_ctrl.GetItemData(hItem));

        CDialog** const last_page_visited = reinterpret_cast<CDialog**>(data);
        *last_page_visited = page;

        if( page != nullptr )
        {
            ASSERT_VALID(page);
            page->SendMessage(WM_COMMAND, IDOK, 0);
        }

        return true;
    }
}

void PropertiesDlg::OnOK()
{
    CDialog* last_page_visited = nullptr;

    try
    {
        ForEachTreeItem(&OnOKTreeItemPage, &last_page_visited);

        CDialog::OnOK();
    }

    catch( const CSProException& exception )
    {
        if( m_pCurrDlg != last_page_visited )
            SetPage(last_page_visited);

        ErrorMessage::Display(exception);
    }
}


void PropertiesDlg::OnResetProperties()
{
    auto page_lookup = m_propertiesDlgPagesMap.find(GetPage());
    ASSERT(page_lookup != m_propertiesDlgPagesMap.end());

    page_lookup->second->ResetProperties();
}
