#pragma once

#include <zEngineO/zEngineO.h>
#include <zLogicO/Symbol.h>
#include <zCaseO/Case.h>

struct EngineData;
class EngineDictionary;
class EngineRecord;
class Serializer;


class ZENGINEO_API EngineCase
{
    friend class EngineDictionary;
    friend class EngineRecord;

    // methods accessed by the friend classes
private:
    EngineCase(const EngineDictionary& engine_dictionary, EngineData& engine_data);

    std::unique_ptr<EngineCase> CloneInInitialState() const;

    Symbol* FindChildSymbol(const std::wstring& symbol_name) const;

    // register an EngineRecord as belonging to this EngineCase
    void RegisterEngineRecord(EngineRecord* engine_record) { m_engineRecords.emplace(engine_record); }

    void serialize(Serializer& ar);

public:
    const EngineDictionary& GetEngineDictionary() const { return m_engineDictionary; }


    // compilation methods
private:
    void AddEngineRecord(const CDictRecord& dict_record);


    // runtime methods
private:
    void InitializeRuntime(std::shared_ptr<CaseConstructionReporter> case_construction_reporter_override, bool initialize_records = true);

    void ResetCasePointers();

public:
    const Case& GetCase() const { return *m_case; }
    Case& GetCase()             { return *m_case; }

    const std::optional<CaseKey>& GetInitialCaseKey() const { return m_initialCaseKey; }
    const std::optional<CaseKey>& CalculateInitialCaseKey();

    CaseLevel* GetCurrentCaseLevel_base1(int level_number_base1)
    {
        ASSERT(m_currentCaseLevels[level_number_base1 - 1] != nullptr);
        return m_currentCaseLevels[level_number_base1 - 1];
    }

    void Reset();

    // these two methods are used for assigning a case to a user-defined function's parameter
    void ShareCase(EngineCase& engine_case);
    void CreateNewCase();

    // resets a case and, if part of a working storage dictionary, sets the records to
    // the maximum occurrences with the default values for each item
    void ClearCase();


private:
    EngineData& m_engineData;
    const EngineDictionary& m_engineDictionary;
    std::set<EngineRecord*> m_engineRecords;


    // runtime variables
    std::shared_ptr<Case> m_case;
    std::vector<CaseLevel*> m_currentCaseLevels;

    std::optional<CaseKey> m_initialCaseKey;
};
