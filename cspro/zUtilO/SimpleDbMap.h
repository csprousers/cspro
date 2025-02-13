#pragma once

#include <zUtilO/zUtilO.h>
#include <zUtilO/TransactionManager.h>
#include <SQLite/SQLite.h>
#include <SQLite/SQLiteStatement.h>


// for storing key-value pairs in a database that can be used across CSPro applications;
// look at SettingsDb and WinRegistry for similar functionality

class CLASS_DECL_ZUTILO SimpleDbMap : public TransactionGenerator
{
public:
    enum class ValueType { String, Long };

    SimpleDbMap();
    SimpleDbMap(const SimpleDbMap&) = delete;
    virtual ~SimpleDbMap();

    SimpleDbMap& operator=(const SimpleDbMap&) = delete;

    bool Open(std::wstring filename,
              const std::vector<std::tuple<std::wstring, ValueType>>& table_names_and_value_types,
              bool throw_exceptions = false);

    virtual void Close();

    bool IsOpen() const { return ( m_db != nullptr ); }

    bool WrapInTransaction();
    bool CommitTransactions() override;

    virtual bool Clear();
    virtual bool Delete(wstring_view key);

    bool Exists(wstring_view key);

    virtual bool PutString(wstring_view key, wstring_view value);
    virtual std::optional<std::wstring> GetString(wstring_view key);

    bool PutLong(wstring_view key, long value);
    std::optional<long> GetLong(wstring_view key);
    std::optional<long> GetLongUsingKeyPrefix(CString key_prefix);

    void ResetIterator();
    bool NextString(std::wstring* key, std::wstring* value);
    bool NextLong(std::wstring* key, long* value);

    const std::wstring& GetDbFilename() const { return m_dbFilename; }

protected:
    struct TableDetails
    {
        TableDetails(sqlite3* db, std::wstring table_name_, ValueType value_type_);

        std::wstring table_name;
        ValueType value_type;

        SQLiteStatement stmt_put;
        SQLiteStatement stmt_clear;
        SQLiteStatement stmt_delete;
        SQLiteStatement stmt_exists;
        SQLiteStatement stmt_get;
        SQLiteStatement stmt_iterator;
        std::unique_ptr<std::tuple<TCHAR, SQLiteStatement>> escape_char_and_stmt_get_using_key_prefix;
    };

    TableDetails* CreateTableIfNotExists(std::wstring table_name, ValueType value_type);

private:
    template<typename T>
    bool Put(wstring_view key, T value);

    template<typename T>
    std::optional<T> Get(wstring_view key);

    template<typename T>
    bool Next(std::wstring* key, T* value);

protected:
    std::wstring m_dbFilename;
    sqlite3* m_db;

    std::vector<std::unique_ptr<TableDetails>> m_tableDetails;
    TableDetails* m_currentTable;

private:
    unsigned m_transactions;
};
