#include "stdafx.h"
#include "IncludesRT.h"
#include "SystemApp.h"
#include <zParadataO/Logger.h>


Engine::Value LogicInterpreter::ex_SystemApp_clear(const int program_index)
{
    const auto& symbol_va_node = GetNode<Nodes::SymbolVariableArguments>(program_index);
    SystemApp& system_app = GetSymbolSystemApp(symbol_va_node.symbol_index);

    system_app.Reset();

    return Engine::Value::Bool(true);
}


Engine::Value LogicInterpreter::ex_SystemApp_setArgument(const int program_index)
{
    const auto& symbol_va_node = GetNode<Nodes::SymbolVariableArguments>(program_index);
    SystemApp& system_app = GetSymbolSystemApp(symbol_va_node.symbol_index);

    std::string argument_name = Evaluate<std::string>(symbol_va_node.arguments[0]);

    std::optional<std::variant<double, SharableString>> value = ( symbol_va_node.arguments[1] != -1 )
        ? std::make_optional(EvaluateVariant(static_cast<DataType>(symbol_va_node.arguments[2]), symbol_va_node.arguments[1]))
        : std::nullopt;

    system_app.SetArgument(std::move(argument_name), std::move(value));

    return Engine::Value::Bool(true);
}


Engine::Value LogicInterpreter::ex_SystemApp_getResult(const int program_index)
{
    const auto& symbol_va_node = GetNode<Nodes::SymbolVariableArguments>(program_index);
    const SystemApp& system_app = GetSymbolSystemApp(symbol_va_node.symbol_index);

    const SharableString result_name = Evaluate<SharableString>(symbol_va_node.arguments[0]);

    return system_app.GetResult(*result_name);
}


Engine::Value LogicInterpreter::ex_SystemApp_exec(const int program_index)
{
    const auto& symbol_va_node = GetNode<Nodes::SymbolVariableArguments>(program_index);
    EngineUI::ExecSystemAppNode exec_system_app_node { GetSymbolSystemApp(symbol_va_node.symbol_index) };

    if( symbol_va_node.arguments[0] >= 0 )
    {
        exec_system_app_node.package_name = Evaluate<SharableString>(symbol_va_node.arguments[0]);
        exec_system_app_node.package_name.MakeTrimRight();
    }

    // the package name (executable) must be specified on Windows
    if( OnWindowsDesktop() && exec_system_app_node.package_name->empty() )
        return Engine::Value::Bool(false);

    // on Android, evaluate the activity
    if( OnAndroid() && symbol_va_node.arguments[1] >= 0 )
        exec_system_app_node.activity_name = Evaluate<SharableString>(symbol_va_node.arguments[1]);

    // for Windows execution, and for the paradata log, get a string of the evaluated arguments
    exec_system_app_node.evaluated_call = *exec_system_app_node.package_name;

    if( exec_system_app_node.activity_name.IsSet() )
        exec_system_app_node.evaluated_call.append(FormatText("(%s)", exec_system_app_node.activity_name->c_str()));

    // add each of the arguments
    for( const SystemApp::Argument& argument : exec_system_app_node.system_app.GetArguments() )
    {
        if( argument.value.has_value() )
        {
            constexpr const char* ArgumentFormatter = OnAndroid() ? " %s=%s" :
                                                                    " %s%s";

            SharableString string_value = std::holds_alternative<SharableString>(*argument.value)
                ? std::get<SharableString>(*argument.value)
                : DoubleToString(std::get<double>(*argument.value));

            if constexpr(OnAndroid())
            {
                string_value = EscapeCommandLineArgument(*string_value);
            }

            exec_system_app_node.evaluated_call.append(
                FormatText(ArgumentFormatter, argument.name.c_str(), string_value->c_str())
            );
        }

        else
        {
            SO::AppendWithSeparator(exec_system_app_node.evaluated_call, EscapeCommandLineArgument(argument.name), ' ');
        }
    }

    std::unique_ptr<Paradata::ExternalApplicationEvent> external_application_event;

    if( Paradata::Logger::IsOpen() )
    {
        external_application_event = std::make_unique<Paradata::ExternalApplicationEvent>(
            Paradata::ExternalApplicationEvent::Source::SystemAppExec,
            exec_system_app_node.evaluated_call,
            false
        );
    }

    bool success = ( SendEngineUIMessage(EngineUI::Type::ExecSystemApp, exec_system_app_node) != 0 );

    // on Windows, the execution command has to be run in this thread
    if( success && exec_system_app_node.function_to_run_in_engine_thread )
        success = exec_system_app_node.function_to_run_in_engine_thread();

    if( external_application_event != nullptr )
    {
        external_application_event->SetPostExecutionValues(success, true);
        RegisterAndLogEvent_INTERPRETER_DLL_TODO(std::move(external_application_event));
    }

    return Engine::Value::Bool(success);
}
