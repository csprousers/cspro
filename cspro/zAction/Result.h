#pragma once

#include <zToolsO/Encoders.h>
#include <zToolsO/NumberToString.h>
#include <zToolsO/Special.h>
#include <zJson/ValidJsonAsserter.h>

namespace ActionInvoker { class Result; }


class ActionInvoker::Result
{
public:
    enum class Type { Undefined, Bool, Number, String, JsonText };

private:
    Result(Type type, std::variant<double, SharableString> result);

public:
    // creation
    static Result Undefined();

    static Result Bool(bool result);

    template<typename T>
    static Result Number(T result);

    static Result String(SharableString result);

    static Result NumberOrString(std::variant<double, SharableString> result);

    static Result JsonText(SharableString result);

    static Result JsonText(JsonStringWriter& json_writer);

    // creates a result based on the type of the result
    static Result FromJsonNode(const JsonNode& json_node);
    static Result FromJsonNode(SharableString result);

    // access
    Type GetType() const { return m_type; }

    const std::variant<double, SharableString>& GetResult() const { return m_result; }

    // returns only bool/numeric results
    double GetNumericResult() const;

    // returns only string/JSON results
    SharableString GetStringResult() const;

    // returns all but undefined results;
    // bools are converted to 1/0 or true/false;
    // numerics are converted using DoubleToString
    template<bool use_1_0_for_bools>
    SharableString GetResultAsString() const;

    // returns all but undefined results;
    // bools are converted to 1/0 or true/false;
    // numerics are converted using DoubleToString (with special values encoded using Encoders::ToJsonString)
    // strings are converted using Encoders::ToJsonString
    template<bool use_1_0_for_bools>
    SharableString GetResultAsJsonText() const;

private:
    template<bool use_1_0_for_bools>
    const char* GetBoolText() const;

private:
    const Type m_type;
    std::variant<double, SharableString> m_result;
};



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

inline ActionInvoker::Result::Result(const Type type, std::variant<double, SharableString> result)
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
                  std::is_same_v<T, size_t> ||
                  std::is_same_v<T, double>);

    return Result(Type::Number, static_cast<double>(result));
}


inline ActionInvoker::Result ActionInvoker::Result::String(SharableString result)
{
    return Result(Type::String, std::move(result));
}


inline ActionInvoker::Result ActionInvoker::Result::NumberOrString(std::variant<double, SharableString> result)
{
    const Type type = std::holds_alternative<double>(result) ? Type::Number :
                                                               Type::String;

    return Result(type, std::move(result));
}


inline ActionInvoker::Result ActionInvoker::Result::JsonText(SharableString result)
{
    AssertValidJson(*result);
    return Result(Type::JsonText, std::move(result));
}


inline ActionInvoker::Result ActionInvoker::Result::JsonText(JsonStringWriter& json_writer)
{
    return Result(Type::JsonText, json_writer.ReleaseSharableString());
}


inline ActionInvoker::Result ActionInvoker::Result::FromJsonNode(const JsonNode& json_node)
{
    return json_node.IsString()  ? String(json_node.Get<SharableString>()) :
           json_node.IsNumber()  ? Number(json_node.GetDouble()) :
           json_node.IsBoolean() ? Bool(json_node.Get<bool>()) :
                                   JsonText(json_node.GetNodeAsSharableString());
}


inline ActionInvoker::Result ActionInvoker::Result::FromJsonNode(SharableString result)
{
    if( !result->empty() )
    {
        // don't bother parsing the result if the first character indicates that it is an object or array
        switch( result->front() )
        {
            case '{':
            case '[':
                return JsonText(std::move(result));
        }
    }

    try
    {
        return FromJsonNode(Json::Parse(*result));
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


inline SharableString ActionInvoker::Result::GetStringResult() const
{
    ASSERT(std::holds_alternative<SharableString>(m_result) && ( m_type == Type::String ||
                                                                 m_type == Type::JsonText ));

    return std::get<SharableString>(m_result);
}


template<bool use_1_0_for_bools>
const char* ActionInvoker::Result::GetBoolText() const
{
    ASSERT(m_type == Type::Bool);
    const bool is_true = ( std::get<double>(m_result) != 0 );

    if constexpr(use_1_0_for_bools)
    {
        return is_true ? "1" : "0";
    }

    else
    {
        return Json::Text::Bool(is_true);
    }
}


template<bool use_1_0_for_bools>
SharableString ActionInvoker::Result::GetResultAsString() const
{
    ASSERT(m_type != Type::Undefined);

    if( std::holds_alternative<double>(m_result) )
    {
        return ( m_type == Type::Number ) ? DoubleToString(std::get<double>(m_result)) :
                                            GetBoolText<use_1_0_for_bools>();
    }

    return std::get<SharableString>(m_result);
}


template<bool use_1_0_for_bools>
SharableString ActionInvoker::Result::GetResultAsJsonText() const
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
            return Encoders::ToJsonString(std::get<SharableString>(m_result).GetString());

        case Type::JsonText:
            return AssertAndReturnValidJson(std::get<SharableString>(m_result));

        default:
            return ReturnProgrammingError(SharableString());
    }
}
