#include "stdafx.h"
#include "IncludesRT.h"
#include "JavaScriptProcessor.h"
#include "UserFunction.h"
#include <zJavaScript/Executor.h>
#include <zJavaScript/Value.h>


template<typename CF>
auto LogicInterpreter::ExecuteWithJavaScriptProcessor(const CF& callback_function)
{
    ASSERT(!IsExecutionInterrupted());

    EngineJavaScriptProcessor& javascript_processor = m_engineData->GetJavaScriptProcessor();

    // forward any cancelation requests to the JavaScript executor
    javascript_processor.GetExecutor().SetCancelFlag(&m_bStopProc);
    const CancelFlag::ListenerHolder cancel_flag_listener_holder = m_bStopProc.AddListener([&]() { javascript_processor.GetExecutor().CancelEvaluation(); });

    auto result = callback_function(javascript_processor);

    // because JavaScript functions can call back into user-defined functions that might
    // trigger a propgram control exception, we will rethrow the exception once the
    // JavaScript access is complete
    RethrowProgramControlExceptions();

    return result;
}


double LogicInterpreter::ex_JavaScript_eval(const int program_index)
{
    return ExecuteWithJavaScriptProcessor(
        [&](EngineJavaScriptProcessor& javascript_processor)
        {
            const auto& va_node = GetNode<Nodes::VariableArguments>(program_index);
            const int& bytecode_index = va_node.arguments[0];
            const int& script_expression = va_node.arguments[1];
            bool exception_is_from_compilation = false;

            try
            {
                // compile and evaluate the script if it was not already compiled
                if( bytecode_index == -1 )
                {
                    const SharableString script = EvaluateSharableString(script_expression);

                    exception_is_from_compilation = true;
                    return AssignString(javascript_processor.EvaluateScript(script.GetString(), exception_is_from_compilation));
                }

                // evaluate scripts that were already compiled (because the script was a string literal)
                else
                {
                    return AssignString(javascript_processor.EvaluateBytecode(bytecode_index));
                }
            }

            catch( const CSProException& exception )
            {
                const int message = exception_is_from_compilation ? MGF::JavaScript_compilation_error_100463 :
                                                                    MGF::JavaScript_evaluation_error_100464;
                IssueMessage(MessageType::Error, message, exception.what());
                return AssignStringNull();
            }
    });
}


double LogicInterpreter::ex_JavaScript_invoke(const int program_index)
{
    return ExecuteWithJavaScriptProcessor(
        [&](EngineJavaScriptProcessor& javascript_processor)
        {
            const auto& va_node = GetNode<Nodes::VariableArguments>(program_index);
            const SharableString function_name = EvaluateSharableString(va_node.arguments[0]);
            const Nodes::List& arguments_list = GetListNode(va_node.arguments[1]);

            ASSERT(( arguments_list.number_elements % 2 ) == 0);
            const size_t number_arguments = arguments_list.number_elements / 2;
            std::unique_ptr<JavaScript::Value[]> js_arguments;

            // evaluate each argument, which are defined in the same way used by the (logic) invoke function, pairs of:
            //     - -1 * expression type (as a SymbolType) + expression index
            //     - symbol index + symbol subscript compilation index
            if( arguments_list.number_elements > 0 )
            {
                js_arguments = std::make_unique_for_overwrite<JavaScript::Value[]>(number_arguments);

                JavaScript::Value* js_argument_itr = js_arguments.get();
                const int* elements_itr = arguments_list.elements;

                for( size_t i = 0; i < number_arguments; ++i, ++js_argument_itr, elements_itr += 2 )
                {
                    std::optional<JavaScript::Value> js_value = ConvertValueToJavaScript(javascript_processor, elements_itr[0], elements_itr[1]);

                    if( !js_value.has_value() )
                        return AssignStringNull();

                    new (js_argument_itr) JavaScript::Value(std::move(*js_value));
                }
            }

            try
            {
                const JavaScript::Value js_result = javascript_processor.InvokeFunction(function_name.GetString(),
                                                                                        number_arguments, js_arguments.get());
                return AssignString(js_result.ToString());
            }

            catch( const CSProException& exception )
            {
                IssueMessage(MessageType::Error, MGF::JavaScript_evaluation_error_100464, exception.what());
                return AssignStringNull();
            }
    });
}


double LogicInterpreter::ex_JavaScript_hasValue(const int program_index)
{
    return ExecuteWithJavaScriptProcessor(
        [&](EngineJavaScriptProcessor& javascript_processor)
        {
            const auto& va_node = GetNode<Nodes::VariableArguments>(program_index);
            const SharableString name = EvaluateSharableString(va_node.arguments[0]);

            return javascript_processor.HasPropertyValue(name.GetString());
        });
}


double LogicInterpreter::ex_JavaScript_getValueJson(const int program_index)
{
    return ExecuteWithJavaScriptProcessor(
        [&](EngineJavaScriptProcessor& javascript_processor)
        {
            const auto& va_node = GetNode<Nodes::VariableArguments>(program_index);
            const SharableString name = EvaluateSharableString(va_node.arguments[0]);

            try
            {
                return AssignString(javascript_processor.GetValueJson(name.GetString()));
            }

            catch( const CSProException& exception )
            {
                IssueMessage(MessageType::Error, MGF::JavaScript_evaluation_error_100464, exception.what());
                return AssignStringNull();
            }
        });
}


double LogicInterpreter::ex_JavaScript_setValueFromJson(const int program_index)
{
    return ExecuteWithJavaScriptProcessor(
        [&](EngineJavaScriptProcessor& javascript_processor)
        {
            const auto& va_node = GetNode<Nodes::VariableArguments>(program_index);
            const SharableString name = EvaluateSharableString(va_node.arguments[0]);
            const SharableString json_text = EvaluateSharableString(va_node.arguments[1]);
            bool exception_is_from_json_parsing = true;

            try
            {
                javascript_processor.SetValueFromJson(name.GetString(), json_text.GetString(), exception_is_from_json_parsing);
                return true;
            }

            catch( const CSProException& exception )
            {
                if( exception_is_from_json_parsing )
                {
                    IssueMessage(MessageType::Error, MGF::JSON_invalid_text_100430, json_text->c_str());
                }

                else
                {
                    IssueMessage(MessageType::Error, MGF::JavaScript_evaluation_error_100464, exception.what());
                }

                return false;
            }
    });
}


double LogicInterpreter::ex_JavaScript_getValue(const int program_index)
{
    return ExecuteWithJavaScriptProcessor(
        [&](EngineJavaScriptProcessor& javascript_processor)
        {
            const auto& va_node = GetNode<Nodes::VariableArguments>(program_index);
            const SharableString name = EvaluateSharableString(va_node.arguments[0]);
            std::optional<JavaScript::Value> js_value;
            std::optional<SymbolType> evaluated_symbol_type;

            try
            {
                js_value.emplace(javascript_processor.GetValue(name.GetString()));

                // if the method returns false, a runtime error should have already given
                // information about the error (e.g., an invalid Array index)
                return ConvertValueFromJavaScript(javascript_processor, *js_value,
                                                  va_node.arguments[1], va_node.arguments[2],
                                                  evaluated_symbol_type);
            }

            catch( const CSProException& exception )
            {
                if( js_value.has_value() )
                {
                    IssueMessage(MessageType::Error, MGF::JavaScript_value_conversion_error_100467,
                                                     evaluated_symbol_type.has_value() ? ToDisplayString(*evaluated_symbol_type) : ReturnProgrammingError(""),
                                                     exception.what());
                }

                else
                {
                    IssueMessage(MessageType::Error, MGF::JavaScript_evaluation_error_100464, exception.what());
                }

                return false;
            }
        });
}


double LogicInterpreter::ex_JavaScript_setValue(const int program_index)
{
    return ExecuteWithJavaScriptProcessor(
        [&](EngineJavaScriptProcessor& javascript_processor)
        {
            const auto& va_node = GetNode<Nodes::VariableArguments>(program_index);
            const SharableString name = EvaluateSharableString(va_node.arguments[0]);

            try
            {
                std::optional<JavaScript::Value> js_value = ConvertValueToJavaScript(javascript_processor, va_node.arguments[1], va_node.arguments[2]);

                if( js_value.has_value() )
                {
                    javascript_processor.SetValue(name.GetString(), std::move(*js_value));
                    return true;
                }
            }

            catch( const CSProException& exception )
            {
                IssueMessage(MessageType::Error, MGF::JavaScript_evaluation_error_100464, exception.what());
            }

            return false;
        });
}


std::optional<JavaScript::Value> LogicInterpreter::ConvertValueToJavaScript(EngineJavaScriptProcessor& javascript_processor,
                                                                            const int symbol_type_or_index,
                                                                            const int expression_or_symbol_subscript_compilation)
{
    // convert numeric or string expressions
    if( symbol_type_or_index < 0 )
    {
        const SymbolType symbol_type = static_cast<SymbolType>(-1 * symbol_type_or_index);

        if( symbol_type == SymbolType::WorkVariable )
        {
            const double value = Evaluate(expression_or_symbol_subscript_compilation);
            return javascript_processor.CreateValue(value);
        }

        else
        {
            ASSERT(symbol_type == SymbolType::WorkString);
            const SharableString value = EvaluateSharableString(expression_or_symbol_subscript_compilation);
            return javascript_processor.CreateValue(value.GetString());
        }
    }

    // convert symbols
    else
    {
        const Symbol* const symbol = GetFromSymbolOrEngineItem(symbol_type_or_index, expression_or_symbol_subscript_compilation);

        if( symbol == nullptr )
            return std::nullopt;

        return JavaScript::Value(javascript_processor.CreateValue(*symbol));
    }
}


double LogicInterpreter::ex_JavaScript_UserFunctionCall(const int program_index)
{
    return ExecuteWithJavaScriptProcessor(
        [&](EngineJavaScriptProcessor& javascript_processor)
        {
            const auto& javascript_function_call_node = GetNode<Nodes::FunctionCall>(program_index);
            UserFunction& user_function = GetSymbolUserFunction(javascript_function_call_node.expression);
            std::optional<JavaScript::Value> js_result;

            try
            {
                // convert all of the function's parameters to JavaScript values
                auto js_arguments = std::make_unique_for_overwrite<JavaScript::Value[]>(user_function.GetNumberParameters());
                JavaScript::Value* js_argument_itr = js_arguments.get();

                for( size_t i = 0; i < user_function.GetNumberParameters(); ++i, ++js_argument_itr )
                {
                    const Symbol& parameter_symbol = user_function.GetParameterSymbol(i);
                    new (js_argument_itr) JavaScript::Value(javascript_processor.CreateValue(parameter_symbol));
                }

                // execute the function
                js_result = javascript_processor.InvokeFunction(user_function.GetName(),
                                                                user_function.GetNumberParameters(), js_arguments.get());

                // when not undefined, convert the return value
                if( !js_result->IsUndefined() )
                {
                    if( IsNumeric(user_function.GetReturnDataType()) )
                    {
                        user_function.SetReturnValue(javascript_processor.ConvertNumeric(*js_result));
                    }

                    else
                    {
                        ASSERT(IsString(user_function.GetReturnDataType()));
                        user_function.SetReturnValue(javascript_processor.ConvertString(*js_result));
                    }
                }

                return 1;
            }

            catch( const CSProException& exception )
            {
                if( js_result.has_value() )
                {
                    IssueMessage(MessageType::Error, MGF::JavaScript_value_conversion_error_100467,
                                                     ToDisplayString(user_function.GetReturnType()), exception.what());
                }

                else
                {
                    IssueMessage(MessageType::Error, MGF::JavaScript_evaluation_error_100464, exception.what());
                }

                return 0;
            }
    });
}


bool LogicInterpreter::ConvertValueFromJavaScript(EngineJavaScriptProcessor& javascript_processor, const JavaScript::Value& js_value,
                                                  const int symbol_type_or_index, const int expression_or_symbol_subscript_compilation,
                                                  std::optional<SymbolType>& evaluated_symbol_type)
{
    ASSERT(!js_value.IsException());

    // convert numeric or string values
    if( symbol_type_or_index < 0 )
    {
        evaluated_symbol_type = static_cast<SymbolType>(-1 * symbol_type_or_index);
        const Nodes::SymbolValue& symbol_value_node = GetNode<Nodes::SymbolValue>(expression_or_symbol_subscript_compilation);

        if( *evaluated_symbol_type == SymbolType::WorkVariable )
        {
            return AssignValueToSymbol(symbol_value_node, javascript_processor.ConvertNumeric(js_value));
        }

        else
        {
            ASSERT(*evaluated_symbol_type == SymbolType::WorkString);
            return AssignValueToSymbol(symbol_value_node, javascript_processor.ConvertString(js_value));
        }
    }

    // convert symbols
    else
    {
        Symbol* const symbol = GetFromSymbolOrEngineItem(symbol_type_or_index, expression_or_symbol_subscript_compilation);

        if( symbol == nullptr )
        {
            evaluated_symbol_type = NPT_Ref(symbol_type_or_index).GetType();
            return false;
        }

        else
        {
            evaluated_symbol_type = symbol->GetType();
            javascript_processor.ConvertSymbol(js_value, *symbol);
            return true;
        }
    }
}
