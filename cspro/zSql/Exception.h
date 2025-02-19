#pragma once

#include <zSql/Definitions.h>
#include <zToolsO/CSProException.h>


class Sqlite::Exception : public CSProException
{
public:
    Exception();

    template<typename... Args>
    explicit Exception(const char* formatter, Args const&... args);

    // When db is non-null, the error message retrieved using sqlite3_errmsg will be appended to the error message.
    Exception(sqlite3* db);
    Exception(sqlite3* db, std::string error_prefix);
    Exception(sqlite3* db, const std::string& file_path, std::string error_prefix);

    template<typename... Args>
    explicit Exception(sqlite3* db, const std::string& file_path, const char* formatter, Args const&... args);

private:
    static std::string GetErrorMessage(sqlite3* db, const std::string* file_path, std::string error_prefix);
};



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

template<typename... Args>
Sqlite::Exception::Exception(const char* const formatter, Args const&... args)
    :   CSProException(FormatText(formatter, args...))
{
}


template<typename... Args>
Sqlite::Exception::Exception(sqlite3* const db, const std::string& file_path, const char* const formatter, Args const&... args)
    :   CSProException(GetErrorMessage(db, &file_path, FormatText(formatter, args...)))
{
}
