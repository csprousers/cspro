#pragma once

#include <zSql/zSql.h>

struct sqlite3;


#ifndef ZSQL_EXPORTS

struct sqlite3_stmt;
SQLITE_API int sqlite3_finalize(sqlite3_stmt* pStmt);

inline void safe_sqlite3_finalize(sqlite3_stmt*& pStmt)
{
    sqlite3_finalize(pStmt);
    pStmt = nullptr;
}

#endif


class ZSQL_API SQLiteHelpers
{
public:
    static void SetTemporaryKeyValuePair(sqlite3* db, const char* key, const char* value) noexcept;
    static bool TemporaryKeyValuePairExists(sqlite3* db, const char* key) noexcept;

    static std::vector<std::string> SplitSqlStatement(std::string_view sql_sv);

    static std::string GetTextPrefixBoundary(std::string text);

    // convert ' -> ''
    static std::string EscapeText(std::string text);
};
