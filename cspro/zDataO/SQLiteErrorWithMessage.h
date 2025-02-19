#pragma once

#include <zDataO/DataRepositoryException.h>


// Exception class that uses current message from SQLite database
class SQLiteErrorWithMessage : public DataRepositoryException::SQLiteError
{
public:
    SQLiteErrorWithMessage(sqlite3* pDB)
        :   DataRepositoryException::SQLiteError(sqlite3_errmsg(pDB))
    {
    }

    SQLiteErrorWithMessage(sqlite3* pDB, const char* prefix)
        :   DataRepositoryException::SQLiteError(FormatText("%s%s", prefix, sqlite3_errmsg(pDB)).c_str())
    {
    }
};
