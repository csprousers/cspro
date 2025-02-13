#include "StandardSystemIncludes.h"
#include "INTERPRE.H"
#include "Engine.h"
#include "FrequencyDriver.h"
#include <zEngineO/AllSymbols.h>


namespace
{
    template<typename T>
    T GetDefaultValue()
    {
        if constexpr(std::is_same_v<T, double>)
        {
            return DEFAULT;
        }

        else
        {
            return T();
        }
    }
}


template<typename T>
void CIntDriver::AssignValueToVART(int variable_compilation, T value)
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
            SO::MakeExactLength(value, pVarT->GetLength());
            _tmemcpy(static_cast<TCHAR*>(value_storage), value.data(), pVarT->GetLength());
        }

        // update items/subitems and other related data
        if( need_to_update_related_data )
            pVarX->VarxRefreshRelatedData(aIndex);
    }
}

template void CIntDriver::AssignValueToVART<double>(int variable_compilation, double value);
template void CIntDriver::AssignValueToVART<std::wstring>(int variable_compilation, std::wstring value);


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
        return GetDefaultValue<T>();
    }

    else
    {
        if constexpr(std::is_same_v<T, double>)
        {
            return *static_cast<double*>(value_storage);
        }

        else
        {
            return std::wstring(static_cast<const TCHAR*>(value_storage), pVarT->GetLength());
        }
    }
}

template double CIntDriver::EvaluateVARTValue<double>(int variable_compilation);
template std::wstring CIntDriver::EvaluateVARTValue<std::wstring>(int variable_compilation);


template<typename T>
void CIntDriver::ModifyVARTValue(int variable_compilation, const std::function<void(T&)>& modify_value_function,
                                 std::shared_ptr<Paradata::FieldInfo>* paradata_field_info/* = nullptr*/)
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
            std::wstring value(static_cast<const TCHAR*>(value_storage), pVarT->GetLength());
            modify_value_function(value);
            SO::MakeExactLength(value, pVarT->GetLength());
            _tmemcpy(static_cast<TCHAR*>(value_storage), value.data(), pVarT->GetLength());
        }

        // update items/subitems and other related data
        if( need_to_update_related_data )
            pVarX->VarxRefreshRelatedData(aIndex);
    }

    if( paradata_field_info != nullptr )
        *paradata_field_info = m_pParadataDriver->CreateFieldInfo(pVarT, dIndex);
}

template void CIntDriver::ModifyVARTValue(int variable_compilation, const std::function<void(double&)>& modify_value_function,
                                          std::shared_ptr<Paradata::FieldInfo>* paradata_field_info/* = nullptr*/);
template void CIntDriver::ModifyVARTValue(int variable_compilation, const std::function<void(std::wstring&)>& modify_value_function,
                                          std::shared_ptr<Paradata::FieldInfo>* paradata_field_info/* = nullptr*/);



template<typename T>
void CIntDriver::AssignValueToSymbol(const Nodes::SymbolValue& symbol_value_node, T value)
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
        if constexpr(std::is_same_v<T, std::wstring>)
        {
            WorkString& work_string = assert_cast<WorkString&>(symbol);
            work_string.SetString(std::move(value));
        }
    }


    // variable
    else if( symbol.IsA(SymbolType::Variable) )
    {
        VART* pVarT = assert_cast<VART*>(&symbol);

        // working string
        if( pVarT->GetLogicStringPtr() != nullptr )
        {
            if constexpr(std::is_same_v<T, std::wstring>)
            {
                *(pVarT->GetLogicStringPtr()) = WS2CS(value);
            }
        }

        else
        {
            AssignValueToVART(symbol_value_node.symbol_compilation, value);
        }
    }


    // List object
    else if( symbol.IsA(SymbolType::List) )
    {
        LogicList* logic_list;
        std::optional<size_t> index = EvaluateListIndex(symbol_value_node.symbol_compilation, &logic_list, true);

        if( index.has_value() )
        {
            if constexpr(std::is_same_v<T, double>)
            {
                logic_list->SetValue(*index, std::move(value));
            }

            else
            {
                logic_list->SetString(*index, std::move(value));
            }
        }
    }


    // HashMap object
    else if( symbol.IsA(SymbolType::HashMap) )
    {
        LogicHashMap* hashmap;
        std::vector<LogicHashMap::Data> dimension_values = EvaluateHashMapIndex(symbol_value_node.symbol_compilation, &hashmap, false);
        hashmap->SetValue(dimension_values, std::move(value));
    }


    // Array object
    else if( symbol.IsA(SymbolType::Array) )
    {
        LogicArray* logic_array;
        std::vector<size_t> indices = EvaluateArrayIndex(symbol_value_node.symbol_compilation, &logic_array);

        if( !indices.empty() )
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
}

template void CIntDriver::AssignValueToSymbol<double>(const Nodes::SymbolValue& symbol_value_node, double value);
template void CIntDriver::AssignValueToSymbol<std::wstring>(const Nodes::SymbolValue& symbol_value_node, std::wstring value);


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
        if constexpr(std::is_same_v<T, std::wstring>)
        {
            const WorkString& work_string = assert_cast<const WorkString&>(symbol);
            return work_string.GetString();
        }
    }


    // variable
    else if( symbol.IsA(SymbolType::Variable) )
    {
        const VART* pVarT = assert_cast<const VART*>(&symbol);

        // working string
        if( pVarT->GetLogicStringPtr() != nullptr )
        {
            if constexpr(std::is_same_v<T, std::wstring>)
                return CS2WS(*(pVarT->GetLogicStringPtr()));
        }

        else
        {
            return EvaluateVARTValue<T>(symbol_value_node.symbol_compilation);
        }
    }


    // List object
    else if( symbol.IsA(SymbolType::List) )
    {
        const LogicList* logic_list;
        std::optional<size_t> index = EvaluateListIndex(symbol_value_node.symbol_compilation, const_cast<LogicList**>(&logic_list), false);

        if( !index.has_value() )
            return GetDefaultValue<T>();

        if constexpr(std::is_same_v<T, double>)
        {
            return logic_list->GetValue(*index);
        }

        else
        {
            return logic_list->GetString(*index);
        }
    }

    // HashMap object
    else if( symbol.IsA(SymbolType::HashMap) )
    {
        const LogicHashMap* hashmap;
        std::vector<LogicHashMap::Data> dimension_values = EvaluateHashMapIndex(symbol_value_node.symbol_compilation, const_cast<LogicHashMap**>(&hashmap), true);

        if( !dimension_values.empty() )
            return std::get<T>(*hashmap->GetValue(dimension_values));

        return GetDefaultValue<T>();
    }


    // Array object
    else if( symbol.IsA(SymbolType::Array) )
    {
        const LogicArray* logic_array;
        std::vector<size_t> indices = EvaluateArrayIndex(symbol_value_node.symbol_compilation, const_cast<LogicArray**>(&logic_array));

        return indices.empty() ? GetDefaultValue<T>() : 
                                 logic_array->GetValue<T>(indices);
    }


    // user-defined function
    else if( symbol.IsA(SymbolType::UserFunction) )
    {
        UserFunction& user_function = GetSymbolUserFunction(symbol_value_node.symbol_index);
        return user_function.GetReturnValue<T>();
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
template std::wstring CIntDriver::EvaluateSymbolValue<std::wstring>(const Nodes::SymbolValue& symbol_value_nodevalue);



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
        if constexpr(std::is_same_v<T, std::wstring>)
        {
            WorkString& work_string = assert_cast<WorkString&>(symbol);

            T value = work_string.GetString();
            modify_value_function(value);
            work_string.SetString(std::move(value));
        }
    }


    // variable
    else if( symbol.IsA(SymbolType::Variable) )
    {
        VART* pVarT = assert_cast<VART*>(&symbol);

        // working string
        if( pVarT->GetLogicStringPtr() != nullptr )
        {
            if constexpr(std::is_same_v<T, std::wstring>)
            {
                std::wstring value = CS2WS(*(pVarT->GetLogicStringPtr()));
                modify_value_function(value);
                *(pVarT->GetLogicStringPtr()) = WS2CS(value);
            }
        }

        else
        {
            ModifyVARTValue(symbol_value_node.symbol_compilation, modify_value_function);
        }
    }


    // List object
    else if( symbol.IsA(SymbolType::List) )
    {
        LogicList* logic_list;
        std::optional<size_t> index = EvaluateListIndex(symbol_value_node.symbol_compilation, &logic_list, false);

        if( index.has_value() )
        {
            if constexpr(std::is_same_v<T, double>)
            {
                T value = logic_list->GetValue(*index);
                modify_value_function(value);
                logic_list->SetValue(*index, std::move(value));
            }

            else
            {
                T value = logic_list->GetString(*index);
                modify_value_function(value);
                logic_list->SetString(*index, std::move(value));
            }
        }
    }


    // HashMap object
    else if( symbol.IsA(SymbolType::HashMap) )
    {
        LogicHashMap* hashmap;
        std::vector<LogicHashMap::Data> dimension_values = EvaluateHashMapIndex(symbol_value_node.symbol_compilation, &hashmap, true);

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
        std::vector<size_t> indices = EvaluateArrayIndex(symbol_value_node.symbol_compilation, &logic_array);

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
        T value = user_function.GetReturnValue<T>();
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
template void CIntDriver::ModifySymbolValue<std::wstring>(const Nodes::SymbolValue& symbol_value_node, const std::function<void(std::wstring&)>& modify_value_function);
