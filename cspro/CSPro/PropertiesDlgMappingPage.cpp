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


PropertiesDlgMappingPage::PropertiesDlgMappingPage(MappingProperties& mapping_properties, CWnd* pParent/* = nullptr*/)
    :   CDialog(PropertiesDlgMappingPage::IDD, pParent),
        m_mappingProperties(mapping_properties),
        m_currentLocation(CurrentLocation::GetCurrentLocationOrCensusBureau()),
        m_coordinateDisplayRadioEnumHelper({ CoordinateDisplay::Decimal, CoordinateDisplay::DMS }),
        m_mappingTileProviderRadioEnumHelper({ MappingTileProvider::Esri, MappingTileProvider::Mapbox })
{
    PropertiesToForm(m_mappingProperties);
}


void PropertiesDlgMappingPage::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);

    // show an example of how coordinates will look
    if( !pDX->m_bSaveAndValidate )
    {
        std::wstring coordinate_text = CoordinateConverter::ToString(m_coordinateDisplayRadioEnumHelper.FromForm(m_coordinateDisplay), m_currentLocation);
        m_coordinateDisplayExample.Format(_T("(%s)"), coordinate_text.c_str());
    }

    DDX_Radio(pDX, IDC_DECIMAL, m_coordinateDisplay);
    DDX_Text(pDX, IDC_COORDINATE_DISPLAY_EXAMPLE, m_coordinateDisplayExample);

    DDX_Control(pDX, IDC_BASE_MAP, m_defaultBaseMap);

    DDX_Radio(pDX, IDC_ESRI, m_windowsMappingTileProvider);
}


BOOL PropertiesDlgMappingPage::OnInitDialog()
{
    CDialog::OnInitDialog();

    RefreshDefaultBaseMapComboBox(&m_mappingProperties);

    return TRUE;
}


void PropertiesDlgMappingPage::RefreshDefaultBaseMapComboBox(const MappingProperties* mapping_properties)
{
    m_defaultBaseMap.ResetContent();
    m_customDefaultBaseMapIndex.reset();
    
    std::optional<int> default_base_map_selected_index;

    for( const std::wstring& base_map_string : GetBaseMapStrings() )
    {
        int index = m_defaultBaseMap.AddString(base_map_string.c_str());

        if( !default_base_map_selected_index.has_value() && mapping_properties != nullptr )
        {
            // the BaseMap enum starts at 1
            if( std::holds_alternative<BaseMap>(mapping_properties->GetDefaultBaseMap()) &&
                (int)std::get<BaseMap>(mapping_properties->GetDefaultBaseMap()) == ( index + 1 ) )
            {
                default_base_map_selected_index = index;
            }
        }
    }

    // the Custom default base map option will only appear when a file has been provided
    if( !m_defaultBaseMapFilename.IsEmpty() )
    {
        // remove an existing Custom entry if one existed
        if( m_customDefaultBaseMapIndex.has_value() )
            m_defaultBaseMap.DeleteString(*m_customDefaultBaseMapIndex);

        m_customDefaultBaseMapIndex = m_defaultBaseMap.AddString(
            FormatText(_T("Custom: %s"), PortableFunctions::PathGetFilename(m_defaultBaseMapFilename)));

        default_base_map_selected_index = m_customDefaultBaseMapIndex;
    }

    ASSERT(default_base_map_selected_index.has_value());

    m_defaultBaseMap.SetCurSel(*default_base_map_selected_index);
}


void PropertiesDlgMappingPage::PropertiesToForm(const MappingProperties& mapping_properties)
{
    m_coordinateDisplay = m_coordinateDisplayRadioEnumHelper.ToForm(mapping_properties.GetCoordinateDisplay());

    m_defaultBaseMapFilename = std::holds_alternative<std::wstring>(mapping_properties.GetDefaultBaseMap()) ?
        WS2CS(std::get<std::wstring>(mapping_properties.GetDefaultBaseMap())) :
        CString();

    m_windowsMappingTileProvider = m_mappingTileProviderRadioEnumHelper.ToForm(mapping_properties.GetWindowsMappingTileProvider());
}


void PropertiesDlgMappingPage::FormToProperties()
{
    UpdateData(TRUE);

    m_mappingProperties.SetCoordinateDisplay(m_coordinateDisplayRadioEnumHelper.FromForm(m_coordinateDisplay));


    int default_base_map_selected_index = m_defaultBaseMap.GetCurSel();

    if( default_base_map_selected_index == m_customDefaultBaseMapIndex )
    {
        ASSERT(!m_defaultBaseMapFilename.IsEmpty());
        m_mappingProperties.SetDefaultBaseMap(CS2WS(m_defaultBaseMapFilename));
    }

    else
    {
        // the BaseMap enum starts at 1
        m_mappingProperties.SetDefaultBaseMap((BaseMap)( default_base_map_selected_index + 1 ));
    }


    m_mappingProperties.SetWindowsMappingTileProvider(m_mappingTileProviderRadioEnumHelper.FromForm(m_windowsMappingTileProvider));
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
    CIMSAFileDialog file_dlg(TRUE,
                             nullptr,
                             m_defaultBaseMapFilename.IsEmpty() ? nullptr : (LPCTSTR)m_defaultBaseMapFilename,
                             OFN_HIDEREADONLY,
                             _T("Offline Map Files (*.mbtiles, *.tpk)|*.mbtiles;*.tpk||"),
                             this);

    file_dlg.m_ofn.lpstrTitle = _T("Select Default Base Map");

    if( file_dlg.DoModal() != IDOK )
        return;

    m_defaultBaseMapFilename = file_dlg.GetPathName();

    RefreshDefaultBaseMapComboBox(nullptr);
}


void PropertiesDlgMappingPage::OnPreviewMap()
{
    FormToProperties();
    TestMappingProperties(m_mappingProperties);
}
