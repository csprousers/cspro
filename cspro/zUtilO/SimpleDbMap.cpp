#include "StdAfx.h"
#include "SimpleDbMap.h"
#include <zSql/DB.h>


namespace Constants
{
    constexpr size_t MaxNumberSqlInsertsInOneTransaction = 25000;
}

namespace Sqlite::Commands
{
    constexpr const char* CreateTableFormatter =
        "CREATE TABLE IF NOT EXISTS `%s` "
        "(`Key` TEXT PRIMARY KEY UNIQUE NOT NULL, `Value` %s NOT NULL) WITHOUT ROWID;";

    constexpr const char* PutFormatter =
        "INSERT OR REPLACE INTO `%s` "
        "(`Key`, `Value`) "
        "VALUES (?, ?);";

    constexpr const char* ClearFormatter =
        "DELETE FROM `%s`;";

    constexpr const char* DeleteFormatter =
        "DELETE FROM `%s` WHERE `Key` = ?;";

    constexpr const char* ExistsFormatter =
        "SELECT 1 FROM `%s` WHERE `Key` = ? LIMIT 1;";

    constexpr const char* GetFormatter =
        "SELECT `Value` FROM `%s` WHERE `Key` = ?;";

    constexpr const char* GetUsingKeyPrefixFormatter =
        "SELECT `Value` FROM `%s` WHERE `Key` LIKE ? ESCAPE '%c';";

    constexpr const char* IteratorFormatter =
        "SELECT `Key`, `Value` FROM `%s`;";
}



// --------------------------------------------------------------------------
// SimpleDbMap::TableDetails
// --------------------------------------------------------------------------

struct SimpleDbMap::TableDetails
{
    std::string table_name;
    ValueType value_type;

    Sqlite::Statement stmt_put;
    Sqlite::Statement stmt_clear;
    Sqlite::Statement stmt_delete;
    Sqlite::Statement stmt_exists;
    Sqlite::Statement stmt_get;
    Sqlite::Statement stmt_iterator;
    std::unique_ptr<std::tuple<char, Sqlite::Statement>> escape_char_and_stmt_get_using_key_prefix;

    TableDetails(Sqlite::DB& db, std::string table_name_, ValueType value_type_)
        :   table_name(std::move(table_name_)),
            value_type(value_type_),
            stmt_put(db.PrepareStatement(FormatText(Sqlite::Commands::PutFormatter, table_name.c_str()))),
            stmt_clear(db.PrepareStatement(FormatText(Sqlite::Commands::ClearFormatter, table_name.c_str()))),
            stmt_delete(db.PrepareStatement(FormatText(Sqlite::Commands::DeleteFormatter, table_name.c_str()))),
            stmt_exists(db.PrepareStatement(FormatText(Sqlite::Commands::ExistsFormatter, table_name.c_str()))),
            stmt_get(db.PrepareStatement(FormatText(Sqlite::Commands::GetFormatter, table_name.c_str()))),
            stmt_iterator(db.PrepareStatement(FormatText(Sqlite::Commands::IteratorFormatter, table_name.c_str())))
    {
    }
};



// --------------------------------------------------------------------------
// SimpleDbMap
// --------------------------------------------------------------------------

SimpleDbMap::SimpleDbMap() noexcept
    :   m_currentTable(nullptr)
{
}


SimpleDbMap::~SimpleDbMap() noexcept
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
        m_db = std::make_unique<Sqlite::DB>(
            std::move(file_path),
            Sqlite::OpenFlags::ReadWrite | Sqlite::OpenFlags::Create
        );

        // create each table (as necessary) and prepare the SQL statements
        for( const auto& [table_name, value_type] : table_names_and_value_types )
            CreateTableIfNotExists(table_name, value_type);

        ASSERT(!m_tableDetails.empty());
        m_currentTable = m_tableDetails.front().get();

        TransactionManager::Register(*this);

        return true;
    }

    catch(...)
    {
        Close();

        if( throw_exceptions )
            throw;

        return false;
    }
}


const std::string& SimpleDbMap::GetCurrentTableName() const noexcept
{
    ASSERT(m_db != nullptr && m_currentTable != nullptr);
    return m_currentTable->table_name;
}


void SimpleDbMap::CreateTableIfNotExists(std::string table_name, const ValueType value_type)
{
    ASSERT(m_db != nullptr);

    const std::string create_table_sql = FormatText(
        Sqlite::Commands::CreateTableFormatter,
        table_name.c_str(),
        ( value_type == ValueType::String ) ? "TEXT" : "INTEGER"
    );

    m_db->Execute(create_table_sql);

    m_tableDetails.emplace_back(std::make_unique<TableDetails>(*m_db, std::move(table_name), value_type));
}


void SimpleDbMap::SwitchTable(const cs::string_sz table_name, const cs::cref_optional<ValueType> value_type_if_creating)
{
    ASSERT(m_db != nullptr && m_currentTable != nullptr);

    // the table may already be in use
    if( SO::EqualsNoCase(m_currentTable->table_name, table_name.c_str()) )
        return;

    // the table may already have been created
    const auto& lookup = std::find_if(m_tableDetails.cbegin(), m_tableDetails.cend(),
        [&](const std::unique_ptr<TableDetails>& table_details)
        {
            return SO::EqualsNoCase(table_details->table_name, table_name.c_str());
        });

    if( lookup != m_tableDetails.cend() )
    {
        m_currentTable = lookup->get();
    }

    // otherwise create the table
    else if( value_type_if_creating.has_value() )
    {
        CreateTableIfNotExists(table_name.c_str(), *value_type_if_creating);
        m_currentTable = m_tableDetails.back().get();
    }

    else
    {
        ASSERT(false);
    }
}


void SimpleDbMap::Close() noexcept
{
    if( m_db == nullptr )
        return;

    try
    {
        CommitTransactions();
        ASSERT(m_transaction == nullptr);
        TransactionManager::Deregister(*this);

        m_tableDetails.clear();
        m_currentTable = nullptr;

        m_db.reset();
    }
    catch(...) { ASSERT(false); }
}


const std::string& SimpleDbMap::GetDbFilePath() const noexcept
{
    return ( m_db != nullptr ) ? m_db->GetFilePath() :
                                 SO::Empty_string;
}


void SimpleDbMap::EnsureTransactionInProgress()
{
    ASSERT(m_db != nullptr);

    if( m_transaction != nullptr )
    {
        m_transaction->CommitPeriodically();
    }

    else
    {
        m_transaction = std::make_unique<Sqlite::Transaction>(m_db->CreateTransaction());
        m_transaction->Begin(Constants::MaxNumberSqlInsertsInOneTransaction);
    }
}


bool SimpleDbMap::CommitTransactions()
{
    if( m_transaction != nullptr )
    {
        try
        {
            m_transaction->Commit();
            m_transaction.reset();
        }
        catch(...) { ASSERT(false); }
    }

    return true;
}


bool SimpleDbMap::Exists(const std::string& key) noexcept
{
    ASSERT(m_db != nullptr && m_currentTable != nullptr);

    try
    {
        Sqlite::Statement& stmt = m_currentTable->stmt_exists;
        stmt.Reset()
            .Bind(1, key);

        if( stmt.Step() == Sqlite::Result::Row )
            return true;
    }
    catch(...) { ASSERT(false); }

    return false;
}


bool SimpleDbMap::Clear() noexcept
{
    ASSERT(m_db != nullptr && m_currentTable != nullptr);

    try
    {
        EnsureTransactionInProgress();

        Sqlite::Statement& stmt = m_currentTable->stmt_clear;
        stmt.Reset();

        return ( stmt.Step() == Sqlite::Result::Done );
    }
    catch(...) { ASSERT(false); }

    return false;
}


bool SimpleDbMap::Delete(const std::string& key) noexcept
{
    ASSERT(m_db != nullptr && m_currentTable != nullptr);

    try
    {
        EnsureTransactionInProgress();

        Sqlite::Statement& stmt = m_currentTable->stmt_delete;
        stmt.Reset()
            .Bind(1, key);

        return ( stmt.Step() == Sqlite::Result::Done );
    }
    catch(...) { ASSERT(false); }

    return false;
}


template<typename T>
bool SimpleDbMap::Put(const std::string& key, const T& value) noexcept
{
    try
    {
        EnsureTransactionInProgress();

        Sqlite::Statement& stmt = m_currentTable->stmt_put;
        stmt.Reset()
            .Bind(1, key)
            .Bind(2, value);

        return ( stmt.Step() == Sqlite::Result::Done );
    }
    catch(...) { ASSERT(false); }

    return false;
}


bool SimpleDbMap::PutString(const std::string& key, const std::string& value) noexcept
{
    ASSERT(m_db != nullptr && m_currentTable != nullptr && m_currentTable->value_type == ValueType::String);
    return Put(key, value);
}


bool SimpleDbMap::PutLong(const std::string& key, const long value) noexcept
{
    ASSERT(m_db != nullptr && m_currentTable != nullptr && m_currentTable->value_type == ValueType::Long);
    return Put(key, value);
}


template<typename T>
std::optional<T> SimpleDbMap::Get(const std::string& key) noexcept
{
    ASSERT(m_db != nullptr && m_currentTable != nullptr);

    try
    {
        Sqlite::Statement& stmt = m_currentTable->stmt_get;
        stmt.Reset()
            .Bind(1, key);

        if( stmt.Step() == Sqlite::Result::Row  )
            return stmt.GetColumn<T>(0);
    }
    catch(...) { ASSERT(false); }

    return std::nullopt;
}


std::optional<std::string> SimpleDbMap::GetString(const std::string& key) noexcept
{
    ASSERT(m_db != nullptr && m_currentTable != nullptr && m_currentTable->value_type == ValueType::String);
    return Get<std::string>(key);
}


std::optional<long> SimpleDbMap::GetLong(const std::string& key) noexcept
{
    ASSERT(m_db != nullptr && m_currentTable != nullptr && m_currentTable->value_type == ValueType::Long);
    return Get<long>(key);
}


std::optional<long> SimpleDbMap::GetLongUsingKeyPrefix(std::string key_prefix) noexcept
{
    try
    {
        ASSERT(m_db != nullptr && m_currentTable != nullptr && m_currentTable->value_type == ValueType::Long);

        // make sure that all values can be escaped properly (starting above LIKE's % and _ escapes)
        const char escape_ch = GetUnusedCharacter(key_prefix, 'a');

        // create the prepared statement if this is the first call to this method, or if
        // the escape character is different from the previously executed statement
        if( m_currentTable->escape_char_and_stmt_get_using_key_prefix == nullptr ||
            std::get<0>(*m_currentTable->escape_char_and_stmt_get_using_key_prefix) != escape_ch )
        {
            const std::string sql = FormatText(Sqlite::Commands::GetUsingKeyPrefixFormatter, m_currentTable->table_name.c_str(), escape_ch);

            m_currentTable->escape_char_and_stmt_get_using_key_prefix =
                std::make_unique<std::tuple<char, Sqlite::Statement>>(escape_ch, m_db->PrepareStatement(sql));
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

        Sqlite::Statement& stmt = std::get<1>(*m_currentTable->escape_char_and_stmt_get_using_key_prefix);
        stmt.Reset()
            .Bind(1, key_prefix);

        if( stmt.Step() == Sqlite::Result::Row )
            return stmt.GetColumn<long>(0);
    }
    catch(...) { ASSERT(false); }

    return std::nullopt;
}


void SimpleDbMap::ResetIterator() noexcept
{
    ASSERT(m_db != nullptr && m_currentTable != nullptr);

    try
    {
        m_currentTable->stmt_iterator.Reset();
    }
    catch(...) { ASSERT(false); }
}


template<typename T>
bool SimpleDbMap::Next(std::string& key, T& value) noexcept
{
    ASSERT(m_db != nullptr && m_currentTable != nullptr);

    try
    {
        Sqlite::Statement& stmt = m_currentTable->stmt_iterator;

        if( stmt.Step() == Sqlite::Result::Row )
        {
            key = stmt.GetColumn<std::string>(0);
            value = stmt.GetColumn<T>(1);
            return true;
        }
    }
    catch(...) { ASSERT(false); }

    return false;
}


bool SimpleDbMap::NextString(std::string& key, std::string& value) noexcept
{
    ASSERT(m_db != nullptr && m_currentTable != nullptr && m_currentTable->value_type == ValueType::String);
    return Next(key, value);
}


bool SimpleDbMap::NextLong(std::string&  key, long& value) noexcept
{
    ASSERT(m_db != nullptr && m_currentTable != nullptr && m_currentTable->value_type == ValueType::Long);
    return Next(key, value);
}
