#include "stdafx.h"
#include "JsonExecutor.h"


ActionInvoker::JsonExecutor::JsonExecutor(const bool is_parser_only)
    :   m_runtimeData(is_parser_only ? nullptr : std::make_unique<RuntimeData>()),
        m_abortOnException(true)
{
}


void ActionInvoker::JsonExecutor::ParseActions(const std::wstring& text)
{
    const JsonNode<wchar_t> json_node = Json::Parse(text);

    if( json_node.IsObject() )
    {
        ParseActionNode(json_node);
    }

    else if( json_node.IsArray() )
    {
        if( m_runtimeData != nullptr )
            m_runtimeData->write_results_as_array = true;

        for( const auto& action_node : json_node.GetArray() )
            ParseActionNode(action_node);
    }

    else
    {
        throw CSProException("The Action Invoker expects a single action or an array of actions.");
    }
}


void ActionInvoker::JsonExecutor::ParseActionNode(const JsonNode<wchar_t>& action_node)
{
    if( !action_node.IsObject() || !action_node.Contains(JK::action) )
        throw CSProException(_T("Each Action Invoker entry must contain an \"%s\" key."), JK::action);

    if( m_runtimeData != nullptr )
        m_runtimeData->action_node_jsons.emplace_back(action_node.GetNodeAsString());
}


void ActionInvoker::JsonExecutor::RunActions(Caller& caller)
{
    ASSERT(m_runtimeData != nullptr);

    std::shared_ptr<Runtime> action_invoker_runtime;

    try
    {
        action_invoker_runtime = ObjectTransporter::GetActionInvokerRuntime();
        ASSERT(action_invoker_runtime != nullptr);

        // make sure the access token is valid
        if( caller.IsExternalCaller() )
            action_invoker_runtime->CheckAccessToken(nullptr, caller);
    }

    catch( const CSProException& exception )
    {
        ProcessActionResult(exception);
        return;
    }
    
    // process each action
    for( const std::wstring& json_arguments : m_runtimeData->action_node_jsons )
    {
        try
        {
            ProcessActionResult(action_invoker_runtime->ProcessExecute(json_arguments, caller));
        }

        catch( const CSProException& exception )
        {
            ProcessActionResult(exception);

            if( m_abortOnException )
                return;
        }

        if( caller.GetCancelFlag() )
            return;
    }
}


void ActionInvoker::JsonExecutor::ProcessActionResult(const JsonResponse& json_response)
{
    ASSERT(m_runtimeData != nullptr);

    if( m_runtimeData->results_json == nullptr )
        m_runtimeData->results_json = std::make_shared<std::wstring>();

    if( m_runtimeData->write_results_as_array )
        m_runtimeData->results_json->push_back(m_runtimeData->results_json->empty() ? '[' : ',');

    m_runtimeData->results_json->append(json_response.GetResponseText());
}


std::shared_ptr<const std::wstring> ActionInvoker::JsonExecutor::GetResultsJson() const
{
    ASSERT(m_runtimeData != nullptr);

    std::shared_ptr<const std::wstring> results_json = m_runtimeData->results_json;

    // if there were no actions (and no exceptions), return an empty array
    if( results_json == nullptr )
    {
        ASSERT(m_runtimeData->action_node_jsons.empty());

        results_json = std::make_shared<std::wstring>(Json::Text::EmptyArray);
    }

    // otherwise end the array (if necessary)
    else if( m_runtimeData->write_results_as_array )
    {
        ASSERT(results_json->front() == '[');

        results_json = std::make_shared<std::wstring>(*results_json + _T("]"));
    }

    else
    {
        ASSERT(results_json->front() == '{');
    }

    AssertValidJson(*results_json);

    return results_json;
}
