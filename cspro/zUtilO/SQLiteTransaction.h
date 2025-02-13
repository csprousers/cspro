#pragma once

#include <SQLite/SQLite.h>

///<summary>
/// SQLite transaction/savepoint wrapper.
/// Creates a transaction or savepoint that
/// is automatically rolled back on destruction
/// if not comitted first to support automated rollback
/// when exceptions are thrown.
///</summary>
class SQLiteTransaction
{
public:

    ///<summary>
    /// Create transaction. Doesn't start the transaction,
    /// for that use Begin() or Savepoint().
    ///</summary>
    SQLiteTransaction(sqlite3* db) :
        m_db(db),
        m_bStartedTransaction(false),
        m_bStartedSavepoint(false)
    {}

    ///<summary>
    /// Start a transaction on the database.
    ///</summary>
    void Begin()
    {
        ASSERT(!m_bStartedTransaction && !m_bStartedSavepoint);
        sqlite3_exec(m_db, "BEGIN", NULL, NULL, NULL);
        m_bStartedTransaction = true;
    }

    ///<summary>
    /// Create a savepoint on the database.
    /// SQLite does not support nested transactions so if
    /// there is already a transaction you can use a savepoint
    /// to act like a nested transaction.
    /// Savepoints are named so that you can rollback to a
    /// a specific savepoint.
    ///</summary>
    void Savepoint(const char* name)
    {
        ASSERT(!m_bStartedTransaction && !m_bStartedSavepoint);
        m_savepointName = name;
        sqlite3_exec(m_db, (std::string("SAVEPOINT") + m_savepointName).c_str(), NULL, NULL, NULL);
        m_bStartedSavepoint = true;
    }

    ///<summary>
    /// Rolllback the current transaction or rollback to
    /// the savepoint. If neither Begin() nor Savepoint() has
    /// been called then this is noop.
    ///</summary>
    void Rollback()
    {
        if (m_bStartedTransaction) {
            sqlite3_exec(m_db, "ROLLBACK", NULL, NULL, NULL);
            m_bStartedTransaction = false;
        } else if (m_bStartedSavepoint) {
            sqlite3_exec(m_db, ("ROLLBACK to " + m_savepointName).c_str(), NULL, NULL, NULL);
            m_bStartedSavepoint = false;
        }
    }

    ///<summary>
    /// Commit the current transaction or savepoint.
    /// If neither Begin() nor Savepoint() has
    /// been called then this is noop.
    ///</summary>
    void Commit()
    {
        if (m_bStartedTransaction) {
            sqlite3_exec(m_db, "COMMIT", NULL, NULL, NULL);
            m_bStartedTransaction = false;
        } else if (m_bStartedSavepoint) {
            // Savepoints are not really comitted since nothing is really comitted
            // until the parent transaction is comitted so we just get rid of the
            // savepoint since we will no longer need it to rollback to.
            sqlite3_exec(m_db, ("RELEASE " + m_savepointName).c_str(), NULL, NULL, NULL);
            m_bStartedSavepoint = false;
        }
    }

    ~SQLiteTransaction()
    {
        Rollback();
    }

private:
    sqlite3* m_db;
    bool m_bStartedSavepoint;
    bool m_bStartedTransaction;
    std::string m_savepointName;
};
