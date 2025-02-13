#pragma once

#include <CSPro/PropertiesDlgPage.h>


class PropertiesDlgMappingTileProviderPage : public CDialog, public PropertiesDlgPage
{
public:
    enum { IDD = IDD_PROPERTIES_MAPPING_TILE_PROVIDER };

    PropertiesDlgMappingTileProviderPage(const MappingProperties& mapping_properties,
        MappingTileProviderProperties& mapping_tile_provider_properties, CWnd* pParent = nullptr);

    void FormToProperties() override;
    void ResetProperties() override;

protected:
    DECLARE_MESSAGE_MAP()

    void DoDataExchange(CDataExchange* pDX) override;
    BOOL OnInitDialog() override;

    void OnOK() override;

    void OnPreviewMap();

private:
    void SetupTileLayersPropertiesGridCtrl();

private:
    const MappingProperties& m_mappingProperties;
    MappingTileProviderProperties& m_mappingTileProviderProperties;

    std::wstring m_accessToken;
    CMFCPropertyGridCtrl m_tileLayersPropertiesGridCtrl;
};
