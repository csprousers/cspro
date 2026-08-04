#pragma once

#include <zEngineO/zEngineO.h>
#include <zToolsO/CSProException.h>

enum class DataType;
namespace Engine { class Value; }


// --------------------------------------------------------------------------
// Engine::Value is a wrapper around values returned the interpreter's
// instructions. The engine value can be:
//
// - double         | decimal values
// - SharableString | string values
// --------------------------------------------------------------------------

class Engine::Value
{
public:
    // --------------------------------------------------------------------------
    // Constructors for creating engine values.
    // --------------------------------------------------------------------------

    Value(double value) noexcept;
    Value(SharableString value) noexcept;
    Value(std::string value) noexcept;

    // EV_TODO: Creates an object of the type "bool" in case such a type is ever added to the language.
    // For now, the value is stored as a double.
    [[nodiscard]] static Value Bool(bool value) noexcept;

    // EV_TODO: Creates an object of the type "integer" in case such a type is ever added to the language.
    // For now, the value is stored as a double.
    [[nodiscard]] static Value Integer(int value) noexcept;
    [[nodiscard]] static Value Integer(unsigned int value) noexcept;
    [[nodiscard]] static Value Integer(int64_t value) noexcept;
    [[nodiscard]] static Value Integer(size_t value) noexcept;

    // EV_TODO: Creates an object of the type "undefined" in case such a type is ever added to the language.
    // For now, double is mapped to NOTAPPL and SharableString to a blank string.
    template<typename T>
    [[nodiscard]] ZENGINEO_API static Value Undefined() noexcept;

    [[nodiscard]] ZENGINEO_API static Value Undefined(DataType data_type) noexcept;

    // EV_TODO: Creates an object of the type "invalid" in case such a type is ever added to the language.
    // For now, double is mapped to DEFAULT and SharableString to a blank string.
    template<typename T>
    [[nodiscard]] ZENGINEO_API static Value Invalid() noexcept;

    [[nodiscard]] ZENGINEO_API static Value Invalid(DataType data_type) noexcept;


    // --------------------------------------------------------------------------
    // Access methods.
    // --------------------------------------------------------------------------

    // Returns true if the value is of the specified type.
    template<typename T>
    [[nodiscard]] bool is() const noexcept;

    // Returns the value if it is of the specified type, throwing an
    // exception if the value is not of the specified type.
    template<typename T>
    [[nodiscard]] const T& get() const &;

    template<typename T>
    [[nodiscard]] T& get() &;

    template<typename T>
    [[nodiscard]] T get() &&;

    // Casts the value to the specified type, throwing an exception
    // if an implicit conversion is not possible.
    template<typename T>
    [[nodiscard]] T as() const &;

    template<typename T>
    [[nodiscard]] T as() &&;


    // --------------------------------------------------------------------------
    // Other methods.
    // --------------------------------------------------------------------------

    // Converts the value to a string representation for display.
    ZENGINEO_API [[nodiscard]] SharableString ToString() const;


    // --------------------------------------------------------------------------
    // Exceptions are thrown when access to a value fails.
    // ConversionException is a subclass of AccessException, thrown when
    // implicit conversion errors occur.
    // --------------------------------------------------------------------------
    class AccessException     : public CSProException  { using CSProException::CSProException; };
    class ConversionException : public AccessException { using AccessException::AccessException; };


    // --------------------------------------------------------------------------
    // Private methods.
    // --------------------------------------------------------------------------
private:
    template<typename T>
    ZENGINEO_API static const char* GetTypeText();

    template<typename ExceptionT, typename T>
    ExceptionT CreateException() const;

    ZENGINEO_API std::string CreateExceptionMessage(bool is_access_exception, const char* value_type) const;

    template<typename T>
    ZENGINEO_API T Convert() const;

    template<typename T, typename ValueT>
    T Convert(const ValueT& value) const;

    // Helpers to determine if a value is in a variant:
    template<typename T, typename Variant>
    struct is_in_variant : std::false_type { };

    template<typename T, typename... Ts>
    struct is_in_variant<T, std::variant<Ts...>> : std::bool_constant<(std::is_same_v<T, Ts> || ...)> { };


    // --------------------------------------------------------------------------
    // Members.
    // --------------------------------------------------------------------------
private:
    using StorageT = std::variant<
        double,
        SharableString
    >;

    StorageT m_value;
};



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

inline Engine::Value::Value(const double value) noexcept
    :   m_value(value)
{
}


inline Engine::Value::Value(SharableString value) noexcept
    :   m_value(std::move(value))
{
}


inline Engine::Value::Value(std::string value) noexcept
    :   m_value(std::move(value))
{
}


inline Engine::Value Engine::Value::Bool(const bool value) noexcept
{
    return Value(static_cast<double>(value));
}


inline Engine::Value Engine::Value::Integer(const int value) noexcept
{
    return Value(static_cast<double>(value));
}


inline Engine::Value Engine::Value::Integer(const unsigned int value) noexcept
{
    return Value(static_cast<double>(value));
}


inline Engine::Value Engine::Value::Integer(const int64_t value) noexcept
{
    return Value(static_cast<double>(value));
}


inline Engine::Value Engine::Value::Integer(const size_t value) noexcept
{
    return Value(static_cast<double>(value));
}


template<typename T>
bool Engine::Value::is() const noexcept
{
    return std::holds_alternative<T>(m_value);
}


template<typename T>
const T& Engine::Value::get() const &
{
    if( std::holds_alternative<T>(m_value) )
        return std::get<T>(m_value);

    throw CreateException<AccessException, T>();
}


template<typename T>
T& Engine::Value::get() &
{
    if( std::holds_alternative<T>(m_value) )
        return std::get<T>(m_value);

    throw CreateException<AccessException, T>();
}


template<typename T>
T Engine::Value::get() &&
{
    if( std::holds_alternative<T>(m_value) )
        return std::get<T>(std::move(m_value));

    throw CreateException<AccessException, T>();
}


template<typename T>
T Engine::Value::as() const &
{
    if constexpr(is_in_variant<T, StorageT>::value)
    {
        if( std::holds_alternative<T>(m_value) )
            return std::get<T>(m_value);
    }

    return Convert<T>();
}


template<typename T>
T Engine::Value::as() &&
{
    if constexpr(is_in_variant<T, StorageT>::value)
    {
        if( std::holds_alternative<T>(m_value) )
            return std::get<T>(std::move(m_value));
    }

    return Convert<T>();
}


template<typename ExceptionT, typename T>
ExceptionT Engine::Value::CreateException() const
{
    constexpr bool is_access_exception = std::is_same_v<ExceptionT, AccessException>;
    return ExceptionT(CreateExceptionMessage(is_access_exception, GetTypeText<T>()));
}
