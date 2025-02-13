#pragma once

#include <zCaseO/zCaseO.h>
#include <zCaseO/CaseAccess.h>
#include <zCaseO/CaseItem.h>
#include <zCaseO/CaseItemIndex.h>
#include <zDictO/DDClass.h>

class CaseLevelMetadata;
class CaseLevel;


class ZCASEO_API CaseRecordMetadata
{
    friend class CaseRecord;

public:
    CaseRecordMetadata(const CaseLevelMetadata& case_level_metadata, const CDictRecord& dictionary_record,
                       const CaseAccess& case_access, std::tuple<size_t, size_t, size_t, size_t>& attribute_counter);
    CaseRecordMetadata(const CaseRecordMetadata&) = delete;
    ~CaseRecordMetadata();

    const CaseLevelMetadata& GetCaseLevelMetadata() const { return m_caseLevelMetadata; }

    const CDictRecord& GetDictionaryRecord() const { return m_dictionaryRecord; }

    size_t GetRecordIndex() const { return m_recordIndex; }
    size_t GetTotalRecordIndex() const { return m_totalRecordIndex; }

    const std::vector<const CaseItem*>& GetCaseItems() const { return m_caseItems; }

private:
    const CaseLevelMetadata& m_caseLevelMetadata;
    const CDictRecord& m_dictionaryRecord;
    size_t m_recordIndex;
    size_t m_totalRecordIndex;
    std::vector<const CaseItem*> m_caseItems;
    size_t m_recordSizeForMemoryAllocation;
};


class ZCASEO_API CaseRecord
{
    friend class CaseItem;

public:
    CaseRecord(CaseLevel& case_level, const CaseRecordMetadata& case_record_metadata);
    CaseRecord(const CaseRecord&) = delete;
    ~CaseRecord();

    const CaseLevel& GetCaseLevel() const { return m_caseLevel; }
    CaseLevel& GetCaseLevel()             { return m_caseLevel; }

    const CaseRecordMetadata& GetCaseRecordMetadata() const { return m_caseRecordMetadata; }

    void Reset();

    size_t GetNumberOccurrences() const { return m_numberOccurrences; }
    bool HasOccurrences() const         { return ( m_numberOccurrences > 0 ); }
    void SetNumberOccurrences(size_t number_occurrences);

    size_t GetNumberCaseItems() const                        { return m_caseRecordMetadata.m_caseItems.size(); }
    const std::vector<const CaseItem*>& GetCaseItems() const { return m_caseRecordMetadata.GetCaseItems(); }
    const CaseItem& GetCaseItem(size_t item_number) const    { return *(m_caseRecordMetadata.m_caseItems[item_number]); }

    CaseItemIndex GetCaseItemIndex(size_t record_occurrence = 0) const { return CaseItemIndex(*this, record_occurrence); }
    CaseItemIndex GetCaseItemIndex(size_t record_occurrence = 0)       { return CaseItemIndex(*this, record_occurrence); }

    /// <summary>
    /// Copies the values from one record to another.
    /// </summary>
    void CopyValues(const CaseRecord& copy_case_record, size_t record_occurrence);

    /// <summary>
    /// Gets the record data in binary form.
    /// </summary>
    std::vector<std::byte> GetBinaryValues(size_t record_occurrence) const;

    /// <summary>
    /// Sets the record data in binary form
    /// </summary>
    void SetBinaryValues(size_t record_occurrence, const std::byte* binary_buffer);

    /// <summary>
    /// Writes the record to JSON format.
    /// </summary>
    void WriteJson(JsonWriter& json_writer) const;

private:
    CaseLevel& m_caseLevel;
    const CaseRecordMetadata& m_caseRecordMetadata;

    std::vector<std::byte*> m_recordData;
    size_t m_numberOccurrences;
};
