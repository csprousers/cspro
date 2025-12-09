#pragma once

#include <zUtilO/zUtilO.h>


// --------------------------------------------------------------------------
// ExpansiveList
//
// This class allows for the storage of a large number of values, first
// storing them in a std::vector, and after that vector is "full," it will
// use an in-memory SQLite database to store values. Once iteration on the
// list has started, no more values can be added to the list.
//
// A related class is ExpansiveMap.
// --------------------------------------------------------------------------

template<typename T>
class CLASS_DECL_ZUTILO ExpansiveList
{
public:
    static constexpr size_t VectorSize = 200000;

    ExpansiveList(size_t vector_size = VectorSize);
    ~ExpansiveList();

    size_t GetSize() const { return m_size; }

    // AddValue will throw an exception if there is an error interacting with the SQLite database.
    template<typename VT>
    void AddValue(VT&& value);

    bool GetValue(T& value);

    void RestartIterator();

private:
    void SetUpDb();
    void AddValueToDb(const T& value);

private:
    size_t m_size;

    size_t m_vectorSlotsRemaining;
    std::vector<T> m_vector;
    typename std::vector<T>::const_iterator m_vectorIterator;

    struct DbData;
    std::unique_ptr<DbData> m_dbData;

    enum class IteratorMode { NotStarted, Vector, SQLite, Finished };
    IteratorMode m_iteratorMode;
};



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

template<typename T>
template<typename VT>
void ExpansiveList<T>::AddValue(VT&& value)
{
    ASSERT(m_iteratorMode == IteratorMode::NotStarted);

    if( m_vectorSlotsRemaining == 0 )
    {
        AddValueToDb(value);
    }

    else
    {
        m_vector.emplace_back(std::forward<VT>(value));
        --m_vectorSlotsRemaining;
    }

    ++m_size;
}
