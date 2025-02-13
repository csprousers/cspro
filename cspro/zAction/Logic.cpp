#include "stdafx.h"


template<typename CF>
ActionInvoker::Result ActionInvoker::Runtime::Logic_executeWorker(const CF callback_function)
{
    InterpreterExecuteResult execute_result = callback_function(GetInterpreterAccessor());

    if( execute_result.program_control_executed )
    {
        // let all listeners know that a program control statement was executed (which will generally mean that dialogs will close)
        IterateOverListeners(
            [&](Listener& listener)
            {
                listener.OnEngineProgramControlExecuted();
                return true;
            });
    }

    return Result::NumberOrString(std::move(execute_result.result));
}


ActionInvoker::Result ActionInvoker::Runtime::Logic_eval(const JsonNode<wchar_t>& json_node, Caller& caller)
{
    return Logic_executeWorker(
        [&](InterpreterAccessor& interpreter_accessor)
        {
            return interpreter_accessor.RunEvaluateLogic(json_node.Get<std::wstring>(JK::logic),
                                                         caller.GetCancelFlag());
        });
}


ActionInvoker::Result ActionInvoker::Runtime::Logic_invoke(const JsonNode<wchar_t>& json_node, Caller& caller)
{
    return Logic_executeWorker(
        [&](InterpreterAccessor& interpreter_accessor)
        {
            return interpreter_accessor.RunInvoke(json_node.Get<std::wstring>(JK::function),
                                                  json_node.GetOrEmpty(JK::arguments),
                                                  caller.GetCancelFlag());
        });
}


template<typename SJO>
ActionInvoker::Result ActionInvoker::Runtime::Logic_getSymbolWorker(const JsonNode<wchar_t>& json_node, const SJO symbol_json_output)
{
    const std::wstring symbol_name = json_node.Get<std::wstring>(JK::name);

    std::unique_ptr<const JsonNode<wchar_t>> serialization_options_node;

    if( json_node.Contains(JK::serializationOptions) )
        serialization_options_node = std::make_unique<JsonNode<wchar_t>>(json_node.Get(JK::serializationOptions));

    return Result::JsonText(GetInterpreterAccessor().GetSymbolJson(symbol_name, symbol_json_output, serialization_options_node.get()));
}


ActionInvoker::Result ActionInvoker::Runtime::Logic_getSymbol(const JsonNode<wchar_t>& json_node, Caller& /*caller*/)
{
    return Logic_getSymbolWorker(json_node, Symbol::SymbolJsonOutput::MetadataAndValue);
}


ActionInvoker::Result ActionInvoker::Runtime::Logic_getSymbolMetadata(const JsonNode<wchar_t>& json_node, Caller& /*caller*/)
{
    return Logic_getSymbolWorker(json_node, Symbol::SymbolJsonOutput::Metadata);
}


ActionInvoker::Result ActionInvoker::Runtime::Logic_getSymbolValue(const JsonNode<wchar_t>& json_node, Caller& /*caller*/)
{
    return Logic_getSymbolWorker(json_node, Symbol::SymbolJsonOutput::Value);
}


ActionInvoker::Result ActionInvoker::Runtime::Logic_updateSymbolValue(const JsonNode<wchar_t>& json_node, Caller& /*caller*/)
{
    const std::wstring symbol_name = json_node.Get<std::wstring>(JK::name);
    GetInterpreterAccessor().UpdateSymbolValueFromJson(symbol_name, json_node.Get(JK::value));

    return Result::Undefined();
}
