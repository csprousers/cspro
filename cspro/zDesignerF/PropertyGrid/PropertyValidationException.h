#pragma once

#include <zToolsO/CSProException.h>


namespace PropertyGrid { template<typename T> class PropertyValidationException;
                         class PropertyValidationExceptionBase; }


class PropertyGrid::PropertyValidationExceptionBase : public CSProException
{
public:
    using CSProException::CSProException;
};


template<typename T>
class PropertyGrid::PropertyValidationException : public PropertyValidationExceptionBase
{
public:
    template<typename... Args>
    PropertyValidationException(T valid_value, Args const&... args)
        :   PropertyValidationExceptionBase(args...),
            m_validValue(std::move(valid_value))
    {
    }

    const T& GetValidValue() const { return m_validValue; }

private:
    T m_validValue;
};
