#include "stdafx.h"
#include <zUtilO/Versioning.h>


template<typename CF>
ActionInvoker::Result ActionInvoker::Runtime::Logic_executeWorker(const CF callback_function)
{
    InterpreterAccessor& interpreter_accessor = GetInterpreterAccessor();
    InterpreterExecuteResult execute_result = callback_function(interpreter_accessor);

    if( execute_result.program_control_executed )
    {
        // let all listeners know that a program control statement was executed
        // (which will generally mean that dialogs will close)
        IterateOverListeners(
            [&](Listener& listener)
            {
                listener.OnEngineProgramControlExecuted();
                return true;
            });
    }

    return Result::NumberOrString(interpreter_accessor.CreateVariantFromEngineValue(std::move(execute_result.result)));
}


ActionInvoker::Result ActionInvoker::Runtime::Logic_eval(const JsonNode& json_node, Caller& caller)
{
    return Logic_executeWorker(
        [&](InterpreterAccessor& interpreter_accessor)
        {
            return interpreter_accessor.RunEvaluateLogic(json_node.Get<SharableString>(JK::logic),
                                                         caller.GetCancelFlag());
        });
}


ActionInvoker::Result ActionInvoker::Runtime::Logic_invoke(const JsonNode& json_node, Caller& caller)
{
    return Logic_executeWorker(
        [&](InterpreterAccessor& interpreter_accessor)
        {
            return interpreter_accessor.RunInvoke(json_node.Get<std::string_view>(JK::function),
                                                  json_node.GetOrEmpty(JK::arguments),
                                                  caller.GetCancelFlag());
        });
}


template<typename SJO>
ActionInvoker::Result ActionInvoker::Runtime::Logic_getSymbolWorker(const JsonNode& json_node, const SJO symbol_json_output)
{
    const std::string symbol_name = json_node.Get<std::string>(JK::name);

    std::unique_ptr<const JsonNode> serialization_options_node;

    if( json_node.Contains(JK::serializationOptions) )
        serialization_options_node = std::make_unique<JsonNode>(json_node.Get(JK::serializationOptions));

    return Result::JsonText(GetInterpreterAccessor().GetSymbolJson(symbol_name, symbol_json_output, serialization_options_node.get()));
}


ActionInvoker::Result ActionInvoker::Runtime::Logic_getSymbol(const JsonNode& json_node, Caller& /*caller*/)
{
    return Logic_getSymbolWorker(json_node, Symbol::SymbolJsonOutput::MetadataAndValue);
}


ActionInvoker::Result ActionInvoker::Runtime::Logic_getSymbolMetadata(const JsonNode& json_node, Caller& /*caller*/)
{
    return Logic_getSymbolWorker(json_node, Symbol::SymbolJsonOutput::Metadata);
}


ActionInvoker::Result ActionInvoker::Runtime::Logic_getSymbolValue(const JsonNode& json_node, Caller& /*caller*/)
{
    return Logic_getSymbolWorker(json_node, Symbol::SymbolJsonOutput::Value);
}


ActionInvoker::Result ActionInvoker::Runtime::Logic_setSymbolValue(const JsonNode& json_node, Caller& /*caller*/)
{
    const std::string symbol_name = json_node.Get<std::string>(JK::name);
    GetInterpreterAccessor().SetSymbolValueFromJson(symbol_name, json_node.Get(JK::value));

    return Result::Undefined();
}


ActionInvoker::Result ActionInvoker::Runtime::Logic_updateSymbolValue(const JsonNode& json_node, Caller& caller)
{
    static_assert(Versioning::Number <= 8.1, "Start adding runtime warnings when using Logic.updateSymbolValue as opposed to Logic.setSymbolValue");
    return Logic_setSymbolValue(json_node, caller);
}
