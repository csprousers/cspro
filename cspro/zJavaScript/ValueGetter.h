#pragma once

#include <zJavaScript/Value.h>
#include <zJavaScript/QuickJSAccess.h>


inline const auto& JavaScript::Value::GetValue() const
{
    return *reinterpret_cast<const JSValue*>(m_value);
}


inline auto& JavaScript::Value::GetValue()
{
    return *reinterpret_cast<JSValue*>(m_value);
}
