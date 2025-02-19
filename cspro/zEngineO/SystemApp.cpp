#include "stdafx.h"
#include "SystemApp.h"


// --------------------------------------------------------------------------
// SystemApp
// --------------------------------------------------------------------------

SystemApp::SystemApp(std::string system_app_name)
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


void SystemApp::SetArgument(std::string argument_name, std::optional<std::variant<double, SharableString>> value)
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


SharableString SystemApp::GetResult(const std::string& result_name) const
{
    const auto& result_lookup = std::find_if(m_results.cbegin(), m_results.cend(),
                                             [&](const Result& result) { return SO::EqualsNoCase(result.name, result_name); });

    return ( result_lookup != m_results.cend() ) ? result_lookup->value :
                                                   SharableString();
}


void SystemApp::SetResult(std::string result_name, SharableString value)
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


void SystemApp::SetValueFromJson(const JsonNode& json_node)
{
    std::vector<Argument> arguments;

    for( const JsonNode& argument_node : json_node.GetArrayOrEmpty(JK::arguments) )
    {
        arguments.emplace_back(
            Argument
            {
                argument_node.Get<std::string>(JK::name),
                argument_node.Contains(JK::value) ? std::make_optional(argument_node.GetEngineValue<std::variant<double, SharableString>>(JK::value)) : std::nullopt
            });
    }

    std::vector<Result> results;

    for( const JsonNode& result_node : json_node.GetArrayOrEmpty(JK::results) )
    {
        results.emplace_back(
            Result
            {
                result_node.Get<std::string>(JK::name),
                result_node.GetEngineValue<SharableString>(JK::value)
            });
    }

    m_arguments = std::move(arguments);
    m_results = std::move(results);
}
