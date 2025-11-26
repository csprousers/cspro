#pragma once

#include <zUtilO/zUtilO.h>
#include <zUtilO/TransactionManager.h>

namespace Sqlite { class DB; class Transaction; }


// --------------------------------------------------------------------------
// SimpleDbMap
//
// For storing key-value pairs in a database that can be used across CSPro
// applications.
//
// Other than Open, which can conditionally throw exceptions, the public
// methods in this class do not throw exceptions.
//
// Look at SettingsDb and WinRegistry for similar functionality.
// --------------------------------------------------------------------------

class CLASS_DECL_ZUTILO SimpleDbMap : public TransactionGenerator
{
public:
    enum class ValueType { String, Long };

    SimpleDbMap() noexcept;
    SimpleDbMap(const SimpleDbMap&) = delete;
    virtual ~SimpleDbMap() noexcept;

    SimpleDbMap& operator=(const SimpleDbMap&) = delete;

    bool Open(std::string file_path,
              const std::vector<std::tuple<std::string, ValueType>>& table_names_and_value_types,
              bool throw_exceptions = false);

    virtual void Close() noexcept;

    bool IsOpen() const noexcept { return ( m_db != nullptr ); }

    const std::string& GetDbFilePath() const noexcept;

    virtual bool Clear() noexcept;
    virtual bool Delete(const std::string& key) noexcept;

    bool Exists(const std::string& key) noexcept;

    virtual bool PutString(const std::string& key, const std::string& value) noexcept;
    virtual std::optional<std::string> GetString(const std::string& key) noexcept;

    bool PutLong(const std::string& key, long value) noexcept;
    std::optional<long> GetLong(const std::string& key) noexcept;
    std::optional<long> GetLongUsingKeyPrefix(std::string key_prefix) noexcept;

    void ResetIterator() noexcept;
    bool NextString(std::string& key, std::string& value) noexcept;
    bool NextLong(std::string& key, long& value) noexcept;

    // TransactionGenerator override
    bool CommitTransactions() override;

protected:
    Sqlite::DB& GetDb() const noexcept { ASSERT(m_db != nullptr); return *m_db; }

    const std::string& GetCurrentTableName() const noexcept;

    void CreateTableIfNotExists(std::string table_name, ValueType value_type);

    // Switches to the specified table, creating it as necessary (when value_type_if_creating is defined).
    void SwitchTable(cs::string_sz table_name, cs::cref_optional<ValueType> value_type_if_creating);

private:
    // If a transaction has been started, it is periodically committed.
    // Otherwise, a new transaction is started.
    void EnsureTransactionInProgress();

    template<typename T>
    bool Put(const std::string& key, const T& value) noexcept;

    template<typename T>
    std::optional<T> Get(const std::string& key) noexcept;

    template<typename T>
    bool Next(std::string& key, T& value) noexcept;

private:
    std::unique_ptr<Sqlite::DB> m_db;

    struct TableDetails;
    std::vector<std::unique_ptr<TableDetails>> m_tableDetails;
    TableDetails* m_currentTable;

    std::unique_ptr<Sqlite::Transaction> m_transaction;
};
