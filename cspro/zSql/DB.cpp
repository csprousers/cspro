#include "stdafx.h"
#include "DB.h"
#include "Encryption.h"


Sqlite::DB::DB(std::string file_path, const int open_flags/* = DefaultOpenFlags*/)
    :   DB()
{
    Open(std::move(file_path), open_flags);
}


Sqlite::DB::DB(std::string file_path, const std::vector<std::byte>& password_hash, const int open_flags/* = DefaultOpenFlags*/)
    :   DB()
{
    OpenEncrypted(std::move(file_path), password_hash, open_flags);
}


Sqlite::DB::DB(DB&& rhs) noexcept
    :   m_db(rhs.m_db),
        m_ownDb(rhs.m_ownDb),
        m_filePath(std::move(rhs.m_filePath)),
        m_statementPtrs(std::move(rhs.m_statementPtrs)),
        m_attachedFilePathsAndSchemaNames(std::move(rhs.m_attachedFilePathsAndSchemaNames))
{
    rhs.m_db = nullptr;
}


Sqlite::DB::~DB()
{
    Close_noexcept();
}


Sqlite::DB& Sqlite::DB::operator=(DB&& rhs) noexcept
{
    Close_noexcept();

    m_db = rhs.m_db;
    m_ownDb = rhs.m_ownDb;
    m_filePath = std::move(rhs.m_filePath);
    m_statementPtrs = std::move(rhs.m_statementPtrs);
    m_attachedFilePathsAndSchemaNames = std::move(rhs.m_attachedFilePathsAndSchemaNames);

    rhs.m_db = nullptr;

    return *this;
}


Sqlite::DB Sqlite::DB::CreateWrapper(sqlite3* const db, const bool assume_ownership)
{
    DB wrapped_db(db, assume_ownership);

    if( db != nullptr )
    {
        const char* const filename = sqlite3_db_filename(db, nullptr);

        if( filename != nullptr )
            wrapped_db.m_filePath = filename;
    }

    return wrapped_db;
}


void Sqlite::DB::Open(std::string file_path, const int open_flags/* = DefaultOpenFlags*/)
{
    ASSERT(m_ownDb);

    if( m_db != nullptr )
        throw Exception("A database is already open: %s", m_filePath.c_str());

    if( ( open_flags & SQLITE_OPEN_CREATE ) == 0 && !PortableFunctions::FileIsRegular(file_path) )
        throw Exception("The SQLite database does not exist: %s", file_path.c_str());

    sqlite3* db;

    if( sqlite3_open_v2(file_path.c_str(), &db, open_flags, nullptr) != SQLITE_OK )
        throw Exception(db, file_path, "Error opening a SQLite database");

    m_db = db;
    m_filePath = std::move(file_path);
}


void Sqlite::DB::OpenEncrypted(const std::string& file_path, const std::vector<std::byte>& password_hash, const int open_flags/* = DefaultOpenFlags*/)
{
    const bool file_already_exists = PortableFunctions::FileIsRegular(file_path);
    Open(file_path, open_flags);

    const BinaryBlock key = GetEncryptionKey(password_hash);

    try
    {
        KeyDatabase(&key, file_already_exists);
    }

    catch(...)
    {
        Close_noexcept();
        throw Exception(static_cast<sqlite3*>(nullptr), file_path, "Could not open an encrypted SQLite database with the supplied password");
    }
}


BinaryBlock Sqlite::DB::GetEncryptionKey(const std::vector<std::byte>& password_hash)
{
    constexpr std::string_view EncryptionType_sv = "aes256:";
    ASSERT(!password_hash.empty());

    // to get the SQLite key, prefix the password hash with the encryption type
    const size_t key_length = EncryptionType_sv.length() + password_hash.size();
    BinaryBlock key(key_length);

    memcpy(key.data(), EncryptionType_sv.data(), EncryptionType_sv.length());
    memcpy(key.data() + EncryptionType_sv.length(), password_hash.data(), password_hash.size());

    return key;
}


bool Sqlite::DB::Close_noexcept() noexcept
{
    bool success = true;

    if( m_db != nullptr )
    {
        // finalize all statements
        for( std::shared_ptr<sqlite3_stmt*>& statement_ptr : m_statementPtrs )
        {
            ASSERT(statement_ptr != nullptr);

            if( *statement_ptr != nullptr )
            {
                sqlite3_finalize(*statement_ptr);
                *statement_ptr = nullptr;
            }
        }

        // close the database
        if( m_ownDb && sqlite3_close(m_db) != SQLITE_OK )
            success = false;

        m_db = nullptr;
        m_filePath.clear();
        m_statementPtrs.clear();
        m_attachedFilePathsAndSchemaNames.reset();
    }

    return success;
}


void Sqlite::DB::Close()
{
    if( !Close_noexcept() )
        throw Exception(m_db, m_filePath, "Error closing SQLite database");
}


void Sqlite::DB::CheckDatabaseIsOpen() const
{
    if( m_db == nullptr )
        throw Exception("No SQLite database is open.");
}


void Sqlite::DB::KeyDatabase(const void* const encryption_key_data, const size_t encryption_key_size,
                             const bool db_has_already_been_keyed)
{
    CheckDatabaseIsOpen();

    auto throw_exception = [&]()
    {
        const char* const message_prefix = ( encryption_key_data == nullptr ) ? "The file is not a valid SQLite database: " :
                                                                                "The file is not a valid SQLite database or the encryption key is invalid: ";
        throw CSProException(message_prefix + m_filePath);
    };

    if( SqliteEncryption::sqlite3_key(m_db, encryption_key_data, int32_cast(encryption_key_size)) != SQLITE_OK )
        throw_exception();

    Sqlite::Statement stmt = PrepareStatement("PRAGMA user_version;");

    // make sure the database is valid
    if( stmt.Step() == SQLITE_NOTADB )
        throw_exception();

    // when creating a new database, prevent a 0-byte file
    if( !db_has_already_been_keyed )
    {
        try
        {
            const std::string sql = FormatText("PRAGMA user_version = %d;", stmt.GetColumn<int>(0));
            Execute(sql);
        }
        catch(...) { throw_exception(); }
    }
}


void Sqlite::DB::KeyDatabase(const BinaryBlock* const encryption_key, const bool db_has_already_been_keyed)
{
    const void* encryption_key_data;
    size_t encryption_key_size;

    if( encryption_key == nullptr )
    {
        encryption_key_data = nullptr;
        encryption_key_size = 0;
    }

    else
    {
        encryption_key_data = encryption_key->data();
        encryption_key_size = encryption_key->size();
    }

    KeyDatabase(encryption_key_data, encryption_key_size, db_has_already_been_keyed);
}


void Sqlite::DB::Execute(const cs::string_sz sql)
{
    CheckDatabaseIsOpen();

    if( sqlite3_exec(m_db, sql.c_str(), nullptr, nullptr, nullptr) != SQLITE_OK )
        throw Exception(m_db, m_filePath, "Error executing SQLite statement: %s", sql.c_str());
}


Sqlite::Statement Sqlite::DB::PrepareStatement(const char* const sql, const int sql_length)
{
    auto statement_ptr = std::make_shared<sqlite3_stmt*>();

    if( sqlite3_prepare_v2(m_db, sql, sql_length, statement_ptr.get(), nullptr) != SQLITE_OK )
        throw Exception(m_db, m_filePath, "Error creating SQLite statement: %s", sql);

    m_statementPtrs.emplace_back(statement_ptr);

    return Statement(std::move(statement_ptr));
}


Sqlite::Transaction Sqlite::DB::CreateTransaction()
{
    CheckDatabaseIsOpen();

    return Transaction(m_db);
}


void Sqlite::DB::Attach(std::string file_path, std::string schema_name)
{
    CheckDatabaseIsOpen();

    if( m_attachedFilePathsAndSchemaNames == nullptr )
    {
        m_attachedFilePathsAndSchemaNames = std::make_unique<std::vector<std::tuple<std::string, std::string>>>();
    }

    else
    {
        // only attach the database once when using the same schema name
        const auto& lookup = std::find_if(m_attachedFilePathsAndSchemaNames->cbegin(), m_attachedFilePathsAndSchemaNames->cend(),
            [&](const auto& file_path_and_schema_name)
            {
                return SO::EqualsNoCase(schema_name, std::get<1>(file_path_and_schema_name));
            });

        if( lookup != m_attachedFilePathsAndSchemaNames->cend() )
        {
            if( !SO::EqualsNoCase(file_path, std::get<0>(*lookup)) )
            {
                throw Exception(static_cast<sqlite3*>(nullptr), m_filePath, "A different database '%s' is already attached as '%s'",
                                                                            Path::GetFilename(std::get<0>(*lookup)).c_str(),
                                                                            schema_name.c_str());
            }

            return;
        }
    }

    Statement stmt = PrepareStatement("ATTACH ? AS ?;");
    stmt.Bind(1, file_path)
        .Bind(2, schema_name);

    if( stmt.Step() != SQLITE_DONE )
    {
        throw Exception(m_db, m_filePath, "Could not attach '%s' as '%s'",
                                          Path::GetFilename(file_path).c_str(),
                                          schema_name.c_str());
    }

    m_attachedFilePathsAndSchemaNames->emplace_back(std::move(file_path), std::move(schema_name));
}


void Sqlite::DB::Attach(const DB& db, std::string schema_name)
{
    db.CheckDatabaseIsOpen();

    Attach(db.m_filePath, std::move(schema_name));
}


void Sqlite::DB::Detach(const std::string& schema_name)
{
    CheckDatabaseIsOpen();

    if( m_attachedFilePathsAndSchemaNames != nullptr )
    {
        const auto& lookup = std::find_if(m_attachedFilePathsAndSchemaNames->cbegin(), m_attachedFilePathsAndSchemaNames->cend(),
            [&](const auto& file_path_and_schema_name)
            {
                return SO::EqualsNoCase(schema_name, std::get<1>(file_path_and_schema_name));
            });

        if( lookup != m_attachedFilePathsAndSchemaNames->cend() )
        {
            Statement stmt = PrepareStatement("DETACH ?;");
            stmt.Bind(1, schema_name);

            if( stmt.Step() != SQLITE_DONE )
                throw Exception(m_db, m_filePath, "Could not detach '%s'", schema_name.c_str());

            m_attachedFilePathsAndSchemaNames->erase(lookup);
            return;
        }
    }

    throw Exception(static_cast<sqlite3*>(nullptr), m_filePath, "No database is attached as '%s'", schema_name.c_str());
}


int64_t Sqlite::DB::GetLastInsertedRowId() const noexcept
{
    return ( m_db != nullptr ) ? sqlite3_last_insert_rowid(m_db) :
                                 0;
}


bool Sqlite::DB::TableExists(const std::string_view table_name_sv)
{
    Sqlite::Statement stmt = PrepareStatement(
        "SELECT 1 "
        "FROM `sqlite_master` "
        "WHERE `type` = 'table' AND `name` = ?;"
    );

    stmt.Bind(1, table_name_sv);

    return ( stmt.Step() == Sqlite::Result::Row );
}
