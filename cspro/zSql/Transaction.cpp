#include "stdafx.h"
#include "Transaction.h"


Sqlite::Transaction::Transaction(sqlite3* const db)
    :   m_db(db),
        m_startedTransaction(false)
{
    ASSERT(m_db != nullptr);
}


Sqlite::Transaction::Transaction(Transaction&& rhs) noexcept
    :   m_db(rhs.m_db),
        m_startedTransaction(rhs.m_startedTransaction),
        m_commitPeriodicallyCounter(std::move(rhs.m_commitPeriodicallyCounter)),
        m_savepointName(std::move(rhs.m_savepointName))
{
    rhs.m_db = nullptr;
}


Sqlite::Transaction::~Transaction()
{
    if( m_db != nullptr && IsTransactionOrSavepointInUse() )
        Rollback();
}


bool Sqlite::Transaction::IsTransactionOrSavepointInUse() const
{
    return ( m_startedTransaction || m_savepointName != nullptr );
}


void Sqlite::Transaction::Begin()
{
    ASSERT(!IsTransactionOrSavepointInUse());
    m_startedTransaction = true;
    sqlite3_exec(m_db, "BEGIN", nullptr, nullptr, nullptr);
}


void Sqlite::Transaction::Begin(const size_t commit_periodically_counter)
{
    ASSERT(commit_periodically_counter != 0);

    m_commitPeriodicallyCounter = std::make_unique<std::tuple<size_t, size_t>>(0, commit_periodically_counter);

    Begin();
}


void Sqlite::Transaction::Savepoint(std::string name)
{
    ASSERT(!IsTransactionOrSavepointInUse());
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

    else
    {
        ASSERT(false);
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

    else
    {
        ASSERT(false);
    }
}


void Sqlite::Transaction::CommitPeriodically()
{
    if( m_commitPeriodicallyCounter == nullptr )
    {
        ASSERT(false);
    }

    else
    {
        ASSERT(std::get<0>(*m_commitPeriodicallyCounter) < std::get<1>(*m_commitPeriodicallyCounter));

        if( ++std::get<0>(*m_commitPeriodicallyCounter) < std::get<1>(*m_commitPeriodicallyCounter) )
            return;

        std::get<0>(*m_commitPeriodicallyCounter) = 0;
    }

    Commit();
    Begin();
}
