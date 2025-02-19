#pragma once

#include <zLogicO/ActionInvoker.h>
#include <zLogicO/GeneralizedFunction.h>
#include <zEngineO/Messages/EngineMessages.h>


namespace ActionInvoker
{
    // returns the action's namespace.name
    std::string GetActionName(const Action* action);
    inline std::string GetActionName(Action action) { return GetActionName(&action); }

    // parses the text for a valid action namespace.name, throwing an exception on error
    template<typename T>
    Action GetActionFromText(std::string_view full_name_sv, T& error_issuer);
}



inline std::string ActionInvoker::GetActionName(const Action* const action)
{
    if( action != nullptr )
    {
        const GF::Function* const function_definition = GetFunctionDefinition(*action);

        if( function_definition != nullptr )
        {
            if( function_definition->namespace_name.empty() )
                return function_definition->name;

            return function_definition->namespace_name + "." + function_definition->name;
        }

        ASSERT(false);
    }

    return "unknown";
}


template<typename T>
ActionInvoker::Action ActionInvoker::GetActionFromText(const std::string_view full_name_sv, T& error_issuer)
{
    std::variant<SymbolType, Logic::FunctionNamespace> symbol_type_or_function_namespace = Logic::FunctionNamespace::CS;

    auto [namespace_sv, action_name_sv] = SO::GetTextOnEitherSideOfCharacter(full_name_sv, '.');
    namespace_sv = SO::Trim(namespace_sv);
    action_name_sv = SO::Trim(action_name_sv);

    if( action_name_sv.empty() )
    {
        action_name_sv = namespace_sv;
    }

    // validate the namespace
    else
    {
        const Logic::FunctionNamespaceDetails* function_namespace_details;

        if( !Logic::FunctionTable::IsFunctionNamespace(namespace_sv, symbol_type_or_function_namespace, &function_namespace_details) )
        {
            error_issuer.IssueError(MGF::CS_action_invalid_9202, std::string(full_name_sv).c_str());
        }

        if( !SO::Equals(namespace_sv, function_namespace_details->name) )
        {
            error_issuer.IssueError(MGF::CS_action_invalid_case_9203, std::string(namespace_sv).c_str(),
                                                                      function_namespace_details->name);
        }

        symbol_type_or_function_namespace = function_namespace_details->function_namespace;
    }

    // validate the action
    const Logic::FunctionDetails* function_details;

    if( !Logic::FunctionTable::IsFunction(action_name_sv, symbol_type_or_function_namespace, &function_details) )
        error_issuer.IssueError(MGF::CS_action_invalid_9202, std::string(full_name_sv).c_str());

    if( !SO::Equals(action_name_sv, function_details->name) )
        error_issuer.IssueError(MGF::CS_action_invalid_case_9203, std::string(action_name_sv).c_str(), function_details->name);

    return static_cast<Action>(function_details->number_arguments);
}
