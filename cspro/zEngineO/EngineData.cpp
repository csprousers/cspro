#include "stdafx.h"
#include "EngineData.h"
#include "AllSymbols.h"
#include <engine/Ctab.h>


EngineData::EngineData(std::shared_ptr<EngineAccessor> engine_accessor_)
    :   engine_accessor(std::move(engine_accessor_)),
        application(nullptr),
        runtime_events_processor(*this)
{
    ASSERT(engine_accessor != nullptr);
}


EngineData::~EngineData()
{
}


int EngineData::AddSymbol(std::shared_ptr<Symbol> symbol, Logic::SymbolTable::NameMapAddition name_map_addition/* = Logic::SymbolTable::NameMapAddition::ToCurrentScope*/)
{
    ASSERT(symbol != nullptr);

    // add the symbol to the symbol table
    symbol_table.AddSymbol(symbol, name_map_addition);

    // some symbols are also stored in container-based tables
    auto add_symbol_to_container = [&](auto& objects, auto* object)
    {
        object->SetContainerIndex(objects.size());
        objects.emplace_back(object);
    };

    switch( symbol->GetType() )
    {
        case SymbolType::Array:
        {
            arrays.emplace_back(assert_cast<LogicArray*>(symbol.get()));
            break;
        }

        case SymbolType::Dictionary:
        {
            engine_dictionaries.emplace_back(assert_cast<EngineDictionary*>(symbol.get()));
            break;
        }

        case SymbolType::File:
        {
            LogicFile& logic_file = assert_cast<LogicFile&>(*symbol);

            if( logic_file.HasGlobalVisibility() )
                files_global_visibility.emplace_back(&logic_file);

            break;
        }

        case SymbolType::Flow:
        {
            flows.emplace_back(assert_cast<Flow*>(symbol.get()));
            break;
        }

        case SymbolType::Pre80Flow:
        {
            flows_pre80.emplace_back(assert_cast<FLOW*>(symbol.get()));
            break;
        }

        case SymbolType::ValueSet:
        {
            ValueSet& value_set = assert_cast<ValueSet&>(*symbol);

            if( !value_set.IsDynamic() )
                value_sets_not_dynamic.emplace_back(&value_set);

            break;
        }

        // contained-based tables...
        case SymbolType::Crosstab:
        {
#ifdef WIN_DESKTOP
            add_symbol_to_container(crosstabs, assert_cast<CTAB*>(symbol.get()));
#endif
            break;
        }

        case SymbolType::Pre80Dictionary:
        {
            add_symbol_to_container(dictionaries_pre80, assert_cast<DICT*>(symbol.get()));
            break;
        }

        case SymbolType::Group:
        {
            add_symbol_to_container(groups, assert_cast<GROUPT*>(symbol.get()));
            break;
        }

        case SymbolType::Section:
        {
            add_symbol_to_container(sections, assert_cast<SECT*>(symbol.get()));
            break;
        }

        case SymbolType::Variable:
        {
            add_symbol_to_container(variables, assert_cast<VART*>(symbol.get()));
            break;
        }
    }

    return symbol->GetSymbolIndex();
}


void EngineData::Clear()
{
    numeric_constants.clear();
    string_literals.clear();

    frequencies.clear();
    imputations.clear();

    symbol_table.Clear();

    arrays.clear();
    engine_dictionaries.clear();
    files_global_visibility.clear();
    flows.clear();
    flows_pre80.clear();
    value_sets_not_dynamic.clear();

    crosstabs.clear();
    dictionaries_pre80.clear();
    groups.clear();
    sections.clear();
    variables.clear();
}
