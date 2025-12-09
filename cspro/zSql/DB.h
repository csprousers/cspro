#pragma once

#include <zSql/zSql.h>
#include <zSql/Definitions.h>
#include <zSql/Statement.h>
#include <zSql/Transaction.h>


// --------------------------------------------------------------------------
// DB is a wrapper around SQLite's C functions, adding exception support
// and the automatic closure of databases in the destructor.
// --------------------------------------------------------------------------

class ZSQL_API Sqlite::DB
{
public:
    // By default, databases are opened in read/write mode, with the requirement that the database exists.
    static constexpr int DefaultOpenFlags = OpenFlags::ReadWrite;

    DB();
    DB(std::string file_path, int open_flags = DefaultOpenFlags);
    DB(std::string file_path, const std::vector<std::byte>& password_hash, int open_flags = DefaultOpenFlags);
    DB(const DB& rhs) = delete;
    DB(DB&& rhs) noexcept;
    ~DB();

    DB& operator=(const DB& rhs) = delete;
    DB& operator=(DB&& rhs) noexcept;

    // Wraps in a DB object a pointer to a SQLite database.
    // If assume_ownership is false, the database object should outlive this object
    // and the database will not be closed on destruction.
    static DB CreateWrapper(sqlite3* db, bool assume_ownership);

    // Opens a SQLite database, throwing exceptions on error.
    void Open(std::string file_path, int open_flags = DefaultOpenFlags);

    // Opens an encrypted SQLite database (in "AES-256 in OFB mode"), throwing exceptions on error.
    // If the database does not exist, a pragma will be set to ensure that the database is not closed
    // as a 0-byte file (without the encryption parameters written).
    void OpenEncrypted(const std::string& file_path, const std::vector<std::byte>& password_hash, int open_flags = DefaultOpenFlags);

    // Returns the SQLite encryption key for the given password hash.
    static BinaryBlock GetEncryptionKey(const std::vector<std::byte>& password_hash);

    // Closes the SQLite database, throwing exceptions on error.
    void Close();

    // Returns true if a SQLite database is open.
    bool IsOpen() const noexcept { return ( m_db != nullptr ); }

    // Returns the SQLite database pointer.
    sqlite3* GetDb() noexcept { return m_db; }

    // Returns the file path of the open database.
    const std::string& GetFilePath() const noexcept { return m_filePath; }

    // Returns the "English language explanation of the most recent error."
    std::string GetLastErrorMessage() const noexcept { return sqlite3_errmsg(m_db); }

    // Specifies the encryption key on a newly opened database connection, throwing an exception if the
    // key is invalid (when encryption_key_data is non-null and db_has_already_been_keyed is true).
    // If db_has_already_been_keyed is false, the user_version pragma will be set, which
    // will prevent a 0-byte file when creating a new database.
    void KeyDatabase(const void* encryption_key_data, size_t encryption_key_size, bool db_has_already_been_keyed);
    void KeyDatabase(const BinaryBlock* encryption_key, bool db_has_already_been_keyed);

    // Executes the SQL statement, throwing exceptions on error.
    void Execute(cs::string_sz sql);

    // Prepares a statement, throwing exceptions on error.
    // The statement will be finalized when the Statement class is destructed, or when the database is closed.
    Statement PrepareStatement(const char* sql);
    Statement PrepareStatement(std::string_view sql_sv);

    // Using the Statement object, prepares a statement if not already done so, throwing exceptions on error.
    // If already prepared, the statement is reset.
    void PrepareOrResetStatement(Statement& statement, const char* sql)         { PrepareOrResetStatementWorker(statement, sql); }
    void PrepareOrResetStatement(Statement& statement, std::string_view sql_sv) { PrepareOrResetStatementWorker(statement, sql_sv); }

    // Returns a Transaction object for the SQLite database. The transaction is not immediately started.
    Transaction CreateTransaction();

    // Attaches a SQLite database, throwing exceptions on error.
    // Attaching the same database using the same schema name will not result in an error.
    void Attach(std::string file_path, std::string schema_name);
    void Attach(const DB& db, std::string schema_name);

    // Detaches a SQLite database, throwing exceptions on error.
    void Detach(const std::string& schema_name);

    // Returns the row ID of the last inserted row, or 0 if no row has been inserted or if the database is not open.
    int64_t GetLastInsertedRowId() const noexcept;

    // Returns true if a table with the given name exists in the database (from `sqlite_master`).
    bool TableExists(std::string_view table_name_sv);

private:
    DB(sqlite3* db, bool own_db);

    bool Close_noexcept() noexcept;

    // Throws an exception if no database is open.
    void CheckDatabaseIsOpen() const;

    Statement PrepareStatement(const char* sql, int sql_length);

    template<typename T>
    void PrepareOrResetStatementWorker(Statement& statement, T&& args);

private:
    sqlite3* m_db;
    bool m_ownDb;
    std::string m_filePath;
    std::vector<std::shared_ptr<sqlite3_stmt*>> m_statementPtrs;
    std::unique_ptr<std::vector<std::tuple<std::string, std::string>>> m_attachedFilePathsAndSchemaNames;
};



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

inline Sqlite::DB::DB(sqlite3* const db, const bool own_db)
    :   m_db(db),
        m_ownDb(own_db)
{
}


inline Sqlite::DB::DB()
    :   DB(nullptr, true)
{
}


inline Sqlite::Statement Sqlite::DB::PrepareStatement(const char* const sql)
{
    return PrepareStatement(sql, -1);
}


inline Sqlite::Statement Sqlite::DB::PrepareStatement(const std::string_view sql_sv)
{
    return PrepareStatement(sql_sv.data(), static_cast<int>(sql_sv.length()));
}


template<typename T>
void Sqlite::DB::PrepareOrResetStatementWorker(Statement& statement, T&& args)
{
    if( statement.IsPrepared() )
    {
        statement.Reset();
    }

    else
    {
        statement = PrepareStatement(std::forward<T>(args));
    }
}
