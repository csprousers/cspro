#pragma once

#include <zAppO/zAppO.h>

template<typename CharType> class JsonNode;
class JsonWriter;
class Serializer;


class ZAPPO_API ParadataProperties
{
public:
    enum class CollectionType { No, SomeEvents, AllEvents };

    ParadataProperties();

    bool operator==(const ParadataProperties& rhs) const;
    bool operator!=(const ParadataProperties& rhs) const { return !( *this == rhs ); }

    CollectionType GetCollectionType() const               { return m_collectionType; }
    void SetCollectionType(CollectionType collection_type) { m_collectionType = collection_type; }

    bool GetRecordIteratorLoadCases() const    { return m_recordIteratorLoadCases; }
    void SetRecordIteratorLoadCases(bool flag) { m_recordIteratorLoadCases = flag; }

    bool GetRecordValues() const    { return m_recordValues; }
    void SetRecordValues(bool flag) { m_recordValues = flag; }

    bool GetRecordCoordinates() const    { return m_recordCoordinates; }
    void SetRecordCoordinates(bool flag) { m_recordCoordinates = flag; }

    bool GetRecordInitialPropertyValues() const    { return m_recordInitialPropertyValues; }
    void SetRecordInitialPropertyValues(bool flag) { m_recordInitialPropertyValues = flag; }

    int GetDeviceStateIntervalMinutes() const { return m_deviceStateIntervalMinutes; }
    void SetDeviceStateIntervalMinutes(int minutes);

    int GetGpsLocationIntervalMinutes() const { return m_gpsLocationIntervalMinutes; }
    void SetGpsLocationIntervalMinutes(int minutes);

    const std::set<std::wstring>& GetEventNames() const    { return m_eventNames; }
    void SetEventNames(std::set<std::wstring> event_names) { m_eventNames = std::move(event_names); }

    bool IncludeEvent(CollectionType collection_type, const std::wstring& event_name) const;
    bool IncludeEvent(const std::wstring& event_name) const { return IncludeEvent(m_collectionType, event_name); }


    // serialization
    // -----------------------------------------------------
    static ParadataProperties CreateFromJson(const JsonNode<wchar_t>& json_node);
    void WriteJson(JsonWriter& json_writer) const;

    void serialize(Serializer& ar);


private:
    CollectionType m_collectionType;
    bool m_recordIteratorLoadCases;
    bool m_recordValues;
    bool m_recordCoordinates;
    bool m_recordInitialPropertyValues;
    int m_deviceStateIntervalMinutes;
    int m_gpsLocationIntervalMinutes;
    std::set<std::wstring> m_eventNames;
};
