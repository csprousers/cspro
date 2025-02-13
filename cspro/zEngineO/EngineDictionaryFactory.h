#pragma once

#include <zEngineO/EngineDictionary.h>
#include <zEngineO/EngineRecord.h>


class EngineDictionaryFactory
{
public:
    // Create a dictionary with all of the record/item symbols added immediately.
    static std::unique_ptr<EngineDictionary> CreateDictionary(std::shared_ptr<const CDataDict> dictionary,
                                                              EngineData& engine_data);

    // Create a case with the record/item symbols only added as needed. The dictionary and case access will come
    // from the passed EngineDictionary.
    static std::unique_ptr<EngineDictionary> CreateCase(std::wstring case_name, const EngineDictionary& base_engine_dictionary,
                                                        EngineData& engine_data);

    // Create a (DataSource) data repository. The dictionary and case access will come from the passed EngineDictionary.
    static std::unique_ptr<EngineDictionary> CreateDataRepository(std::wstring datasource_name, const EngineDictionary& base_engine_dictionary,
                                                                  EngineData& engine_data);

    // Create a dictionary symbol on deserialization.
    static std::unique_ptr<Symbol> CreateSymbolOnDeserialization(std::wstring symbol_name, SymbolType symbol_type,
                                                                 EngineData& engine_data);

    // Construct a name for EngineRecord and EngineItem objects to be added to the symbol table.
    template<typename T>
    static std::wstring GetSymbolName(const EngineCase& engine_case, const T& dictionary_record_or_item);
};



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

inline std::unique_ptr<EngineDictionary> EngineDictionaryFactory::CreateDictionary(std::shared_ptr<const CDataDict> dictionary,
                                                                                   EngineData& engine_data)
{
    ASSERT(dictionary != nullptr);

    auto case_access = std::make_unique<CaseAccess>(*dictionary);

    return std::unique_ptr<EngineDictionary>(new EngineDictionary(
        EngineDictionary::Contents::Dictionary,
        CS2WS(dictionary->GetName()),
        dictionary,
        std::move(case_access),
        engine_data));
}


inline std::unique_ptr<EngineDictionary> EngineDictionaryFactory::CreateCase(std::wstring case_name, const EngineDictionary& base_engine_dictionary,
                                                                             EngineData& engine_data)
{
    std::unique_ptr<EngineDictionary> engine_dictionary(new EngineDictionary(
        EngineDictionary::Contents::Case,
        std::move(case_name),
        base_engine_dictionary.GetSharedDictionary(),
        base_engine_dictionary.GetSharedCaseAccess(),
        engine_data));

    engine_dictionary->SetSubType(SymbolSubType::External);

    return engine_dictionary;
}



inline std::unique_ptr<EngineDictionary> EngineDictionaryFactory::CreateDataRepository(std::wstring datasource_name, const EngineDictionary& base_engine_dictionary,
                                                                                       EngineData& engine_data)
{
    std::unique_ptr<EngineDictionary> engine_dictionary(new EngineDictionary(
        EngineDictionary::Contents::DataRepository,
        std::move(datasource_name),
        base_engine_dictionary.GetSharedDictionary(),
        base_engine_dictionary.GetSharedCaseAccess(),
        engine_data));

    engine_dictionary->SetSubType(SymbolSubType::External);

    return engine_dictionary;
}


// Create a dictionary symbol on deserialization.
inline std::unique_ptr<Symbol> EngineDictionaryFactory::CreateSymbolOnDeserialization(std::wstring symbol_name, SymbolType symbol_type,
                                                                                      EngineData& engine_data)
{
    if( symbol_type == SymbolType::Dictionary )
    {
        return std::unique_ptr<EngineDictionary>(new EngineDictionary(std::move(symbol_name), engine_data));
    }

    else
    {
        ASSERT(symbol_type == SymbolType::Record);
        return std::unique_ptr<EngineRecord>(new EngineRecord(std::move(symbol_name), engine_data));
    }             
}


template<typename T>
std::wstring EngineDictionaryFactory::GetSymbolName(const EngineCase& engine_case, const T& dictionary_record_or_item)
{
    if( engine_case.GetEngineDictionary().IsDictionaryObject() )
    {
        return CS2WS(dictionary_record_or_item.GetName());
    }

    else
    {
        return FormatTextCS2WS(_T("%s.%s"), engine_case.GetEngineDictionary().GetName().c_str(),
                                            dictionary_record_or_item.GetName().GetString());
    }
}
