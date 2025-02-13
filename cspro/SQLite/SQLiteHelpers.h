#pragma once

#include <SQLite/sqlite_dll.h>
#include <string>
#include <vector>

struct sqlite3;


#ifndef SQLITE_EXPORTS

#include <zToolsO/Utf8Convert.h>

// even if this is "preprocessor abuse," do not change the following functions to inline functions as they will not work properly
// because the UTF8Convert functions' return values will not be accessible when the inline function returns
#define ToUtf8(text)     ( UTF8Convert::WideToUTF8(text).c_str() )
#define FromUtf8(text)   ( UTF8Convert::UTF8ToWide<CString>(reinterpret_cast<const char*>(text)) )
#define FromUtf8WS(text) ( UTF8Convert::UTF8ToWide(reinterpret_cast<const char*>(text)) )

inline void safe_sqlite3_finalize(sqlite3_stmt*& pStatement)
{
    if( pStatement != nullptr )
    {
        sqlite3_finalize(pStatement);
        pStatement = nullptr;
    }
}

#endif


namespace SQLiteHelpers
{
    SQLITE_API void SetTemporaryKeyValuePair(sqlite3* db,const char* lpszKey,const char* lpszValue);
    SQLITE_API bool TemporaryKeyValuePairExists(sqlite3* db,const char* lpszKey);

    SQLITE_API std::vector<std::string> SplitSqlStatement(const std::string& sSql);

    SQLITE_API std::string GetTextPrefixBoundary(std::string text);

#ifndef SQLITE_EXPORTS
    inline std::wstring EscapeText(std::wstring text)
    {
        SO::Replace(text, _T("'"), _T("''"));
        return text;
    }
#endif
}


namespace SqlStatements
{
    constexpr const char* BeginTransaction = "BEGIN;";
    constexpr const char* EndTransaction   = "COMMIT;";
}
