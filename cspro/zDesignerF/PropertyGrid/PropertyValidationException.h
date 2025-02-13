#pragma once

#include <zToolsO/CSProException.h>


namespace PropertyGrid
{
    class PropertyValidationExceptionBase : public CSProException
    {
    public:
        PropertyValidationExceptionBase(const TCHAR* message)
            :   CSProException(message)
        {
        }
    };


    template<typename T>
    class PropertyValidationException : public PropertyValidationExceptionBase
    {
    public:
        PropertyValidationException(T valid_value, const TCHAR* message)
            :   PropertyValidationExceptionBase(message),
                m_validValue(std::move(valid_value))
        {
        }

        const T& GetValidValue() const { return m_validValue; }

    private:
        T m_validValue;
    };
}
