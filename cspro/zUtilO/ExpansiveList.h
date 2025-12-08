#pragma once

#include <zSql/DataStorage.h>
#include <zSql/SQLite.h>


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
class ExpansiveList
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
    static constexpr const char* ExceptionMessage = "The ExpansiveList could not create an in-memory database.";

    void AddValueToSQLite(const T& value);
    void SetUpDatabase();

    void BindValue(int value)                           { sqlite3_bind_int(m_stmtPut, 1, value); }
    void GetValueFromSQLite(int& value)                 { value = sqlite3_column_int(m_stmtIterator, 0); }

    void BindValue(double value)                        { sqlite3_bind_double(m_stmtPut, 1, value); }
    void GetValueFromSQLite(double& value)              { value = sqlite3_column_double(m_stmtIterator, 0); }

    void BindValue(const std::string& value)            { sqlite3_bind_text(m_stmtPut, 1, value.data(), int32_cast(value.length()), SQLITE_TRANSIENT); }
    void GetValueFromSQLite(std::string& value)         { value = reinterpret_cast<const char*>(sqlite3_column_text(m_stmtIterator, 0)); }

private:
    size_t m_size;

    size_t m_vectorSlotsRemaining;
    std::vector<T> m_vector;
    typename std::vector<T>::const_iterator m_vectorIterator;

    sqlite3* m_db;
    sqlite3_stmt* m_stmtPut;
    sqlite3_stmt* m_stmtIterator;

    enum class IteratorMode { NotStarted, Vector, SQLite, Finished };
    IteratorMode m_iteratorMode;
};



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

template<typename T>
ExpansiveList<T>::ExpansiveList(const size_t vector_size/* = VectorSize*/)
    :   m_size(0),
        m_vectorSlotsRemaining(vector_size),
        m_db(nullptr),
        m_stmtPut(nullptr),
        m_stmtIterator(nullptr),
        m_iteratorMode(IteratorMode::NotStarted)
{
}


template<typename T>
ExpansiveList<T>::~ExpansiveList()
{
    if( m_db != nullptr )
    {
        sqlite3_finalize(m_stmtIterator);
        sqlite3_finalize(m_stmtPut);
        sqlite3_close(m_db);
    }
}


template<typename T>
template<typename VT>
void ExpansiveList<T>::AddValue(VT&& value)
{
    ASSERT(m_iteratorMode == IteratorMode::NotStarted);

    if( m_vectorSlotsRemaining == 0 )
    {
        AddValueToSQLite(value);
    }

    else
    {
        m_vector.emplace_back(std::forward<VT>(value));
        --m_vectorSlotsRemaining;
    }

    ++m_size;
}


template<typename T>
bool ExpansiveList<T>::GetValue(T& value)
{
    if( m_iteratorMode == IteratorMode::SQLite )
    {
        if( sqlite3_step(m_stmtIterator) != SQLITE_ROW )
        {
            m_iteratorMode = IteratorMode::Finished;
        }

        else
        {
            GetValueFromSQLite(value);
            return true;
        }
    }

    else if( m_iteratorMode == IteratorMode::Vector )
    {
        if( m_vectorIterator == m_vector.cend() )
        {
            if( m_db == nullptr )
            {
                m_iteratorMode = IteratorMode::Finished;
            }

            else
            {
                m_iteratorMode = IteratorMode::SQLite;
                return GetValue(value);
            }
        }

        else
        {
            value = *m_vectorIterator;
            ++m_vectorIterator;
            return true;
        }
    }

    else if( m_iteratorMode == IteratorMode::NotStarted )
    {
        m_iteratorMode = IteratorMode::Vector;

        m_vectorIterator = m_vector.cbegin();

        return GetValue(value);
    }

    ASSERT(m_iteratorMode == IteratorMode::Finished);

    return false;
}


template<typename T>
void ExpansiveList<T>::RestartIterator()
{
    m_iteratorMode = IteratorMode::NotStarted;
}


template<typename T>
void ExpansiveList<T>::AddValueToSQLite(const T& value)
{
    // set up the database if necessary
    if( m_db == nullptr )
        SetUpDatabase();

    sqlite3_reset(m_stmtPut);

    BindValue(value);

    if( sqlite3_step(m_stmtPut) != SQLITE_DONE )
        throw CSProException(ExceptionMessage);
}


template<typename T>
void ExpansiveList<T>::SetUpDatabase()
{
    const std::string create_table_sql = FormatText("CREATE TABLE `values` ( `value` %s );", Sqlite::GetDataType<T>());

    if( sqlite3_open("", &m_db) != SQLITE_OK ||
        sqlite3_exec(m_db, create_table_sql.c_str(), nullptr, nullptr, nullptr) != SQLITE_OK ||
        sqlite3_prepare_v2(m_db, "INSERT INTO `values` ( `value` ) VALUES ( ? );", -1, &m_stmtPut, nullptr) != SQLITE_OK ||
        sqlite3_prepare_v2(m_db, "SELECT `value` FROM `values` ORDER BY `_rowid_`;", -1, &m_stmtIterator, nullptr) != SQLITE_OK )
    {
        throw CSProException(ExceptionMessage);
    }
}
