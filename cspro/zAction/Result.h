#pragma once

#include <zToolsO/Encoders.h>
#include <zToolsO/NumberToString.h>
#include <zToolsO/Special.h>
#include <zJson/ValidJsonAsserter.h>


namespace ActionInvoker
{
    class Result
    {
    public:
        enum class Type { Undefined, Bool, Number, String, JsonText };

    private:
        Result(Type type, std::variant<double, std::wstring> result);

    public:
        // creation
        static Result Undefined();

        static Result Bool(bool result);

        template<typename T>
        static Result Number(T result);

        static Result String(std::wstring result);

        static Result NumberOrString(std::variant<double, std::wstring> result);

        static Result JsonText(std::wstring result);

        template<typename JW>
        static Result JsonText(JW& json_writer);

        // creates a result based on the type of the result
        static Result FromJsonNode(const JsonNode<wchar_t>& json_node);
        static Result FromJsonNode(std::wstring result);

        // access
        Type GetType() const { return m_type; }

        const std::variant<double, std::wstring>& GetResult() const { return m_result; }

        // returns only bool/numeric results
        double GetNumericResult() const;

        // returns only string/JSON results
        const std::wstring& GetStringResult() const;

        std::wstring ReleaseStringResult();

        // returns all but undefined results;
        // bools are converted to 1/0 or true/false;
        // numerics are converted using DoubleToString
        template<bool use_1_0_for_bools>
        std::wstring GetResultAsString() const;

        template<bool use_1_0_for_bools>
        std::wstring ReleaseResultAsString();

        // returns all but undefined results;
        // bools are converted to 1/0 or true/false;
        // numerics are converted using DoubleToString (with special values encoded using Encoders::ToJsonString)
        // strings are converted using Encoders::ToJsonString
        template<bool use_1_0_for_bools>
        std::wstring GetResultAsJsonText() const;

        template<bool use_1_0_for_bools>
        std::wstring ReleaseResultAsJsonText();

    private:
        template<bool use_1_0_for_bools>
        const TCHAR* GetBoolText() const;

    private:
        const Type m_type;
        std::variant<double, std::wstring> m_result;
    };
}



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

inline ActionInvoker::Result::Result(const Type type, std::variant<double, std::wstring> result)
    :   m_type(type),
        m_result(std::move(result))
{
}


inline ActionInvoker::Result ActionInvoker::Result::Undefined()
{
    return Result(Type::Undefined, 0.0);
}


inline ActionInvoker::Result ActionInvoker::Result::Bool(const bool result)
{
    return Result(Type::Bool, static_cast<double>(result));
}


template<typename T>
inline ActionInvoker::Result ActionInvoker::Result::Number(const T result)
{
    static_assert(std::is_same_v<T, int> ||
                  std::is_same_v<T, int64_t> ||
                  std::is_same_v<T, double>);

    return Result(Type::Number, static_cast<double>(result));
}


inline ActionInvoker::Result ActionInvoker::Result::String(std::wstring result)
{
    return Result(Type::String, std::move(result));
}


inline ActionInvoker::Result ActionInvoker::Result::NumberOrString(std::variant<double, std::wstring> result)
{
    const Type type = std::holds_alternative<double>(result) ? Type::Number : Type::String;
    return Result(type, std::move(result));
}


inline ActionInvoker::Result ActionInvoker::Result::JsonText(std::wstring result)
{
    return Result(Type::JsonText, std::move(result));
}


template<typename JW>
inline ActionInvoker::Result ActionInvoker::Result::JsonText(JW& json_writer)
{
    ASSERT(json_writer != nullptr);
    return Result(Type::JsonText, json_writer->GetString());
}


inline ActionInvoker::Result ActionInvoker::Result::FromJsonNode(const JsonNode<wchar_t>& json_node)
{
    return json_node.IsString()  ? String(json_node.Get<std::wstring>()) :
           json_node.IsNumber()  ? Number(json_node.GetDouble()) :
           json_node.IsBoolean() ? Bool(json_node.Get<bool>()) :
                                   JsonText(json_node.GetNodeAsString());
}


inline ActionInvoker::Result ActionInvoker::Result::FromJsonNode(std::wstring result)
{
    if( !result.empty() )
    {
        // don't bother parsing the result if the first character indicates that it is an object or array
        switch( result.front() )
        {
            case '{':
            case '[':
                return JsonText(std::move(result));
        }
    }

    try
    {
        return FromJsonNode(Json::Parse(result));
    }

    catch(...)
    {
        return ReturnProgrammingError(JsonText(std::move(result)));
    }
}


inline double ActionInvoker::Result::GetNumericResult() const
{
    ASSERT(std::holds_alternative<double>(m_result) && ( m_type == Type::Bool ||
                                                         m_type == Type::Number ));

    return std::get<double>(m_result);
}


inline const std::wstring& ActionInvoker::Result::GetStringResult() const
{
    ASSERT(std::holds_alternative<std::wstring>(m_result) && ( m_type == Type::String ||
                                                               m_type == Type::JsonText ));

    return std::get<std::wstring>(m_result);
}


inline std::wstring ActionInvoker::Result::ReleaseStringResult()
{
    ASSERT(std::holds_alternative<std::wstring>(m_result) && ( m_type == Type::String ||
                                                               m_type == Type::JsonText ));

    return std::move(std::get<std::wstring>(m_result));
}


template<bool use_1_0_for_bools>
const TCHAR* ActionInvoker::Result::GetBoolText() const
{
    ASSERT(m_type == Type::Bool);
    const bool is_true = ( std::get<double>(m_result) != 0 );

    if constexpr(use_1_0_for_bools)
    {
        return is_true ? _T("1") : _T("0");
    }

    else
    {
        return Json::Text::Bool(is_true);
    }
}


template<bool use_1_0_for_bools>
std::wstring ActionInvoker::Result::GetResultAsString() const
{
    ASSERT(m_type != Type::Undefined);

    if( std::holds_alternative<double>(m_result) )
    {
        return ( m_type == Type::Number ) ? DoubleToString(std::get<double>(m_result)) :
                                            GetBoolText<use_1_0_for_bools>();
    }

    return std::get<std::wstring>(m_result);
}


template<bool use_1_0_for_bools>
std::wstring ActionInvoker::Result::ReleaseResultAsString()
{
    ASSERT(m_type != Type::Undefined);

    if( std::holds_alternative<double>(m_result) )
        return GetResultAsString<use_1_0_for_bools>();

    return ReleaseStringResult();
}


template<bool use_1_0_for_bools>
std::wstring ActionInvoker::Result::GetResultAsJsonText() const
{
    ASSERT(m_type != Type::Undefined);

    switch( m_type )
    {
        case Type::Bool:
            return GetBoolText<use_1_0_for_bools>();

        case Type::Number:
            return IsSpecial(std::get<double>(m_result)) ? Encoders::ToJsonString(SpecialValues::ValueToString(std::get<double>(m_result))) :
                                                           AssertAndReturnValidJson(DoubleToString(std::get<double>(m_result)));

        case Type::String:
            return Encoders::ToJsonString(std::get<std::wstring>(m_result));

        case Type::JsonText:
            return AssertAndReturnValidJson(std::get<std::wstring>(m_result));

        default:
            return ReturnProgrammingError(std::wstring());
    }
}


template<bool use_1_0_for_bools>
std::wstring ActionInvoker::Result::ReleaseResultAsJsonText()
{
    ASSERT(m_type != Type::Undefined);

    return ( m_type == Type::JsonText ) ? ReleaseStringResult() :
                                          GetResultAsJsonText<use_1_0_for_bools>();
}
