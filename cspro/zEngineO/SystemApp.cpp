#include "stdafx.h"
#include "SystemApp.h"


// --------------------------------------------------------------------------
// SystemApp
// --------------------------------------------------------------------------

SystemApp::SystemApp(std::wstring system_app_name)
    :   Symbol(std::move(system_app_name), SymbolType::SystemApp)
{
}


SystemApp::SystemApp(const SystemApp& system_app)
    :   Symbol(system_app)
{
}


std::unique_ptr<Symbol> SystemApp::CloneInInitialState() const
{
    return std::unique_ptr<SystemApp>(new SystemApp(*this));
}


void SystemApp::Reset()
{
    m_arguments.clear();
    m_results.clear();
}


void SystemApp::SetArgument(std::wstring argument_name, std::optional<std::variant<double, std::wstring>> value)
{
    auto argument_lookup = std::find_if(m_arguments.begin(), m_arguments.end(),
                                        [&](const Argument& argument) { return SO::EqualsNoCase(argument.name, argument_name); });

    // add a new argument
    if( argument_lookup == m_arguments.end() )
    {
        m_arguments.emplace_back(Argument { std::move(argument_name), std::move(value) });
    }

    // or replace the existing one
    else
    {
        argument_lookup->value = std::move(value);
    }
}


const std::wstring& SystemApp::GetResult(const std::wstring& result_name) const
{
    const auto& result_lookup = std::find_if(m_results.cbegin(), m_results.cend(),
                                             [&](const Result& result) { return SO::EqualsNoCase(result.name, result_name); });

    return ( result_lookup != m_results.cend() ) ? result_lookup->value :
                                                   SO::EmptyString;
}


void SystemApp::SetResult(std::wstring result_name, std::wstring value)
{
    auto result_lookup = std::find_if(m_results.begin(), m_results.end(),
                                      [&](const Result& result) { return SO::EqualsNoCase(result.name, result_name); });

    // add a new result
    if( result_lookup == m_results.end() )
    {
        m_results.emplace_back(Result { std::move(result_name), std::move(value) });
    }

    // or replace the existing one
    else
    {
        result_lookup->value = std::move(value);
    }
}


void SystemApp::WriteValueToJson(JsonWriter& json_writer) const
{
    json_writer.BeginObject();

    json_writer.WriteObjects(JK::arguments, m_arguments,
        [&](const Argument& argument)
        {
            json_writer.Write(JK::name, argument.name);

            if( argument.value.has_value() )
                json_writer.WriteEngineValue(JK::value, *argument.value);
        });

    json_writer.WriteObjects(JK::results, m_results,
        [&](const Result& result)
        {
            json_writer.Write(JK::name, result.name)
                       .WriteEngineValue(JK::value, result.value);
        });

    json_writer.EndObject();
}


void SystemApp::UpdateValueFromJson(const JsonNode<wchar_t>& json_node)
{
    std::vector<Argument> arguments;    

    for( const auto& argument_node : json_node.GetArrayOrEmpty(JK::arguments) )
    {
        arguments.emplace_back(
            Argument
            {
                argument_node.Get<std::wstring>(JK::name),
                argument_node.Contains(JK::value) ? std::make_optional(argument_node.GetEngineValue<std::variant<double, std::wstring>>(JK::value)) : std::nullopt
            });
    }

    std::vector<Result> results;

    for( const auto& result_node : json_node.GetArrayOrEmpty(JK::results) )
    {
        results.emplace_back(
            Result
            {
                result_node.Get<std::wstring>(JK::name),
                result_node.GetEngineValue<std::wstring>(JK::value)
            });
    }

    m_arguments = std::move(arguments);
    m_results = std::move(results);
}
