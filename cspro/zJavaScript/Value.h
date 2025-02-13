#pragma once

#include <zJavaScript/zJavaScript.h>

template<typename CharType> class JsonNode;
namespace JavaScript { struct QuickJSAccess; }


namespace JavaScript
{
    // a wrapper around JSValue values, where the value is freed on destruction

    class ZJAVASCRIPT_API Value
    {
    public:
        Value();
        Value(QuickJSAccess& qjs, int value);
        Value(QuickJSAccess& qjs, std::string_view value_sv);
        Value(QuickJSAccess& qjs, wstring_view value_sv);
        Value(QuickJSAccess& qjs, const JsonNode<wchar_t>& json_node);

        Value(const Value& rhs) = delete;
        Value(Value&& rhs) noexcept;

        Value& operator=(const Value& rhs) = delete;
        Value& operator=(Value&& rhs) noexcept;

        ~Value();

        const auto& GetValue() const; // returns the underlying JSValue value
        auto& GetValue();

    private:
        QuickJSAccess* m_qjs;
        std::byte m_value[OnAndroid() ? 16 : 8]; // wraps JSValue
    };
}
