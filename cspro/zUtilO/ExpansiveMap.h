#pragma once

#include <zUtilO/zUtilO.h>
#include <zSql/DataStorage.h>
#include <zSql/SQLiteStatement.h>

class ExpansiveMap_SqliteDb;


// --------------------------------------------------------------------------
// ExpansiveMap
//
// This class allows for the storage of a large number of values to use as
// a lookup. Values are initially stored in a std::map, but after the map is
// "full," it will use an in-memory SQLite database to store values.
//
// Complex keys can be bound by specializing ExpansiveMap_KeyBinder.
//
// A related class is ExpansiveList.
// --------------------------------------------------------------------------

template<typename Key, typename Value>
class ExpansiveMap
{
public:
    static constexpr size_t MapSize = 200000;

    // If providing a database, it is assumed that the database is open, does not have a table named "ex_map,"
    // and that the owner of this object will close the database.
    ExpansiveMap(sqlite3* db = nullptr, size_t map_size = MapSize);
    ~ExpansiveMap();

    // If enabled, when the std::map is full, its contents will be written to the SQLite database
    // in a single transaction, leaving the std::map empty. This setting can be used when the most
    // recent insertions are going to be the most likely values queried. If enabled and using this
    // map with a shared database, you must ensure that all prepared statements are always reset or
    // finalized before doing any insertions, otherwise the dropping of the index will result in
    // a SQLITE_LOCKED error. Because rebuilding the index may not make sense when many values have
    // already been added, a proportion can be specified above which the index will not be rebuilt.
    // For example, if 4 chunks of 10 entries are inserted with a specified proportion of 0.7,
    // the index would be rebuilt three times (for calculated proportions 0.0, 0.5, 0.67), and
    // would not be rebuilt starting with the fourth insertion chunk (0.75).
    void OptimizeForRecentInsertions(double rebuild_index_max_proportion = 1);

    // Insert adds the key/value pair only if the key does not already exist.
    // An exception can occur if there are problems interacting with the SQLite database.
    template<typename KeyT, typename ValueT>
    void Insert(KeyT&& key, ValueT&& value);

    // Returns true if the key exists.
    bool Exists(const Key& key) const;

    // Returns the value if the key exists, and null otherwise.
    cs::shared_or_raw_ptr<const Value> Find(const Key& key) const;

    // Clears the map.
    void Clear();

private:
    template<typename KeyT, typename ValueT>
    void InsertInSqlite(KeyT&& key, ValueT&& value);

    template<typename T>
    T FindWorker(const Key& key) const;

    void WriteMapToSqlite();

private:
    std::map<Key, Value> m_map;
    size_t m_mapSlotsRemaining;

    sqlite3* m_db;
    bool m_ownDb;
    std::optional<double> m_optimizeForRecentInsertionsMaxProportion;
    std::unique_ptr<ExpansiveMap_SqliteDb> m_sqliteDb;
};


// --------------------------------------------------------------------------
// ExpansiveMap_KeyBinder
// --------------------------------------------------------------------------

template<typename Key>
class ExpansiveMap_KeyBinder
{
public:
    // Returns the number of keys.
    static constexpr int GetNumberKeys();

    // Returns a list of the data types for each key column.
    static std::vector<const char*> GetDataTypes();

    // Binds the key values, starting at position 1.
    template<typename KeyT>
    static void Bind(SQLiteStatement& stmt, KeyT&& key);
};


// --------------------------------------------------------------------------
// ExpansiveMap_SqliteDb
// --------------------------------------------------------------------------

class CLASS_DECL_ZUTILO ExpansiveMap_SqliteDb
{
    template<typename Key, typename Value> friend class ExpansiveMap;

    static constexpr const char* ExceptionMessage = "Error with ExpansiveMap's in-memory database.";

private:
    ExpansiveMap_SqliteDb(std::string create_index_sql, SQLiteStatement stmt_insert, SQLiteStatement stmt_find);

    static std::unique_ptr<ExpansiveMap_SqliteDb> Create(sqlite3*& db, int number_keys, const std::vector<const char*>& key_data_types,
                                                         const char* value_data_type);

    static void DoExec(sqlite3* db, cs::string_sz sql);

    void DoBulkInsertions(sqlite3* db, double rebuild_index_max_proportion, size_t number_insertions, const std::function<void()>& callback_function);

private:
    std::string m_createIndexSql;
    SQLiteStatement m_stmtInsert;
    SQLiteStatement m_stmtFind;
    std::unique_ptr<SQLiteStatement> m_stmtCount;
};



// --------------------------------------------------------------------------
// ExpansiveMap inline implementations
// --------------------------------------------------------------------------

template<typename Key, typename Value>
ExpansiveMap<Key, Value>::ExpansiveMap(sqlite3* const db/* = nullptr*/, const size_t map_size/* = MapSize*/)
    :   m_mapSlotsRemaining(map_size),
        m_db(db),
        m_ownDb(m_db == nullptr)
{
    ASSERT(m_mapSlotsRemaining != 0);
}


template<typename Key, typename Value>
ExpansiveMap<Key, Value>::~ExpansiveMap()
{
    if( m_sqliteDb != nullptr )
    {
        m_sqliteDb.reset();

        if( m_ownDb )
            sqlite3_close(m_db);
    }
}


template<typename Key, typename Value>
void ExpansiveMap<Key, Value>::OptimizeForRecentInsertions(const double rebuild_index_max_proportion/* = 1*/)
{
    m_optimizeForRecentInsertionsMaxProportion = rebuild_index_max_proportion;
    ASSERT(m_optimizeForRecentInsertionsMaxProportion >= 0 && m_optimizeForRecentInsertionsMaxProportion <= 1);
}


template<typename Key, typename Value>
template<typename KeyT, typename ValueT>
void ExpansiveMap<Key, Value>::Insert(KeyT&& key, ValueT&& value)
{
    ASSERT(!Exists(key));

    if( m_mapSlotsRemaining == 0 )
    {
        if( m_sqliteDb == nullptr )
        {
            m_sqliteDb = ExpansiveMap_SqliteDb::Create(m_db, ExpansiveMap_KeyBinder<Key>::GetNumberKeys(),
                                                             ExpansiveMap_KeyBinder<Key>::GetDataTypes(),
                                                             Sqlite::GetDataType<Value>());
        }

        if( !m_optimizeForRecentInsertionsMaxProportion.has_value() )
        {
            InsertInSqlite(std::forward<KeyT>(key), std::forward<ValueT>(value));
            return;
        }

        WriteMapToSqlite();
        m_mapSlotsRemaining = m_map.size();
        m_map.clear();
    }

    ASSERT(m_mapSlotsRemaining != 0);

    m_map.try_emplace(std::forward<KeyT>(key), std::forward<ValueT>(value));
    --m_mapSlotsRemaining;
}


template<typename Key, typename Value>
template<typename KeyT, typename ValueT>
void ExpansiveMap<Key, Value>::InsertInSqlite(KeyT&& key, ValueT&& value)
{
    ASSERT(m_sqliteDb != nullptr);

    const SQLiteResetOnDestruction rod(m_sqliteDb->m_stmtInsert);

    ExpansiveMap_KeyBinder<Key>::Bind(m_sqliteDb->m_stmtInsert, std::forward<KeyT>(key));

    m_sqliteDb->m_stmtInsert.Bind(1 + ExpansiveMap_KeyBinder<Key>::GetNumberKeys(), std::forward<ValueT>(value));

    if( m_sqliteDb->m_stmtInsert.Step() != SQLITE_DONE )
        throw CSProException(ExpansiveMap_SqliteDb::ExceptionMessage);
}


template<typename Key, typename Value>
bool ExpansiveMap<Key, Value>::Exists(const Key& key) const
{
    return FindWorker<bool>(key);
}


template<typename Key, typename Value>
cs::shared_or_raw_ptr<const Value> ExpansiveMap<Key, Value>::Find(const Key& key) const
{
    return FindWorker<cs::shared_or_raw_ptr<const Value>>(key);
}


template<typename Key, typename Value>
template<typename T>
T ExpansiveMap<Key, Value>::FindWorker(const Key& key) const
{
    // look in the map
    const auto& map_lookup = m_map.find(key);

    if( map_lookup != m_map.cend() )
    {
        if constexpr(std::is_same_v<T, bool>)
        {
            return true;
        }

        else
        {
            return &map_lookup->second;
        }
    }

    // look in SQLite
    if( m_sqliteDb != nullptr )
    {
        const SQLiteResetOnDestruction rod(m_sqliteDb->m_stmtFind);

        ExpansiveMap_KeyBinder<Key>::Bind(m_sqliteDb->m_stmtFind, key);

        if( m_sqliteDb->m_stmtFind.Step() == SQLITE_ROW )
        {
            if constexpr(std::is_same_v<T, bool>)
            {
                return true;
            }

            else
            {
                return std::make_unique<Value>(m_sqliteDb->m_stmtFind.GetColumn<Value>(0));
            }
        }
    }

    if constexpr(std::is_same_v<T, bool>)
    {
        return false;
    }

    else
    {
        return nullptr;
    }
}


template<typename Key, typename Value>
void ExpansiveMap<Key, Value>::Clear()
{
    m_mapSlotsRemaining += m_map.size();
    m_map.clear();

    if( m_sqliteDb != nullptr )
        ExpansiveMap_SqliteDb::DoExec(m_db, "DELETE FROM `ex_map`;");
}


template<typename Key, typename Value>
void ExpansiveMap<Key, Value>::WriteMapToSqlite()
{
    ASSERT(!m_map.empty() && m_sqliteDb != nullptr && m_optimizeForRecentInsertionsMaxProportion.has_value());

    m_sqliteDb->DoBulkInsertions(m_db, *m_optimizeForRecentInsertionsMaxProportion, m_map.size(),
        [&]()
        {
            for( const auto& [key, value] : m_map )
                InsertInSqlite(key, value);
        });
}



// --------------------------------------------------------------------------
// ExpansiveMap_KeyBinder inline implementations
// --------------------------------------------------------------------------

template<typename Key>
constexpr int ExpansiveMap_KeyBinder<Key>::GetNumberKeys()
{
    return 1;
}


template<typename Key>
std::vector<const char*> ExpansiveMap_KeyBinder<Key>::GetDataTypes()
{
    return { Sqlite::GetDataType<Key>() };
}


template<typename Key>
template<typename KeyT>
void ExpansiveMap_KeyBinder<Key>::Bind(SQLiteStatement& stmt, KeyT&& key)
{
    stmt.Bind(1, std::forward<KeyT>(key));
}
