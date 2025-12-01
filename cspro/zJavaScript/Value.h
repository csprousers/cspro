#pragma once

#include <zJavaScript/zJavaScript.h>

namespace JavaScript { class GlobalObjectValue; struct QuickJSAccess; class Value; }


// --------------------------------------------------------------------------
// JavaScript::Value is a wrapper around JSValue values, where the value is
// freed on destruction.
//
// The constructors that take std::unique_ptr<QuickJSAccess>& and
// QuickJSAccess* are intended for use internally within this project.
// --------------------------------------------------------------------------

class ZJAVASCRIPT_API JavaScript::Value
{
public:
    // The default constructor sets the value as undefined.
    Value();

    static Value Undefined() { return Value(); }
    static Value Null();

    template<typename JSValueT>
    Value(QuickJSAccess* qjs, JSValueT&& value);

    template<typename JSValueT>
    Value(QuickJSAccess* qjs, const JSValueT& value);

    template<typename JSValueT>
    Value(std::unique_ptr<QuickJSAccess>& qjs, JSValueT&& value);

    Value(QuickJSAccess& qjs, int value);
    Value(QuickJSAccess& qjs, double value);
    Value(QuickJSAccess& qjs, std::string_view value_sv);
    Value(QuickJSAccess& qjs, const JsonNode& json_node);

    Value(const Value& rhs_value);
    Value(Value&& rhs_value) noexcept;

    Value& operator=(const Value& rhs_value);
    Value& operator=(Value&& rhs) noexcept;

    ~Value();

    bool operator==(const Value& rhs_value) const;
    bool operator!=(const Value& rhs_value) const { return !operator==(rhs_value); }

    // Returns the size of the JSValue value.
    static constexpr size_t GetSizeofJSValue() { return sizeof(m_value); }

    // Returns the underlying JSValue value.
    const auto& GetValue() const;
    auto& GetValue();

    const auto& operator*() const;
    auto& operator*();

    // Duplicate the underlying JSValue value.
    auto Duplicate() const;

    // Releases the underlying JSValue value.
    auto Release();

    // Returns the string representation of the value
    std::string ToString() const;

    // Methods that give information about the value's type.
    const char* GetType() const;

    bool IsUndefined() const;
    bool IsNull() const;
    bool IsArray() const;
    bool IsObject() const;
    bool IsException() const;

private:
    // Frees the value.
    void FreeValue();

    // Duplicates the value, only modifiying m_value, leaving m_qjs unchanged.
    // Callers should make sure that an existing value is freed.
    void DuplicateValue(const Value& rhs_value);

    // Assigns the value, only modifiying m_value, leaving m_qjs unchanged.
    // Callers should make sure that an existing value is freed.
    template<typename JSValueT>
    void SetValue(JSValueT&& value);

    template<typename JSValueT>
    static bool CompareJSValues(const JSValueT& value1, const JSValueT& value2);

private:
    // m_qjs should only be null if:
    //     - the object has been moved, or
    //     - the value is undefined or null
    QuickJSAccess* m_qjs;

    // wraps JSValue
#if !defined(X64_BUILD)
    using ValueT = uint64_t;
#elif defined(WIN32)
    using ValueT = std::tuple<uint64_t, uint64_t>;
#else
    using ValueT = __int128;
#endif

    ValueT m_value;
    static_assert(sizeof(m_value) == ( 2 * sizeof(size_t) ));
};


// --------------------------------------------------------------------------
// JavaScript::GlobalObjectValue is a subclass of Value that wraps the
// global object.
// --------------------------------------------------------------------------

class JavaScript::GlobalObjectValue : public Value
{
public:
    GlobalObjectValue(QuickJSAccess* qjs);
    GlobalObjectValue(std::unique_ptr<QuickJSAccess>& qjs) : GlobalObjectValue(qjs.get()) { }
};



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

template<typename JSValueT>
JavaScript::Value::Value(std::unique_ptr<QuickJSAccess>& qjs, JSValueT&& value)
    :   Value(qjs.get(), std::forward<JSValueT>(value))
{
}


inline JavaScript::Value::Value(const Value& rhs_value)
    :   m_qjs(rhs_value.m_qjs)
{
    DuplicateValue(rhs_value);
}


inline JavaScript::Value::Value(Value&& rhs_value) noexcept
    :   m_qjs(rhs_value.m_qjs),
        m_value(rhs_value.m_value)
{
    rhs_value.m_qjs = nullptr;
}


inline bool JavaScript::Value::operator==(const Value& rhs_value) const
{
    return ( m_value == rhs_value.m_value );
}
