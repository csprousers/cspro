#include "stdafx.h"
#include "EngineCase.h"
#include "EngineDictionary.h"
#include "EngineRecord.h"
#include <zDictO/DictionaryIterator.h>


EngineCase::EngineCase(const EngineDictionary& engine_dictionary, EngineData& engine_data)
    :   m_engineData(engine_data),
        m_engineDictionary(engine_dictionary)
{
    // automatically create the records only for dictionary objects
    if( m_engineDictionary.IsDictionaryObject() )
    {
        DictionaryIterator::Foreach<CDictRecord>(m_engineDictionary.GetDictionary(),
            [&](const CDictRecord& dict_record)
            {
                AddEngineRecord(dict_record);
            });
    }
}


std::unique_ptr<EngineCase> EngineCase::CloneInInitialState() const
{
    ASSERT(m_engineDictionary.IsCaseObject());

    auto engine_case = std::unique_ptr<EngineCase>(new EngineCase(m_engineDictionary, m_engineData));

    engine_case->m_engineRecords = m_engineRecords;

    // do not initialize the records because they will be shared with original case object
    engine_case->InitializeRuntime(m_case->GetSharedCaseConstructionReporter(), false);

    return engine_case;
}


void EngineCase::AddEngineRecord(const CDictRecord& dict_record)
{
    auto engine_record = std::make_shared<EngineRecord>(*this, dict_record, m_engineData);

    m_engineData.AddSymbol(engine_record, m_engineDictionary.IsDictionaryObject() ? Logic::SymbolTable::NameMapAddition::ToGlobalScope :
                                                                                    Logic::SymbolTable::NameMapAddition::DoNotAdd);

    // link the dictionary record for dictionary objects
    if( m_engineDictionary.IsDictionaryObject() )
        const_cast<CDictRecord&>(dict_record).SetSymbol(engine_record->GetSymbolIndex());

    RegisterEngineRecord(engine_record.get());
}


Symbol* EngineCase::FindChildSymbol(const std::wstring& symbol_name) const
{
    for( EngineRecord* engine_record : m_engineRecords )
    {
        // check if the record name matches
        if( SO::EqualsNoCase(symbol_name, engine_record->GetDictionaryRecord().GetName()) )
            return engine_record;

        // check if the name matches something underneath the record
        Symbol* child_symbol = engine_record->FindChildSymbol(symbol_name);

        if( child_symbol != nullptr )
            return child_symbol;
    }

    // if a symbol was not found for a case object, it is possible that it exists in the dictionary
    // but has not been added yet, so we must search the dictionary for it
    if( m_engineDictionary.IsCaseObject() )
    {
        const CDictRecord* dictionary_record_to_add = m_engineDictionary.GetDictionary().FindRecord(symbol_name);

        if( dictionary_record_to_add == nullptr )
        {
            const CDictItem* dictionary_item_to_add = m_engineDictionary.GetDictionary().FindItem(symbol_name);

            if( dictionary_item_to_add != nullptr )
                dictionary_record_to_add = dictionary_item_to_add->GetRecord();
        }

        if( dictionary_record_to_add != nullptr )
        {
            const_cast<EngineCase*>(this)->AddEngineRecord(*const_cast<CDictRecord*>(dictionary_record_to_add));

            // call the function again so the symbol is found [ENGINECR_TODO test once EngineItems exist]
            return FindChildSymbol(symbol_name);
        }
    }

    // no symbol found
    return nullptr;
}


void EngineCase::serialize(Serializer& /*ar*/)
{
    // ENGINECR_TODO serialization
}


void EngineCase::InitializeRuntime(std::shared_ptr<CaseConstructionReporter> case_construction_reporter_override,
    bool initialize_records/* = true*/)
{
    // create the case object
    bool use_default_case_construction_reporter = ( case_construction_reporter_override == nullptr );
    m_case = m_engineDictionary.GetCaseAccess()->CreateCase(use_default_case_construction_reporter);

    if( !use_default_case_construction_reporter )
        m_case->SetCaseConstructionReporter(std::move(case_construction_reporter_override));

    if( initialize_records )
    {
        // initialize each EngineRecord
        for( EngineRecord* engine_record : m_engineRecords )
            engine_record->InitializeRuntime();
    }

    ResetCasePointers();
}


void EngineCase::ResetCasePointers()
{
    m_currentCaseLevels.clear();

    // setup the current CaseLevel objects
    for( const CaseLevelMetadata* case_level_metadata : m_case->GetCaseMetadata().GetCaseLevelsMetadata() )
    {
        if( case_level_metadata->GetDictLevel().GetLevelNumber() == 0 )
        {
            m_currentCaseLevels.emplace_back(&m_case->GetRootCaseLevel());
        }

        else
        {
            ASSERT(false); // ENGINECR_TODO figure out how to handle multiple levels
            m_currentCaseLevels.emplace_back(nullptr);
        }
    }

    // reset the pointers for each EngineRecord
    for( EngineRecord* engine_record : m_engineRecords )
        engine_record->ResetCasePointers();
}


const std::optional<CaseKey>& EngineCase::CalculateInitialCaseKey()
{
    m_initialCaseKey = *m_case;

    return m_initialCaseKey;
}


void EngineCase::Reset()
{
    m_case->Reset();
    m_initialCaseKey.reset();
}


void EngineCase::ShareCase(EngineCase& engine_case)
{
    // when using a case as a user-defined function's argument, we can share the source case,
    // but we need to update any record/items references that are using the old case data
    ASSERT(m_engineDictionary.IsCaseObject());

    m_initialCaseKey = engine_case.m_initialCaseKey;

    m_case = engine_case.m_case;    

    ResetCasePointers();
}

void EngineCase::CreateNewCase()
{
    // when a case is a user-defined function parameter and is marked as optional but no argument
    // is provided, this method will be used to create a new case during the function call
    ASSERT(m_engineDictionary.IsCaseObject());

    m_initialCaseKey.reset();

    InitializeRuntime(m_case->GetSharedCaseConstructionReporter(), false);    
}


void EngineCase::ClearCase()
{
    Reset();

    // if working storage, reset all the record occurrences to the maximum values
    if( m_engineDictionary.GetSubType() != SymbolSubType::Work )
        return;

    ASSERT(m_currentCaseLevels.size() == 1);

    for( EngineRecord* engine_record : m_engineRecords )
    {
        CaseRecord& case_record = engine_record->GetCurrentCaseRecord();
        case_record.SetNumberOccurrences(engine_record->m_dictionaryRecord->GetMaxRecs());
        // ENGINECR_TODO as part of clear, set items to 0/""
    }
}
