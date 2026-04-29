#include "stdafx.h"
#include "Transaction.h"
#include <zToolsO/UniqueId.h>


Sqlite::Transaction::Transaction(sqlite3* const db) noexcept
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


Sqlite::Transaction::~Transaction() noexcept
{
    if( m_db != nullptr && IsTransactionOrSavepointInUse() )
    {
        try
        {
            Rollback();
        }
        catch(...) { ASSERT(false); }
    }
}


bool Sqlite::Transaction::IsTransactionOrSavepointInUse() const noexcept
{
    return ( m_startedTransaction || m_savepointName != nullptr );
}


void Sqlite::Transaction::ExecuteSql(const cs::string_sz sql, const char* const exception_message) const
{
    ASSERT(m_db != nullptr);

    if( sqlite3_exec(m_db, sql.c_str(), nullptr, nullptr, nullptr) != SQLITE_OK )
        throw CSProException(exception_message);
}


void Sqlite::Transaction::Begin()
{
    ASSERT(!IsTransactionOrSavepointInUse());

    ExecuteSql("BEGIN", "Could not start a SQLite transaction.");

    m_startedTransaction = true;
}


void Sqlite::Transaction::Begin(const size_t commit_periodically_counter)
{
    ASSERT(commit_periodically_counter != 0);

    m_commitPeriodicallyCounter = std::make_unique<std::tuple<size_t, size_t>>(0, commit_periodically_counter);

    Begin();
}


void Sqlite::Transaction::Savepoint(std::unique_ptr<std::string> name)
{
    ASSERT(name != nullptr);
    ASSERT(!IsTransactionOrSavepointInUse());

    ExecuteSql("SAVEPOINT " + *name, "Could not start a SQLite savepoint.");

    m_savepointName = std::move(name);
}


void Sqlite::Transaction::Savepoint(std::string name)
{
    ASSERT(!SO::StartsWithNoCase(name, SavepointUniqueNamePrefix));
    Savepoint(std::make_unique<std::string>(std::move(name)));
}


void Sqlite::Transaction::Savepoint()
{
    Savepoint(std::make_unique<std::string>(
        FormatText("%s%d", SavepointUniqueNamePrefix, UniqueId::CreateInt())
    ));
}


void Sqlite::Transaction::Rollback()
{
    constexpr const char* exception_message = "Could not rollback a SQLite transaction.";

    if( m_startedTransaction )
    {
        ExecuteSql("ROLLBACK", exception_message);
        m_startedTransaction = false;
    }

    else if( m_savepointName != nullptr )
    {
        ExecuteSql("ROLLBACK TO " + *m_savepointName, exception_message);
        ExecuteSql("RELEASE " + *m_savepointName, "Could not release a SQLite savepoint following rollback.");
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
        ExecuteSql("COMMIT", "Could not commit a SQLite transaction.");
        m_startedTransaction = false;
    }

    else if( m_savepointName != nullptr )
    {
        // savepoints are not really committed since nothing is really committed
        // until the parent transaction is committed so we just get rid of the
        // savepoint since we will no longer need it to rollback to
        ExecuteSql("RELEASE " + *m_savepointName, "Could not release a SQLite savepoint.");
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
