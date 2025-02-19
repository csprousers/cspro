#pragma once

#include <zUtilO/zUtilO.h>
#include <zUtilO/TransactionManager.h>
#include <zSql/SQLite.h>
#include <zSql/SQLiteStatement.h>


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

    bool Open(std::string file_path,
              const std::vector<std::tuple<std::string, ValueType>>& table_names_and_value_types,
              bool throw_exceptions = false);

    virtual void Close();

    bool IsOpen() const { return ( m_db != nullptr ); }

    bool WrapInTransaction();
    bool CommitTransactions() override;

    virtual bool Clear();
    virtual bool Delete(const std::string& key);

    bool Exists(const std::string& key);

    virtual bool PutString(const std::string& key, const std::string& value);
    virtual std::optional<std::string> GetString(const std::string& key);

    bool PutLong(const std::string& key, long value);
    std::optional<long> GetLong(const std::string& key);
    std::optional<long> GetLongUsingKeyPrefix(std::string key_prefix);

    void ResetIterator();
    bool NextString(std::string* key, std::string* value);
    bool NextLong(std::string* key, long* value);

    const std::string& GetDbFilePath() const { return m_dbFilePath; }

protected:
    struct TableDetails
    {
        TableDetails(sqlite3* db, std::string table_name_, ValueType value_type_);

        const std::string table_name;
        const ValueType value_type;

        SQLiteStatement stmt_put;
        SQLiteStatement stmt_clear;
        SQLiteStatement stmt_delete;
        SQLiteStatement stmt_exists;
        SQLiteStatement stmt_get;
        SQLiteStatement stmt_iterator;
        std::unique_ptr<std::tuple<char, SQLiteStatement>> escape_char_and_stmt_get_using_key_prefix;
    };

    TableDetails* CreateTableIfNotExists(std::string table_name, ValueType value_type);

private:
    template<typename T>
    bool Put(const std::string& key, const T& value);

    template<typename T>
    std::optional<T> Get(const std::string& key);

    template<typename T>
    bool Next(std::string* key, T* value);

protected:
    std::string m_dbFilePath;
    sqlite3* m_db;

    std::vector<std::unique_ptr<TableDetails>> m_tableDetails;
    TableDetails* m_currentTable;

private:
    size_t m_transactions;
};
