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
        m_filePath(std::move(rhs.m_filePath)),
        m_statementPtrs(std::move(rhs.m_statementPtrs)),
        m_attachedFilePathsAndSchemaNames(std::move(rhs.m_attachedFilePathsAndSchemaNames))
{
    m_db = nullptr;
}


Sqlite::DB::~DB()
{
    if( m_db != nullptr )
    {
        try
        {
            Close();
        }
        catch(...) { }
    }
}


void Sqlite::DB::Open(std::string file_path, const int open_flags/* = DefaultOpenFlags*/)
{
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
    constexpr std::string_view EncryptionType_sv = "aes256:";
    ASSERT(!password_hash.empty());

    const bool file_already_exists = PortableFunctions::FileIsRegular(file_path);
    Open(file_path, open_flags);

    // to get the SQLite key, prefix the password hash with the encryption type
    const size_t key_length = EncryptionType_sv.length() + password_hash.size();
    auto key = std::make_unique_for_overwrite<char[]>(key_length);
    memcpy(key.get(), EncryptionType_sv.data(), EncryptionType_sv.length());
    memcpy(key.get() + EncryptionType_sv.length(), password_hash.data(), password_hash.size());

    try
    {
        if( SqliteEncryption::sqlite3_key(m_db, key.get(), static_cast<int>(key_length)) != SQLITE_OK )
            throw std::exception();

        // if the file already exists, check that the password is correct with a simple query
        if( file_already_exists )
        {
            Statement stmt = PrepareStatement("PRAGMA user_version;");

            if( stmt.Step() == SQLITE_NOTADB )
                throw std::exception();
        }

        // when creating a new database, prevent a 0-byte file by setting the user_version pragma
        else
        {
            Execute("PRAGMA user_version = 0;");
        }
    }

    catch(...)
    {
        try
        {
            Close();
        }
        catch(...) { }

        throw Exception(static_cast<sqlite3*>(nullptr), file_path, "Could not open an encrypted SQLite database with the supplied password");
    }
}


void Sqlite::DB::Close()
{
    if( m_db == nullptr )
        return;

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

    if( sqlite3_close(m_db) != SQLITE_OK )
        throw Exception(m_db, m_filePath, "Error closing SQLite database");

    m_db = nullptr;
    m_filePath.clear();
}


void Sqlite::DB::CheckDatabaseIsOpen() const
{
    if( m_db == nullptr )
        throw Exception("No SQLite database is open.");
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
