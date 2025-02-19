#pragma once


template<typename T>
inline std::string GetSubscriptText(const std::vector<T>& indices)
{
    static_assert(std::is_same_v<T, size_t> ||
                  std::is_same_v<T, double> ||
                  std::is_same_v<T, std::variant<double, SharableString>>);

    std::string subscript_text;

    for( const T& index : indices )
    {
        if( !subscript_text.empty() )
            subscript_text.append(", ");

        if constexpr(std::is_same_v<T, size_t>)
        {
            subscript_text.append(DoubleToString(static_cast<int>(index)));
        }

        else if constexpr(std::is_same_v<T, double>)
        {
            subscript_text.append(DoubleToString(index));
        }

        else if constexpr(std::is_same_v<T, std::variant<double, SharableString>>)
        {
            if( std::holds_alternative<double>(index) )
            {
                subscript_text.append(DoubleToString(std::get<double>(index)));
            }

            else
            {
                subscript_text.push_back('"');
                subscript_text.append(std::get<SharableString>(index).GetString());
                subscript_text.push_back('"');
            }
        }
    }

    return subscript_text;
}


inline std::string GetSubscriptText(size_t index)
{
    return GetSubscriptText(std::vector<size_t>{ index });
}
