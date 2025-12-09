#include "StdAfx.h"
#include "ExpansiveList.h"
#include <zSql/DataStorage.h>
#include <zSql/DB.h>


template<typename T>
struct ExpansiveList<T>::DbData
{
    Sqlite::DB db;
    Sqlite::Statement stmt_put;
    Sqlite::Statement stmt_iterator;
};


template<typename T>
ExpansiveList<T>::ExpansiveList(const size_t vector_size/* = VectorSize*/)
    :   m_size(0),
        m_vectorSlotsRemaining(vector_size),
        m_iteratorMode(IteratorMode::NotStarted)
{
}


template<typename T>
ExpansiveList<T>::~ExpansiveList()
{
}


template<typename T>
bool ExpansiveList<T>::GetValue(T& value)
{
    if( m_iteratorMode == IteratorMode::SQLite )
    {
        if( m_dbData->stmt_iterator.Step() != Sqlite::Result::Row )
        {
            m_iteratorMode = IteratorMode::Finished;
        }

        else
        {
            value = m_dbData->stmt_iterator.template GetColumn<T>(0);
            return true;
        }
    }

    else if( m_iteratorMode == IteratorMode::Vector )
    {
        if( m_vectorIterator == m_vector.cend() )
        {
            if( m_dbData == nullptr )
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
void ExpansiveList<T>::SetUpDb()
{
    ASSERT(m_dbData == nullptr);

    try
    {
        m_dbData = std::make_unique<DbData>();

        m_dbData->db.Open("", Sqlite::OpenFlags::ReadWrite | Sqlite::OpenFlags::Create);

        const std::string create_table_sql = FormatText("CREATE TABLE `values` ( `value` %s );", Sqlite::GetDataType<T>());
        m_dbData->db.Execute(create_table_sql);

        m_dbData->stmt_put = m_dbData->db.PrepareStatement("INSERT INTO `values` ( `value` ) VALUES ( ? );");
        m_dbData->stmt_iterator = m_dbData->db.PrepareStatement("SELECT `value` FROM `values` ORDER BY `_rowid_`;");
    }
    catch(...) { throw CSProException("The ExpansiveList could not create an in-memory database."); }
}


template<typename T>
void ExpansiveList<T>::AddValueToDb(const T& value)
{
    // set up the database if necessary
    if( m_dbData == nullptr )
        SetUpDb();

    m_dbData->stmt_put.Reset()
                      .Bind(1, value);

    if( m_dbData->stmt_put.Step() != Sqlite::Result::Done )
        throw CSProException("Error adding a value to an ExpansiveList.");
}



// --------------------------------------------------------------------------
// class instantiations
// --------------------------------------------------------------------------

template class ExpansiveList<int>;
template class ExpansiveList<double>;
template class ExpansiveList<std::string>;
