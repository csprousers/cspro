#include "stdafx.h"
#include "Exception.h"


Sqlite::Exception::Exception()
    :   CSProException("There was an error communicating with the SQLite database.")
{
}


Sqlite::Exception::Exception(sqlite3* const db)
    :   CSProException(sqlite3_errmsg(db))
{
    ASSERT(db != nullptr);
}


Sqlite::Exception::Exception(sqlite3* const db, std::string error_prefix)
    :   CSProException(GetErrorMessage(db, nullptr, std::move(error_prefix)))
{
}


Sqlite::Exception::Exception(sqlite3* const db, const std::string& file_path, std::string error_prefix)
    :   CSProException(GetErrorMessage(db, &file_path, std::move(error_prefix)))
{
}


std::string Sqlite::Exception::GetErrorMessage(sqlite3* const db, const std::string* const file_path, std::string error_prefix)
{
    ASSERT(!error_prefix.empty());

    if( file_path != nullptr )
    {
        error_prefix.append(" (using '")
                    .append(Path::GetFilename(*file_path))
                    .append("')");
    }

    if( db != nullptr )
    {
        error_prefix.append(": ")
                    .append(sqlite3_errmsg(db));
    }

    return error_prefix;
}
