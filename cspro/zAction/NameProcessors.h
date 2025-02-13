#pragma once

#include <zLogicO/ActionInvoker.h>
#include <zLogicO/GeneralizedFunction.h>
#include <zEngineO/Messages/EngineMessages.h>


namespace ActionInvoker
{
    // returns the action's namespace.name
    std::wstring GetActionName(const Action* action);
    inline std::wstring GetActionName(Action action) { return GetActionName(&action); }

    // parses the text for a valid action namespace.name, throwing an exception on error
    template<typename T>
    Action GetActionFromText(wstring_view full_name_sv, T& error_issuer);
}


inline std::wstring ActionInvoker::GetActionName(const Action* action)
{
    if( action != nullptr )
    {
        const GF::Function* function_definition = GetFunctionDefinition(*action);

        if( function_definition != nullptr )
        {
            if( function_definition->namespace_name.empty() )
                return function_definition->name;

            return function_definition->namespace_name + _T(".") + function_definition->name;
        }

        ASSERT(false);
    }

    return _T("unknown");
}


template<typename T>
ActionInvoker::Action ActionInvoker::GetActionFromText(wstring_view full_name_sv, T& error_issuer)
{
    std::variant<SymbolType, Logic::FunctionNamespace> symbol_type_or_function_namespace = Logic::FunctionNamespace::CS;

    size_t dot_pos = full_name_sv.find('.');

    static_assert(( wstring_view::npos + 1 ) == 0);
    wstring_view action_name_sv = SO::Trim(full_name_sv.substr(dot_pos + 1));

    // validate the namespace
    if( dot_pos != wstring_view::npos )
    {
        wstring_view namespace_sv = SO::Trim(full_name_sv.substr(0, dot_pos));

        const Logic::FunctionNamespaceDetails* function_namespace_details;

        if( !Logic::FunctionTable::IsFunctionNamespace(namespace_sv, symbol_type_or_function_namespace, &function_namespace_details) )
            error_issuer.IssueError(MGF::CS_action_invalid_9202, std::wstring(full_name_sv).c_str());

        if( !SO::Equals(namespace_sv, function_namespace_details->name) )
            error_issuer.IssueError(MGF::CS_action_invalid_case_9203, std::wstring(namespace_sv).c_str(), function_namespace_details->name);

        symbol_type_or_function_namespace = function_namespace_details->function_namespace;
    }

    // validate the action
    const Logic::FunctionDetails* function_details;

    if( !Logic::FunctionTable::IsFunction(action_name_sv, symbol_type_or_function_namespace, &function_details) )
        error_issuer.IssueError(MGF::CS_action_invalid_9202, std::wstring(full_name_sv).c_str());

    if( !SO::Equals(action_name_sv, function_details->name) )
        error_issuer.IssueError(MGF::CS_action_invalid_case_9203, std::wstring(action_name_sv).c_str(), function_details->name);

    return static_cast<Action>(function_details->number_arguments);
}
