#include "StandardSystemIncludes.h"
#include "INTERPRE.H"
#include "Engine.h"
#include <zEngineO/SystemApp.h>
#include <zEngineF/EngineUI.h>
#include <zPlatformO/PlatformInterface.h>
#include <zParadataO/Logger.h>


double CIntDriver::exsystemapp_clear(int iExpr)
{
    const auto& symbol_va_node = GetNode<Nodes::SymbolVariableArguments>(iExpr);
    SystemApp& system_app = GetSymbolSystemApp(symbol_va_node.symbol_index);

    system_app.Reset();

    return 1;
}


double CIntDriver::exsystemapp_setargument(int iExpr)
{
    const auto& symbol_va_node = GetNode<Nodes::SymbolVariableArguments>(iExpr);
    SystemApp& system_app = GetSymbolSystemApp(symbol_va_node.symbol_index);

    std::wstring argument_name = EvalAlphaExpr(symbol_va_node.arguments[0]);

    int value_expression = symbol_va_node.arguments[1];
    DataType value_type = static_cast<DataType>(symbol_va_node.arguments[2]);
    std::optional<std::variant<double, std::wstring>> value;

    if( value_type == DataType::String )
    {
        value = EvalAlphaExpr(value_expression);
    }

    else if( value_type == DataType::Numeric )
    {
        value = evalexpr(value_expression);
    }

    system_app.SetArgument(std::move(argument_name), std::move(value));

    return 1;
}


double CIntDriver::exsystemapp_getresult(int iExpr)
{
    const auto& symbol_va_node = GetNode<Nodes::SymbolVariableArguments>(iExpr);
    const SystemApp& system_app = GetSymbolSystemApp(symbol_va_node.symbol_index);

    std::wstring result_name = EvalAlphaExpr(symbol_va_node.arguments[0]);
    std::wstring result = system_app.GetResult(result_name);

    return AssignAlphaValue(std::move(result));
}


double CIntDriver::exsystemapp_exec(int iExpr)
{
    const auto& symbol_va_node = GetNode<Nodes::SymbolVariableArguments>(iExpr);
    SystemApp& system_app = GetSymbolSystemApp(symbol_va_node.symbol_index);

    std::wstring package_name;

    if( symbol_va_node.arguments[0] >= 0 )
    {
        package_name = EvalAlphaExpr(symbol_va_node.arguments[0]);
        SO::MakeTrimRight(package_name);
    }

    // the package name (executable) must be specified on Windows
    if( OnWindowsDesktop() && package_name.empty() )
        return 0;

    std::optional<std::wstring> activity_name;
#ifdef ANDROID
    if( symbol_va_node.arguments[1] >= 0 )
        activity_name = EvalAlphaExpr(symbol_va_node.arguments[1]);
#endif

    EngineUI::ExecSystemAppNode exec_system_app_node { system_app, package_name, package_name, activity_name };

    // for Windows, and for paradata, get a string of the evaluated arguments
    if( activity_name.has_value() )
        SO::AppendFormat(exec_system_app_node.evaluated_call, _T("(%s)"), activity_name->c_str());

    // add each of the arguments
    for( const SystemApp::Argument& argument : system_app.GetArguments() )
    {
        if( argument.value.has_value() )
        {
            constexpr const TCHAR* ArgumentFormatter = OnAndroid() ? _T(" %s=%s") :
                                                                     _T(" %s%s");

            std::wstring string_value = std::holds_alternative<std::wstring>(*argument.value) ? std::get<std::wstring>(*argument.value) :
                                                                                                DoubleToString(std::get<double>(*argument.value));

            if constexpr(OnAndroid())
            {
                string_value = EscapeCommandLineArgument(string_value);
            }

            SO::AppendFormat(exec_system_app_node.evaluated_call, ArgumentFormatter, argument.name.c_str(), string_value.c_str());
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
            Paradata::ExternalApplicationEvent::Source::SystemAppExec, exec_system_app_node.evaluated_call, false);
    }

    bool success = ( SendEngineUIMessage(EngineUI::Type::ExecSystemApp, exec_system_app_node) != 0 );

    // on Windows, the execution command has to be run in this thread
    if( success && exec_system_app_node.function_to_run_in_engine_thread )
        success = exec_system_app_node.function_to_run_in_engine_thread();
    
    if( external_application_event != nullptr )
    {
        external_application_event->SetPostExecutionValues(success, true);
        m_pParadataDriver->RegisterAndLogEvent(std::move(external_application_event));
    }

    return success ? 1 : 0;
}
