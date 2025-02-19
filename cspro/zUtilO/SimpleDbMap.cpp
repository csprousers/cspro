#include "StdAfx.h"
#include "SimpleDbMap.h"
#include <zSql/Commands.h>


namespace Constants
{
    constexpr size_t MaxNumberSqlInsertsInOneTransaction = 25000;
}

namespace Sqlite::Commands
{
    constexpr const char* CreateTableFormatter       = "CREATE TABLE IF NOT EXISTS `%s` (`Key` TEXT PRIMARY KEY UNIQUE NOT NULL, `Value` %s NOT NULL) WITHOUT ROWID;";
    constexpr const char* PutFormatter               = "INSERT OR REPLACE INTO `%s` (`Key`, `Value`) VALUES ( ?, ? );";
    constexpr const char* ClearFormatter             = "DELETE FROM `%s`;";
    constexpr const char* DeleteFormatter            = "DELETE FROM `%s` WHERE `Key` = ?;";

    constexpr const char* ExistsFormatter            = "SELECT 1 FROM `%s` WHERE `Key` = ? LIMIT 1;";
    constexpr const char* GetFormatter               = "SELECT `Value` FROM `%s` WHERE `Key` = ?;";
    constexpr const char* GetUsingKeyPrefixFormatter = "SELECT `Value` FROM `%s` WHERE `Key` LIKE ? ESCAPE '%c';";
    constexpr const char* IteratorFormatter          = "SELECT `Key`, `Value` FROM `%s`;";
};


SimpleDbMap::TableDetails::TableDetails(sqlite3* db, std::string table_name_, const ValueType value_type_)
    :   table_name(std::move(table_name_)),
        value_type(value_type_),
        stmt_put(db, FormatText(Sqlite::Commands::PutFormatter, table_name.c_str()), true),
        stmt_clear(db, FormatText(Sqlite::Commands::ClearFormatter, table_name.c_str()), true),
        stmt_delete(db, FormatText(Sqlite::Commands::DeleteFormatter, table_name.c_str()), true),
        stmt_exists(db, FormatText(Sqlite::Commands::ExistsFormatter, table_name.c_str()), true),
        stmt_get(db, FormatText(Sqlite::Commands::GetFormatter, table_name.c_str()), true),
        stmt_iterator(db, FormatText(Sqlite::Commands::IteratorFormatter, table_name.c_str()), true)
{
}


SimpleDbMap::SimpleDbMap()
    :   m_db(nullptr),
        m_currentTable(nullptr),
        m_transactions(0)
{
}


SimpleDbMap::~SimpleDbMap()
{
    Close();
}


bool SimpleDbMap::Open(std::string file_path,
                       const std::vector<std::tuple<std::string, ValueType>>& table_names_and_value_types,
                       const bool throw_exceptions/* = false*/)
{
    ASSERT(!table_names_and_value_types.empty());

    try
    {
        // open the database
        if( sqlite3_open(file_path.c_str(), &m_db) != SQLITE_OK )
            throw SQLiteStatementException("SQLite: Could not open: %s" + file_path);

        // create each table (as necessary) and prepare the SQL statements
        for( const auto& [table_name, value_type] : table_names_and_value_types )
            CreateTableIfNotExists(table_name, value_type);

        ASSERT(!m_tableDetails.empty());

        m_dbFilePath = std::move(file_path);
        m_currentTable = m_tableDetails.front().get();

        TransactionManager::Register(*this);

        return true;
    }

    catch( const SQLiteStatementException& )
    {
        Close();

        if( throw_exceptions )
            throw;

        return false;
    }
}


SimpleDbMap::TableDetails* SimpleDbMap::CreateTableIfNotExists(std::string table_name, const ValueType value_type)
{
    const std::string create_table_sql = FormatText(Sqlite::Commands::CreateTableFormatter, table_name.c_str(),
                                                    ( value_type == ValueType::String ) ? "TEXT" : "INTEGER");

    if( sqlite3_exec(m_db, create_table_sql.c_str(), nullptr, nullptr, nullptr) != SQLITE_OK )
        throw SQLiteStatementException("SQLite: Could not create table: " + table_name);

    return m_tableDetails.emplace_back(std::make_unique<TableDetails>(m_db, std::move(table_name), value_type)).get();
}


void SimpleDbMap::Close()
{
    if( m_db != nullptr )
    {
        CommitTransactions();
        TransactionManager::Deregister(*this);

        // clearing the table details will finalize all prepared statements
        m_tableDetails.clear();

        sqlite3_close(m_db);
        m_db = nullptr;
    }
}


bool SimpleDbMap::WrapInTransaction()
{
    if( m_transactions == Constants::MaxNumberSqlInsertsInOneTransaction )
    {
        if( !CommitTransactions() )
            return false;
    }

    if( m_transactions > 0 || sqlite3_exec(m_db, Sqlite::Commands::BeginTransaction, nullptr, nullptr, nullptr) == SQLITE_OK )
    {
        ++m_transactions;
        return true;
    }

    return false;
}


bool SimpleDbMap::CommitTransactions()
{
    if( m_transactions == 0 )
        return true;

    const bool success = ( sqlite3_exec(m_db, Sqlite::Commands::EndTransaction, nullptr, nullptr, nullptr) == SQLITE_OK );
    m_transactions = 0;

    return success;
}


bool SimpleDbMap::Exists(const std::string& key)
{
    ASSERT(m_db != nullptr && m_currentTable != nullptr);

    SQLiteStatement& stmt = m_currentTable->stmt_exists;
    const SQLiteResetOnDestruction rod(stmt);

    stmt.Bind(1, key);

    return ( stmt.Step() == SQLITE_ROW );
}


bool SimpleDbMap::Clear()
{
    ASSERT(m_db != nullptr && m_currentTable != nullptr);

    if( WrapInTransaction() )
    {
        SQLiteStatement& stmt = m_currentTable->stmt_clear;
        const SQLiteResetOnDestruction rod(stmt);

        return ( stmt.Step() == SQLITE_DONE );
    }

    return false;
}


bool SimpleDbMap::Delete(const std::string& key)
{
    ASSERT(m_db != nullptr && m_currentTable != nullptr);

    if( WrapInTransaction() )
    {
        SQLiteStatement& stmt = m_currentTable->stmt_delete;
        const SQLiteResetOnDestruction rod(stmt);

        stmt.Bind(1, key);

        return ( stmt.Step() == SQLITE_DONE );
    }

    return false;
}


template<typename T>
bool SimpleDbMap::Put(const std::string& key, const T& value)
{
    if( WrapInTransaction() )
    {
        SQLiteStatement& stmt = m_currentTable->stmt_put;
        const SQLiteResetOnDestruction rod(stmt);

        stmt.Bind(1, key);
        stmt.Bind(2, value);

        return ( stmt.Step() == SQLITE_DONE );
    }

    return false;
}


bool SimpleDbMap::PutString(const std::string& key, const std::string& value)
{
    ASSERT(m_db != nullptr && m_currentTable != nullptr && m_currentTable->value_type == ValueType::String);
    return Put(key, value);
}


bool SimpleDbMap::PutLong(const std::string& key, const long value)
{
    ASSERT(m_db != nullptr && m_currentTable != nullptr && m_currentTable->value_type == ValueType::Long);
    return Put(key, value);
}


template<typename T>
std::optional<T> SimpleDbMap::Get(const std::string& key)
{
    SQLiteStatement& stmt = m_currentTable->stmt_get;
    const SQLiteResetOnDestruction rod(stmt);

    stmt.Bind(1, key);

    if( stmt.Step() == SQLITE_ROW  )
        return stmt.GetColumn<T>(0);

    return std::nullopt;
}


std::optional<std::string> SimpleDbMap::GetString(const std::string& key)
{
    ASSERT(m_db != nullptr && m_currentTable != nullptr && m_currentTable->value_type == ValueType::String);
    return Get<std::string>(key);
}


std::optional<long> SimpleDbMap::GetLong(const std::string& key)
{
    ASSERT(m_db != nullptr && m_currentTable != nullptr && m_currentTable->value_type == ValueType::Long);
    return Get<long>(key);
}


std::optional<long> SimpleDbMap::GetLongUsingKeyPrefix(std::string key_prefix)
{
    ASSERT(m_db != nullptr && m_currentTable != nullptr && m_currentTable->value_type == ValueType::Long);

    // make sure that all values can be escaped properly (starting above LIKE's % and _ escapes)
    const char escape_ch = GetUnusedCharacter(key_prefix, 'a');

    // create the prepared statement if this is the first call to this method, or if
    // the escape character is different from the previously executed statement
    if( m_currentTable->escape_char_and_stmt_get_using_key_prefix == nullptr ||
        std::get<0>(*m_currentTable->escape_char_and_stmt_get_using_key_prefix) != escape_ch )
    {
        try
        {
            const std::string sql = FormatText(Sqlite::Commands::GetUsingKeyPrefixFormatter, m_currentTable->table_name.c_str(), escape_ch);

            m_currentTable->escape_char_and_stmt_get_using_key_prefix =
                std::make_unique<std::tuple<char, SQLiteStatement>>(escape_ch, SQLiteStatement(m_db, sql, true));
        }

        catch( const SQLiteStatementException& )
        {
            ASSERT(false);
            return std::nullopt;
        }
    }

    // escape any % and _ characters
    constexpr char EscapeCharacters[] = { '%', '_' };

    for( int i = 0; i < _countof(EscapeCharacters); ++i )
    {
        size_t escape_pos = 0;

        while( ( escape_pos = key_prefix.find(EscapeCharacters[i], escape_pos) ) != std::string_view::npos )
        {
            key_prefix.insert(key_prefix.begin() + escape_pos, escape_ch);
            escape_pos += 2;
        }
    }

    // add the % for the LIKE operation
    key_prefix.append("%");

    SQLiteStatement& stmt = std::get<1>(*m_currentTable->escape_char_and_stmt_get_using_key_prefix);
    const SQLiteResetOnDestruction rod(stmt);

    stmt.Bind(1, key_prefix);

    if( stmt.Step() == SQLITE_ROW )
        return stmt.GetColumn<long>(0);

    return std::nullopt;
}


void SimpleDbMap::ResetIterator()
{
    ASSERT(m_db != nullptr && m_currentTable != nullptr);
    m_currentTable->stmt_iterator.Reset();
}


template<typename T>
bool SimpleDbMap::Next(std::string* key, T* value)
{
    SQLiteStatement& stmt = m_currentTable->stmt_iterator;

    if( stmt.Step() == SQLITE_ROW )
    {
        *key = stmt.GetColumn<std::string>(0);
        *value = stmt.GetColumn<T>(1);
        return true;
    }

    return false;
}


bool SimpleDbMap::NextString(std::string* key, std::string* value)
{
    ASSERT(m_db != nullptr && m_currentTable != nullptr && m_currentTable->value_type == ValueType::String);
    return Next(key, value);
}


bool SimpleDbMap::NextLong(std::string* key, long* value)
{
    ASSERT(m_db != nullptr && m_currentTable != nullptr && m_currentTable->value_type == ValueType::Long);
    return Next(key, value);
}
