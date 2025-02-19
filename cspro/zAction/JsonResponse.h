#pragma once

#include <zAction/ActionInvoker.h>

namespace ActionInvoker { class JsonResponse; }


class ActionInvoker::JsonResponse
{
public:
    JsonResponse(const Result& result);
    JsonResponse(const CSProException& exception);

    SharableString GetResponseText() const { return m_responseText; }

    template<bool parse_json_text_for_type>
    static const char* GetResultTypeText(const Result& result);

private:
    static SharableString CreateResponseText(const Result& result);
    static constexpr const char* GetResultTypeText(Result::Type result_type);

    static std::string GetExceptionJson(const CSProException& exception);

private:
    SharableString m_responseText;
};



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

inline ActionInvoker::JsonResponse::JsonResponse(const Result& result)
    :   m_responseText(CreateResponseText(result))
{
    AssertValidJson(*m_responseText);
}


inline SharableString ActionInvoker::JsonResponse::CreateResponseText(const Result& result)
{
    if( result.GetType() == Result::Type::JsonText )
    {
        return SO::Concatenate("{\"type\":\"json\",\"value\":",
                               result.GetStringResult().GetString(),
                               "}");
    }

    else if( result.GetType() == Result::Type::Undefined )
    {
        const std::string_view UndefinedResponse_sv = "{\"type\":\"undefined\"}";
        return UndefinedResponse_sv;
    }

    else
    {
        ASSERT(result.GetType() == Result::Type::Bool ||
               result.GetType() == Result::Type::Number ||
               result.GetType() == Result::Type::String);

        return SO::Concatenate("{\"type\":\"",
                               GetResultTypeText(result.GetType()),
                               "\",\"value\":",
                               result.GetResultAsJsonText<false>().GetString(),
                               "}");
    }
}


template<bool parse_json_text_for_type>
const char* ActionInvoker::JsonResponse::GetResultTypeText(const Result& result)
{
    if constexpr(parse_json_text_for_type)
    {
        if( result.GetType() == Result::Type::JsonText )
        {
            try
            {
                const JsonNode json_node = Json::Parse(result.GetResultAsString<false>().GetString());

                // listed in order of likely occurrence
                return json_node.IsObject()  ? "object" :
                       json_node.IsArray()   ? "array" :
                       json_node.IsString()  ? GetResultTypeText(Result::Type::String) :
                       json_node.IsNumber()  ? GetResultTypeText(Result::Type::Number) :
                       json_node.IsBoolean() ? GetResultTypeText(Result::Type::Bool) :
                       json_node.IsNull()    ? "null" :
                                               ReturnProgrammingError(GetResultTypeText(Result::Type::Undefined));
            }
            catch(...) { ASSERT(false); }
        }
    }

    return GetResultTypeText(result.GetType());
}


inline constexpr const char* ActionInvoker::JsonResponse::GetResultTypeText(const Result::Type result_type)
{
    constexpr const char* ResultTypes[] = { "undefined", "boolean", "number", "string", "json" };

    ASSERT(static_cast<size_t>(result_type) < _countof(ResultTypes));
    ASSERT(static_cast<size_t>(Result::Type::JsonText) == ( _countof(ResultTypes) - 1 ));

    return ResultTypes[static_cast<size_t>(result_type)];
}


inline ActionInvoker::JsonResponse::JsonResponse(const CSProException& exception)
    :   m_responseText(SO::Concatenate("{\"type\":\"exception\",\"value\":", GetExceptionJson(exception), "}"))
{
    AssertValidJson(*m_responseText);
}


inline std::string ActionInvoker::JsonResponse::GetExceptionJson(const CSProException& exception)
{
    const ActionInvoker::Exception* const action_invoker_exception = dynamic_cast<const ActionInvoker::Exception*>(&exception);

    if( action_invoker_exception == nullptr )
        return ReturnProgrammingError(Encoders::ToJsonString(exception.what()));

    std::string json_text = SO::Concatenate("{\"name\":", Encoders::ToJsonString(action_invoker_exception->GetName()),
                                            ",\"message\":", Encoders::ToJsonString(action_invoker_exception->what()));

    if( !action_invoker_exception->GetCause().empty() )
    {
        AssertValidJson(action_invoker_exception->GetCause());

        json_text.append(",\"cause\":")
                 .append(action_invoker_exception->GetCause());
    }

    json_text.push_back('}');

    return json_text;
}
