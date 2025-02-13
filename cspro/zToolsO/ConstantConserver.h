#pragma once


template<typename T>
class ConstantConserver
{
public:
    ConstantConserver(std::vector<T>& vector)
        :   m_vector(vector)
    {
    }

    template<typename VT>
    int Add(VT&& t)
    {
        const auto& conserver_search = m_conserver.find(t);

        if( conserver_search != m_conserver.cend() )
        {
            return conserver_search->second;
        }

        else
        {
            int index = static_cast<int>(m_vector.size());
            auto& t_in_vector = m_vector.emplace_back(std::forward<VT>(t));
            m_conserver.try_emplace(t_in_vector, index);

            return index;
        }
    }

private:
    std::vector<T>& m_vector;
    std::map<T, int> m_conserver;
};
