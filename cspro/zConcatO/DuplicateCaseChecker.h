#pragma once

struct sqlite3;
class SQLiteStatement;


// A class that creates a temporary database of keys to check for duplicates.

class DuplicateCaseChecker
{
public:
    DuplicateCaseChecker();
    ~DuplicateCaseChecker();

    void Add(wstring_view key_sv, size_t file_number);

    std::optional<size_t> LookupDuplicate(wstring_view key_sv);

private:
    sqlite3* m_db;
    std::unique_ptr<SQLiteStatement> m_stmtInsert;
    std::unique_ptr<SQLiteStatement> m_stmtFind;
};
