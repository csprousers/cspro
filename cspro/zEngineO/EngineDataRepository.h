#pragma once

#include <zEngineO/zEngineO.h>
#include <zUtilO/ConnectionString.h>
#include <zCaseO/CaseKey.h>
#include <zDataO/CaseIterator.h>

class DataRepository;
class EngineCase;
class EngineDictionary;
class Serializer;
class SystemMessageIssuer;


struct DictionaryAccessParameters
{
    CaseIterationMethod case_iteration_method = CaseIterationMethod::KeyOrder;
    CaseIterationOrder case_iteration_order = CaseIterationOrder::Ascending;
    CaseIterationCaseStatus case_iteration_status = CaseIterationCaseStatus::NotDeletedOnly;
};


class ZENGINEO_API EngineDataRepository
{
    friend class EngineDictionary;

    // methods accessed by the friend classes
private:
    EngineDataRepository(const EngineDictionary& engine_dictionary);

    std::unique_ptr<EngineDataRepository> CloneInInitialState() const;

    void serialize(Serializer& ar);

public:
    ~EngineDataRepository();

    const EngineDictionary& GetEngineDictionary() const { return m_engineDictionary; }

    // compilation flags that determine how a repository will be opened
    void ApplyPermissions(const EngineDataRepository& rhs_engine_data_repository);

    bool GetNeedsIndex() const { return m_needsIndex; }
    void SetNeedsIndex()       { m_needsIndex = true; }
         
    bool GetCannotHaveIndex() const { return m_cannotHaveIndex; }
    void SetCannotHaveIndex()       { m_cannotHaveIndex = true; }
         
    bool GetIsWriteable() const { return m_isWriteable; }
    void SetIsWriteable()       { m_isWriteable = true ;}
         
    bool GetHasDynamicFileManagement() const { return m_hasDynamicFileManagement; }
    void SetHasDynamicFileManagement()       { m_hasDynamicFileManagement = true; }
         
    bool GetUsesSync() const { return m_usesSync; }
    void SetUsesSync()       { m_usesSync = true; }


    // runtime methods (only CloseDataRepository and StopCaseIterator catch and log exceptions thrown by the data repository)
private:
    void InitializeRuntime(std::shared_ptr<SystemMessageIssuer> system_message_issuer);

public:
    DataRepository& GetDataRepository();
    const DataRepository& GetDataRepository() const;

    void SetDataRepository(std::shared_ptr<DataRepository> data_repository);
    void CloseDataRepository();

    const ConnectionString& GetLastClosedConnectionString() const { return m_lastClosedConnectionString; }

    const std::optional<CaseKey>& GetLastLoadedCaseKey() const                   { return m_lastLoadedCaseKey; }

    void ReadCase(EngineCase& engine_case, const CString& key);
    void ReadCase(EngineCase& engine_case, double position_in_repository);

    bool IsLastSearchedCaseKeyDefined() const                                    { return m_lastSearchedCaseKey.has_value(); }
    const std::optional<CaseKey>& GetLastSearchedCaseKey() const                 { return m_lastSearchedCaseKey; }
    void ClearLastSearchedKey()                                                  { m_lastSearchedCaseKey.reset(); }
    void SetLastSearchedCaseKey(const std::optional<CaseKey>& optional_case_key) { m_lastSearchedCaseKey = optional_case_key; }

    const DictionaryAccessParameters& GetDictionaryAccessParameters()            { return m_dictionaryAccessParameters; }
    DictionaryAccessParameters GetDictionaryAccessParameters(int dictionary_access) const;
    void SetDictionaryAccessParameters(std::variant<int, DictionaryAccessParameters> dictionary_access_or_parameters);

    enum class CaseIteratorStyle { FromBoundary, FromNextKey, FromLastSearchedCaseKey, FromCurrentPosition };
    void CreateCaseIterator(CaseIteratorStyle case_iterator_style, const std::optional<CaseKey>& starting_key = std::nullopt,
                            int dictionary_access = 0, std::optional<CString> key_prefix = std::nullopt,
                            CaseIterationContent iteration_content = CaseIterationContent::Case);

    bool IsCaseIteratorActive() const           { return ( m_caseIterator != nullptr ); }
    bool StepCaseIterator(EngineCase& engine_case);
    bool StepCaseKeyIterator(CaseKey& case_key) { ASSERT(IsCaseIteratorActive()); return m_caseIterator->NextCaseKey(case_key); }
    int GetCaseIteratorPercentRead() const      { ASSERT(IsCaseIteratorActive()); return m_caseIterator->GetPercentRead(); }
    void StopCaseIterator();


private:
    const EngineDictionary& m_engineDictionary;

    bool m_needsIndex;
    bool m_cannotHaveIndex;
    bool m_isWriteable;
    bool m_hasDynamicFileManagement;
    bool m_usesSync;

    // runtime variables
    std::shared_ptr<DataRepository> m_dataRepository;

    // the connection string for the last closed data repository
    ConnectionString m_lastClosedConnectionString;

    // the key of the last case loaded by the data repository
    std::optional<CaseKey> m_lastLoadedCaseKey;

    // the last key found by the find / locate functions
    std::optional<CaseKey> m_lastSearchedCaseKey;

	// case iterators
    DictionaryAccessParameters m_dictionaryAccessParameters;
    std::unique_ptr<CaseIterator> m_caseIterator;

    // for reporting errors
    std::shared_ptr<SystemMessageIssuer> m_systemMessageIssuer;
};
