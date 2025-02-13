#include "StdAfx.h"
#include "PropertiesDlgMappingTileProviderPage.h"
#include <zMapping/MappingPropertiesTester.h>


BEGIN_MESSAGE_MAP(PropertiesDlgMappingTileProviderPage, CDialog)
    ON_BN_CLICKED(IDC_PREVIEW_MAP, OnPreviewMap)
END_MESSAGE_MAP()


PropertiesDlgMappingTileProviderPage::PropertiesDlgMappingTileProviderPage(const MappingProperties& mapping_properties,
    MappingTileProviderProperties& mapping_tile_provider_properties, CWnd* pParent/* = nullptr*/)
    :   CDialog(PropertiesDlgMappingTileProviderPage::IDD, pParent),
        m_mappingProperties(mapping_properties),
        m_mappingTileProviderProperties(mapping_tile_provider_properties),
        m_accessToken(m_mappingTileProviderProperties.GetAccessToken())
{
}


void PropertiesDlgMappingTileProviderPage::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);

    DDX_Text(pDX, IDC_ACCESS_TOKEN, m_accessToken, true);
    DDX_Control(pDX, IDC_TILE_LAYERS, m_tileLayersPropertiesGridCtrl);
}


BOOL PropertiesDlgMappingTileProviderPage::OnInitDialog()
{
    CDialog::OnInitDialog();

    // the access token is called an API Key for Esri
    if( m_mappingTileProviderProperties.GetMappingTileProvider() == MappingTileProvider::Esri )
        GetDlgItem(IDC_ACCESS_TOKEN_TEXT)->SetWindowText(_T("API Key"));

    SetupTileLayersPropertiesGridCtrl();

    return TRUE;
}


void PropertiesDlgMappingTileProviderPage::SetupTileLayersPropertiesGridCtrl()
{
    // add the tile layers
    m_tileLayersPropertiesGridCtrl.SetRedraw(FALSE);

    m_tileLayersPropertiesGridCtrl.RemoveAll();

    m_tileLayersPropertiesGridCtrl.SetVSDotNetLook(TRUE);
    m_tileLayersPropertiesGridCtrl.EnableHeaderCtrl(TRUE, _T("Name"), _T("Tile Layer"));

    for( const auto& [base_map, tile_layer] : m_mappingTileProviderProperties.GetTileLayers() )
    {
        m_tileLayersPropertiesGridCtrl.AddProperty(new CMFCPropertyGridProperty(
            ToString(base_map), COleVariant(tile_layer.c_str()), nullptr, (DWORD)base_map));
    }

    // set the column widths (from https://stackoverflow.com/questions/3453244/how-to-set-a-cmfcpropertylistctrls-column-width)
    CRect rect;
    m_tileLayersPropertiesGridCtrl.GetWindowRect(&rect);
    m_tileLayersPropertiesGridCtrl.PostMessage(WM_SIZE, 0, MAKELONG(rect.Width(), rect.Height()));

    m_tileLayersPropertiesGridCtrl.AdjustLayout();

    m_tileLayersPropertiesGridCtrl.SetRedraw(TRUE);
    m_tileLayersPropertiesGridCtrl.Invalidate(TRUE);
}


void PropertiesDlgMappingTileProviderPage::FormToProperties()
{
    UpdateData(TRUE);

    m_mappingTileProviderProperties.SetAccessToken(m_accessToken);

    for( int i = 0; i < m_tileLayersPropertiesGridCtrl.GetPropertyCount(); ++i )
    {
        CMFCPropertyGridProperty* property = m_tileLayersPropertiesGridCtrl.GetProperty(i);
        BaseMap base_map = (BaseMap)property->GetData();
        m_mappingTileProviderProperties.SetTileLayer(base_map, CS2WS(CString(property->GetValue())));
    }
}


void PropertiesDlgMappingTileProviderPage::ResetProperties()
{
    m_mappingTileProviderProperties = MappingTileProviderProperties(m_mappingTileProviderProperties.GetMappingTileProvider());

    m_accessToken = m_mappingTileProviderProperties.GetAccessToken();

    UpdateData(FALSE);

    SetupTileLayersPropertiesGridCtrl();
}


void PropertiesDlgMappingTileProviderPage::OnOK()
{
    FormToProperties();

    CDialog::OnOK();
}


void PropertiesDlgMappingTileProviderPage::OnPreviewMap()
{
    FormToProperties();
    TestMappingProperties(m_mappingProperties, m_mappingTileProviderProperties.GetMappingTileProvider());
}
