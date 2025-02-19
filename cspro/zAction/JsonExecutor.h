#pragma once

#include <zAction/zAction.h>
#include <zAction/JsonResponse.h>

namespace ActionInvoker { class JsonExecutor; }


// the ActionInvoker::JsonExecutor class runs actions specified in JSON format
// (by CSCode or gov.census.cspro.ActionInvokerActivity)

class ZACTION_API ActionInvoker::JsonExecutor
{
public:
    JsonExecutor(bool is_parser_only);
    virtual ~JsonExecutor() { }

    void SetAbortOnException(bool abort_on_exception) { m_abortOnException = abort_on_exception; }

    // parses the text into actions, validating that each action is defined properly
    void ParseActions(std::string_view text_sv);

    // runs the actions
    void RunActions(Caller& caller);

    // returns the results as JSON text
    SharableString ReleaseResultsJson();

protected:
    // the base implementations add the result to a JSON object or array of objects
    virtual void ProcessActionResult(Result result)                   { ProcessActionResult(JsonResponse(result)); }
    virtual void ProcessActionResult(const CSProException& exception) { ProcessActionResult(JsonResponse(exception)); }

private:
    void ParseActionNode(const JsonNode& action_node);

    void ProcessActionResult(const JsonResponse& json_response);

private:
    struct RuntimeData
    {
        std::vector<std::string> action_node_jsons;
        bool write_results_as_array = false;
        std::unique_ptr<std::string> results_json;
    };

    std::unique_ptr<RuntimeData> m_runtimeData;
    bool m_abortOnException;
};
