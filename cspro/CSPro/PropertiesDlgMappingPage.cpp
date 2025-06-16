#include "StdAfx.h"
#include "PropertiesDlgMappingPage.h"
#include <zMapping/CoordinateConverter.h>
#include <zMapping/CurrentLocation.h>
#include <zMapping/MappingPropertiesTester.h>


BEGIN_MESSAGE_MAP(PropertiesDlgMappingPage, CDialog)
    ON_BN_CLICKED(IDC_DECIMAL, OnCoordinateDisplayChange)
    ON_BN_CLICKED(IDC_DMS, OnCoordinateDisplayChange)
    ON_BN_CLICKED(IDC_SELECT_OFFLINE_MAP, OnSelectOfflineMap)
    ON_BN_CLICKED(IDC_PREVIEW_MAP, OnPreviewMap)
END_MESSAGE_MAP()


PropertiesDlgMappingPage::PropertiesDlgMappingPage(MappingProperties& mapping_properties, CWnd* const pParent/* = nullptr*/)
    :   CDialog(PropertiesDlgMappingPage::IDD, pParent),
        m_mappingProperties(mapping_properties),
        m_currentLocation(CurrentLocation::GetCurrentLocationOrCensusBureau()),
        m_coordinateDisplayRadioEnumHelper({ CoordinateDisplay::Decimal, CoordinateDisplay::DMS }),
        m_mappingEngineRadioEnumHelper({ MappingEngine::Default, MappingEngine::Leaflet}),
        m_mappingTileProviderRadioEnumHelper({ MappingTileProvider::Esri, MappingTileProvider::Mapbox })
{
    PropertiesToForm(m_mappingProperties);
}


void PropertiesDlgMappingPage::DoDataExchange(CDataExchange* const pDX)
{
    CDialog::DoDataExchange(pDX);

    // show an example of how coordinates will look
    if( !pDX->m_bSaveAndValidate )
    {
        const std::string coordinate_text = CoordinateConverter::ToString(m_coordinateDisplayRadioEnumHelper.FromForm(m_coordinateDisplay), m_currentLocation);
        m_coordinateDisplayExample = FormatText("(%s)", coordinate_text.c_str());
    }

    DDX_Radio(pDX, IDC_DECIMAL, m_coordinateDisplay);
    DDX_Text(pDX, IDC_COORDINATE_DISPLAY_EXAMPLE, m_coordinateDisplayExample);

    DDX_Control(pDX, IDC_BASE_MAP, m_defaultBaseMap);

    DDX_Radio(pDX, IDC_MAPPING_ENGINE_DEFAULT, m_mappingEngine);

    DDX_Radio(pDX, IDC_ESRI, m_mappingTileProvider);
}


BOOL PropertiesDlgMappingPage::OnInitDialog()
{
    CDialog::OnInitDialog();

    RefreshDefaultBaseMapComboBox(&m_mappingProperties);

    return TRUE;
}


void PropertiesDlgMappingPage::RefreshDefaultBaseMapComboBox(const MappingProperties* const mapping_properties)
{
    m_defaultBaseMap.ResetContent();
    m_customDefaultBaseMapIndex.reset();

    std::optional<int> default_base_map_selected_index;

    for( const char* const base_map_string : GetBaseMapStrings() )
    {
        const int index = m_defaultBaseMap.AddString(TC::ToWide(base_map_string).c_str());

        if( !default_base_map_selected_index.has_value() && mapping_properties != nullptr )
        {
            // the BaseMap enum starts at 1
            if( std::holds_alternative<BaseMap>(mapping_properties->GetDefaultBaseMap()) &&
                static_cast<int>(std::get<BaseMap>(mapping_properties->GetDefaultBaseMap())) == ( index + 1 ) )
            {
                default_base_map_selected_index = index;
            }
        }
    }

    // the Custom default base map option will only appear when a file has been provided
    if( !m_defaultBaseMapFilePath.empty() )
    {
        // remove an existing Custom entry if one existed
        if( m_customDefaultBaseMapIndex.has_value() )
            m_defaultBaseMap.DeleteString(*m_customDefaultBaseMapIndex);

        const std::string display_text = "Custom: " + PortableFunctions::PathGetFilename(m_defaultBaseMapFilePath);
        m_customDefaultBaseMapIndex = m_defaultBaseMap.AddString(TC::ToWide(display_text).c_str());

        default_base_map_selected_index = m_customDefaultBaseMapIndex;
    }

    ASSERT(default_base_map_selected_index.has_value());

    m_defaultBaseMap.SetCurSel(*default_base_map_selected_index);
}


void PropertiesDlgMappingPage::PropertiesToForm(const MappingProperties& mapping_properties)
{
    m_coordinateDisplay = m_coordinateDisplayRadioEnumHelper.ToForm(mapping_properties.GetCoordinateDisplay());

    m_defaultBaseMapFilePath = std::holds_alternative<std::string>(mapping_properties.GetDefaultBaseMap()) ?
        std::get<std::string>(mapping_properties.GetDefaultBaseMap()) :
        std::string();

    m_mappingEngine = m_mappingEngineRadioEnumHelper.ToForm(mapping_properties.GetMappingEngine());

    m_mappingTileProvider = m_mappingTileProviderRadioEnumHelper.ToForm(mapping_properties.GetMappingTileProvider());
}


void PropertiesDlgMappingPage::FormToProperties()
{
    UpdateData(TRUE);

    m_mappingProperties.SetCoordinateDisplay(m_coordinateDisplayRadioEnumHelper.FromForm(m_coordinateDisplay));


    const int default_base_map_selected_index = m_defaultBaseMap.GetCurSel();

    if( default_base_map_selected_index == m_customDefaultBaseMapIndex )
    {
        ASSERT(!m_defaultBaseMapFilePath.empty());
        m_mappingProperties.SetDefaultBaseMap(m_defaultBaseMapFilePath);
    }

    else
    {
        // the BaseMap enum starts at 1
        m_mappingProperties.SetDefaultBaseMap(static_cast<BaseMap>(default_base_map_selected_index + 1));
    }


    m_mappingProperties.SetMappingEngine(m_mappingEngineRadioEnumHelper.FromForm(m_mappingEngine));

    m_mappingProperties.SetMappingTileProvider(m_mappingTileProviderRadioEnumHelper.FromForm(m_mappingTileProvider));
}


void PropertiesDlgMappingPage::ResetProperties()
{
    MappingProperties default_mapping_properties;

    PropertiesToForm(default_mapping_properties);

    UpdateData(FALSE);

    RefreshDefaultBaseMapComboBox(&default_mapping_properties);
}


void PropertiesDlgMappingPage::OnOK()
{
    FormToProperties();

    CDialog::OnOK();
}


void PropertiesDlgMappingPage::OnCoordinateDisplayChange()
{
    UpdateData(TRUE);

    // m_coordinateDisplayExample will be updated in DoDataExchange
    UpdateData(FALSE);
}


void PropertiesDlgMappingPage::OnSelectOfflineMap()
{
    OpenFileDlg open_file_dlg(0, nullptr, m_defaultBaseMapFilePath, L"Offline Map Files (*.mbtiles, *.tpk, *.tpkx)|*.mbtiles;*.tpk;*.tpkx||", this);
    open_file_dlg.SetTitle(L"Select Default Base Map");

    if( open_file_dlg.DoModal() != IDOK )
        return;

    m_defaultBaseMapFilePath = open_file_dlg.GetFilePath();

    RefreshDefaultBaseMapComboBox(nullptr);
}


void PropertiesDlgMappingPage::OnPreviewMap()
{
    FormToProperties();
    MappingPropertiesTester::Test(m_mappingProperties);
}
