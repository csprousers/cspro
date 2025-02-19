#include "stdafx.h"
#include "SyncDictionaryInfo.h"
#include <zDictO/DDClass.h>


CREATE_JSON_KEY(caseCount)
CREATE_JSON_KEY(dictionaryName)


SyncDictionaryInfo::SyncDictionaryInfo(std::string syncable_name, std::string dictionary_name, std::string label,
                                       const int case_count/* = -1*/, const int64_t dictionary_modified_time/* = 0*/)
    :   m_syncableName(std::move(syncable_name)),
        m_dictionaryName(std::move(dictionary_name)),
        m_label(std::move(label)),
        m_caseCount(case_count),
        m_dictionaryModifiedTime(dictionary_modified_time)
{
}


SyncDictionaryInfo::SyncDictionaryInfo(const CDataDict& dictionary, const int case_count/* = -1*/)
    :   SyncDictionaryInfo(dictionary.GetSyncableName(), dictionary.GetName(), UTF8_TODO::GetUtf8(dictionary.GetLabel()), case_count)
{
}


SyncDictionaryInfo SyncDictionaryInfo::CreateFromJson(const JsonNode& json_node)
{
    std::string syncable_name = json_node.Get<std::string>(JK::name);
    std::string dictionary_name = json_node.GetOrConstruct<std::string>(JK::dictionaryName);

    // prior to CSPro 8.1, no dictionary name, as opposed to the syncable name, was defined
    if( dictionary_name.empty() )
        dictionary_name = syncable_name;

    return SyncDictionaryInfo(std::move(syncable_name),
                              std::move(dictionary_name),
                              json_node.Get<std::string>(JK::label),
                              json_node.GetOrDefault<int>(JK::caseCount, -1),
                              json_node.Contains(JK::modifiedTime) ? json_node.GetDate(JK::modifiedTime) : 0);
}


void SyncDictionaryInfo::WriteJson(JsonWriter& json_writer) const
{
    json_writer.BeginObject()
               .Write(JK::name, m_syncableName)
               .Write(JK::dictionaryName, m_dictionaryName)
               .Write(JK::label, m_label);

    if( m_caseCount >= 0 )
        json_writer.Write(JK::caseCount, m_caseCount);

    json_writer.WriteIfNot(JK::modifiedTime, m_dictionaryModifiedTime, int64_t(0));

    json_writer.EndObject();
}
