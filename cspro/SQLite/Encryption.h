#pragma once

#include <external/SQLite/sqlite3.h>


// --------------------------------------------------------------------------
// SqliteEncryption
//
// Because the SQLite Encryption Extension (SEE) is not open source, the
// functions in this namespace wrap SEE functionality. If called in a build
// that does not include SEE, the functions throw exceptions.
// --------------------------------------------------------------------------

namespace SqliteEncryption
{
    constexpr const char* NoSeeExceptionMessage = "SQLite encryption is not enabled in this CSPro build.";

    constexpr bool IsEnabled()
    {
#ifdef SQLITE_HAS_CODEC
        return true;
#else
        return false;
#endif
    }

    inline int sqlite3_key(sqlite3* db, const void* pKey, int nKey)
    {
#ifdef SQLITE_HAS_CODEC
        return ::sqlite3_key(db, pKey, nKey);
#else
        db;pKey;nKey;
        throw CSProException(NoSeeExceptionMessage);
#endif
    }

    inline int sqlite3_rekey(sqlite3* db, const void* pKey, int nKey)
    {
#ifdef SQLITE_HAS_CODEC
        return ::sqlite3_rekey(db, pKey, nKey);
#else
        db;pKey;nKey;
        throw CSProException(NoSeeExceptionMessage);
#endif
    }
}
