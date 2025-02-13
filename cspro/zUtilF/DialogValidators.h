#pragma once

#include <zUtilO/WindowsWS.h>


template<typename T>
class RadioEnumHelper
{
public:
    RadioEnumHelper(std::vector<T> values)
        :   m_values(std::move(values))
    {
    }

    int ToForm(T value) const
    {
        const auto& lookup = std::find(m_values.cbegin(), m_values.cend(), value);
        return ( lookup != m_values.cend() ) ? std::distance(m_values.cbegin(), lookup) :
                                               ReturnProgrammingError(0);
    }

    auto FromForm(int value) const
    {
        ASSERT(static_cast<size_t>(value) < m_values.size());

        // because std::vector<bool> is specialized, we can't return a reference to the value
        if constexpr(std::is_same_v<T, bool>)
        {
            return m_values[value];
        }

        else
        {
            return static_cast<const T&>(m_values[value]);
        }
    }

private:
    const std::vector<T> m_values;
};
