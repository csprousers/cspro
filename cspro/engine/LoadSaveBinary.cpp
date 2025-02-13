#include "StandardSystemIncludes.h"
#include "Exappl.h"
#include "COMPILAD.H"
#include "Engine.h"
#include <zEngineO/AllSymbols.h>
#include <zEngineO/Imputation.h>
#include <zEngineO/Versioning.h>
#include <zToolsO/Serializer.h>
#include <zAppO/Application.h>
#include <zFreqO/Frequency.h>


void CEngineDriver::serialize(Serializer& ar)
{
    ar & m_bHasOutputDict;
    ar & m_bHasSomeInsDelSortOcc;
}


size_t CEngineDriver::LoadBaseSymbols(Serializer& ar)
{
    size_t symbol_table_size = ar.Read<size_t>();

    for( size_t i = 1; i < symbol_table_size; ++i )
    {
        std::wstring symbol_name = ar.Read<std::wstring>();

        SymbolType symbol_type;
        ar.SerializeEnum(symbol_type);

        SymbolSubType symbol_subtype;
        ar.SerializeEnum(symbol_subtype);

        int symbol_index = ar.Read<int>();
        ASSERT(symbol_index == static_cast<int>(i));

        // see if the symbol should be added to the global namespace
        Logic::SymbolTable::NameMapAddition name_map_addition = Logic::SymbolTable::NameMapAddition::DoNotAdd;

        if( ar.PredatesVersionIteration(Serializer::Iteration_8_0_000_1) || ar.Read<bool>() )
            name_map_addition = Logic::SymbolTable::NameMapAddition::ToGlobalScope;

        if( i < GetSymbolTable().GetTableSize() )
        {
            ASSERT(symbol_name == NPT_Ref(i).GetName());
        }

        else
        {
            // add any symbols created during logic compilation
            std::unique_ptr<Symbol> symbol = m_pEngineArea->CreateSymbol(std::move(symbol_name), symbol_type, symbol_subtype);
            ASSERT(symbol != nullptr);
            m_engineData->AddSymbol(std::move(symbol), name_map_addition);
        }
    }

    return symbol_table_size;
}


// LoadCompiledBinary()
//   Iterates through binary Archive and [hopefully] loads relevant data ...
//
void CEngineDriver::LoadCompiledBinary()
{
    Serializer& ar = APP_LOAD_TODO_GetArchive();

    Versioning::SetCompiledLogicVersion(ar.GetArchiveVersion());

    ar >> *this;

    // first deserialize the base symbols
    size_t symbol_table_size = LoadBaseSymbols(ar);

    if( ar.MeetsVersionIteration(Serializer::Iteration_8_0_000_1) )
    {
        // then deserialize the subclasses
        for( size_t i = Logic::SymbolTable::FirstValidSymbolIndex; i < symbol_table_size; ++i )
            NPT_Ref(i).serialize_subclass(ar);

        m_pIntDriver->AllocExecTables();

        // deserialize other engine data
        ar >> m_engineData->numeric_constants;
        ar >> m_engineData->string_literals;

        ar >> m_engineData->frequencies;
        Imputation::serialize(ar, *m_engineData);

        ar >> m_engineData->logic_byte_code;
        ar >> m_engineData->runtime_events_processor;
    }

    else
    {
        // prior to 8.0, subclasses were deserialized in a specific order
        auto deserialize_symbols = [&](SymbolType symbol_type)
        {
            for( size_t i = Logic::SymbolTable::FirstValidSymbolIndex; i < symbol_table_size; ++i )
            {
                Symbol& symbol = NPT_Ref(i);

                if( symbol.IsA(symbol_type) )
                    symbol.serialize_subclass(ar);
                    
            }
        };

        deserialize_symbols(SymbolType::Variable);
        deserialize_symbols(SymbolType::Group);
        deserialize_symbols(SymbolType::ValueSet);
        deserialize_symbols(SymbolType::WorkVariable);
        deserialize_symbols(SymbolType::Array);
        deserialize_symbols(SymbolType::UserFunction);
        deserialize_symbols(SymbolType::Relation);
        deserialize_symbols(SymbolType::File);
        deserialize_symbols(SymbolType::Pre80Dictionary);

        ar >> m_engineData->numeric_constants;
        ar >> m_engineData->string_literals;

        if( ar.MeetsVersionIteration(Serializer::Iteration_7_6_000_1) )
        {
            ar >> m_engineData->frequencies;
            Imputation::serialize(ar, *m_engineData);
        }

        m_pIntDriver->AllocExecTables();

        ar >> m_engineData->logic_byte_code;

        InitCompiledWorkDict();

        ar >> m_engineData->runtime_events_processor;

        deserialize_symbols(SymbolType::List);
        deserialize_symbols(SymbolType::Block);
        deserialize_symbols(SymbolType::Map);
        deserialize_symbols(SymbolType::Pff);
        deserialize_symbols(SymbolType::SystemApp);
        deserialize_symbols(SymbolType::Audio);
        deserialize_symbols(SymbolType::HashMap);
        deserialize_symbols(SymbolType::NamedFrequency);
        deserialize_symbols(SymbolType::WorkString);
        deserialize_symbols(SymbolType::Image);
        deserialize_symbols(SymbolType::Document);
        deserialize_symbols(SymbolType::Geometry);
        deserialize_symbols(SymbolType::Report);
    }

    m_bBinaryLoaded = true;
}


void CEngineDriver::SaveCompiledBinary()
{
    ASSERT(BinaryGen::isGeneratingBinary());

    Serializer& ar = APP_LOAD_TODO_GetArchive();

    ar << *this;

    // serialize the symbol table...
    size_t symbol_table_size = GetSymbolTable().GetTableSize();
    ar << symbol_table_size;

    // first serialize the base symbols
    for( size_t i = Logic::SymbolTable::FirstValidSymbolIndex; i < symbol_table_size; ++i )
    {
        const Symbol& symbol = NPT_Ref(i);
        ar << symbol;

        // indicate if the symbol should be added to the global namespace
        ar.Write<bool>(GetSymbolTable().NameExists(symbol.GetName()));
    }

    // then serialize the subclasses
    for( size_t i = Logic::SymbolTable::FirstValidSymbolIndex; i < symbol_table_size; ++i )
        NPT_Ref(i).serialize_subclass(ar);

    // serialize other engine data
    ar << m_engineData->numeric_constants;
    ar << m_engineData->string_literals;

    ar << m_engineData->frequencies;
    Imputation::serialize(ar, *m_engineData);

    ar << m_engineData->logic_byte_code;

    ar << m_engineData->runtime_events_processor;
}


void CEngineDriver::InitCompiledWorkDict()
{
    static_assert(Serializer::GetEarliestSupportedVersion() < Serializer::Iteration_7_6_000_1);
    ASSERT(Workdict != nullptr);

    // set up the working sections
    std::vector<SECT*> working_sections;

    for( SECT* pSecT : m_engineData->sections )
    {
        if( pSecT->SYMTowner == -1 )
        {
            ASSERT(Versioning::PredatesCompiledLogicVersion(Serializer::Iteration_7_6_000_1));
            ASSERT(pSecT->GetSubType() == SymbolSubType::Work);
            ASSERT(SO::StartsWith(pSecT->GetName(), _T("WRK_")));

            working_sections.emplace_back(pSecT);

            pSecT->SetMinOccs(1);
            pSecT->SetMaxOccs(1);
            pSecT->SetLevel(0);
            pSecT->SetSpecialSection(true);
            pSecT->SYMTowner = Workdict->GetSymbolIndex();
        }
    }

    if( working_sections.empty() )
        return;

    // add the working variables to each working section
    auto working_section_itr = working_sections.begin();

    for( VART* pVarT : m_engineData->variables )
    {
        if( pVarT->SYMTowner == -1 )
        {
            ASSERT(Versioning::PredatesCompiledLogicVersion(Serializer::Iteration_7_6_000_1));
            ASSERT(pVarT->GetSubType() == SymbolSubType::Work);

            // potentially move to the next section
            while( (*working_section_itr)->GetLastLoc() >= SECT::MAX_WORKSECLEN )
            {
                working_section_itr++;
                ASSERT(working_section_itr != working_sections.end());
            }

            SECT* pSecT = *working_section_itr;
            pVarT->SYMTowner = pSecT->GetSymbolIndex();
            pVarT->SetSPT(pSecT);
            pSecT->AddVariable(pVarT);
        }
    }

    // chain the working sections
    for( SECT* pSecT : working_sections )
        m_pEngineArea->ChainSymbol(Workdict->SYMTlsec, pSecT);
}
