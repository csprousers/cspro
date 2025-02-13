#pragma once

#include <CSPro/PropertiesDlgPage.h>


class PropertiesDlgMappingPage : public CDialog, public PropertiesDlgPage
{
public:
    enum { IDD = IDD_PROPERTIES_MAPPING };

    PropertiesDlgMappingPage(MappingProperties& mapping_properties, CWnd* pParent = nullptr);

    void FormToProperties() override;
    void ResetProperties() override;

protected:
    DECLARE_MESSAGE_MAP()

    void DoDataExchange(CDataExchange* pDX) override;
    BOOL OnInitDialog() override;

    void OnOK() override;

    void OnCoordinateDisplayChange();
    void OnSelectOfflineMap();

    void OnPreviewMap();

private:
    void PropertiesToForm(const MappingProperties& mapping_properties);

    void RefreshDefaultBaseMapComboBox(const MappingProperties* mapping_properties);

private:
    MappingProperties& m_mappingProperties;

    std::tuple<double, double> m_currentLocation;

    int m_coordinateDisplay;
    RadioEnumHelper<CoordinateDisplay> m_coordinateDisplayRadioEnumHelper;

    CString m_coordinateDisplayExample;

    CComboBox m_defaultBaseMap;
    CString m_defaultBaseMapFilename;
    std::optional<int> m_customDefaultBaseMapIndex;

    int m_windowsMappingTileProvider;
    RadioEnumHelper<MappingTileProvider> m_mappingTileProviderRadioEnumHelper;
};
