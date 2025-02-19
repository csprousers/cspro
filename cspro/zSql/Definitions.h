#pragma once

#include <zSql/zSql.h>

struct sqlite3;
struct sqlite3_stmt;


// --------------------------------------------------------------------------
// In addition to forward declarations of objects in the Sqlite namespace,
// this file forward declares:
//
//     - Relevant functions in SQLite's C API.
//
//     - Flags that are identical to the SQLite values so that it is not
//       necessary to include sqlite3.h. These values are checked in
//       Statement::CheckDefinitionsAtCompileTime().
// --------------------------------------------------------------------------

namespace Sqlite
{
    class DB;
    class Exception;
    class Statement;


    namespace Result
    {
        static constexpr int OK   =   0;
        static constexpr int Row  = 100;
        static constexpr int Done = 101;
    }


    namespace ColumnType
    {
        static constexpr int Integer = 1;
        static constexpr int Float   = 2;
        static constexpr int Text    = 3;
        static constexpr int Blob    = 4;
        static constexpr int Null    = 5;
    }


    namespace OpenFlags
    {
        static constexpr int ReadOnly  = 0x01;
        static constexpr int ReadWrite = 0x02;
        static constexpr int Create    = 0x04;
    }
}


extern "C"
{
#ifdef WIN32
    #define sqlite_int64_t int64_t
#else
    #define sqlite_int64_t long long int
#endif

    SQLITE_API const char* sqlite3_errmsg(sqlite3* db);

    SQLITE_API int sqlite3_bind_null(sqlite3_stmt* pStmt, int i);
    SQLITE_API int sqlite3_bind_int(sqlite3_stmt* pStmt, int i, int iValue);
    SQLITE_API int sqlite3_bind_int64(sqlite3_stmt* pStmt, int i, sqlite_int64_t iValue);
    SQLITE_API int sqlite3_bind_double(sqlite3_stmt* pStmt, int i, double rValue);
    SQLITE_API int sqlite3_bind_text(sqlite3_stmt* pStmt, int i, const char* zData, int nData, void (*xDel)(void*));

    SQLITE_API int sqlite3_step(sqlite3_stmt* pStmt);
    SQLITE_API int sqlite3_reset(sqlite3_stmt* pStmt);

    SQLITE_API int sqlite3_column_count(sqlite3_stmt* pStmt);
    SQLITE_API const char* sqlite3_column_name(sqlite3_stmt* pStmt, int N);
    SQLITE_API int sqlite3_column_type(sqlite3_stmt* pStmt, int i);

    SQLITE_API int sqlite3_column_int(sqlite3_stmt* pStmt, int iCol);
    SQLITE_API sqlite_int64_t sqlite3_column_int64(sqlite3_stmt* pStmt, int iCol);
    SQLITE_API double sqlite3_column_double(sqlite3_stmt* pStmt, int iCol);
    SQLITE_API const unsigned char* sqlite3_column_text(sqlite3_stmt* pStmt, int iCol);
    SQLITE_API int sqlite3_column_bytes(sqlite3_stmt* pStmt, int iCol);
    SQLITE_API const void *sqlite3_column_blob(sqlite3_stmt* pStmt, int iCol);

#undef sqlite_int64_t
}
