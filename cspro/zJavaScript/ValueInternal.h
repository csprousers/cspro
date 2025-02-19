#pragma once

#include <zJavaScript/Value.h>
#include <zJavaScript/QuickJSAccess.h>


// --------------------------------------------------------------------------
// Value
// --------------------------------------------------------------------------

static_assert(sizeof(JSValue) == JavaScript::Value::GetSizeofJSValue());


template<typename JSValueT>
JavaScript::Value::Value(QuickJSAccess* const qjs, JSValueT&& value)
    :   m_qjs(qjs)
{
    static_assert(!std::is_lvalue_reference_v<JSValueT>);

    ASSERT(m_qjs != nullptr || ( CompareJSValues(value, JS_UNDEFINED) || CompareJSValues(value, JS_NULL) ));

    SetValue(std::forward<JSValueT>(value));
}


template<typename JSValueT>
JavaScript::Value::Value(QuickJSAccess* const qjs, const JSValueT& value)
    :   m_qjs(qjs)
{
    ASSERT(m_qjs != nullptr);

    SetValue(JS_DupValue(m_qjs->ctx, value));
}


inline const auto& JavaScript::Value::GetValue() const
{
    return *reinterpret_cast<const JSValue*>(&m_value);
}


inline auto& JavaScript::Value::GetValue()
{
    return *reinterpret_cast<JSValue*>(&m_value);
}


inline void JavaScript::Value::FreeValue()
{
    if( m_qjs != nullptr )
        JS_FreeValue(m_qjs->ctx, GetValue());
}


template<typename JSValueT>
void JavaScript::Value::SetValue(JSValueT&& value)
{
    ASSERT(m_qjs != nullptr || ( CompareJSValues(value, JS_UNDEFINED) || CompareJSValues(value, JS_NULL) ));
    GetValue() = std::forward<JSValueT>(value);
}


inline const auto& JavaScript::Value::operator*() const
{
    return GetValue();
}


inline auto& JavaScript::Value::operator*()
{
    return GetValue();
}


inline auto JavaScript::Value::Duplicate() const
{
    ASSERT(m_qjs != nullptr);
    return JS_DupValue(m_qjs->ctx, GetValue());
}


inline auto JavaScript::Value::Release()
{
    m_qjs = nullptr;
    return GetValue();
}



// --------------------------------------------------------------------------
// GlobalObjectValue
// --------------------------------------------------------------------------

inline JavaScript::GlobalObjectValue::GlobalObjectValue(QuickJSAccess* const qjs)
    :   Value(qjs, JS_GetGlobalObject(qjs->ctx))
{
}
