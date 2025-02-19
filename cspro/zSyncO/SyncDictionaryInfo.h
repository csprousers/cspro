#pragma once

#include <zSyncO/zSyncO.h>


class SyncDictionaryInfo
{
private:
    SyncDictionaryInfo(std::string syncable_name, std::string dictionary_name, std::string label,
                       int case_count = -1, int64_t dictionary_modified_time = 0);

public:
    SyncDictionaryInfo(const CDataDict& dictionary, int case_count = -1);

    const std::string& GetSyncableName() const   { return m_syncableName; }
    const std::string& GetDictionaryName() const { return m_dictionaryName; }

    // returns the syncable name, followed by the dictionary name in parenthesis (when different)
    static std::string GetDisplayName(const std::string& syncable_name, const std::string& dictionary_name);
    std::string GetDisplayName() const { return GetDisplayName(m_syncableName, m_dictionaryName); }

    const std::string& GetLabel() const { return m_label; }

    int GetCaseCount() const { return m_caseCount; }

    // the dictionary modified time is only available when communicating with CSWeb and will be 0 otherwise
    int64_t GetDictionaryModifiedTime() const { return m_dictionaryModifiedTime; }

    // throws an exception if not valid
    SYNC_API static SyncDictionaryInfo CreateFromJson(const JsonNode& json_node);
    SYNC_API void WriteJson(JsonWriter& json_writer) const;

private:
    std::string m_syncableName;
    std::string m_dictionaryName;
    std::string m_label;
    int m_caseCount;
    int64_t m_dictionaryModifiedTime;
};



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

inline std::string SyncDictionaryInfo::GetDisplayName(const std::string& syncable_name, const std::string& dictionary_name)
{
    return ( syncable_name != dictionary_name ) ? SO::CreateParentheticalExpression(syncable_name, dictionary_name) :
                                                  syncable_name;
}
