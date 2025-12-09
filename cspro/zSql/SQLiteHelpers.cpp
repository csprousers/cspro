#include "stdafx.h"
#include "SQLiteHelpers.h"
#include <zSql/Statement.h>


namespace
{
    constexpr const char* CreateTable = "CREATE TEMPORARY TABLE IF NOT EXISTS `cspro_sqlite_helpers` (`Key` TEXT PRIMARY KEY UNIQUE NOT NULL, `Value` TEXT NOT NULL) WITHOUT ROWID;";
    constexpr const char* InsertRow   = "INSERT OR REPLACE INTO `cspro_sqlite_helpers` (`Key`, `Value`) VALUES ( ?, ? );";
    constexpr const char* ExistsRow   = "SELECT 1 FROM `cspro_sqlite_helpers` WHERE `Key` = ? LIMIT 1;";
}


void SQLiteHelpers::SetTemporaryKeyValuePair(sqlite3* const db, const char* const key, const char* const value) noexcept
{
    if( sqlite3_exec(db, CreateTable, nullptr, nullptr, nullptr) == SQLITE_OK )
    {
        try
        {
            Sqlite::Statement stmt = Sqlite::Statement::Prepare(db, InsertRow);
            stmt.Bind(1, key)
                .Bind(2, value)
                .StepCheckResult(Sqlite::Result::Done);
        }
        catch(...) { ASSERT(false); }
    }
}


bool SQLiteHelpers::TemporaryKeyValuePairExists(sqlite3* const db, const char* const key) noexcept
{
    try
    {
        Sqlite::Statement stmt = Sqlite::Statement::Prepare(db, ExistsRow);
        stmt.Bind(1, key);

        if( stmt.Step() == Sqlite::Result::Row )
            return true;
    }
    catch(...) { }

    return false;
}


std::vector<std::string> SQLiteHelpers::SplitSqlStatement(const std::string_view sql_sv)
{
    constexpr char SingleQuoteChar = '\'';
    constexpr char DoubleQuoteChar = '"';
    constexpr char SemicolonChar = ';';

    std::vector<std::string> sql_statements;

    const char* itr = sql_sv.data();
    const char* const itr_end = itr + sql_sv.length();

    const char* statement_start = itr;
    std::optional<char> delimiter_ch;

    auto add_statement = [&](const size_t itr_offset)
    {
        ASSERT(statement_start < itr_end);
        ASSERT(itr <= itr_end);

        std::string this_sql(statement_start, itr - statement_start + itr_offset);

        // only add the string if it isn't blank
        for( const char ch : this_sql )
        {
            if( !std::isspace(ch) )
            {
                sql_statements.emplace_back(std::move(this_sql));
                return;
            }
        }
    };

    for( ; itr != itr_end; ++itr )
    {
        const char ch = *itr;

        if( ch == delimiter_ch )
        {
            if( ( itr + 1 ) != itr_end && itr[1] == *delimiter_ch )
            {
                // an escaped quote
            }

            else
            {
                delimiter_ch.reset(); // the end of the quote
            }
        }

        else if( ch == SemicolonChar && !delimiter_ch.has_value() )
        {
            add_statement(1);
            statement_start = itr + 1;
        }

        else if( ch == SingleQuoteChar ||
                 ch == DoubleQuoteChar )
        {
            delimiter_ch = ch;
        }
    }

    if( statement_start != itr_end )
        add_statement(0);

    return sql_statements;
}


std::string SQLiteHelpers::GetTextPrefixBoundary(std::string text)
{
    if( !text.empty() )
    {
        // instead of using LIKE ...%, we will do the following so as to not worry about having to escape characters
        char& last_ch = text.back();
        ++last_ch;

        // if the last character + 1 is 0, append a character so that checks for >= boundary && < boundary are still valid
        if( last_ch == 0 )
        {
            --last_ch;
            text.push_back(static_cast<char>(1));
        }
    }

    return text;
}


std::string SQLiteHelpers::EscapeText(std::string text)
{
    constexpr char QuoteChar = '\'';
    size_t ch_pos = 0;

    while( ( ch_pos = text.find(QuoteChar, ch_pos) ) != std::string::npos )
    {
        text.insert(ch_pos, 1, QuoteChar);
        ch_pos += 2;
    }

    return text;
}
