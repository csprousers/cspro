#pragma once

#include <zCaseO/zCaseO.h>
#include <zCaseO/CaseRecord.h>

class Case;
class DictLevel;


// --------------------------------------------------------------------------
// CaseLevelMetadata
// --------------------------------------------------------------------------

class ZCASEO_API CaseLevelMetadata
{
    friend class CaseLevel;
    friend class CaseMetadata;

private:
    CaseLevelMetadata(const CaseMetadata& case_metadata, const DictLevel& dict_level, const CaseAccess& case_access,
                      std::tuple<size_t&, size_t&, size_t&>& attribute_counter);

public:
    CaseLevelMetadata(const CaseLevelMetadata&) = delete;
    CaseLevelMetadata(CaseLevelMetadata&&) = default;
    ~CaseLevelMetadata();

    const CaseMetadata& GetCaseMetadata() const { return *m_caseMetadata; }

    const DictLevel& GetDictLevel() const { return m_dictLevel; }

    size_t GetLevelKeyLength() const { return m_levelKeyLength; }

    const CaseRecordMetadata& GetIdCaseRecordMetadata() const { return m_idCaseRecordMetadata; }

    const std::vector<CaseRecordMetadata>& GetCaseRecordsMetadata() const { return m_caseRecordsMetadata; }

    const CaseRecordMetadata* FindCaseRecordMetadata(std::string_view record_name_sv) const;

    const CaseLevelMetadata* GetChildCaseLevelMetadata() const;

    // iterates over CaseRecordMetadata for the ID record and then each record
    template<typename CF>
    void ForeachCaseRecordMetadata(const CF& callback_function) const;

private:
    template<typename CF>
    void ForeachCaseRecordMetadata(const CF& callback_function);

private:
    const CaseMetadata* m_caseMetadata; // non-null
    const DictLevel& m_dictLevel;
    size_t m_levelKeyLength;
    CaseRecordMetadata m_idCaseRecordMetadata;
    std::vector<CaseRecordMetadata> m_caseRecordsMetadata;
};


// --------------------------------------------------------------------------
// CaseLevel
// --------------------------------------------------------------------------

class ZCASEO_API CaseLevel
{
public:
    CaseLevel(Case& data_case, const CaseLevelMetadata& case_level_metadata, CaseLevel* parent_case_level);
    CaseLevel(const CaseLevel&) = delete;
    ~CaseLevel();

    bool operator==(const CaseLevel& rhs) const;
    bool operator!=(const CaseLevel& rhs) const { return !operator==(rhs); }

    const Case& GetCase() const { return m_case; }
    Case& GetCase()             { return m_case; }

    const CaseLevelMetadata& GetCaseLevelMetadata() const { return m_caseLevelMetadata; }

    void Reset();

    const CaseLevel& GetParentCaseLevel() const;
    CaseLevel& GetParentCaseLevel() { return *const_cast<CaseLevel*>(&const_cast<const CaseLevel*>(this)->GetParentCaseLevel()); }

    size_t GetNumberChildCaseLevels() const                      { return m_numberChildCaseLevels; }
    const CaseLevel& GetChildCaseLevel(size_t level_index) const { return *(m_childCaseLevels[level_index]); }
    CaseLevel& GetChildCaseLevel(size_t level_index)             { return *(m_childCaseLevels[level_index]); }

    CaseLevel& AddChildCaseLevel();

    void RemoveChildCaseLevel(CaseLevel& child_case_level);

    const CaseRecord& GetIdCaseRecord() const { return m_idCaseRecord; }
    CaseRecord& GetIdCaseRecord()             { return m_idCaseRecord; }

    const size_t GetNumberCaseRecords() const                   { return m_caseRecords.size(); }
    const CaseRecord& GetCaseRecord(size_t record_number) const { return ( record_number == SIZE_MAX ) ? GetIdCaseRecord() : m_caseRecords[record_number]; }
    CaseRecord& GetCaseRecord(size_t record_number)             { return ( record_number == SIZE_MAX ) ? GetIdCaseRecord() : m_caseRecords[record_number]; }
    CaseRecord& GetCaseRecord(const CaseRecordMetadata& case_record_metadata);

    const CString& GetLevelKey() const;
    const CString& GetLevelIdentifier() const;
    void RecalculateLevelIdentifier(bool adjust_level_keys = true);

private:
    Case& m_case;
    const CaseLevelMetadata& m_caseLevelMetadata;

    CaseLevel* m_parentCaseLevel;

    std::vector<std::unique_ptr<CaseLevel>> m_childCaseLevels;
    size_t m_numberChildCaseLevels;

    CaseRecord m_idCaseRecord;
    std::vector<CaseRecord> m_caseRecords;

    mutable CString m_levelIdentifier;
};



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

template<typename CF>
void CaseLevelMetadata::ForeachCaseRecordMetadata(const CF& callback_function) const
{
    if( !CallbackFunctionProcessor::KeepProcessing(callback_function, m_idCaseRecordMetadata) )
        return;

    for( const CaseRecordMetadata& case_record_metadata : m_caseRecordsMetadata )
    {
        if( !CallbackFunctionProcessor::KeepProcessing(callback_function, case_record_metadata) )
            return;
    }
}


template<typename CF>
void CaseLevelMetadata::ForeachCaseRecordMetadata(const CF& callback_function)
{
    if( !CallbackFunctionProcessor::KeepProcessing(callback_function, m_idCaseRecordMetadata) )
        return;

    for( CaseRecordMetadata& case_record_metadata : m_caseRecordsMetadata )
    {
        if( !CallbackFunctionProcessor::KeepProcessing(callback_function, case_record_metadata) )
            return;
    }
}
