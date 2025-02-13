#pragma once


template<typename T>
inline std::wstring GetSubscriptText(const std::vector<T>& indices)
{
    static_assert(std::is_same_v<T, size_t> || std::is_same_v<T, double> || std::is_same_v<T, std::variant<double, std::wstring>>);

    std::wstring subscript_text;

    for( const T& index : indices )
    {
        if( !subscript_text.empty() )
            subscript_text.append(_T(", "));

        if constexpr(std::is_same_v<T, size_t>)
        {
            subscript_text.append(DoubleToString((int)index));
        }

        else if constexpr(std::is_same_v<T, double>)
        {
            subscript_text.append(DoubleToString(index));
        }

        else if constexpr(std::is_same_v<T, std::variant<double, std::wstring>>)
        {
            if( std::holds_alternative<double>(index) )
            {
                subscript_text.append(DoubleToString(std::get<double>(index)));
            }

            else
            {
                SO::Append(subscript_text, _T("\""), std::get<std::wstring>(index), _T("\""));
            }
        }
    }

    return subscript_text;
}


inline std::wstring GetSubscriptText(size_t index)
{
    return GetSubscriptText(std::vector<size_t>{ index });
}
