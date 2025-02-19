#pragma once

#include <mutex>


// this class stores values from one thread with the intention that they
// will be retrieved on another thread, using an integer to identify the value

template<typename T>
class ThreadCacher
{
public:
    ThreadCacher();

    // stores the value, returning its index
    template<typename VT>
    int Store(VT&& value);

    // retrieves the value at the index, or throws an exception if not present
    T Retrieve(int index) { return DoRetrieve<false>(index); }

    // retrieves the value at the index, or returns T() if not present
    T RetrieveOrDefault(int index) { return DoRetrieve<true>(index); }

private:
    template<bool use_default_if_not_present>
    T DoRetrieve(int index);

private:
    std::mutex m_mutex;
    std::map<int, std::unique_ptr<T>> m_values;
    int m_nextIndex;
};



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

template<typename T>
ThreadCacher<T>::ThreadCacher()
    :   m_nextIndex(1)
{
}


template<typename T>
template<typename VT>
int ThreadCacher<T>::Store(VT&& value)
{
    std::lock_guard lock(m_mutex);
    m_values.try_emplace(m_nextIndex, std::make_unique<T>(std::forward<VT>(value)));
    return m_nextIndex++;
}


template<typename T>
template<bool use_default_if_not_present>
T ThreadCacher<T>::DoRetrieve(const int index)
{
    std::unique_ptr<T> value;

    // lock
    {
        std::lock_guard lock(m_mutex);
        auto lookup = m_values.find(index);

        if( lookup != m_values.cend() )
        {
            value = std::move(lookup->second);
            m_values.erase(lookup);
        }
    }

    if( value != nullptr )
    {
        return std::move(*value);
    }

    else if constexpr(use_default_if_not_present)
    {
        return T();
    }

    else
    {
        throw CSProException("The ThreadCacher does not contain a value at index %d", index);
    }
}
