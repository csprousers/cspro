#include "stdafx.h"
#include "EngineRecord.h"
#include "EngineCase.h"
#include "EngineDictionary.h"
#include "EngineDictionaryFactory.h"


EngineRecord::EngineRecord(std::wstring record_name, EngineData& engine_data)
    :   Symbol(std::move(record_name), SymbolType::Record),
        m_engineData(engine_data),
        m_engineCase(nullptr),
        m_dictionaryRecord(nullptr),
        m_caseRecordMetadata(nullptr),
        m_levelNumber(SIZE_MAX),
        m_recordIndex(SIZE_MAX),
        m_caseRecordIfOnRootCaseLevel(nullptr)
{
}


EngineRecord::EngineRecord(EngineCase& engine_case, const CDictRecord& dictionary_record, EngineData& engine_data)
    :   EngineRecord(EngineDictionaryFactory::GetSymbolName(engine_case, dictionary_record), engine_data)
{
    m_engineCase = &engine_case;
    m_dictionaryRecord = &dictionary_record;
}


std::unique_ptr<Symbol> EngineRecord::CloneInInitialState() const
{
    // EngineRecord objects added as part of an EngineCase that is used in a recursive function call
    // do not need to be cloned because EngineCase::ResetCasePointers will properly reassign
    // the applicable case data to the single set of EngineRecord objects
    return nullptr;
}


const EngineDictionary& EngineRecord::GetEngineDictionary() const
{
    ASSERT(m_engineCase != nullptr);
    return m_engineCase->GetEngineDictionary();
}


Symbol* EngineRecord::FindChildSymbol(const std::wstring& /*symbol_name*/) const
{
#ifdef ENGINECR_TODO // hook this up when the EngineItems are stored
    for( Symbol* symbol : m_symbolTable.FindSymbols(symbol_name) )
    {
        // only items and value sets can be children of the record
        if( symbol->IsOneOf(SymbolType::Variable, SymbolType::ValueSet) )
        {
            const VART* pVarT = symbol->IsA(SymbolType::Variable) ? assert_cast<const VART*>(symbol) :
                                                                    assert_cast<const ValueSet*>(symbol)->GetVarT());

            if( pVarT != nullptr &&
                pVarT->GetDictItem() != nullptr &&
                pVarT->GetDictItem()->GetRecord() == &m_dictionaryRecord )
            {
                return symbol;
            }
        }
    }
#endif

    return nullptr;
}


void EngineRecord::serialize_subclass(Serializer& ar)
{
    // no need to serialize anything for dictionary objects
    if( m_engineCase != nullptr && m_engineCase->GetEngineDictionary().IsDictionaryObject() )
        return;

    // link to the EngineCase and dictionary record
    if( ar.IsSaving() )
    {
        ar.Write<int>(m_engineCase->GetEngineDictionary().GetSymbolIndex());
        ar.Write<int>(m_dictionaryRecord->GetSymbol());
    }

    else
    {
        m_engineCase = &assert_cast<EngineDictionary&>(m_engineData.symbol_table.GetAt(ar.Read<int>())).GetEngineCase();
        m_engineCase->RegisterEngineRecord(this);

        // the dictionary record's symbol is from the EngineRecord of the dictionary object
        m_dictionaryRecord = assert_cast<EngineRecord&>(m_engineData.symbol_table.GetAt(ar.Read<int>())).m_dictionaryRecord;
    }
}


void EngineRecord::InitializeRuntime()
{
    ASSERT(m_engineCase != nullptr && m_dictionaryRecord != nullptr);

    m_caseRecordMetadata = m_engineCase->GetCase().GetCaseMetadata().FindCaseRecordMetadata(m_dictionaryRecord->GetName());

    ASSERT(m_caseRecordMetadata != nullptr && m_dictionaryRecord == &m_caseRecordMetadata->GetDictionaryRecord());

    m_levelNumber = m_caseRecordMetadata->GetCaseLevelMetadata().GetDictLevel().GetLevelNumber();
    m_recordIndex = m_caseRecordMetadata->GetRecordIndex();
}


void EngineRecord::ResetCasePointers()
{
    // if this is on the root level, get the CaseRecord; otherwise when working with this record
    // we will have to evaluate the current CaseLevel
    if( m_levelNumber == 0 )
        m_caseRecordIfOnRootCaseLevel = &m_engineCase->m_currentCaseLevels.front()->GetCaseRecord(m_recordIndex);

    // ENGINECR_TODO call ResetCasePointers for EngineItems
}


const CaseRecord& EngineRecord::GetCurrentCaseRecord() const
{
    if( m_levelNumber == 0 )
    {
        return *m_caseRecordIfOnRootCaseLevel;
    }

    else
    {
        const CaseLevel* case_level = m_engineCase->m_currentCaseLevels[m_levelNumber];
        ASSERT(case_level != nullptr);
        return case_level->GetCaseRecord(m_recordIndex);
    }
}


unsigned EngineRecord::GetNumberOccurrences() const
{
    // ENGINECR_TODO if this is the approach we keep, perhaps we should have a variable pointing directly
    // to the CaseRecord and then, if processing multiple cases (like in a multi-threaded batch app), we have
    // a method like SetCaseLevel(...) that will reset values to the proper pointers, or those could be
    // set only as needed


    // ENGINECR_TODO should methods like this return size_t or should we change all indexing stuff to use unsigned?
    return GetCurrentCaseRecord().GetNumberOccurrences();
}



// --------------------------------------------------------------------------
// JSON serialization
// --------------------------------------------------------------------------

void EngineRecord::WriteJsonMetadata_subclass(JsonWriter& json_writer) const
{
    ASSERT(m_engineCase != nullptr);

    json_writer.Write(JK::record, *m_dictionaryRecord);
}


void EngineRecord::WriteValueToJson(JsonWriter& json_writer) const
{
    ASSERT(m_engineCase != nullptr);

    // ENGINECR_TODO call CIntDriver::IsDataAccessible to make sure the case data is available (see CSymbolDict::WriteValueToJson)

    json_writer.Write(GetCurrentCaseRecord());
}
