#pragma once

#include <zCaseO/zCaseO.h>
#include <zCaseO/CaseLevel.h>
#include <zCaseO/CaseRecord.h>
#include <zCaseO/CaseSummary.h>
#include <zCaseO/CaseAccess.h>
#include <zCaseO/CaseItem.h>
#include <zCaseO/Note.h>
#include <zCaseO/VectorClock.h>
#include <zDictO/DDClass.h>
#include <zCaseO/Pre74_Case.h>

class BinaryCaseItem;
class CaseConstructionReporter;
class CaseItemReference;


class ZCASEO_API CaseMetadata
{
    friend class Case;

public:
    CaseMetadata(const CDataDict& dictionary, const CaseAccess& case_access);
    CaseMetadata(const CaseMetadata&) = delete;
    ~CaseMetadata();

    const CDataDict& GetDictionary() const { return m_dictionary; }

    const std::vector<const CaseLevelMetadata*>& GetCaseLevelsMetadata() const { return m_caseLevelsMetadata; }

    const CaseLevelMetadata* FindCaseLevelMetadata(wstring_view level_name) const;
    const CaseRecordMetadata* FindCaseRecordMetadata(wstring_view record_name) const;
    const CaseItem* FindCaseItem(wstring_view item_name) const;

    size_t GetTotalNumberRecords() const         { return m_totalNumberRecords; }
    size_t GetTotalNumberCaseItems() const       { return m_totalNumberCaseItems; }
    size_t GetTotalNumberBinaryCaseItems() const { return m_totalNumberBinaryCaseItems; }

    bool UsesBinaryData() const { return ( m_totalNumberBinaryCaseItems != 0 ); }

private:
    const CDataDict& m_dictionary;
    std::vector<const CaseLevelMetadata*> m_caseLevelsMetadata;
    size_t m_totalNumberRecords;
    size_t m_totalNumberCaseItems;
    size_t m_totalNumberBinaryCaseItems;
};


class ZCASEO_API Case : public CaseSummary
{
public:
    Case(const CaseMetadata& case_metadata);
    Case(const Case&) = delete;
    ~Case();

    Case& operator=(const Case& rhs);

    const CaseMetadata& GetCaseMetadata() const { return m_caseMetadata; }

    void Reset();

    const CaseLevel& GetRootCaseLevel() const { return *m_rootCaseLevel; }
    CaseLevel& GetRootCaseLevel()             { return *m_rootCaseLevel; }

    const CString& GetKey() const override    { return m_rootCaseLevel->GetLevelIdentifier(); }

private:
    void SetKey(CString key) override;

public:
    // Iterates over each of the case's levels.
    template<typename CF>
    void ForeachCaseLevel(const CF& callback_function) const { return ForeachCaseLevelWorker<const CaseLevel>(callback_function, *m_rootCaseLevel); }
    template<typename CF>
    void ForeachCaseLevel(const CF& callback_function)       { return ForeachCaseLevelWorker<CaseLevel>(callback_function, *m_rootCaseLevel); }

    // Gets all of the case's levels.
    std::vector<const CaseLevel*> GetAllCaseLevels() const;
    std::vector<CaseLevel*> GetAllCaseLevels();

    // Adds a record occurrence to any record that is required but has no occurrences.
    void AddRequiredRecords(bool report_additions_using_case_construction_reporter = false);

    // case UUID
    const CString& GetUuid() const  { return m_uuid; }
    void SetUuid(std::wstring uuid) { m_uuid = WS2CS(uuid); }
    const CString& GetOrCreateUuid();

    // partial save status
    std::shared_ptr<CaseItemReference> GetSharedPartialSaveCaseItemReference() { return m_partialSaveCaseItemReference; }
    const CaseItemReference* GetPartialSaveCaseItemReference() const           { return m_partialSaveCaseItemReference.get(); }
    CaseItemReference* GetPartialSaveCaseItemReference()                       { return m_partialSaveCaseItemReference.get(); }

    void SetPartialSaveStatus(PartialSaveMode mode, std::shared_ptr<CaseItemReference> case_item_reference = nullptr);

    // Returns the case's notes.
    const std::vector<Note>& GetNotes() const { return m_notes; }
    std::vector<Note>& GetNotes()             { return m_notes; }
    void SetNotes(std::vector<Note> notes)    { m_notes = std::move(notes); }

    const CString& GetCaseNote() const override;

private:
    void SetCaseNote(CString case_note) override;

public:
    // Returns the vector clock for the case, which is only available for cases
    // in repositories that can synced (like the SQLite repository).
    const VectorClock& GetVectorClock() const            { return m_vectorClock; }
    VectorClock& GetVectorClock()                        { return m_vectorClock; }
    void SetVectorClock(const VectorClock& vector_clock) { m_vectorClock = vector_clock; }

    // Sets an object that can optionally receive reports issued by case construction operations.
    // Cases constructed using CaseAccess will be initialized with CaseAccess' case construction
    // reporter (if one has been set).
    void SetCaseConstructionReporter(std::shared_ptr<CaseConstructionReporter> case_construction_reporter) { m_caseConstructionReporter = std::move(case_construction_reporter); }

    CaseConstructionReporter* GetCaseConstructionReporter() const                       { return m_caseConstructionReporter.get(); }
    std::shared_ptr<CaseConstructionReporter> GetSharedCaseConstructionReporter() const { return m_caseConstructionReporter; }

    // Iterates over each defined BinaryCaseItem.
    void ForeachDefinedBinaryCaseItem(const std::function<void(const BinaryCaseItem&, const CaseItemIndex&)>& callback_function) const;
    void ForeachDefinedBinaryCaseItem(const std::function<void(const BinaryCaseItem&, CaseItemIndex&)>& callback_function);

    // Loads all binary data that is part of the case rather than relying on lazy loading.
    void LoadAllBinaryData();

    // Writes the case to JSON format.
    void WriteJson(JsonWriter& json_writer) const;

    // Parses the JSON, replacing the current case with the contents of the JSON node.
    void ParseJson(const JsonNode<wchar_t>& json_node);

private:
    template<typename T, typename CF>
    void ForeachCaseLevelWorker(const CF& callback_function, T& case_level) const;

    template<typename T>
    void GetAllCaseLevelsWorker(std::vector<T*>& case_levels, T& case_level) const;

    template<typename CF>
    void ForeachDefinedBinaryCaseItemWorker(const CF& callback_function) const;

private:
    const CaseMetadata& m_caseMetadata;
    CaseLevel* m_rootCaseLevel;

    CString m_uuid;
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
// inline implementations
// --------------------------------------------------------------------------

inline void Case::SetKey(CString key)
{
    // the key must be set by modifying case items directly
    throw ProgrammingErrorException();
}


inline void Case::SetPartialSaveStatus(PartialSaveMode mode, std::shared_ptr<CaseItemReference> case_item_reference/* = nullptr*/)
{
    ASSERT(( mode != PartialSaveMode::None ) == ( case_item_reference != nullptr ));

    SetPartialSaveMode(mode);
    m_partialSaveCaseItemReference = std::move(case_item_reference);
}


inline void Case::SetCaseNote(CString case_note)
{
    // the case note must be set by modifying the notes directly
    throw ProgrammingErrorException();
}


template<typename T, typename CF>
void Case::ForeachCaseLevelWorker(const CF& callback_function, T& case_level) const
{
    callback_function(case_level);

    for( size_t level_index = 0; level_index < case_level.GetNumberChildCaseLevels(); ++level_index )
        ForeachCaseLevelWorker(callback_function, case_level.GetChildCaseLevel(level_index));
}


template<typename T>
void Case::GetAllCaseLevelsWorker(std::vector<T*>& case_levels, T& case_level) const
{
    case_levels.emplace_back(&case_level);

    for( size_t level_index = 0; level_index < case_level.GetNumberChildCaseLevels(); ++level_index )
        GetAllCaseLevelsWorker(case_levels, case_level.GetChildCaseLevel(level_index));
}


inline std::vector<const CaseLevel*> Case::GetAllCaseLevels() const
{
    std::vector<const CaseLevel*> case_levels;
    GetAllCaseLevelsWorker<const CaseLevel>(case_levels, *m_rootCaseLevel);
    return case_levels;
}


inline std::vector<CaseLevel*> Case::GetAllCaseLevels()
{
    std::vector<CaseLevel*> case_levels;
    GetAllCaseLevelsWorker<CaseLevel>(case_levels, *m_rootCaseLevel);
    return case_levels;
}
