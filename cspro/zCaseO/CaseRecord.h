#pragma once

#include <zCaseO/zCaseO.h>
#include <zCaseO/CaseItem.h>

class CaseAccess;
class CaseLevel;
class CaseLevelMetadata;
class CDictRecord;


// --------------------------------------------------------------------------
// CaseRecordMetadata
// --------------------------------------------------------------------------

class ZCASEO_API CaseRecordMetadata
{
    friend class CaseLevelMetadata;
    friend class CaseMetadata;
    friend class CaseRecord;

private:
    CaseRecordMetadata(const CaseLevelMetadata& case_level_metadata, const CDictRecord& dict_record, const CaseAccess& case_access,
                       size_t record_index, std::tuple<size_t&, size_t&, size_t&>& attribute_counter);

public:
    CaseRecordMetadata(const CaseRecordMetadata&) = delete;
    CaseRecordMetadata(CaseRecordMetadata&&) = default;
    ~CaseRecordMetadata();

    const CaseLevelMetadata& GetCaseLevelMetadata() const { return *m_caseLevelMetadata; }

    const CDictRecord& GetDictRecord() const { return m_dictRecord; }

    size_t GetRecordIndex() const      { return m_recordIndex; }
    size_t GetTotalRecordIndex() const { return m_totalRecordIndex; }

    bool IsIdRecord() const { return ( m_recordIndex == SIZE_MAX ); }

    // case items are all non-null
    const std::vector<const CaseItem*>& GetCaseItems() const { return m_caseItems; }

private:
    const CaseLevelMetadata* m_caseLevelMetadata; // non-null
    const CDictRecord& m_dictRecord;
    size_t m_recordIndex;
    size_t m_totalRecordIndex;
    std::vector<const CaseItem*> m_caseItems;
    std::vector<std::unique_ptr<const CaseItem>> m_caseItemsStorage;
    size_t m_recordSizeForMemoryAllocation;
};


// --------------------------------------------------------------------------
// CaseRecord
// --------------------------------------------------------------------------

class ZCASEO_API CaseRecord
{
    friend class CaseItem;

public:
    CaseRecord(CaseLevel& case_level, const CaseRecordMetadata& case_record_metadata);
    CaseRecord(const CaseRecord&) = delete;
    CaseRecord(CaseRecord&&) = default;
    ~CaseRecord();

    bool operator==(const CaseRecord& rhs) const;
    bool operator!=(const CaseRecord& rhs) const { return !operator==(rhs); }

    const CaseLevel& GetCaseLevel() const { return m_caseLevel; }
    CaseLevel& GetCaseLevel()             { return m_caseLevel; }

    const CaseRecordMetadata& GetCaseRecordMetadata() const { return m_caseRecordMetadata; }

    void Reset() { m_numberOccurrences = 0; }

    size_t GetNumberOccurrences() const { return m_numberOccurrences; }
    bool HasOccurrences() const         { return ( m_numberOccurrences > 0 ); }
    void SetNumberOccurrences(size_t number_occurrences);

    size_t GetNumberCaseItems() const                        { return m_caseRecordMetadata.m_caseItems.size(); }
    const std::vector<const CaseItem*>& GetCaseItems() const { return m_caseRecordMetadata.GetCaseItems(); }
    const CaseItem& GetCaseItem(size_t item_number) const    { return *(m_caseRecordMetadata.m_caseItems[item_number]); }

    CaseItemIndex GetCaseItemIndex(size_t record_occurrence = 0) const { return CaseItemIndex(*this, record_occurrence); }
    CaseItemIndex GetCaseItemIndex(size_t record_occurrence = 0)       { return CaseItemIndex(*this, record_occurrence); }

    // Copies the values from one record to another.
    void CopyValues(const CaseRecord& copy_case_record, size_t record_occurrence);

    // Gets the record data in binary form.
    std::vector<std::byte> GetBinaryValues(size_t record_occurrence) const;

    // Sets the record data in binary form
    void SetBinaryValues(size_t record_occurrence, const std::byte* binary_buffer);

    // Writes the record to JSON format.
    void WriteJson(JsonWriter& json_writer) const;

private:
    CaseLevel& m_caseLevel;
    const CaseRecordMetadata& m_caseRecordMetadata;

    std::vector<std::unique_ptr<std::byte[]>> m_recordData;
    size_t m_numberOccurrences;
};
