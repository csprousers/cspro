#include "stdafx.h"
#include "Value.h"
#include "ValueInternal.h"
#include <zJson/Json.h>


JavaScript::Value::Value()
    :   Value(nullptr, JS_UNDEFINED)
{
}


JavaScript::Value::~Value()
{
    FreeValue();
}


JavaScript::Value JavaScript::Value::Null()
{
    return Value(nullptr, JS_NULL);
}


JavaScript::Value::Value(QuickJSAccess& qjs, const int value)
    :   m_qjs(&qjs)
{
    SetValue(JS_NewInt32(m_qjs->ctx, value));
}


JavaScript::Value::Value(QuickJSAccess& qjs, const double value)
    :   m_qjs(&qjs)
{
    SetValue(JS_NewFloat64(m_qjs->ctx, value));
}


JavaScript::Value::Value(QuickJSAccess& qjs, const std::string_view value_sv)
    :   m_qjs(&qjs)
{
    SetValue(m_qjs->NewString(value_sv));
}


JavaScript::Value::Value(QuickJSAccess& qjs, const JsonNode& json_node)
    :   m_qjs(&qjs)
{
    const std::string json_utf8 = json_node.GetNodeAsString();
    SetValue(JS_ParseJSON(m_qjs->ctx, json_utf8.c_str(), json_utf8.length(), nullptr));

    if( JS_IsException(GetValue()) )
        qjs.ThrowException();
}


void JavaScript::Value::DuplicateValue(const Value& rhs_value)
{
    ASSERT(rhs_value.m_qjs != nullptr || ( rhs_value.IsUndefined() || rhs_value.IsNull() ));
    ASSERT(m_qjs == rhs_value.m_qjs);

    if( m_qjs != nullptr )
    {
        SetValue(JS_DupValue(m_qjs->ctx, rhs_value.GetValue()));
    }

    else
    {
        ASSERT(rhs_value.IsUndefined() || rhs_value.IsNull());
        SetValue(rhs_value.GetValue());
    }
}


JavaScript::Value& JavaScript::Value::operator=(const Value& rhs_value)
{
    FreeValue();

    m_qjs = rhs_value.m_qjs;
    DuplicateValue(rhs_value);

    return *this;
}


JavaScript::Value& JavaScript::Value::operator=(Value&& rhs_value) noexcept
{
    ASSERT(rhs_value.m_qjs != nullptr || ( rhs_value.IsUndefined() || rhs_value.IsNull() ));

    FreeValue();

    m_qjs = rhs_value.m_qjs;
    SetValue(rhs_value.GetValue());

    rhs_value.m_qjs = nullptr;

    return *this;
}


template<typename JSValueT>
bool JavaScript::Value::CompareJSValues(const JSValueT& value1, const JSValueT& value2)
{
    static_assert(sizeof(value1) == sizeof(value2));

    if constexpr(GetSizeofJSValue() == sizeof(uint64_t))
    {
        ASSERT(( value1 == value2 ) == ( memcmp(&value1, &value2, sizeof(value1)) == 0 ));
        return ( value1 == value2 );
    }

    else
    {
        return ( memcmp(&value1, &value2, sizeof(value1)) == 0 );
    }
}


const char* JavaScript::Value::GetType() const
{
    switch( JS_VALUE_GET_TAG(GetValue()) )
    {
        case JS_TAG_UNDEFINED:
            return "undefined";

        case JS_TAG_NULL:
            return "null";

        case JS_TAG_BOOL:
            return "boolean";

        case JS_TAG_INT:
            return "number";

        case JS_TAG_STRING:
            return "string";

        case JS_TAG_OBJECT:
            ASSERT(m_qjs != nullptr);
            return JS_IsArray(GetValue()) ? "array" : "object";

        case JS_TAG_SYMBOL:
            return "symbol";

        case JS_TAG_EXCEPTION:
            return "exception";

        default:
            return JS_IsNumber(GetValue()) ? "number" : ReturnProgrammingError("unknown");
    }
}


std::string JavaScript::Value::ToString() const
{
    if( m_qjs != nullptr )
    {
        return m_qjs->GetString(GetValue());
    }

    else
    {
        // m_qjs should only be null for moved objects, which wouldn't be here, and undefined / null values
        ASSERT(IsUndefined() || IsNull());
        return GetType();
    }
}


bool JavaScript::Value::IsUndefined() const
{
    return JS_IsUndefined(GetValue());
}


bool JavaScript::Value::IsNull() const
{
    return JS_IsNull(GetValue());
}


bool JavaScript::Value::IsArray() const
{
    return JS_IsArray(GetValue());
}


bool JavaScript::Value::IsObject() const
{
    return JS_IsObject(GetValue());
}


bool JavaScript::Value::IsException() const
{
    return JS_IsException(GetValue());
}
