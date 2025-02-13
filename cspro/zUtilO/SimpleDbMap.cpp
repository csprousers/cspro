#include "StdAfx.h"
#include "SimpleDbMap.h"
#include <SQLite/SQLiteHelpers.h>


namespace Constants
{
    constexpr unsigned MaxNumberSqlInsertsInOneTransaction = 25000;
}

namespace SqlStatements
{
    constexpr const TCHAR* CreateTableFormatter       = _T("CREATE TABLE IF NOT EXISTS `%s` (`Key` TEXT PRIMARY KEY UNIQUE NOT NULL, `Value` %s NOT NULL) WITHOUT ROWID;");
    constexpr const TCHAR* PutFormatter               = _T("INSERT OR REPLACE INTO `%s` (`Key`, `Value`) VALUES ( ?, ? );");
    constexpr const TCHAR* ClearFormatter             = _T("DELETE FROM `%s`;");
    constexpr const TCHAR* DeleteFormatter            = _T("DELETE FROM `%s` WHERE `Key` = ?;");

    constexpr const TCHAR* ExistsFormatter            = _T("SELECT 1 FROM `%s` WHERE `Key` = ? LIMIT 1;");
    constexpr const TCHAR* GetFormatter               = _T("SELECT `Value` FROM `%s` WHERE `Key` = ?;");
    constexpr const TCHAR* GetUsingKeyPrefixFormatter = _T("SELECT `Value` FROM `%s` WHERE `Key` LIKE ? ESCAPE '%c';");
    constexpr const TCHAR* IteratorFormatter          = _T("SELECT `Key`, `Value` FROM `%s`;");
};


SimpleDbMap::TableDetails::TableDetails(sqlite3* db, std::wstring table_name_, ValueType value_type_)
    :   table_name(std::move(table_name_)),
        value_type(value_type_),
        stmt_put(db, FormatText(SqlStatements::PutFormatter, table_name.c_str()), true),
        stmt_clear(db, FormatText(SqlStatements::ClearFormatter, table_name.c_str()), true),
        stmt_delete(db, FormatText(SqlStatements::DeleteFormatter, table_name.c_str()), true),
        stmt_exists(db, FormatText(SqlStatements::ExistsFormatter, table_name.c_str()), true),
        stmt_get(db, FormatText(SqlStatements::GetFormatter, table_name.c_str()), true),
        stmt_iterator(db, FormatText(SqlStatements::IteratorFormatter, table_name.c_str()), true)
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


bool SimpleDbMap::Open(std::wstring filename,
                       const std::vector<std::tuple<std::wstring, ValueType>>& table_names_and_value_types,
                       bool throw_exceptions/* = false*/)
{
    ASSERT(!table_names_and_value_types.empty());

    try
    {
        // open the database
        if( sqlite3_open(ToUtf8(filename), &m_db) != SQLITE_OK )
            throw SQLiteStatementException(_T("SQLite: Could not open: %s"), filename.c_str());

        // create each table (as necessary) and prepare the SQL statements
        for( const auto& [table_name, value_type] : table_names_and_value_types )
            CreateTableIfNotExists(table_name, value_type);

        ASSERT(!m_tableDetails.empty());

        m_dbFilename = std::move(filename);
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


SimpleDbMap::TableDetails* SimpleDbMap::CreateTableIfNotExists(std::wstring table_name, ValueType value_type)
{
    CString create_table_sql = FormatText(SqlStatements::CreateTableFormatter, table_name.c_str(),
                                          ( value_type == ValueType::String ) ? _T("TEXT") : _T("INTEGER"));

    if( sqlite3_exec(m_db, ToUtf8(create_table_sql), nullptr, nullptr, nullptr) != SQLITE_OK )
        throw SQLiteStatementException(_T("SQLite: Could not create table: %s"), table_name.c_str());

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

    if( m_transactions > 0 || sqlite3_exec(m_db, SqlStatements::BeginTransaction, nullptr, nullptr, nullptr) == SQLITE_OK )
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

    bool success = ( sqlite3_exec(m_db, SqlStatements::EndTransaction, nullptr, nullptr, nullptr) == SQLITE_OK );
    m_transactions = 0;

    return success;
}


bool SimpleDbMap::Exists(wstring_view key)
{
    ASSERT(m_db != nullptr && m_currentTable != nullptr);

    SQLiteStatement& stmt = m_currentTable->stmt_exists;
    SQLiteResetOnDestruction rod(stmt);

    stmt.Bind(1, key);

    return ( stmt.Step() == SQLITE_ROW );
}


bool SimpleDbMap::Clear()
{
    ASSERT(m_db != nullptr && m_currentTable != nullptr);

    if( WrapInTransaction() )
    {
        SQLiteStatement& stmt = m_currentTable->stmt_clear;
        SQLiteResetOnDestruction rod(stmt);

        return ( stmt.Step() == SQLITE_DONE );
    }

    return false;
}


bool SimpleDbMap::Delete(wstring_view key)
{
    ASSERT(m_db != nullptr && m_currentTable != nullptr);

    if( WrapInTransaction() )
    {
        SQLiteStatement& stmt = m_currentTable->stmt_delete;
        SQLiteResetOnDestruction rod(stmt);

        stmt.Bind(1, key);

        return ( stmt.Step() == SQLITE_DONE );
    }

    return false;
}


template<typename T>
bool SimpleDbMap::Put(wstring_view key, T value)
{
    if( WrapInTransaction() )
    {
        SQLiteStatement& stmt = m_currentTable->stmt_put;
        SQLiteResetOnDestruction rod(stmt);

        stmt.Bind(1, key);
        stmt.Bind(2, value);

        return ( stmt.Step() == SQLITE_DONE );
    }

    return false;
}


bool SimpleDbMap::PutString(wstring_view key, wstring_view value)
{
    ASSERT(m_db != nullptr && m_currentTable != nullptr && m_currentTable->value_type == ValueType::String);
    return Put(key, value);
}


bool SimpleDbMap::PutLong(wstring_view key, long value)
{
    ASSERT(m_db != nullptr && m_currentTable != nullptr && m_currentTable->value_type == ValueType::Long);
    return Put(key, value);
}


template<typename T> std::optional<T> SimpleDbMap::Get(wstring_view key)
{
    SQLiteStatement& stmt = m_currentTable->stmt_get;
    SQLiteResetOnDestruction rod(stmt);

    stmt.Bind(1, key);

    if( stmt.Step() == SQLITE_ROW  )
        return stmt.GetColumn<T>(0);

    return std::nullopt;
}


std::optional<std::wstring> SimpleDbMap::GetString(wstring_view key)
{
    ASSERT(m_db != nullptr && m_currentTable != nullptr && m_currentTable->value_type == ValueType::String);
    return Get<std::wstring>(key);
}


std::optional<long> SimpleDbMap::GetLong(wstring_view key)
{
    ASSERT(m_db != nullptr && m_currentTable != nullptr && m_currentTable->value_type == ValueType::Long);
    return Get<long>(key);
}


std::optional<long> SimpleDbMap::GetLongUsingKeyPrefix(CString key_prefix) 
{
    ASSERT(m_db != nullptr && m_currentTable != nullptr && m_currentTable->value_type == ValueType::Long);

    // make sure that all values can be escaped properly (starting above LIKE's % and _ escapes)
    TCHAR escape_ch = GetUnusedCharacter(key_prefix, _T('a'));

    // create the prepared statement if this is the first call to this method, or if
    // the escape character is different from the previously executed statement
    if( m_currentTable->escape_char_and_stmt_get_using_key_prefix == nullptr ||
        std::get<0>(*m_currentTable->escape_char_and_stmt_get_using_key_prefix) != escape_ch )
    {
        try
        {
            CString sql = FormatText(SqlStatements::GetUsingKeyPrefixFormatter, m_currentTable->table_name.c_str(), escape_ch);
            m_currentTable->escape_char_and_stmt_get_using_key_prefix = std::make_unique<std::tuple<TCHAR, SQLiteStatement>>(
                escape_ch, SQLiteStatement(m_db, sql, true));
        }

        catch( const SQLiteStatementException& )
        {
            ASSERT(false);
            return std::nullopt;
        }
    }

    // escape any % and _ characters
    const TCHAR EscapeCharacters[] = { _T('%'), _T('_') };

    for( int i = 0; i < _countof(EscapeCharacters); ++i )
    {
        int escape_pos = 0;

        while( ( escape_pos = key_prefix.Find(EscapeCharacters[i], escape_pos) ) >= 0 )
        {
            key_prefix.Insert(escape_pos, escape_ch);
            escape_pos += 2;
        }
    }

    // add the % for the LIKE operation
    key_prefix.Append(_T("%"));

    SQLiteStatement& stmt = std::get<1>(*m_currentTable->escape_char_and_stmt_get_using_key_prefix);
    SQLiteResetOnDestruction rod(stmt);

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
bool SimpleDbMap::Next(std::wstring* key, T* value)
{
    SQLiteStatement& stmt = m_currentTable->stmt_iterator;

    if( stmt.Step() == SQLITE_ROW )
    {
        *key = stmt.GetColumn<std::wstring>(0);
        *value = stmt.GetColumn<T>(1);
        return true;
    }

    return false;
}


bool SimpleDbMap::NextString(std::wstring* key, std::wstring* value)
{
    ASSERT(m_db != nullptr && m_currentTable != nullptr && m_currentTable->value_type == ValueType::String);
    return Next(key, value);
}


bool SimpleDbMap::NextLong(std::wstring* key, long* value)
{
    ASSERT(m_db != nullptr && m_currentTable != nullptr && m_currentTable->value_type == ValueType::Long);
    return Next(key, value);
}
