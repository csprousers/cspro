#pragma once


template<typename T>
class ValueConserver
{
public:
    template<typename VT>
    ValueConserver(T& value, VT&& new_value)
        :   m_value(value),
            m_originalValue(value)
    {
        value = std::forward<VT>(new_value);
    }

    ~ValueConserver()
    {
        m_value = std::move(m_originalValue);
    }

private:
    T& m_value;
    T m_originalValue;
};
