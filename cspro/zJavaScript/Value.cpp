#include "stdafx.h"
#include "Value.h"
#include "ValueGetter.h"
#include <zJson/Json.h>


JavaScript::Value::Value()
    :   m_qjs(nullptr)
{
    static_assert(sizeof(JSValue) == sizeof(m_value));

    GetValue() = JS_UNDEFINED;
}


JavaScript::Value::Value(QuickJSAccess& qjs, int value)
    :   m_qjs(&qjs)
{
    GetValue() = JS_NewInt32(m_qjs->ctx, value);
}


JavaScript::Value::Value(QuickJSAccess& qjs, std::string_view value_sv)
    :   m_qjs(&qjs)
{
    GetValue() = m_qjs->NewString(value_sv);
}


JavaScript::Value::Value(QuickJSAccess& qjs, wstring_view value_sv)
    :   Value(qjs, UTF8Convert::WideToUTF8(value_sv))
{
}


JavaScript::Value::Value(QuickJSAccess& qjs, const JsonNode<wchar_t>& json_node)
    :   m_qjs(&qjs)
{
    const std::string json_utf8 = UTF8Convert::WideToUTF8(json_node.GetNodeAsString());
    GetValue() = JS_ParseJSON(m_qjs->ctx, json_utf8.c_str(), json_utf8.length(), nullptr);

    if( JS_IsException(GetValue()) )
        qjs.ThrowException();
}


JavaScript::Value::Value(Value&& rhs) noexcept
{
    operator=(std::move(rhs));
}


JavaScript::Value& JavaScript::Value::operator=(Value&& rhs) noexcept
{
    m_qjs = rhs.m_qjs;
    rhs.m_qjs = nullptr;

    memcpy(m_value, rhs.m_value, sizeof(m_value));

    return *this;
}


JavaScript::Value::~Value()
{
    if( m_qjs != nullptr )
        JS_FreeValue(m_qjs->ctx, GetValue());
}
