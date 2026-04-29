#pragma once

#include <zSql/zSql.h>

namespace Sqlite { class Transaction; }

struct sqlite3;


// --------------------------------------------------------------------------
// Sqlite::Transaction is a transaction / savepoint wrapper.
// If a transaction or savepoint is created (using this class), it is
// automatically rolled back on destruction if not committed first
// (to support automated rollback when exceptions are thrown).
//
// Because transactions cannot be nested, it is best to use savepoints.
// If creating a savepoint without providing a name, a unique name will be
// created.
// --------------------------------------------------------------------------

class ZSQL_API Sqlite::Transaction
{
public:
    // Creating an object does not start the transaction. For that, use Begin() or Savepoint().
    Transaction(sqlite3* db) noexcept;

    Transaction(const Transaction& rhs) = delete;
    Transaction(Transaction&& rhs) noexcept;

    ~Transaction() noexcept;

    // Starts a transaction on the database.
    // An exception is thrown on error.
    void Begin();

    // Starts a transaction on the database with the counter stored for use by CommitPeriodically.
    // An exception is thrown on error.
    void Begin(size_t commit_periodically_counter);

    // Creates a savepoint on the database.
    // SQLite does not support nested transactions so if there is already a transaction you can
    // use a savepoint to act like a nested transaction. Savepoints are named so that you can
    // rollback to a specific savepoint.
    // An exception is thrown on error.
    void Savepoint(std::string name);

    // A unique savepoint name will be created, starting with "sql_sp_".
    void Savepoint();

    // Rollsback the current transaction or rollbacks to the savepoint.
    // If neither Begin() nor Savepoint() has been called then this is a noop.
    // An exception is thrown on error.
    void Rollback();

    // Commits the current transaction or savepoint.
    // If neither Begin() nor Savepoint() has been called then this is a noop.
    // An exception is thrown on error.
    void Commit();

    // Registers that a write activity has happened on the SQLite database.
    // Periodically, using the number registered during a call to Begin, the
    // transaction will be committed and a new one started.
    // An exception is thrown on error.
    void CommitPeriodically();

private:
    bool IsTransactionOrSavepointInUse() const noexcept;

    void ExecuteSql(cs::string_sz sql, const char* exception_message) const;

    static constexpr const char* SavepointUniqueNamePrefix = "sql_sp_";
    void Savepoint(std::unique_ptr<std::string> name);

private:
    sqlite3* m_db;
    bool m_startedTransaction;
    std::unique_ptr<std::tuple<size_t, size_t>> m_commitPeriodicallyCounter; // current count + "commit every" count
    std::unique_ptr<std::string> m_savepointName;
};
