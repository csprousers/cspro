#pragma once

#include <zSql/zSql.h>
#include <string>
#include <vector>

struct sqlite3;


#ifndef ZSQL_EXPORTS

inline void safe_sqlite3_finalize(sqlite3_stmt*& pStatement)
{
    if( pStatement != nullptr )
    {
        sqlite3_finalize(pStatement);
        pStatement = nullptr;
    }
}


inline std::string sqlite3_column_string(sqlite3_stmt* stmt, const int i)
{
    const char* text = reinterpret_cast<const char*>(sqlite3_column_text(stmt, i));
    return ( text != nullptr ) ? std::string(text) :
                                 std::string();
}

#endif


class ZSQL_API SQLiteHelpers
{
public:
    static void SetTemporaryKeyValuePair(sqlite3* db, const char* key, const char* value);
    static bool TemporaryKeyValuePairExists(sqlite3* db, const char* key);

    static std::vector<std::string> SplitSqlStatement(std::string_view sql_sv);

    static std::string GetTextPrefixBoundary(std::string text);

    // convert ' -> ''
    static std::string EscapeText(std::string text);
};



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

inline std::string SQLiteHelpers::EscapeText(std::string text)
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
