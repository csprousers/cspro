#include "stdafx.h"
#include "DuplicateCaseChecker.h"
#include <SQLite/SQLiteStatement.h>


DuplicateCaseChecker::DuplicateCaseChecker()
{
    // Passing "" as db name creates a temporary database that is automatically
    // cleaned up when connection is closed. SQLite will create the db in
    // memory although it may flush certain parts to temp files on disk
    // if needed.
    sqlite3_open("", &m_db);

    // Disable synchronous writes and journaling to get max performance
    // of writes. Not needed in this case since we don't have multiple
    // users and are not using transactions.
    sqlite3_exec(m_db, "PRAGMA synchronous = OFF", nullptr, nullptr, nullptr);
    sqlite3_exec(m_db, "PRAGMA journal_mode = OFF", nullptr, nullptr, nullptr);

    sqlite3_exec(m_db, "CREATE TABLE `cases` ( `key` TEXT PRIMARY KEY NOT NULL, `file_number` INTEGER NOT NULL );", nullptr, nullptr, nullptr);

    try
    {
        m_stmtInsert = std::make_unique<SQLiteStatement>(m_db, "INSERT INTO `cases` ( `key`, `file_number` ) VALUES(?, ?);", true);
        m_stmtFind = std::make_unique<SQLiteStatement>(m_db, "SELECT `file_number` FROM `cases` WHERE `key` = ? LIMIT 1;", true);
    }

    catch(...)
    {
        sqlite3_close(m_db);
        throw;
    }
}


DuplicateCaseChecker::~DuplicateCaseChecker()
{
    m_stmtFind.reset();
    m_stmtInsert.reset();
    sqlite3_close(m_db);
}


void DuplicateCaseChecker::Add(const wstring_view key_sv, const size_t file_number)
{
    m_stmtInsert->Reset()
                 .Bind(1, key_sv)
                 .Bind(2, file_number)
                 .Step();
}


std::optional<size_t> DuplicateCaseChecker::LookupDuplicate(const wstring_view key_sv)
{
    m_stmtFind->Reset()
               .Bind(1, key_sv);

    if( m_stmtFind->Step() == SQLITE_ROW )
        return m_stmtFind->GetColumn<size_t>(0);

    return std::nullopt;
}
