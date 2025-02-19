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

    ~Transaction();

    // Starts a transaction on the database.
    void Begin();

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

private:
    sqlite3* m_db;
    bool m_startedTransaction;
    std::unique_ptr<std::string> m_savepointName;
};
