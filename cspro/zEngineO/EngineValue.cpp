#include "stdafx.h"
#include "EngineValue.h"


namespace ValueIndex
{
    constexpr size_t Double = 0;
}


template<> ZENGINEO_API const char* Engine::Value::GetTypeText<double>()         { return "number"; }
template<> ZENGINEO_API const char* Engine::Value::GetTypeText<SharableString>() { return "string"; }


std::string Engine::Value::CreateExceptionMessage(const bool is_access_exception, const char* const value_type) const
{
    const char* const exception_type = is_access_exception ? "accessing as" : "converting to";

    try
    {
        return FormatText("There was an error %s '%s' the value: %s", exception_type, value_type, ToString()->c_str());
    }

    catch(...)
    {
        ASSERT(false);
        return FormatText("There was an error %s '%s' a value.", exception_type, value_type);
    }
}



SharableString Engine::Value::ToString() const
{
    switch( m_value.index() )
    {
        case ValueIndex::Double:
            return DoubleToString(std::get<ValueIndex::Double>(m_value));

        default:
            return ReturnProgrammingError(std::string());
    }
}


template<typename T>
T Engine::Value::Convert() const
{
    return std::visit([&](const auto& value) { return Convert<T>(value); }, m_value);
}

template ZENGINEO_API double Engine::Value::Convert() const;
template ZENGINEO_API SharableString Engine::Value::Convert() const;


template<typename T, typename ValueT>
T Engine::Value::Convert(const ValueT& value)
{
    // as() should already have checked that the types are the same,
    // so no conversion is necessary
    return ReturnProgrammingError(value);
}


template<>
SharableString Engine::Value::Convert(const double& value)
{
    return DoubleToString(value);
}
