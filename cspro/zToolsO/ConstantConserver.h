#pragma once


// ConstantConserver is used to conditionally add entries to a vector, only adding an entry once per value;
// the class has special handling to support conserving SharableString objects

template<typename ConserveType>
class ConstantConserver
{
    using LookupType = typename std::conditional<std::is_same_v<ConserveType, SharableString>, std::string, ConserveType>::type;

public:
    ConstantConserver(std::vector<ConserveType>& vector)
        :   m_vector(vector)
    {
    }

    template<typename ValueType>
    int Add(ValueType&& value)
    {
        const auto& conserver_search = m_conserver.find(GetLookupValue(value));

        if( conserver_search != m_conserver.cend() )
        {
            return conserver_search->second;
        }

        else
        {
            int index = static_cast<int>(m_vector.size());
            const auto& value_in_vector = m_vector.emplace_back(std::forward<ValueType>(value));
            m_conserver.try_emplace(GetLookupValue(value_in_vector), index);
            return index;
        }
    }

private:
    template<typename ValueType>
    static const ValueType& GetLookupValue(const ValueType& value)
    {
        return value;
    }

    static const std::string& GetLookupValue(const SharableString& value)
    {
        return *value;
    }

private:
    std::vector<ConserveType>& m_vector;
    std::map<LookupType, int> m_conserver;
};
