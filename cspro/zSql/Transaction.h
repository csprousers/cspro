#pragma once

#include <zSql/zSql.h>

namespace Sqlite { class Transaction; }

struct sqlite3;


// --------------------------------------------------------------------------
// Sqlite::Transaction is a transaction / savepoint wrapper.
// If a transaction or savepoint is created (using this class), it is
// automatically rolled back on destruction if not comitted first
// (to support automated rollback when exceptions are thrown).
// --------------------------------------------------------------------------

class ZSQL_API Sqlite::Transaction
{
public:
    // Creating an object does not start the transaction. For that, use Begin() or Savepoint().
    Transaction(sqlite3* db);

    Transaction(const Transaction& rhs) = delete;
    Transaction(Transaction&& rhs) noexcept;

    ~Transaction();

    // Starts a transaction on the database.
    void Begin();

    // Starts a transaction on the database with the counter stored for use by CommitPeriodically.
    void Begin(size_t commit_periodically_counter);

    // Creates a savepoint on the database.
    // SQLite does not support nested transactions so if there is already a transaction you can
    // use a savepoint to act like a nested transaction. Savepoints are named so that you can rollback
    // to a specific savepoint.
    void Savepoint(std::string name);

    // Rollsback the current transaction or rollbacks to the savepoint.
    // If neither Begin() nor Savepoint() has been called then this is a noop.
    void Rollback();

    // Commits the current transaction or savepoint.
    // If neither Begin() nor Savepoint() has been called then this is a noop.
    void Commit();

    // Registers that a write activity has happened on the SQLite database.
    // Periodically, using the number registered during a call to Begin, the
    // transaction will be committed and a new one started.
    void CommitPeriodically();

private:
    bool IsTransactionOrSavepointInUse() const;

private:
    sqlite3* m_db;
    bool m_startedTransaction;
    std::unique_ptr<std::tuple<size_t, size_t>> m_commitPeriodicallyCounter; // current count + "commit every" count
    std::unique_ptr<std::string> m_savepointName;
};
