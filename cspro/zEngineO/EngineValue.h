#pragma once

#include <zEngineO/zEngineO.h>
#include <zToolsO/CSProException.h>

namespace Engine { class Value; }


// --------------------------------------------------------------------------
// Engine::Value is a wrapper around values returned the interpreter's
// instructions. The engine value can be:
//
// - double: numeric
// --------------------------------------------------------------------------

class Engine::Value
{
public:
    // --------------------------------------------------------------------------
    // Constructors for creating engine values.
    // --------------------------------------------------------------------------

    Engine::Value(double value) noexcept;


    // --------------------------------------------------------------------------
    // Access methods.
    // --------------------------------------------------------------------------

    // Returns the value if it is of the specified type, throwing an
    // exception if the value is not of the specified type.
    template<typename T>
    const T& get() const &;

    template<typename T>
    T get() &&;

    // Casts the value to the specified type, throwing an exception
    // if an implicit conversion is not possible.
    template<typename T>
    T as();


    // --------------------------------------------------------------------------
    // Other methods.
    // --------------------------------------------------------------------------

    // Converts the value to a string representation for display.
    ZENGINEO_API SharableString ToString() const;


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
    static T Convert(const ValueT& value);

    // Helpers to determine if a value is in a variant:
    template<typename T, typename Variant>
    struct is_in_variant : std::false_type { };

    template<typename T, typename... Ts>
    struct is_in_variant<T, std::variant<Ts...>> : std::bool_constant<(std::is_same_v<T, Ts> || ...)> { };


    // --------------------------------------------------------------------------
    // Members.
    // --------------------------------------------------------------------------
private:
    using StorageT = std::variant<double>;
    StorageT m_value;
};



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

inline Engine::Value::Value(const double value) noexcept
    :   m_value(value)
{
}


template<typename T>
const T& Engine::Value::get() const &
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
T Engine::Value::as()
{
    if constexpr(is_in_variant<T, StorageT>::value)
    {
        if( std::holds_alternative<T>(m_value) )
            return std::get<T>(m_value);
    }

    return Convert<T>();
}


template<typename ExceptionT, typename T>
ExceptionT Engine::Value::CreateException() const
{
    constexpr bool is_access_exception = std::is_same_v<ExceptionT, AccessException>;
    return ExceptionT(CreateExceptionMessage(is_access_exception, GetTypeText<T>()));
}
