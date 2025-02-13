#pragma once

#include <zDataO/DataRepositoryException.h>
#include <SQLite/SQLite.h>


// Exception class that uses current message from SQLite database
struct SQLiteErrorWithMessage : public DataRepositoryException::SQLiteError
{
    SQLiteErrorWithMessage(sqlite3* pDB)
        :   DataRepositoryException::SQLiteError(UTF8Convert::UTF8ToWide(sqlite3_errmsg(pDB)).c_str())
    {
    }

    SQLiteErrorWithMessage(sqlite3* pDB, const TCHAR* prefix)
        :   DataRepositoryException::SQLiteError(( prefix + UTF8Convert::UTF8ToWide(sqlite3_errmsg(pDB)) ).c_str())
    {
    }
};
