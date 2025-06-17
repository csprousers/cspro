#include "StandardSystemIncludes.h"
#include "Interpreter.h"
#include "Engine.h"
#include "FrequencyDriver.h"
#include <zEngineO/AllSymbols.h>
#include <zParadataO/FieldInfo.h>


template<typename T>
void CIntDriver::AssignValueToVART(const int variable_compilation, T value)
{
    // TODO: could add checks as in CIntDriver::excpt
    const MVAR_NODE* pMVarNode = &GetNode<MVAR_NODE>(variable_compilation);
    VART* pVarT = VPT(pMVarNode->m_iVarIndex);
    VARX* pVarX = pVarT->GetVarX();
    int aIndex[DIM_MAXDIM];
    void* value_storage = nullptr;

    // multiply occurring
    if( pMVarNode->m_iVarType == MVAR_CODE )
    {
        double dIndex[DIM_MAXDIM];
        mvarGetSubindexes(pMVarNode, dIndex);

        if( pVarX->RemapIndexes(aIndex, dIndex) )
        {
            if constexpr(std::is_same_v<T, double>)
            {
                CNDIndexes theIndex(ZERO_BASED, aIndex);
                value_storage = GetMultVarFloatAddr(pVarX, theIndex);
            }

            else
            {
                value_storage = GetMultVarAsciiAddr(pVarX, aIndex);
            }
        }
    }

    // singling occurring
    else
    {
        memset(aIndex, 0, sizeof(int) * DIM_MAXDIM);
        value_storage = svaraddr(pVarX);
    }

    if( value_storage != nullptr )
    {
        bool need_to_update_related_data = ( pVarX->iRelatedSlot >= 0 );

        if constexpr(std::is_same_v<T, double>)
        {
            *static_cast<double*>(value_storage) = value;

            if( Issamod == ModuleType::Entry || need_to_update_related_data )
            {
                ModuleType eOldMode = Issamod;
                Issamod = ModuleType::Batch; // Truco: in order to call varoutval and dvaltochar
                m_pEngineDriver->prepvar(pVarT, NO_VISUAL_VALUE); // write to ascii buffer
                Issamod = eOldMode;
            }
        }

        else
        {
            std::wstring wide_value = UTF8_TODO::GetWide(*value);
            SO::MakeExactLength(wide_value, pVarT->GetLength());
            _tmemcpy(static_cast<wchar_t*>(value_storage), wide_value.data(), pVarT->GetLength());
        }

        // update items/subitems and other related data
        if( need_to_update_related_data )
            pVarX->VarxRefreshRelatedData(aIndex);
    }
}

template void CIntDriver::AssignValueToVART<double>(int variable_compilation, double value);
template void CIntDriver::AssignValueToVART<SharableString>(int variable_compilation, SharableString value);


template<typename T>
T CIntDriver::EvaluateVARTValue(int variable_compilation)
{
    // TODO: could add checks as in CIntDriver::excpt
    const MVAR_NODE* pMVarNode = &GetNode<MVAR_NODE>(variable_compilation);
    VART* pVarT = VPT(pMVarNode->m_iVarIndex);
    VARX* pVarX = pVarT->GetVarX();
    int aIndex[DIM_MAXDIM];
    void* value_storage = nullptr;

    // multiply occurring
    if( pMVarNode->m_iVarType == MVAR_CODE )
    {
        double dIndex[DIM_MAXDIM];
        mvarGetSubindexes(pMVarNode, dIndex);

        if( pVarX->RemapIndexes(aIndex, dIndex) )
        {
            if constexpr(std::is_same_v<T, double>)
            {
                CNDIndexes theIndex(ZERO_BASED, aIndex);
                value_storage = GetMultVarFloatAddr(pVarX, theIndex);
            }

            else
            {
                value_storage = GetMultVarAsciiAddr(pVarX, aIndex);
            }
        }
    }

    // singling occurring
    else
    {
        value_storage = svaraddr(pVarX);
    }

    if( value_storage == nullptr )
    {
        return GetInvalidValue<T>();
    }

    else
    {
        if constexpr(std::is_same_v<T, double>)
        {
            return *static_cast<double*>(value_storage);
        }

        else
        {
            return UTF8_TODO::GetUtf8(std::wstring_view(static_cast<const wchar_t*>(value_storage), pVarT->GetLength()));
        }
    }
}

template double CIntDriver::EvaluateVARTValue<double>(int variable_compilation);
template SharableString CIntDriver::EvaluateVARTValue<SharableString>(int variable_compilation);


template<typename T>
void CIntDriver::ModifyVARTValue(int variable_compilation, const std::function<void(T&)>& modify_value_function,
                                 std::unique_ptr<Paradata::FieldInfo>* paradata_field_info/* = nullptr*/)
{
    // TODO: could add checks as in CIntDriver::excpt
    const MVAR_NODE* pMVarNode = &GetNode<MVAR_NODE>(variable_compilation);
    VART* pVarT = VPT(pMVarNode->m_iVarIndex);
    VARX* pVarX = pVarT->GetVarX();
    int aIndex[DIM_MAXDIM];
    double dIndex[DIM_MAXDIM] = { 0 };
    void* value_storage = nullptr;

    // multiply occurring
    if( pMVarNode->m_iVarType == MVAR_CODE )
    {
        mvarGetSubindexes(pMVarNode, dIndex);

        if( pVarX->RemapIndexes(aIndex, dIndex) )
        {
            if constexpr(std::is_same_v<T, double>)
            {
                CNDIndexes theIndex(ZERO_BASED, aIndex);
                value_storage = GetMultVarFloatAddr(pVarX, theIndex);
            }

            else
            {
                value_storage = GetMultVarAsciiAddr(pVarX, aIndex);
            }
        }
    }

    // singling occurring
    else
    {
        memset(aIndex, 0, sizeof(int) * DIM_MAXDIM);
        value_storage = svaraddr(pVarX);
    }

    if( value_storage != nullptr )
    {
        bool need_to_update_related_data = ( pVarX->iRelatedSlot >= 0 );

        if constexpr(std::is_same_v<T, double>)
        {
            modify_value_function(*static_cast<double*>(value_storage));

            if( Issamod == ModuleType::Entry || need_to_update_related_data )
            {
                ModuleType eOldMode = Issamod;
                Issamod = ModuleType::Batch; // Truco: in order to call varoutval and dvaltochar
                m_pEngineDriver->prepvar(pVarT, NO_VISUAL_VALUE); // write to ascii buffer
                Issamod = eOldMode;
            }
        }

        else
        {
            std::wstring wide_value(static_cast<const wchar_t*>(value_storage), pVarT->GetLength());
            SharableString value = UTF8_TODO::GetUtf8(wide_value);
            modify_value_function(value);
            wide_value = UTF8_TODO::GetWide(*value);
            SO::MakeExactLength(wide_value, pVarT->GetLength());
            _tmemcpy(static_cast<wchar_t*>(value_storage), wide_value.data(), pVarT->GetLength());
        }

        // update items/subitems and other related data
        if( need_to_update_related_data )
            pVarX->VarxRefreshRelatedData(aIndex);
    }

    if( paradata_field_info != nullptr )
        *paradata_field_info = m_paradataDriver->CreateFieldInfo(pVarT, dIndex);
}

template void CIntDriver::ModifyVARTValue(int variable_compilation, const std::function<void(double&)>& modify_value_function,
                                          std::unique_ptr<Paradata::FieldInfo>* paradata_field_info/* = nullptr*/);
template void CIntDriver::ModifyVARTValue(int variable_compilation, const std::function<void(SharableString&)>& modify_value_function,
                                          std::unique_ptr<Paradata::FieldInfo>* paradata_field_info/* = nullptr*/);



template<typename T>
bool CIntDriver::AssignValueToSymbol(const Nodes::SymbolValue& symbol_value_node, T value)
{
    Symbol& symbol = NPT_Ref(symbol_value_node.symbol_index);

#ifdef _DEBUG
    if constexpr(std::is_same_v<T, double>)
    {
        ASSERT(IsNumeric(symbol));
    }

    else
    {
        ASSERT(IsString(symbol));
    }
#endif

    // instead of having the code in alphabetical order, the symbols are processed
    // here roughly in order of how common this call will be for the variable type


    // work variable
    if( symbol.IsA(SymbolType::WorkVariable) )
    {
        if constexpr(std::is_same_v<T, double>)
        {
            WorkVariable& work_variable = assert_cast<WorkVariable&>(symbol);
            work_variable.SetValue(value);
        }
    }


    // work string
    else if( symbol.IsA(SymbolType::WorkString) )
    {
        if constexpr(std::is_same_v<T, SharableString>)
        {
            WorkString& work_string = assert_cast<WorkString&>(symbol);
            work_string.SetString(std::move(value));
        }
    }


    // variable
    else if( symbol.IsA(SymbolType::Variable) )
    {
        AssignValueToVART(symbol_value_node.symbol_compilation, std::move(value));
    }


    // List object
    else if( symbol.IsA(SymbolType::List) )
    {
        LogicList* logic_list;
        const std::optional<size_t> index = EvaluateListIndex(symbol_value_node.symbol_compilation, &logic_list, true);

        if( !index.has_value() )
            return false;

        logic_list->SetValue(*index, std::move(value));
    }


    // HashMap object
    else if( symbol.IsA(SymbolType::HashMap) )
    {
        LogicHashMap* hashmap;
        std::vector<LogicHashMap::Data> dimension_values = EvaluateHashMapIndex(symbol_value_node.symbol_compilation, &hashmap, false);

        if( dimension_values.empty() )
            return false;

        hashmap->SetValue(dimension_values, std::move(value));
    }


    // Array object
    else if( symbol.IsA(SymbolType::Array) )
    {
        LogicArray* logic_array;
        std::vector<size_t> indices = EvaluateArrayIndex(symbol_value_node.symbol_compilation, &logic_array);

        if( indices.empty() )
            return false;

        logic_array->SetValue(indices, std::move(value));
    }


    // user-defined function
    else if( symbol.IsA(SymbolType::UserFunction) )
    {
        UserFunction& user_function = GetSymbolUserFunction(symbol_value_node.symbol_index);
        user_function.SetReturnValue(std::move(value));
    }


    // named frequency
    else if( symbol.IsA(SymbolType::NamedFrequency) )
    {
        if constexpr(std::is_same_v<T, double>)
            m_frequencyDriver->SetSingleFrequencyCounterCount(symbol_value_node.symbol_compilation, value);
    }


    else
    {
        throw ProgrammingErrorException();
    }


    return true;
}

template bool CIntDriver::AssignValueToSymbol<double>(const Nodes::SymbolValue& symbol_value_node, double value);
template bool CIntDriver::AssignValueToSymbol<SharableString>(const Nodes::SymbolValue& symbol_value_node, SharableString value);


template<typename T>
T CIntDriver::EvaluateSymbolValue(const Nodes::SymbolValue& symbol_value_node)
{
    const Symbol& symbol = NPT_Ref(symbol_value_node.symbol_index);

#ifdef _DEBUG
    if constexpr(std::is_same_v<T, double>)
    {
        ASSERT(IsNumeric(symbol));
    }

    else
    {
        ASSERT(IsString(symbol));
    }
#endif

    // instead of having the code in alphabetical order, the symbols are processed
    // here roughly in order of how common this call will be for the variable type


    // work variable
    if( symbol.IsA(SymbolType::WorkVariable) )
    {
        if constexpr(std::is_same_v<T, double>)
        {
            const WorkVariable& work_variable = assert_cast<const WorkVariable&>(symbol);
            return work_variable.GetValue();
        }
    }


    // work string
    else if( symbol.IsA(SymbolType::WorkString) )
    {
        if constexpr(std::is_same_v<T, SharableString>)
        {
            const WorkString& work_string = assert_cast<const WorkString&>(symbol);
            return work_string.GetSharableString();
        }
    }


    // variable
    else if( symbol.IsA(SymbolType::Variable) )
    {
        return EvaluateVARTValue<T>(symbol_value_node.symbol_compilation);
    }


    // List object
    else if( symbol.IsA(SymbolType::List) )
    {
        const LogicList* logic_list;
        const std::optional<size_t> index = EvaluateListIndex(symbol_value_node.symbol_compilation, const_cast<LogicList**>(&logic_list), false);

        if( !index.has_value() )
            return GetInvalidValue<T>();

        return logic_list->GetValue<T>(*index);
    }

    // HashMap object
    else if( symbol.IsA(SymbolType::HashMap) )
    {
        const LogicHashMap* hashmap;
        const std::vector<LogicHashMap::Data> dimension_values = EvaluateHashMapIndex(symbol_value_node.symbol_compilation, const_cast<LogicHashMap**>(&hashmap), true);

        if( !dimension_values.empty() )
            return std::get<T>(*hashmap->GetValue(dimension_values));

        return GetInvalidValue<T>();
    }


    // Array object
    else if( symbol.IsA(SymbolType::Array) )
    {
        const LogicArray* logic_array;
        const std::vector<size_t> indices = EvaluateArrayIndex(symbol_value_node.symbol_compilation, const_cast<LogicArray**>(&logic_array));

        return indices.empty() ? GetInvalidValue<T>() :
                                 logic_array->GetValue<T>(indices);
    }


    // user-defined function
    else if( symbol.IsA(SymbolType::UserFunction) )
    {
        const UserFunction& user_function = GetSymbolUserFunction(symbol_value_node.symbol_index);
        return std::get<T>(user_function.GetReturnValue());
    }


    // named frequency
    else if( symbol.IsA(SymbolType::NamedFrequency) )
    {
        if constexpr(std::is_same_v<T, double>)
            return m_frequencyDriver->GetSingleFrequencyCounterCount(symbol_value_node.symbol_compilation);
    }


    throw ProgrammingErrorException();
}

template double CIntDriver::EvaluateSymbolValue<double>(const Nodes::SymbolValue& symbol_value_node);
template SharableString CIntDriver::EvaluateSymbolValue<SharableString>(const Nodes::SymbolValue& symbol_value_nodevalue);


template<typename T>
void CIntDriver::ModifySymbolValue(const Nodes::SymbolValue& symbol_value_node, const std::function<void(T&)>& modify_value_function)
{
    Symbol& symbol = NPT_Ref(symbol_value_node.symbol_index);

#ifdef _DEBUG
    if constexpr(std::is_same_v<T, double>)
    {
        ASSERT(IsNumeric(symbol));
    }

    else
    {
        ASSERT(IsString(symbol));
    }
#endif

    // instead of having the code in alphabetical order, the symbols are processed
    // here roughly in order of how common this call will be for the variable type


    // work variable
    if( symbol.IsA(SymbolType::WorkVariable) )
    {
        if constexpr(std::is_same_v<T, double>)
        {
            WorkVariable& work_variable = assert_cast<WorkVariable&>(symbol);

            T value = work_variable.GetValue();
            modify_value_function(value);
            work_variable.SetValue(std::move(value));
        }
    }


    // work string
    else if( symbol.IsA(SymbolType::WorkString) )
    {
        if constexpr(std::is_same_v<T, SharableString>)
        {
            WorkString& work_string = assert_cast<WorkString&>(symbol);

            T value = work_string.GetSharableString();
            modify_value_function(value);
            work_string.SetString(std::move(value));
        }
    }


    // variable
    else if( symbol.IsA(SymbolType::Variable) )
    {
        ModifyVARTValue(symbol_value_node.symbol_compilation, modify_value_function);
    }


    // List object
    else if( symbol.IsA(SymbolType::List) )
    {
        LogicList* logic_list;
        const std::optional<size_t> index = EvaluateListIndex(symbol_value_node.symbol_compilation, &logic_list, false);

        if( index.has_value() )
        {
            T value = logic_list->GetValue<T>(*index);
            modify_value_function(value);
            logic_list->SetValue(*index, std::move(value));
        }
    }


    // HashMap object
    else if( symbol.IsA(SymbolType::HashMap) )
    {
        LogicHashMap* hashmap;
        const std::vector<LogicHashMap::Data> dimension_values = EvaluateHashMapIndex(symbol_value_node.symbol_compilation, &hashmap, true);

        if( !dimension_values.empty() )
        {
            T value = std::get<T>(*hashmap->GetValue(dimension_values));
            modify_value_function(value);
            hashmap->SetValue(dimension_values, std::move(value));
        }
    }


    // Array object
    else if( symbol.IsA(SymbolType::Array) )
    {
        LogicArray* logic_array;
        const std::vector<size_t> indices = EvaluateArrayIndex(symbol_value_node.symbol_compilation, &logic_array);

        if( !indices.empty() )
        {
            T value = logic_array->GetValue<T>(indices);
            modify_value_function(value);
            logic_array->SetValue(indices, std::move(value));
        }
    }


    // user-defined function
    else if( symbol.IsA(SymbolType::UserFunction) )
    {
        UserFunction& user_function = GetSymbolUserFunction(symbol_value_node.symbol_index);

        T value = std::get<T>(user_function.GetReturnValue());
        modify_value_function(value);
        user_function.SetReturnValue(std::move(value));
    }


    // named frequency
    else if( symbol.IsA(SymbolType::NamedFrequency) )
    {
        if constexpr(std::is_same_v<T, double>)
            m_frequencyDriver->ModifySingleFrequencyCounterCount(symbol_value_node.symbol_compilation, modify_value_function);
    }


    else
    {
        throw ProgrammingErrorException();
    }
}

template void CIntDriver::ModifySymbolValue<double>(const Nodes::SymbolValue& symbol_value_node, const std::function<void(double&)>& modify_value_function);
template void CIntDriver::ModifySymbolValue<SharableString>(const Nodes::SymbolValue& symbol_value_node, const std::function<void(SharableString&)>& modify_value_function);
