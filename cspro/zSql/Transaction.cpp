#include "stdafx.h"
#include "Transaction.h"


Sqlite::Transaction::Transaction(sqlite3* const db)
    :   m_db(db),
        m_startedTransaction(false)
{
    ASSERT(m_db != nullptr);
}


Sqlite::Transaction::~Transaction()
{
    Rollback();
}


void Sqlite::Transaction::Begin()
{
    ASSERT(!m_startedTransaction && m_savepointName == nullptr);
    m_startedTransaction = true;
    sqlite3_exec(m_db, "BEGIN", nullptr, nullptr, nullptr);
}


void Sqlite::Transaction::Savepoint(std::string name)
{
    ASSERT(!m_startedTransaction && m_savepointName == nullptr);
    m_savepointName = std::make_unique<std::string>(std::move(name));
    sqlite3_exec(m_db, ( "SAVEPOINT " + *m_savepointName ).c_str(), nullptr, nullptr, nullptr);
}


void Sqlite::Transaction::Rollback()
{
    if( m_startedTransaction )
    {
        sqlite3_exec(m_db, "ROLLBACK", nullptr, nullptr, nullptr);
        m_startedTransaction = false;
    }

    else if( m_savepointName != nullptr )
    {
        sqlite3_exec(m_db, ( "ROLLBACK TO " + *m_savepointName ).c_str(), nullptr, nullptr, nullptr);
        m_savepointName.reset();
    }
}


void Sqlite::Transaction::Commit()
{
    if( m_startedTransaction )
    {
        sqlite3_exec(m_db, "COMMIT", nullptr, nullptr, nullptr);
        m_startedTransaction = false;
    }

    else if( m_savepointName != nullptr )
    {
        // Savepoints are not really comitted since nothing is really comitted
        // until the parent transaction is comitted so we just get rid of the
        // savepoint since we will no longer need it to rollback to.
        sqlite3_exec(m_db, ( "RELEASE " + *m_savepointName ).c_str(), nullptr, nullptr, nullptr);
        m_savepointName.reset();
    }
}
