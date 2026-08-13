#include "stdafx.h"
#include "IncludesRT.h"
#include "AllSymbols.h"
#include <engine/FrequencyDriver.h>


template<typename T>
bool LogicInterpreter::AssignValueToSymbol(const Nodes::SymbolValue& symbol_value_node, T value)
{
    Symbol& symbol = NPT_Ref(symbol_value_node.symbol_index);

    ASSERT(( std::is_same_v<T, double>         ) ? IsNumeric(symbol) :
           ( std::is_same_v<T, SharableString> ) ? IsString(symbol) :
                                                   false);

    switch( symbol.GetType() )
    {
        // Array
        case SymbolType::Array:
        {
            LogicArray* logic_array;
            const std::vector<size_t> indices = EvaluateArrayIndex(symbol_value_node.symbol_compilation, &logic_array);

            if( indices.empty() )
                return false;

            logic_array->SetValue(indices, std::move(value));

            return true;
        }

        // HashMap
        case SymbolType::HashMap:
        {
            LogicHashMap* hashmap;
            const std::vector<LogicHashMap::Data> dimension_values = EvaluateHashMapIndex(symbol_value_node.symbol_compilation, &hashmap, false);

            if( dimension_values.empty() )
                return false;

            hashmap->SetValue(dimension_values, std::move(value));

            return true;
        }

        // List
        case SymbolType::List:
        {
            LogicList* logic_list;
            const std::optional<size_t> index = EvaluateListIndex(symbol_value_node.symbol_compilation, &logic_list, true);

            if( !index.has_value() )
                return false;

            logic_list->SetValue(*index, std::move(value));

            return true;
        }

        // named frequency
        case SymbolType::NamedFrequency:
        {
            if constexpr(std::is_same_v<T, double>)
                GetFrequencyDriver_INTERPRETER_DLL_TODO()->SetSingleFrequencyCounterCount(symbol_value_node.symbol_compilation, value);

            return true;
        }

        // user-defined function
        case SymbolType::UserFunction:
        {
            UserFunction& user_function = GetSymbolUserFunction(symbol_value_node.symbol_index);
            user_function.SetReturnValue(std::move(value));
            return true;
        }

        // variable
        case SymbolType::Variable:
        {
            AssignValueToVART_INTERPRETER_DLL_TODO(symbol_value_node.symbol_compilation, std::move(value));
            return true;
        }

        // work string
        case SymbolType::WorkString:
        {
            if constexpr(std::is_same_v<T, SharableString>)
            {
                WorkString& work_string = assert_cast<WorkString&>(symbol);
                work_string.SetString(std::move(value));
            }

            return true;
        }

        // work variable
        case SymbolType::WorkVariable:
        {
            if constexpr(std::is_same_v<T, double>)
            {
                WorkVariable& work_variable = assert_cast<WorkVariable&>(symbol);
                work_variable.SetValue(value);
            }

            return true;
        }

        // invalid
        default:
        {
            return ReturnProgrammingError(false);
        }
    }
}

template ZENGINEO_API bool LogicInterpreter::AssignValueToSymbol<double>(const Nodes::SymbolValue& symbol_value_node, double value);
template ZENGINEO_API bool LogicInterpreter::AssignValueToSymbol<SharableString>(const Nodes::SymbolValue& symbol_value_node, SharableString value);

template<>
ZENGINEO_API bool LogicInterpreter::AssignValueToSymbol<Engine::Value>(const Nodes::SymbolValue& symbol_value_node, Engine::Value value)
{
    const Symbol& symbol = NPT_Ref(symbol_value_node.symbol_index);
    ASSERT(IsNumeric(symbol) || IsString(symbol));

    return IsNumeric(symbol) ? AssignValueToSymbol(symbol_value_node, std::move(value).as<double>()) :
                               AssignValueToSymbol(symbol_value_node, std::move(value).as<SharableString>());
}


template<typename T>
T LogicInterpreter::EvaluateSymbolValue(const Nodes::SymbolValue& symbol_value_node)
{
    const Symbol& symbol = NPT_Ref(symbol_value_node.symbol_index);

    ASSERT(( std::is_same_v<T, double>         ) ? IsNumeric(symbol) :
           ( std::is_same_v<T, SharableString> ) ? IsString(symbol) :
                                                   false);

    switch( symbol.GetType() )
    {
        // Array
        case SymbolType::Array:
        {
            const LogicArray* logic_array;
            const std::vector<size_t> indices = EvaluateArrayIndex(symbol_value_node.symbol_compilation, const_cast<LogicArray**>(&logic_array));

            if( indices.empty() )
                return Engine::Value::Invalid<T>().template get<T>();

            return logic_array->GetValue<T>(indices);
        }

        // HashMap
        case SymbolType::HashMap:
        {
            const LogicHashMap* hashmap;
            const std::vector<LogicHashMap::Data> dimension_values = EvaluateHashMapIndex(symbol_value_node.symbol_compilation, const_cast<LogicHashMap**>(&hashmap), true);

            if( dimension_values.empty() )
                return Engine::Value::Invalid<T>().template get<T>();

            return std::get<T>(*hashmap->GetValue(dimension_values));
        }

        // List
        case SymbolType::List:
        {
            const LogicList* logic_list;
            const std::optional<size_t> index = EvaluateListIndex(symbol_value_node.symbol_compilation, const_cast<LogicList**>(&logic_list), false);

            if( !index.has_value() )
                return Engine::Value::Invalid<T>().template get<T>();

            return logic_list->GetValue<T>(*index);
        }

        // named frequency
        case SymbolType::NamedFrequency:
        {
            if constexpr(std::is_same_v<T, double>)
                return GetFrequencyDriver_INTERPRETER_DLL_TODO()->GetSingleFrequencyCounterCount(symbol_value_node.symbol_compilation);

            break;
        }

        // user-defined function
        case SymbolType::UserFunction:
        {
            const UserFunction& user_function = GetSymbolUserFunction(symbol_value_node.symbol_index);
            ASSERT(user_function.GetReturnValue().is<T>());
            return user_function.GetReturnValue().get<T>();
        }

        // variable
        case SymbolType::Variable:
        {
            // INTERPRETER_DLL_TODO restore the original version:
            // return EvaluateVARTValue<T>(symbol_value_node.symbol_compilation);
            if constexpr(std::is_same_v<T, double>)
            {
                return EvaluateVARTValue_double_INTERPRETER_DLL_TODO(symbol_value_node.symbol_compilation);
            }

            else
            {
                return EvaluateVARTValue_SharableString_INTERPRETER_DLL_TODO(symbol_value_node.symbol_compilation);
            }
        }

        // work string
        case SymbolType::WorkString:
        {
            if constexpr(std::is_same_v<T, SharableString>)
            {
                const WorkString& work_string = assert_cast<const WorkString&>(symbol);
                return work_string.GetSharableString();
            }

            break;
        }

        // work variable
        case SymbolType::WorkVariable:
        {
            if constexpr(std::is_same_v<T, double>)
            {
                const WorkVariable& work_variable = assert_cast<const WorkVariable&>(symbol);
                return work_variable.GetValue();
            }

            break;
        }

        // invalid
        default:
        {
            break;
        }
    }

    return ReturnProgrammingError(Engine::Value::Invalid<T>().template get<T>());
}

template ZENGINEO_API double LogicInterpreter::EvaluateSymbolValue<double>(const Nodes::SymbolValue& symbol_value_node);
template ZENGINEO_API SharableString LogicInterpreter::EvaluateSymbolValue<SharableString>(const Nodes::SymbolValue& symbol_value_nodevalue);

template<>
ZENGINEO_API Engine::Value LogicInterpreter::EvaluateSymbolValue(const Nodes::SymbolValue& symbol_value_node)
{
    const Symbol& symbol = NPT_Ref(symbol_value_node.symbol_index);
    ASSERT(IsNumeric(symbol) || IsString(symbol));

    return IsNumeric(symbol) ? Engine::Value(EvaluateSymbolValue<double>(symbol_value_node)) :
                               Engine::Value(EvaluateSymbolValue<SharableString>(symbol_value_node));
}


template<typename T>
Engine::Value LogicInterpreter::ModifySymbolValue(const Nodes::SymbolValue& symbol_value_node, const std::function<void(T&)>& modify_value_function)
{
    Symbol& symbol = NPT_Ref(symbol_value_node.symbol_index);

    ASSERT(( std::is_same_v<T, double>         ) ? IsNumeric(symbol) :
           ( std::is_same_v<T, SharableString> ) ? IsString(symbol) :
                                                   false);

    switch( symbol.GetType() )
    {
        // Array
        case SymbolType::Array:
        {
            LogicArray* logic_array;
            const std::vector<size_t> indices = EvaluateArrayIndex(symbol_value_node.symbol_compilation, &logic_array);

            if( indices.empty() )
                break;

            T value = logic_array->GetValue<T>(indices);
            modify_value_function(value);
            logic_array->SetValue(indices, value);

            return value;
        }

        // HashMap
        case SymbolType::HashMap:
        {
            LogicHashMap* hashmap;
            const std::vector<LogicHashMap::Data> dimension_values = EvaluateHashMapIndex(symbol_value_node.symbol_compilation, &hashmap, true);

            if( dimension_values.empty() )
                break;

            T value = std::get<T>(*hashmap->GetValue(dimension_values));
            modify_value_function(value);
            hashmap->SetValue(dimension_values, value);

            return value;
        }

        // List
        case SymbolType::List:
        {
            LogicList* logic_list;
            const std::optional<size_t> index = EvaluateListIndex(symbol_value_node.symbol_compilation, &logic_list, false);

            if( !index.has_value() )
                break;

            T value = logic_list->GetValue<T>(*index);
            modify_value_function(value);
            logic_list->SetValue(*index, value);

            return value;
        }

        // named frequency
        case SymbolType::NamedFrequency:
        {
            if constexpr(std::is_same_v<T, double>)
                return GetFrequencyDriver_INTERPRETER_DLL_TODO()->ModifySingleFrequencyCounterCount(symbol_value_node.symbol_compilation, modify_value_function);

            break;
        }

        // user-defined function
        case SymbolType::UserFunction:
        {
            UserFunction& user_function = GetSymbolUserFunction(symbol_value_node.symbol_index);
            ASSERT(user_function.GetReturnValue().is<T>());

            T value = user_function.GetReturnValue().get<T>();
            modify_value_function(value);
            user_function.SetReturnValue(value);

            return value;
        }

        // variable
        case SymbolType::Variable:
        {
            return ModifyVARTValue_INTERPRETER_DLL_TODO(symbol_value_node.symbol_compilation, modify_value_function);
        }

        // work string
        case SymbolType::WorkString:
        {
            if constexpr(std::is_same_v<T, SharableString>)
            {
                WorkString& work_string = assert_cast<WorkString&>(symbol);

                SharableString value = work_string.GetSharableString();
                modify_value_function(value);
                work_string.SetString(SharableString(value));

                return value;
            }

            break;
        }

        // work variable
        case SymbolType::WorkVariable:
        {
            if constexpr(std::is_same_v<T, double>)
            {
                WorkVariable& work_variable = assert_cast<WorkVariable&>(symbol);

                double value = work_variable.GetValue();
                modify_value_function(value);
                work_variable.SetValue(value);

                return value;
            }

            break;
        }

        // invalid
        default:
        {
            ASSERT(false);
            break;
        }
    }

    return Engine::Value::Invalid<T>();
}

template ZENGINEO_API Engine::Value LogicInterpreter::ModifySymbolValue<double>(const Nodes::SymbolValue& symbol_value_node, const std::function<void(double&)>& modify_value_function);
template ZENGINEO_API Engine::Value LogicInterpreter::ModifySymbolValue<SharableString>(const Nodes::SymbolValue& symbol_value_node, const std::function<void(SharableString&)>& modify_value_function);
