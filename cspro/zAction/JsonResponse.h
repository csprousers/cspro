#pragma once

#include <zAction/ActionInvoker.h>


namespace ActionInvoker
{
    class JsonResponse
    {
    public:
        JsonResponse(const Result& result);
        JsonResponse(const CSProException& exception);

        const std::wstring& GetResponseText() const                       { return *m_responseText; }
        std::shared_ptr<const std::wstring> GetSharedResponseText() const { return m_responseText; }

        template<bool parse_json_text_for_type>
        static const TCHAR* GetResultTypeText(const Result& result);

        static std::wstring GetExceptionText(const CSProException& exception);

    private:
        static constexpr const TCHAR* GetResultTypeText(Result::Type result_type);

    private:
        std::shared_ptr<std::wstring> m_responseText;
    };
}


inline ActionInvoker::JsonResponse::JsonResponse(const Result& result)
{
    switch( result.GetType() )
    {
        case Result::Type::JsonText:
            m_responseText = std::make_shared<std::wstring>(std::wstring(_T("{\"type\":\"json\",\"value\":")) +
                                                            result.GetStringResult() +
                                                            _T("}"));
            break;

        case Result::Type::Undefined:
            m_responseText = std::make_shared<std::wstring>(_T("{\"type\":\"undefined\"}"));
            break;

        default:
            ASSERT(result.GetType() == Result::Type::Bool ||
                   result.GetType() == Result::Type::Number ||
                   result.GetType() == Result::Type::String);

            m_responseText = std::make_shared<std::wstring>(FormatTextCS2WS(_T("{\"type\":\"%s\",\"value\":"), GetResultTypeText(result.GetType())) +
                                                            result.GetResultAsJsonText<false>() +
                                                            _T("}"));
            break;
    }

    AssertValidJson(*m_responseText);
}


template<bool parse_json_text_for_type>
const TCHAR* ActionInvoker::JsonResponse::GetResultTypeText(const Result& result)
{
    if constexpr(parse_json_text_for_type)
    {
        if( result.GetType() == Result::Type::JsonText )
        {
            try
            {
                const JsonNode<wchar_t> json_node = Json::Parse(result.GetResultAsString<false>());

                // listed in order of likely occurrence
                return json_node.IsObject()  ? _T("object") :
                       json_node.IsArray()   ? _T("array") :
                       json_node.IsString()  ? GetResultTypeText(Result::Type::String) :
                       json_node.IsNumber()  ? GetResultTypeText(Result::Type::Number) :
                       json_node.IsBoolean() ? GetResultTypeText(Result::Type::Bool) :
                       json_node.IsNull()    ? _T("null") :
                                               ReturnProgrammingError(GetResultTypeText(Result::Type::Undefined));
            }
            catch(...) { ASSERT(false); }
        }
    }

    return GetResultTypeText(result.GetType());
}


inline constexpr const TCHAR* ActionInvoker::JsonResponse::GetResultTypeText(const Result::Type result_type)
{
    constexpr const TCHAR* ResultTypes[] = { _T("undefined"), _T("boolean"), _T("number"), _T("string"), _T("json") };

    ASSERT(static_cast<size_t>(result_type) < _countof(ResultTypes));
    ASSERT(static_cast<size_t>(Result::Type::JsonText) == ( _countof(ResultTypes) - 1 ));

    return ResultTypes[static_cast<size_t>(result_type)];
}


inline ActionInvoker::JsonResponse::JsonResponse(const CSProException& exception)
{
    m_responseText = std::make_shared<std::wstring>(
        std::wstring(_T("{\"type\":\"exception\",\"value\":")) + Encoders::ToJsonString(GetExceptionText(exception)) + _T("}")
    );

    AssertValidJson(*m_responseText);
}


inline std::wstring ActionInvoker::JsonResponse::GetExceptionText(const CSProException& exception)
{
    const ExceptionWithActionName* exception_with_action_name = dynamic_cast<const ExceptionWithActionName*>(&exception);

    return ( exception_with_action_name != nullptr ) ? exception_with_action_name->GetErrorMessageWithActionName() :
                                                       ExceptionWithActionName::GetErrorMessageFromCSProException(exception);
}
