#pragma once

#include <zEngineO/zEngineO.h>
#include <zLogicO/Symbol.h>
#include <zDictO/DDClass.h>
#include <zCaseO/Case.h>

class EngineCase;
struct EngineData;
class EngineDictionary;


class ZENGINEO_API EngineRecord : public Symbol
{
    friend class EngineCase;
    friend class EngineDictionaryFactory;

private:
    EngineRecord(std::wstring record_name, EngineData& engine_data);

public:
    EngineRecord(EngineCase& engine_case, const CDictRecord& dictionary_record, EngineData& engine_data);

    const CDictRecord& GetDictionaryRecord() const { return *m_dictionaryRecord; }

    const EngineDictionary& GetEngineDictionary() const;


    // symbol overrides
    Symbol* FindChildSymbol(const std::wstring& symbol_name) const override;

    std::unique_ptr<Symbol> CloneInInitialState() const override;

    void serialize_subclass(Serializer& ar) override;

protected:
    void WriteJsonMetadata_subclass(JsonWriter& json_writer) const override;

public:
    void WriteValueToJson(JsonWriter& json_writer) const override;
    // ENGINECR_TODO allow updating of the record with UpdateValueFromJson?


    // runtime methods
private:
    void InitializeRuntime();

    void ResetCasePointers();

    const CaseRecord& GetCurrentCaseRecord() const;
    CaseRecord& GetCurrentCaseRecord() { return const_cast<CaseRecord&>(const_cast<const EngineRecord*>(this)->GetCurrentCaseRecord()); }

public:
    unsigned GetNumberOccurrences() const;


private:
    EngineData& m_engineData;
    EngineCase* m_engineCase;
    const CDictRecord* m_dictionaryRecord;


    // runtime variables
    const CaseRecordMetadata* m_caseRecordMetadata;
    size_t m_levelNumber;
    size_t m_recordIndex;
    CaseRecord* m_caseRecordIfOnRootCaseLevel;
};
