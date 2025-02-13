#pragma once

#include <zEngineO/zEngineO.h>
#include <zEngineO/EngineCase.h>
#include <zEngineO/EngineDataRepository.h>
#include <zLogicO/Symbol.h>
#include <zCaseO/Case.h>

struct EngineData;
class SystemMessageIssuer;


// this class is for:
//      - dictionaries (that have a data repository and a case)
//      - cases (that only have a case)
//      - data repositories (that only have a data repository)

class ZENGINEO_API EngineDictionary : public Symbol
{
    friend class EngineDictionaryFactory;

public:
    enum class Contents { Dictionary, Case, DataRepository };

private:
    EngineDictionary(const EngineDictionary& engine_dictionary);

    EngineDictionary(std::wstring dictionary_name, EngineData& engine_data);

    EngineDictionary(Contents contents, std::wstring dictionary_name,
                     std::shared_ptr<const CDataDict> dictionary, std::shared_ptr<CaseAccess> case_access,
                     EngineData& engine_data);

public:
    ~EngineDictionary();

    Contents GetContents() const         { return m_contents; }
    bool IsDictionaryObject() const      { return ( m_contents == Contents::Dictionary ); }
    bool IsCaseObject() const            { return ( m_contents == Contents::Case ); }
    bool IsDataRepositoryObject() const  { return ( m_contents == Contents::DataRepository ); }

    bool HasEngineCase() const           { return ( m_contents != Contents::DataRepository ); }
    bool HasEngineDataRepository() const { return ( m_contents != Contents::Case ); }

    const CDataDict& GetDictionary() const                       { return *m_dictionary; }
    std::shared_ptr<const CDataDict> GetSharedDictionary() const { return m_dictionary; }

    CaseAccess* GetCaseAccess() const                       { return m_caseAccess.get(); }
    std::shared_ptr<CaseAccess> GetSharedCaseAccess() const { return m_caseAccess; }

    const EngineCase& GetEngineCase() const { ASSERT(HasEngineCase()); return *m_engineCase; }
    EngineCase& GetEngineCase()             { ASSERT(HasEngineCase()); return *m_engineCase; }

    const EngineDataRepository& GetEngineDataRepository() const { ASSERT(HasEngineDataRepository()); return *m_engineDataRepository; }
    EngineDataRepository& GetEngineDataRepository()             { ASSERT(HasEngineDataRepository()); return *m_engineDataRepository; }

    bool DictionaryMatches(const EngineDictionary& engine_dictionary) const { return ( m_dictionary == engine_dictionary.m_dictionary ); }


    // symbol overrides
    Symbol* FindChildSymbol(const std::wstring& symbol_name) const override;

    std::unique_ptr<Symbol> CloneInInitialState() const override;

    void Reset() override;

    void serialize_subclass(Serializer& ar) override;

protected:
    void WriteJsonMetadata_subclass(JsonWriter& json_writer) const override;

public:
    void WriteValueToJson(JsonWriter& json_writer) const override;
    // ENGINECR_TODO allow updating of the case with UpdateValueFromJson?


    // runtime methods
public:
    void InitializeRuntime(std::shared_ptr<SystemMessageIssuer> system_message_issuer,
                           std::shared_ptr<CaseConstructionReporter> case_construction_reporter_override,
                           std::shared_ptr<std::function<void(EngineDataRepository&)>> reset_override);


private:
    EngineData& m_engineData;
    Contents m_contents;

    std::shared_ptr<const CDataDict> m_dictionary;
    std::shared_ptr<CaseAccess> m_caseAccess;

    std::unique_ptr<EngineCase> m_engineCase;
    std::unique_ptr<EngineDataRepository> m_engineDataRepository;

    std::shared_ptr<std::function<void(EngineDataRepository&)>> m_resetOverride;
};
