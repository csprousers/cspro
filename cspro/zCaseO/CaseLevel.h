#pragma once

#include <zCaseO/zCaseO.h>
#include <zCaseO/CaseRecord.h>
#include <zCaseO/CaseAccess.h>
#include <zDictO/DDClass.h>

class CaseMetadata;
class Case;


class ZCASEO_API CaseLevelMetadata
{
    friend class CaseLevel;

public:
    CaseLevelMetadata(const CaseMetadata& case_metadata, const DictLevel& dict_level,
        const CaseAccess& case_access, std::tuple<size_t, size_t, size_t, size_t>& attribute_counter);
    CaseLevelMetadata(const CaseLevelMetadata&) = delete;
    ~CaseLevelMetadata();

    const CaseMetadata& GetCaseMetadata() const { return m_caseMetadata; }

    const DictLevel& GetDictLevel() const { return m_dictLevel; }

    size_t GetLevelKeyLength() const { return m_levelKeyLength; }

    const CaseRecordMetadata* GetIdCaseRecordMetadata() const { return m_idCaseRecordMetadata; }

    const std::vector<const CaseRecordMetadata*>& GetCaseRecordsMetadata() const { return m_caseRecordsMetadata; }

    const CaseRecordMetadata* FindCaseRecordMetadata(const CString& record_name) const;

    const CaseLevelMetadata* GetChildCaseLevelMetadata() const;

private:
    const CaseMetadata& m_caseMetadata;
    const DictLevel& m_dictLevel;
    size_t m_levelKeyLength;
    const CaseRecordMetadata* m_idCaseRecordMetadata;
    std::vector<const CaseRecordMetadata*> m_caseRecordsMetadata;
};


class ZCASEO_API CaseLevel
{
public:
    CaseLevel(Case& data_case, const CaseLevelMetadata& case_level_metadata, CaseLevel* parent_case_level);
    CaseLevel(const CaseLevel&) = delete;
    ~CaseLevel();

    bool operator==(const CaseLevel& rhs_case_level) const;

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

    const CaseRecord& GetIdCaseRecord() const { return *m_idCaseRecord; }
    CaseRecord& GetIdCaseRecord()             { return *m_idCaseRecord; }

    const size_t GetNumberCaseRecords() const                   { return m_caseRecords.size(); }
    const CaseRecord& GetCaseRecord(size_t record_number) const { return ( record_number == SIZE_MAX ) ? GetIdCaseRecord() : *(m_caseRecords[record_number]); }
    CaseRecord& GetCaseRecord(size_t record_number)             { return ( record_number == SIZE_MAX ) ? GetIdCaseRecord() : *(m_caseRecords[record_number]); }
    CaseRecord& GetCaseRecord(const CaseRecordMetadata& case_record_metadata);

    const CString& GetLevelKey() const;
    const CString& GetLevelIdentifier() const;
    void RecalculateLevelIdentifier(bool adjust_level_keys = true);

private:
    Case& m_case;
    const CaseLevelMetadata& m_caseLevelMetadata;

    CaseLevel* m_parentCaseLevel;

    std::vector<CaseLevel*> m_childCaseLevels;
    size_t m_numberChildCaseLevels;

    CaseRecord* m_idCaseRecord;
    std::vector<CaseRecord*> m_caseRecords;

    mutable CString m_levelIdentifier;
};
