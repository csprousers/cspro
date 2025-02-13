#include "stdafx.h"
#include "SymbolCalculator.h"
#include "AllSymbols.h"
#include <engine/Ctab.h>


// --------------------------------------------------------------------------
// SymbolCalculator (functions to centralize some common symbol calculations)
// --------------------------------------------------------------------------

std::wstring SymbolCalculator::GetBaseName(const Symbol& symbol)
{
    switch( symbol.GetType() )
    {
        case SymbolType::Dictionary:
            return CS2WS(assert_cast<const EngineDictionary&>(symbol).GetDictionary().GetName());

        case SymbolType::Record:
            return CS2WS(assert_cast<const EngineRecord&>(symbol).GetDictionaryRecord().GetName());

        default:
            return symbol.GetName();
    }
}


std::wstring SymbolCalculator::GetLabel(const Symbol& symbol)
{
    switch( symbol.GetType() )
    {
        case SymbolType::Dictionary:
        {
            const EngineDictionary& engine_dictionary = assert_cast<const EngineDictionary&>(symbol);
            return engine_dictionary.IsDictionaryObject() ? CS2WS(engine_dictionary.GetDictionary().GetLabel()) :
                                                            engine_dictionary.GetName();
        }

        case SymbolType::Pre80Dictionary:
        {
            return CS2WS(assert_cast<const DICT&>(symbol).GetDataDict()->GetLabel());
        }

        case SymbolType::Record:
        {
            return CS2WS(assert_cast<const EngineRecord&>(symbol).GetDictionaryRecord().GetLabel());
        }

        case SymbolType::Section:
        {
            return CS2WS(assert_cast<const SECT&>(symbol).GetDictRecord()->GetLabel());
        }

        case SymbolType::Variable:
        {
            return CS2WS(assert_cast<const VART&>(symbol).GetDictItem()->GetLabel());
        }

        case SymbolType::Item:
        {
            return CS2WS(assert_cast<const EngineItem&>(symbol).GetDictItem().GetLabel());
        }

        case SymbolType::ValueSet:
        {
            return CS2WS(assert_cast<const ValueSet&>(symbol).GetLabel());
        }

        case SymbolType::Block:
        {
            return CS2WS(assert_cast<const EngineBlock&>(symbol).GetFormBlock().GetLabel());
        }

        default:
        {
            const EngineItemAccessor* engine_item_accessor = symbol.GetEngineItemAccessor();

            return ( engine_item_accessor != nullptr ) ? CS2WS(engine_item_accessor->GetDictItem().GetLabel()) :
                                                         symbol.GetName();
        }
    }
}


const DictBase* SymbolCalculator::GetDictBase(const Symbol& symbol)
{
    switch( symbol.GetType() )
    {
        case SymbolType::Dictionary:
        {
            const EngineDictionary& engine_dictionary = assert_cast<const EngineDictionary&>(symbol);

            return engine_dictionary.IsDictionaryObject() ? &engine_dictionary.GetDictionary() :
                                                            nullptr;
        }

        case SymbolType::Pre80Dictionary:
        {
            return assert_cast<const DICT&>(symbol).GetDataDict();
        }

        case SymbolType::Record:
        {
            return &assert_cast<const EngineRecord&>(symbol).GetDictionaryRecord();
        }

        case SymbolType::Section:
        {
            return assert_cast<const SECT&>(symbol).GetDictRecord();
        }

        case SymbolType::Variable:
        {
            return assert_cast<const VART&>(symbol).GetDictItem();
        }

        case SymbolType::Item:
        {
            return &assert_cast<const EngineItem&>(symbol).GetDictItem();
        }

        case SymbolType::ValueSet:
        {
            const ValueSet& value_set = assert_cast<const ValueSet&>(symbol);

            return !value_set.IsDynamic() ? &value_set.GetDictValueSet() :
                                            nullptr;
        }

        default:
        {
            const EngineItemAccessor* engine_item_accessor = symbol.GetEngineItemAccessor();

            return ( engine_item_accessor != nullptr ) ? &engine_item_accessor->GetDictItem() :
                                                         nullptr;
        }
    }
}


bool SymbolCalculator::DoMultipleLabelsExist(const Symbol& symbol)
{
    const DictBase* dict_base = GetDictBase(symbol);

    if( dict_base != nullptr )
    {
        const std::vector<CString>& labels = dict_base->GetLabelSet().GetLabels();

        for( size_t i = 1; i < labels.size(); ++i )
        {
            if( labels[i] != labels[0] )
                return true;
        }
    }

    return false;
}


namespace
{
    template<typename T>
    T SymbolCalculator_GetDataType(const Symbol& symbol)
    {
        switch( symbol.GetType() )
        {
            case SymbolType::Array:
                return assert_cast<const LogicArray&>(symbol).GetDataType();

            case SymbolType::Crosstab:
                return DataType::Numeric;

            case SymbolType::HashMap:
                return assert_cast<const LogicHashMap&>(symbol).GetValueType();

            case SymbolType::Item:
                return assert_cast<const EngineItem&>(symbol).GetDictItem().GetDataType();

            case SymbolType::List:
                return assert_cast<const LogicList&>(symbol).GetDataType();

            case SymbolType::NamedFrequency:
                return DataType::Numeric;

            case SymbolType::UserFunction:
                return assert_cast<const UserFunction&>(symbol).GetReturnDataType();

            case SymbolType::ValueSet:
                return assert_cast<const ValueSet&>(symbol).GetDataType();

            case SymbolType::Variable:
                return assert_cast<const VART&>(symbol).GetDataType();

            case SymbolType::WorkString:
                return DataType::String;

            case SymbolType::WorkVariable:
                return DataType::Numeric;
        }

        if constexpr(std::is_same_v<T, std::optional<DataType>>)
        {
            return std::nullopt;
        }

        else
        {
            return ReturnProgrammingError(DataType::Numeric);
        }
    }
}


DataType SymbolCalculator::GetDataType(const Symbol& symbol)
{
    return SymbolCalculator_GetDataType<DataType>(symbol);
}


std::optional<DataType> SymbolCalculator::GetOptionalDataType(const Symbol& symbol)
{
    return SymbolCalculator_GetDataType<std::optional<DataType>>(symbol);
}


const EngineDictionary* SymbolCalculator::GetEngineDictionary(const Symbol& symbol)
{
    switch( symbol.GetType() )
    {
        case SymbolType::Dictionary:
            return &assert_cast<const EngineDictionary&>(symbol);

        case SymbolType::Record:
            return &assert_cast<const EngineRecord&>(symbol).GetEngineDictionary();

        case SymbolType::Variable:
        case SymbolType::ValueSet:
        case SymbolType::Group:
            // ENGINECR_TODO hook up once connected with EngineDictionary

        default:
            return nullptr;
    }
}


EngineDictionary* SymbolCalculator::GetEngineDictionary(Symbol& symbol)
{
    return const_cast<EngineDictionary*>(GetEngineDictionary(const_cast<const Symbol&>(symbol)));
}


unsigned SymbolCalculator::GetMaximumOccurrences(const Symbol& symbol)
{
    switch( symbol.GetType() )
    {
        case SymbolType::Record:
            return assert_cast<const EngineRecord&>(symbol).GetDictionaryRecord().GetMaxRecs();

        case SymbolType::Section:
            return assert_cast<const SECT&>(symbol).GetMaxOccs();

        case SymbolType::Variable:
            return assert_cast<const VART&>(symbol).GetMaxOccs();

        case SymbolType::Item:
            return assert_cast<const EngineItem&>(symbol).GetVarT().GetMaxOccs();

        case SymbolType::Group:
            return assert_cast<const GROUPT&>(symbol).GetMaxOccs();

        case SymbolType::Block:
            return GetMaximumOccurrences(*assert_cast<const EngineBlock&>(symbol).GetGroupT());

        default:
            return ReturnProgrammingError(0);
    }
}


const Symbol* SymbolCalculator::GetFirstSymbolWithOccurrences(const Symbol& symbol_)
{
    const Symbol* symbol = &symbol_;

    if( symbol->IsA(SymbolType::Item) )
        symbol = &assert_cast<const EngineItem*>(symbol)->GetVarT();

    if( symbol->IsA(SymbolType::Variable) )
    {
        const VART* pVarT = assert_cast<const VART*>(symbol);
        const CDictItem* item = pVarT->GetDictItem();

        // ignore work variables
        if( item != nullptr )
        {
            // an item can have occurrnces
            if( item->GetOccurs() > 1 )
                return pVarT;

            // otherwise we will evaluate using the parent group
            symbol = pVarT->GetParentGPT();
        }
    }

    // a block should use its group
    else if( symbol->IsA(SymbolType::Block) )
    {
        const EngineBlock* engine_block = assert_cast<const EngineBlock*>(symbol);
        symbol = engine_block->GetGroupT();
    }

    // a record can repeat
    else if( symbol->IsA(SymbolType::Record) )
    {
        const EngineRecord* engine_record = assert_cast<const EngineRecord*>(symbol);

        if( engine_record->GetDictionaryRecord().GetMaxRecs() > 1 )
            return engine_record;
    }

    else if( symbol->IsA(SymbolType::Section) )
    {
        const SECT* pSecT = assert_cast<const SECT*>(symbol);

        if( pSecT->GetMaxOccs() > 1 )
            return pSecT;
    }


    // if a group was provided (or switched to above), keep evaluating parent groups until one with occurrences is found
    if( symbol != nullptr && symbol->IsA(SymbolType::Group) )
    {
        const GROUPT* pGroupT = assert_cast<const GROUPT*>(symbol);

        do
        {
            if( pGroupT->GetMaxOccs() > 1 )
                return pGroupT;

            pGroupT = pGroupT->GetParentGPT();

        } while( pGroupT != nullptr );
    }

    return nullptr;
}


int SymbolCalculator::GetLevelNumber_base1(const Symbol& symbol)
{
    constexpr int NoLevelNumber = 9;

    // ENGINECR_TODO it would be nice GetLevel()/GetLevelNumber() throughout the code was consistent
    // in whether the value is zero- or one-based
    switch( symbol.GetType() )
    {
        case SymbolType::Application:
        case SymbolType::Report:
        case SymbolType::UserFunction:
            return NoLevelNumber;

        case SymbolType::Dictionary:
            return 1;

        case SymbolType::Pre80Dictionary:
            return 1;

        case SymbolType::Record:
            return assert_cast<const EngineRecord&>(symbol).GetDictionaryRecord().GetLevel()->GetLevelNumber() + 1;

        case SymbolType::Section:
            return assert_cast<const SECT&>(symbol).GetLevel();

        case SymbolType::Variable:
            return assert_cast<const VART&>(symbol).GetLevel();

        case SymbolType::Item:
            return assert_cast<const EngineItem&>(symbol).GetVarT().GetLevel();

        case SymbolType::Group:
            return assert_cast<const GROUPT&>(symbol).GetLevel();

        case SymbolType::Block:
            return assert_cast<const EngineBlock&>(symbol).GetGroupT()->GetLevel();

#ifdef WIN_DESKTOP
        case SymbolType::Crosstab:
            return assert_cast<const CTAB&>(symbol).GetTableLevel();
#endif
        default:
            return ReturnProgrammingError(-1);
    }
}


bool SymbolCalculator::IsSymbolDataAccessible(const Symbol& symbol, const int level_number)
{
    switch( symbol.GetType() )
    {
        case SymbolType::Dictionary:
        case SymbolType::Pre80Dictionary:
        case SymbolType::Record:
        case SymbolType::Section:
        case SymbolType::Variable:
        case SymbolType::Item:
        case SymbolType::Group:
        case SymbolType::Block:
        case SymbolType::Crosstab:
        {
            ASSERT(GetLevelNumber_base1(symbol) >= 1 && GetLevelNumber_base1(symbol) < MaxNumberLevels);

            return ( level_number >= GetLevelNumber_base1(symbol) );
        }

        default:
        {
            return true;
        }
    }
}


const DICT* SymbolCalculator::GetDicT(const Symbol& symbol) const
{
    switch( symbol.GetType() )
    {
        case SymbolType::Pre80Dictionary:
        {
            return &assert_cast<const DICT&>(symbol);
        }

        case SymbolType::Group:
        {
            const GROUPT* pGroupT = assert_cast<const GROUPT*>(&symbol);

            // a level
            if( pGroupT->GetGroupType() == GROUPT::Level )
            {
                const FLOW* pFlow = pGroupT->GetFlow();
                ASSERT(pFlow->GetNumberOfDics() == 1);

                return DPT(pFlow->GetSymDicAt(0));
            }

            else
            {
                return ReturnProgrammingError(nullptr);
            }
        }

        case SymbolType::Section:
        {
            return assert_cast<const SECT&>(symbol).GetDicT();
        }

        case SymbolType::Variable:
        {
            return assert_cast<const VART&>(symbol).GetDPT();
        }

        case SymbolType::Item:
        {
            return assert_cast<const EngineItem&>(symbol).GetVarT().GetDPT();
        }

        case SymbolType::ValueSet:
        {
            const VART* pVarT = assert_cast<const ValueSet&>(symbol).GetVarT();
            return ( pVarT != nullptr ) ? pVarT->GetDPT() : nullptr;
        }

        case SymbolType::Dictionary:
        case SymbolType::Record:
        {
            return ReturnProgrammingError(nullptr);
        }

        default:
        {
            const EngineItemAccessor* engine_item_accessor = symbol.GetEngineItemAccessor();

            return ( engine_item_accessor != nullptr ) ? engine_item_accessor->GetVarT().GetDPT() :
                                                         nullptr;
        }
    }
}


DICT* SymbolCalculator::GetDicT(Symbol& symbol) const
{
    return const_cast<DICT*>(GetDicT(const_cast<const Symbol&>(symbol)));
}
