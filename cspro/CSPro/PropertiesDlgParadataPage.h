#pragma once

#include <CSPro/PropertiesDlgPage.h>
#include <zParadataO/TableDefinitions.h>


class PropertiesDlgParadataPage : public CDialog, public PropertiesDlgPage
{
public:
    enum { IDD = IDD_PROPERTIES_PARADATA };

    PropertiesDlgParadataPage(const Application& application, ParadataProperties& paradata_properties, CWnd* pParent = nullptr);

    void FormToProperties() override;
    void ResetProperties() override;

protected:
    DECLARE_MESSAGE_MAP()

    void DoDataExchange(CDataExchange* pDX) override;
    BOOL OnInitDialog() override;

    void OnOK() override;

    void OnCollectionTypeChange(UINT nID);

private:
    void PropertiesToForm(ParadataProperties& paradata_properties);

    void SetupEventNames();

    void SelectEventNamesAndEnableDisableUI();

private:
    const Application& m_application;
    ParadataProperties& m_paradataProperties;

    int m_collectionType;
    RadioEnumHelper<ParadataProperties::CollectionType> m_collectionTypeRadioEnumHelper;

    int m_recordValues;
    int m_recordCoordinates;
    int m_recordIteratorLoadCases;
    int m_recordInitialPropertyValues;

    CString m_deviceStateIntervalMinutes;
    CString m_gpsLocationIntervalMinutes;

    CCheckListBox m_eventsCheckListBox;
    std::vector<std::tuple<std::wstring, const Paradata::TableDefinition*>> m_eventMappings;
};
