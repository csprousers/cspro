#include "stdafx.h"
#include "ParadataProperties.h"
#include "Application.h"


namespace
{
    constexpr int IntervalMinutesMinimum = 0;
    constexpr int IntervalMinutesMaximum = 360;
}


ParadataProperties::ParadataProperties()
    :   m_collectionType(CollectionType::No),
        m_recordIteratorLoadCases(false),
        m_recordValues(false),
        m_recordCoordinates(false),
        m_recordInitialPropertyValues(false),
        m_deviceStateIntervalMinutes(5),
        m_gpsLocationIntervalMinutes(0)
{
}


bool ParadataProperties::operator==(const ParadataProperties& rhs) const
{
    return ( m_collectionType == rhs.m_collectionType &&
             m_recordIteratorLoadCases == rhs.m_recordIteratorLoadCases &&
             m_recordValues == rhs.m_recordValues &&
             m_recordCoordinates == rhs.m_recordCoordinates &&
             m_recordInitialPropertyValues == rhs.m_recordInitialPropertyValues &&
             m_deviceStateIntervalMinutes == rhs.m_deviceStateIntervalMinutes &&
             m_gpsLocationIntervalMinutes == rhs.m_gpsLocationIntervalMinutes &&
             m_eventNames == rhs.m_eventNames );
}


void ParadataProperties::SetDeviceStateIntervalMinutes(const int minutes)
{
    m_deviceStateIntervalMinutes = Application::SetMinutesVariable(minutes, IntervalMinutesMinimum, IntervalMinutesMaximum);
}


void ParadataProperties::SetGpsLocationIntervalMinutes(const int minutes)
{
    m_gpsLocationIntervalMinutes = Application::SetMinutesVariable(minutes, IntervalMinutesMinimum, IntervalMinutesMaximum);
}


bool ParadataProperties::IncludeEvent(const CollectionType collection_type, const char* const event_name) const
{
    switch( collection_type )
    {
        case CollectionType::AllEvents:
        {
            return true;
        }

        case CollectionType::SomeEvents:
        {
            return ( std::find_if(m_eventNames.cbegin(), m_eventNames.cend(),
                                  [&](const std::string& this_event_name) { return ( this_event_name == event_name ); }) != m_eventNames.cend() );
        }

        default:
            ASSERT(collection_type == CollectionType::No);
            return false;
    }
}



// --------------------------------------------------------------------------
// serialization
// --------------------------------------------------------------------------

CREATE_ENUM_JSON_SERIALIZER(ParadataProperties::CollectionType,
    { ParadataProperties::CollectionType::No,         "none" },
    { ParadataProperties::CollectionType::SomeEvents, "partial" },
    { ParadataProperties::CollectionType::AllEvents,  "all" })


ParadataProperties ParadataProperties::CreateFromJson(const JsonNode& json_node)
{
    ParadataProperties paradata_properties;

    paradata_properties.m_collectionType = json_node.GetOrDefault(JK::collection, paradata_properties.m_collectionType);

    if( paradata_properties.m_collectionType == CollectionType::SomeEvents )
        paradata_properties.m_eventNames = json_node.GetArrayOrEmpty(JK::events).GetSet<std::string>();

    paradata_properties.m_recordCoordinates = json_node.GetOrDefault(JK::recordCoordinates, paradata_properties.m_recordCoordinates);
    paradata_properties.m_recordInitialPropertyValues = json_node.GetOrDefault(JK::recordInitialPropertyValues, paradata_properties.m_recordInitialPropertyValues);
    paradata_properties.m_recordIteratorLoadCases = json_node.GetOrDefault(JK::recordIteratorLoadCases, paradata_properties.m_recordIteratorLoadCases);
    paradata_properties.m_recordValues = json_node.GetOrDefault(JK::recordValues, paradata_properties.m_recordValues);

    paradata_properties.SetDeviceStateIntervalMinutes(json_node.GetOrDefault(JK::deviceStateIntervalMinutes, 0));
    paradata_properties.SetGpsLocationIntervalMinutes(json_node.GetOrDefault(JK::gpsLocationIntervalMinutes, 0));

    return paradata_properties;
}


void ParadataProperties::WriteJson(JsonWriter& json_writer) const
{
    json_writer.BeginObject();

    json_writer.Write(JK::collection, m_collectionType);

    if( m_collectionType == CollectionType::SomeEvents )
        json_writer.Write(JK::events, m_eventNames);

    json_writer.Write(JK::recordCoordinates, m_recordCoordinates)
               .Write(JK::recordInitialPropertyValues, m_recordInitialPropertyValues)
               .Write(JK::recordIteratorLoadCases, m_recordIteratorLoadCases)
               .Write(JK::recordValues, m_recordValues);

    json_writer.WriteIfNot(JK::deviceStateIntervalMinutes, m_deviceStateIntervalMinutes, 0)
               .WriteIfNot(JK::gpsLocationIntervalMinutes, m_gpsLocationIntervalMinutes, 0);

    json_writer.EndObject();
}


void ParadataProperties::serialize(Serializer& ar)
{
    ar.SerializeEnum(m_collectionType)
      & m_recordIteratorLoadCases
      & m_recordValues
      & m_recordCoordinates
      & m_recordInitialPropertyValues
      & m_deviceStateIntervalMinutes
      & m_gpsLocationIntervalMinutes
      & m_eventNames;
}
