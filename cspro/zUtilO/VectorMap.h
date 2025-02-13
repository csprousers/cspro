#pragma once


template<typename Key, typename Value>
class VectorMap
{
public:
    using key_value_pair = std::tuple<Key, Value>;

    VectorMap();

    const std::vector<key_value_pair>& GetVector() const { return m_keyValuePairs; }

    const Value* Find(const Key& key) const;
    Value* Find(const Key& key);

    template<typename KeyT, typename ValueT>
    Value& Insert(KeyT&& key, ValueT&& value);

    void Remove(const Key& key);

private:
    void ResetKeyPointers();

    const key_value_pair* FindKVP(const Key& key) const;

private:
    std::vector<key_value_pair> m_keyValuePairs;
    mutable const key_value_pair* m_previouslyFoundKVP;
    const key_value_pair* m_endKVP;
};



template<typename Key, typename Value>
VectorMap<Key, Value>::VectorMap()
{
    ResetKeyPointers();
}


template<typename Key, typename Value>
const Value* VectorMap<Key, Value>::Find(const Key& key) const
{
    const key_value_pair* kvp = FindKVP(key);
    return ( kvp == nullptr ) ? nullptr : &std::get<1>(*kvp);
}


template<typename Key, typename Value>
Value* VectorMap<Key, Value>::Find(const Key& key)
{
    return const_cast<Value*>(const_cast<const VectorMap*>(this)->Find(key));
}


template<typename Key, typename Value>
template<typename KeyT, typename ValueT>
Value& VectorMap<Key, Value>::Insert(KeyT&& key, ValueT&& value)
{
    Value* existing_value = Find(key);

    if( existing_value != nullptr )
    {
        // replace the value
        *existing_value = std::forward<ValueT>(value);
        return *existing_value;
    }

    else
    {
        // add the key and value
        Value& added_value = std::get<1>(m_keyValuePairs.emplace_back(std::forward<KeyT>(key),
                                                                      std::forward<ValueT>(value)));

        ResetKeyPointers(); 

        return added_value;
    }
}


template<typename Key, typename Value>
void VectorMap<Key, Value>::Remove(const Key& key)
{
    const key_value_pair* kvp = FindKVP(key);

    if( kvp != nullptr )
    {
        m_keyValuePairs.erase(m_keyValuePairs.begin() + ( kvp - m_keyValuePairs.data() ));
        ResetKeyPointers();
    }
}


template<typename Key, typename Value>
void VectorMap<Key, Value>::ResetKeyPointers()
{
    m_previouslyFoundKVP = m_keyValuePairs.data();
    m_endKVP = m_previouslyFoundKVP + m_keyValuePairs.size();
}


template<typename Key, typename Value>
const typename VectorMap<Key, Value>::key_value_pair* VectorMap<Key, Value>::FindKVP(const Key& key) const
{
    auto search_for_value = [&](const key_value_pair* start, const key_value_pair* end)
    {
        for( const key_value_pair* key_value_pair_itr = start; key_value_pair_itr != end; ++key_value_pair_itr )
        {
            if( std::get<0>(*key_value_pair_itr) == key )
            {
                m_previouslyFoundKVP = key_value_pair_itr;
                return true;
            }
        }

        return false;
    };

    // search forwards from the previously found key; if not found, search from the beginning of the list to the previously found key
    if( !search_for_value(m_previouslyFoundKVP, m_endKVP) &&
        !search_for_value(m_keyValuePairs.data(), m_previouslyFoundKVP) )
    {
        return nullptr;
    }

    return m_previouslyFoundKVP;
}
