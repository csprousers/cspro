#pragma once

#include <zCaseO/zCaseO.h>
#include <zCaseO/CaseAccess.h>
#include <zCaseO/CaseLevel.h>
#include <zCaseO/CaseSummary.h>
#include <zCaseO/Note.h>
#include <zCaseO/Pre74_Case.h>
#include <zCaseO/VectorClock.h>
#include <zToolsO/CallbackFunctionProcessor.h>
#include <zDictO/DDClass.h>

class BinaryCaseItem;
class CaseConstructionReporter;
class CaseItemReference;


// --------------------------------------------------------------------------
// CaseMetadata
// --------------------------------------------------------------------------

class ZCASEO_API CaseMetadata
{
    friend class Case;

public:
    CaseMetadata(const CDataDict& dictionary, const CaseAccess& case_access);
    CaseMetadata(const CaseMetadata&) = delete;
    CaseMetadata(CaseMetadata&&) = delete;

    const CDataDict& GetDictionary() const { return m_dictionary; }

    const std::vector<CaseLevelMetadata>& GetCaseLevelsMetadata() const { return m_caseLevelsMetadata; }

    const CaseLevelMetadata* FindCaseLevelMetadata(std::string_view level_name_sv) const;
    const CaseRecordMetadata* FindCaseRecordMetadata(std::string_view record_name_sv) const;
    const CaseItem* FindCaseItem(std::string_view item_name_sv) const;

    size_t GetTotalNumberRecords() const         { return m_totalNumberRecords; }
    size_t GetTotalNumberCaseItems() const       { return m_totalNumberCaseItems; }
    size_t GetTotalNumberBinaryCaseItems() const { return m_totalNumberBinaryCaseItems; }

    bool UsesBinaryData() const { return ( m_totalNumberBinaryCaseItems != 0 ); }

private:
    const CDataDict& m_dictionary;
    std::vector<CaseLevelMetadata> m_caseLevelsMetadata;
    size_t m_totalNumberRecords;
    size_t m_totalNumberCaseItems;
    size_t m_totalNumberBinaryCaseItems;
};


// --------------------------------------------------------------------------
// Case
// --------------------------------------------------------------------------

class ZCASEO_API Case : public CaseSummary
{
public:
    Case(const CaseMetadata& case_metadata);
    Case(const Case&) = delete;
    ~Case();

    Case& operator=(const Case& rhs);

    // the position in the repository is not part of the comparison
    bool Equals(const Case& rhs, bool compare_vector_clock = true) const;
    bool operator==(const Case& rhs) const { return Equals(rhs); }
    bool operator!=(const Case& rhs) const { return !Equals(rhs); }

    const CaseMetadata& GetCaseMetadata() const { return m_caseMetadata; }

    void Reset();

    const CaseLevel& GetRootCaseLevel() const { return m_rootCaseLevel; }
    CaseLevel& GetRootCaseLevel()             { return m_rootCaseLevel; }

    const std::string& GetKey() const override { return UTF8_TODO::Create_Reference(m_rootCaseLevel.GetLevelIdentifier()); }

private:
    void SetKey(std::string key) override;

public:
    // Iterates over each of the case's levels.
    template<typename CF>
    void ForeachCaseLevel(const CF& callback_function) const { ForeachCaseLevelWorker<const CaseLevel>(callback_function, m_rootCaseLevel); }
    template<typename CF>
    void ForeachCaseLevel(const CF& callback_function)       { ForeachCaseLevelWorker<CaseLevel>(callback_function, m_rootCaseLevel); }

    // Gets all of the case's levels.
    std::vector<const CaseLevel*> GetAllCaseLevels() const;
    std::vector<CaseLevel*> GetAllCaseLevels();

    // Adds a record occurrence to any record that is required but has no occurrences.
    void AddRequiredRecords(bool report_additions_using_case_construction_reporter);

    // case UUID
    const std::string& GetUuid() const { return m_uuid; }
    void SetUuid(std::string uuid)     { m_uuid = std::move(uuid); }
    const std::string& GetOrCreateUuid();

    // partial save status
    std::shared_ptr<CaseItemReference> GetSharedPartialSaveCaseItemReference() { return m_partialSaveCaseItemReference; }
    const CaseItemReference* GetPartialSaveCaseItemReference() const           { return m_partialSaveCaseItemReference.get(); }
    CaseItemReference* GetPartialSaveCaseItemReference()                       { return m_partialSaveCaseItemReference.get(); }

    void SetPartialSaveStatus(PartialSaveMode mode, std::shared_ptr<CaseItemReference> case_item_reference = nullptr);

    // Returns the case's notes.
    const std::vector<Note>& GetNotes() const { return m_notes; }
    std::vector<Note>& GetNotes()             { return m_notes; }
    void SetNotes(std::vector<Note> notes)    { m_notes = std::move(notes); }

    const std::string& GetCaseNote() const override;

private:
    void SetCaseNote(std::string case_note) override;
    void ResetCaseNote() override;

public:
    // Returns the vector clock for the case, which is only available for cases
    // in repositories that can synced (like the SQLite repository).
    const VectorClock& GetVectorClock() const     { return m_vectorClock; }
    VectorClock& GetVectorClock()                 { return m_vectorClock; }
    void SetVectorClock(VectorClock vector_clock) { m_vectorClock = std::move(vector_clock); }

    // Sets an object that can optionally receive reports issued by case construction operations.
    // Cases constructed using CaseAccess will be initialized with CaseAccess' case construction
    // reporter (if one has been set).
    void SetCaseConstructionReporter(std::shared_ptr<CaseConstructionReporter> case_construction_reporter) { m_caseConstructionReporter = std::move(case_construction_reporter); }

    CaseConstructionReporter* GetCaseConstructionReporter() const                       { return m_caseConstructionReporter.get(); }
    std::shared_ptr<CaseConstructionReporter> GetSharedCaseConstructionReporter() const { return m_caseConstructionReporter; }

    // Iterates over each defined BinaryCaseItem.
    void ForeachDefinedBinaryCaseItem(const std::function<void(const BinaryCaseItem&, const CaseItemIndex&)>& callback_function) const;
    void ForeachDefinedBinaryCaseItem(const std::function<void(const BinaryCaseItem&, CaseItemIndex&)>& callback_function);

    // Indicates if binary data is defined in the case.
    bool HasDefinedBinaryData() const;

    // Loads all binary data that is part of the case rather than relying on lazy loading.
    void LoadAllBinaryData();

    // Writes the case to JSON format.
    void WriteJson(JsonWriter& json_writer) const;

    // Parses the JSON, replacing the current case with the contents of the JSON node.
    void ParseJson(const JsonNode& json_node);

private:
    template<typename T, typename CF>
    bool ForeachCaseLevelWorker(const CF& callback_function, T& case_level) const;

    template<typename T>
    void GetAllCaseLevelsWorker(std::vector<T*>& case_levels, T& case_level) const;

    template<typename CF>
    void ForeachDefinedBinaryCaseItemWorker(const CF& callback_function) const;

private:
    const CaseMetadata& m_caseMetadata;
    CaseLevel m_rootCaseLevel;

    std::string m_uuid;
    std::shared_ptr<CaseItemReference> m_partialSaveCaseItemReference;
    std::vector<Note> m_notes;
    VectorClock m_vectorClock;

    std::shared_ptr<CaseConstructionReporter> m_caseConstructionReporter;

public:
    // CR_TODO remove Pre74_Case stuff
    Pre74_Case* GetPre74_Case();
    const Pre74_Case* GetPre74_Case() const { return const_cast<Case*>(this)->GetPre74_Case(); }
    void ApplyPre74_Case(const Pre74_Case* pre74_case);
    void ApplyBinaryDataFor80(const Pre74_CaseLevel* pre74_case_level, const CaseLevel& case_level);
    std::unique_ptr<Pre74_Case> m_pre74Case;
    std::unique_ptr<class TextToCaseConverter> m_textToCaseConverter;
    bool m_recalculatePre74Case = true;
};



// --------------------------------------------------------------------------
// inline implementations, including CaseKey and CaseSummary copy and move
// constructors and assignment operators, which need to call the virtual
// methods to get they key and case note
// --------------------------------------------------------------------------

inline CaseKey::CaseKey(const Case& data_case)
    :   CaseKey(data_case.GetKey(), data_case.GetPositionInRepository())
{
}


inline CaseKey::CaseKey(Case&& data_case)
    :   CaseKey(static_cast<const Case&>(data_case))
{
}


inline CaseKey& CaseKey::operator=(const Case& data_case)
{
    m_key = data_case.GetKey();
    m_positionInRepository = data_case.GetPositionInRepository();
    return *this;
}


inline CaseKey& CaseKey::operator=(Case&& data_case)
{
    return operator=(static_cast<const Case&>(data_case));
}


inline CaseSummary::CaseSummary(const Case& data_case)
    :   CaseSummary(static_cast<const CaseSummary&>(data_case))
{
    m_key = data_case.GetKey();
    m_caseNote = data_case.GetCaseNote();
}


inline CaseSummary::CaseSummary(Case&& data_case)
    :   CaseSummary(static_cast<CaseSummary&&>(std::move(data_case)))
{
    m_key = data_case.GetKey();
    m_caseNote = data_case.GetCaseNote();
}


inline CaseSummary& CaseSummary::operator=(const Case& data_case)
{
    operator=(static_cast<const CaseSummary&>(data_case));
    m_key = data_case.GetKey();
    m_caseNote = data_case.GetCaseNote();
    return *this;
}


inline CaseSummary& CaseSummary::operator=(Case&& data_case)
{
    operator=(static_cast<CaseSummary&&>(std::move(data_case)));
    m_key = data_case.GetKey();
    m_caseNote = data_case.GetCaseNote();
    return *this;
}


template<typename T, typename CF>
bool Case::ForeachCaseLevelWorker(const CF& callback_function, T& case_level) const
{
    if( !CallbackFunctionProcessor::KeepProcessing(callback_function, case_level) )
        return false;

    for( size_t level_index = 0; level_index < case_level.GetNumberChildCaseLevels(); ++level_index )
    {
        if( !ForeachCaseLevelWorker(callback_function, case_level.GetChildCaseLevel(level_index)) )
            return false;
    }

    return true;
}
