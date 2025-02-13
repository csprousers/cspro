#include "StdAfx.h"
#include "PropertiesDlgParadataPage.h"


BEGIN_MESSAGE_MAP(PropertiesDlgParadataPage, CDialog)
    ON_CONTROL_RANGE(BN_CLICKED, IDC_PARADATA_EVENTS_ALL, IDC_PARADATA_EVENTS_NO, OnCollectionTypeChange)
END_MESSAGE_MAP()


PropertiesDlgParadataPage::PropertiesDlgParadataPage(const Application& application,
    ParadataProperties& paradata_properties, CWnd* pParent/* = nullptr*/)
    :   CDialog(PropertiesDlgParadataPage::IDD, pParent),
        m_application(application),
        m_paradataProperties(paradata_properties),
        m_collectionTypeRadioEnumHelper({ ParadataProperties::CollectionType::AllEvents,
                                          ParadataProperties::CollectionType::SomeEvents,
                                          ParadataProperties::CollectionType::No })
{
    SetupEventNames();

    PropertiesToForm(m_paradataProperties);
}


void PropertiesDlgParadataPage::SetupEventNames()
{
    for( size_t i = 0; i < Paradata::ParadataTable_NumberTables; ++i )
    {
        const Paradata::TableDefinition& table_definition = GetTableDefinition(static_cast<Paradata::ParadataTable>(i));

        if( table_definition.table_code > 0 )
        {
            // the application, session, and case events cannot be disabled
            if( table_definition.type == Paradata::ParadataTable::ApplicationEvent ||
                table_definition.type == Paradata::ParadataTable::SessionEvent ||
                table_definition.type == Paradata::ParadataTable::CaseEvent )
            {
                continue;
            }

            // format the event table names nicely:
            // 1) remove the _event from the table name
            // 2) convert the underscores to spaces
            // 3) capitalize each word
            std::wstring display_text = table_definition.name;

            SO::Replace(display_text, _T("_event"), _T(""));
            SO::Replace(display_text, _T("_"), _T(" "));
            display_text = SO::ToProperCase(display_text);

            // fix the GPS table name
            if( display_text == _T("Gps") )
                SO::MakeUpper(display_text);
            
            m_eventMappings.emplace_back(display_text, &table_definition);
        }
    }

    // sort the events
    std::sort(m_eventMappings.begin(), m_eventMappings.end(),
        [&](const auto& em1, const auto& em2)
        {
            return ( std::get<0>(em1) < std::get<0>(em2) );
        });
}


void PropertiesDlgParadataPage::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);

    DDX_Radio(pDX, IDC_PARADATA_EVENTS_ALL, m_collectionType);
    DDX_Check(pDX, IDC_PARADATA_RECORD_ENTERED_VALUES, m_recordValues);
    DDX_Check(pDX, IDC_PARADATA_RECORD_COORDINATES, m_recordCoordinates);
    DDX_Check(pDX, IDC_PARADATA_RECORD_ITERATOR_LOAD_CASES, m_recordIteratorLoadCases);
    DDX_Check(pDX, IDC_PARADATA_RECORD_INITIAL_PROPERTY_VALUES, m_recordInitialPropertyValues);
    DDX_Text(pDX, IDC_PARADATA_DEVICE_STATE_MINUTES, m_deviceStateIntervalMinutes);
    DDX_Text(pDX, IDC_PARADATA_GPS_LOCATION_MINUTES, m_gpsLocationIntervalMinutes);
    DDX_Control(pDX, IDC_PARADATA_EVENTS, m_eventsCheckListBox);
}


BOOL PropertiesDlgParadataPage::OnInitDialog()
{
    CDialog::OnInitDialog();

    // add the events and make sure the vertical height appears correctly
    CDC* pDC = m_eventsCheckListBox.GetDC();

    for( const auto& [display_text, table_definition] : m_eventMappings )
    {
        int index = m_eventsCheckListBox.AddString(display_text.c_str());
        m_eventsCheckListBox.SetItemHeight(index, pDC->GetTextExtent(display_text.c_str()).cy);
    }

    m_eventsCheckListBox.ReleaseDC(pDC);

    SelectEventNamesAndEnableDisableUI();

    return TRUE;
}


void PropertiesDlgParadataPage::PropertiesToForm(ParadataProperties& paradata_properties)
{
    m_collectionType = m_collectionTypeRadioEnumHelper.ToForm(paradata_properties.GetCollectionType());
    m_recordValues = ToForm::Check(paradata_properties.GetRecordValues());
    m_recordCoordinates = ToForm::Check(paradata_properties.GetRecordCoordinates());
    m_recordIteratorLoadCases = ToForm::Check(paradata_properties.GetRecordIteratorLoadCases());
    m_recordInitialPropertyValues = ToForm::Check(paradata_properties.GetRecordInitialPropertyValues());
    m_deviceStateIntervalMinutes = ToForm::Text(paradata_properties.GetDeviceStateIntervalMinutes());
    m_gpsLocationIntervalMinutes = ToForm::Text(paradata_properties.GetGpsLocationIntervalMinutes());
}


void PropertiesDlgParadataPage::FormToProperties()
{
    UpdateData(TRUE);

    m_paradataProperties.SetCollectionType(m_collectionTypeRadioEnumHelper.FromForm(m_collectionType));
    m_paradataProperties.SetRecordValues(FromForm::Check(m_recordValues));
    m_paradataProperties.SetRecordCoordinates(FromForm::Check(m_recordCoordinates));
    m_paradataProperties.SetRecordIteratorLoadCases(FromForm::Check(m_recordIteratorLoadCases));
    m_paradataProperties.SetRecordInitialPropertyValues(FromForm::Check(m_recordInitialPropertyValues));

    auto validate_minutes = [&](const CString& text_value, const TCHAR* description)
    {
        try
        {
            int minutes = FromForm::Text<int>(text_value, description, 0);

            if( minutes < 0 )
                throw CSProException(_T("The %s cannot be negative."), description);

            return minutes;
        }

        catch( const CSProException& )
        {
            // if not collecting paradata, invalid values can be ignored
            if( m_paradataProperties.GetCollectionType() == ParadataProperties::CollectionType::No )
                return 0;

            throw;
        }
    };

    m_paradataProperties.SetDeviceStateIntervalMinutes(validate_minutes(m_deviceStateIntervalMinutes, _T("device state collection interval")));
    m_paradataProperties.SetGpsLocationIntervalMinutes(validate_minutes(m_gpsLocationIntervalMinutes, _T("GPS location collection interval")));

    std::set<std::wstring> event_names;

    if( m_paradataProperties.GetCollectionType() == ParadataProperties::CollectionType::SomeEvents )
    {
        for( size_t i = 0; i < m_eventMappings.size(); ++i )
        {
            if( FromForm::Check(m_eventsCheckListBox.GetCheck((int)i)) )
                event_names.insert(std::get<1>(m_eventMappings[i])->name);
        }

        if( event_names.empty() )
            throw CSProException(_T("You must select at least one paradata event to collect."));
    }

    m_paradataProperties.SetEventNames(std::move(event_names));
}


void PropertiesDlgParadataPage::ResetProperties()
{
    ParadataProperties default_paradata_properties;
    ASSERT(default_paradata_properties.GetCollectionType() == ParadataProperties::CollectionType::No);

    if( m_application.GetEngineAppType() == EngineAppType::Entry && m_application.GetUseQuestionText() )
        default_paradata_properties.SetCollectionType(ParadataProperties::CollectionType::AllEvents);

    PropertiesToForm(default_paradata_properties);

    UpdateData(FALSE);

    SelectEventNamesAndEnableDisableUI();
}


void PropertiesDlgParadataPage::OnOK()
{
    FormToProperties();

    CDialog::OnOK();
}


void PropertiesDlgParadataPage::OnCollectionTypeChange(UINT /*nID*/)
{
    ParadataProperties::CollectionType previous_collection_type = m_collectionTypeRadioEnumHelper.FromForm(m_collectionType);

    UpdateData(TRUE);

    // if moving to some events from either all or no events, make the events
    // selected match what the user was just seeing
    if( m_collectionTypeRadioEnumHelper.FromForm(m_collectionType) == ParadataProperties::CollectionType::SomeEvents )
    {
        if( previous_collection_type == ParadataProperties::CollectionType::No )
        {
            m_paradataProperties.SetEventNames({ });
        }

        else if( previous_collection_type == ParadataProperties::CollectionType::AllEvents )
        {
            std::set<std::wstring> event_names = m_paradataProperties.GetEventNames();

            for( const auto& [display_text, table_definition] : m_eventMappings )
                event_names.insert(table_definition->name);

            m_paradataProperties.SetEventNames(std::move(event_names));
        }        
    }

    SelectEventNamesAndEnableDisableUI();
}


void PropertiesDlgParadataPage::SelectEventNamesAndEnableDisableUI()
{
    ParadataProperties::CollectionType collection_type =  m_collectionTypeRadioEnumHelper.FromForm(m_collectionType);
    int index = 0;

    for( const auto& [display_text, table_definition] : m_eventMappings )
    {
        m_eventsCheckListBox.SetCheck(index++,
            ToForm::Check(m_paradataProperties.IncludeEvent(collection_type, table_definition->name)));
    }

    // conditionally enable some of the controls
    m_eventsCheckListBox.EnableWindow(( collection_type == ParadataProperties::CollectionType::SomeEvents ));

    BOOL using_paradata = ( collection_type != ParadataProperties::CollectionType::No );

    for( int resource_id : { IDC_PARADATA_RECORD_ENTERED_VALUES,
                             IDC_PARADATA_RECORD_COORDINATES,
                             IDC_PARADATA_RECORD_ITERATOR_LOAD_CASES,
                             IDC_PARADATA_RECORD_INITIAL_PROPERTY_VALUES,
                             IDC_PARADATA_DEVICE_STATE_MINUTES,
                             IDC_PARADATA_GPS_LOCATION_MINUTES } )
    {
        GetDlgItem(resource_id)->EnableWindow(using_paradata);
    }
}
