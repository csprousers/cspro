#include "StdAfx.h"
#include "SettingsDb.h"
#include <zToolsO/Hash.h>
#include <zToolsO/StringNoCase.h>
#include <SQLite/SQLite.h>
#include <SQLite/SQLiteHelpers.h>
#include <SQLite/SQLiteStatement.h>


// --------------------------------------------------------------------------
// SettingsDb::ImplCache
// 
// this class is stored as part of SettingsDb::ImplDb so that the cache is
// available when SettingsDb::ImplDb's destructor executes
// --------------------------------------------------------------------------

class SettingsDb::ImplCache
{
public:
    using CacheKey = std::tuple<const ImplDb*, const ImplTable*, std::wstring>;

    template<typename T>
    struct CacheValue
    {
        T value;
        std::optional<int64_t> expiry_timestamp;
        std::shared_ptr<KeyObfuscator> key_obfuscator;
        bool value_was_written;
    };

    template<typename T>
    std::map<CacheKey, CacheValue<T>>& GetCacheMap()
    {
             if constexpr(std::is_same_v<T, bool>)          return m_bool;
        else if constexpr(std::is_same_v<T, int>)           return m_int;
        else if constexpr(std::is_same_v<T, unsigned int>)  return m_uint;
        else if constexpr(std::is_same_v<T, int64_t>)       return m_int64_t;
        else if constexpr(std::is_same_v<T, size_t>)        return m_size_t;
        else if constexpr(std::is_same_v<T, float>)         return m_float;
        else if constexpr(std::is_same_v<T, double>)        return m_double;
        else if constexpr(std::is_same_v<T, std::wstring>)  return m_wstring;
        else                                                static_assert_false();
    }

private:
    std::map<CacheKey, CacheValue<bool>> m_bool;
    std::map<CacheKey, CacheValue<int>> m_int;
    std::map<CacheKey, CacheValue<unsigned int>> m_uint;
    std::map<CacheKey, CacheValue<int64_t>> m_int64_t;
    std::map<CacheKey, CacheValue<size_t>> m_size_t;
    std::map<CacheKey, CacheValue<float>> m_float;
    std::map<CacheKey, CacheValue<double>> m_double;
    std::map<CacheKey, CacheValue<std::wstring>> m_wstring;
};



// --------------------------------------------------------------------------
// SettingsDb::ImplTable
// --------------------------------------------------------------------------

struct SettingsDb::ImplTable
{
    std::wstring table_name_for_queries;
    SQLiteStatement stmt_read;
    SQLiteStatement stmt_write;
};



// --------------------------------------------------------------------------
// SettingsDb::ImplDb
// --------------------------------------------------------------------------

class SettingsDb::ImplDb
{
public:
    ImplDb(sqlite3* db);
    ~ImplDb();

    ImplTable* OpenTable(std::wstring table_name_for_queries);

    template<typename ReturnType, typename BaseValueType>
    std::optional<ReturnType> Read(SettingsDb& settings_db, wstring_view key, bool cache_value);

    template<typename T>
    void Write(SettingsDb& settings_db, wstring_view key, const T& value, bool cache_value);

private:
    template<typename T>
    void WriteToDb(ImplTable& table, wstring_view key, const T& value, const std::optional<int64_t>& expiry_timestamp,
                   const KeyObfuscator* key_obfuscator);

    void ClearOldValuesAndWriteCachedValues(ImplTable& table);

    static std::string GetDbKey(const KeyObfuscator* key_obfuscator, wstring_view key);

private:
    sqlite3* m_db;
    std::vector<std::unique_ptr<ImplTable>> m_tables;

    static std::unique_ptr<ImplCache> m_cache;
};


std::unique_ptr<SettingsDb::ImplCache> SettingsDb::ImplDb::m_cache;


SettingsDb::ImplDb::ImplDb(sqlite3* db)
    :   m_db(db)
{
    ASSERT(m_db != nullptr);

    if( m_cache == nullptr )
        m_cache = std::make_unique<ImplCache>();
}


SettingsDb::ImplDb::~ImplDb()
{
    if( m_db == nullptr )
        return;

    for( std::unique_ptr<ImplTable>& table : m_tables )
    {
        // clear any old values and write any cached values
        ClearOldValuesAndWriteCachedValues(*table);

        // finalize all prepared statements
        table.reset();
    }

    sqlite3_close(m_db);
}


SettingsDb::ImplTable* SettingsDb::ImplDb::OpenTable(std::wstring table_name_for_queries)
{
    // check if the table has already been opened
    auto lookup = std::find_if(m_tables.begin(), m_tables.end(),
                               [&](const std::unique_ptr<ImplTable>& table) { return SO::EqualsNoCase(table->table_name_for_queries, table_name_for_queries); });

    if( lookup != m_tables.end() )
        return lookup->get();

    // otherwise open (and possible create) a new table
    std::wstring create_sql = FormatTextCS2WS(_T("CREATE TABLE IF NOT EXISTS `%s` ")
                                              _T("(`key` TEXT PRIMARY KEY UNIQUE NOT NULL, ")
                                              _T("`value` TEXT NOT NULL, ")
                                              _T("`expiration` INTEGER NULL) WITHOUT ROWID;"), table_name_for_queries.c_str());

    if( sqlite3_exec(m_db, UTF8Convert::WideToUTF8(create_sql).c_str(), nullptr, nullptr, nullptr) != SQLITE_OK )
        return nullptr;

    std::wstring read_sql = FormatTextCS2WS(_T("SELECT `value`, `expiration` FROM `%s` WHERE `key` = ? LIMIT 1"), table_name_for_queries.c_str());
    std::wstring write_sql = FormatTextCS2WS(_T("INSERT OR REPLACE INTO `%s` (`key`, `value`, `expiration`) VALUES ( ?, ?, ? );"), table_name_for_queries.c_str());

    try
    {
        return m_tables.emplace_back(std::make_unique<ImplTable>(
            ImplTable
            {
                std::move(table_name_for_queries),
                SQLiteStatement(m_db, read_sql, true),
                SQLiteStatement(m_db, write_sql, true)
            })).get();
    }

    catch(...)
    {
        return nullptr;
    }
}


template<typename ReturnType, typename BaseValueType>
std::optional<ReturnType> SettingsDb::ImplDb::Read(SettingsDb& settings_db, wstring_view key, bool cache_value)
{
    ASSERT(std::find_if(m_tables.cbegin(), m_tables.cend(),
                        [&](const std::unique_ptr<ImplTable>& table) { return ( table.get() == settings_db.m_implTable ); }) != m_tables.cend());

    // first see if the value has been cached
    std::map<ImplCache::CacheKey, ImplCache::CacheValue<BaseValueType>>& cache_map = m_cache->GetCacheMap<BaseValueType>();
    ImplCache::CacheKey cache_key(this, settings_db.m_implTable, key);
    auto lookup = cache_map.find(cache_key);

    if( lookup != cache_map.end() )
    {
        const ImplCache::CacheValue<BaseValueType>& already_cached_value = lookup->second;

        // return the value if it is still valid
        if( !already_cached_value.expiry_timestamp.has_value() || *already_cached_value.expiry_timestamp > GetTimestamp<int64_t>() )
        {
            if constexpr(std::is_pointer_v<ReturnType>)
            {
                return &already_cached_value.value;
            }

            else
            {
                return already_cached_value.value;
            }
        }

        // otherwise remove the entry
        cache_map.erase(lookup);
    }

    // if here, the value has not been read, or was stale, so try to read it
    SQLiteStatement& stmt_read = settings_db.m_implTable->stmt_read;
    SQLiteResetOnDestruction rod(stmt_read);
    stmt_read.Bind(1, GetDbKey(settings_db.m_keyObfuscator.get(), key));

    if( stmt_read.Step() == SQLITE_ROW )
    {
        BaseValueType value = stmt_read.GetColumn<BaseValueType>(0);

        auto get_expiry_timestamp = [&]()
        {
            return !stmt_read.IsColumnNull(1) ? std::make_optional(stmt_read.GetColumn<int64_t>(1)) :
                                                std::nullopt;
        };

        if constexpr(std::is_pointer_v<ReturnType>)
        {
            ASSERT(cache_value);
            return &cache_map.try_emplace(std::move(cache_key),
                                          ImplCache::CacheValue<BaseValueType> { std::move(value), get_expiry_timestamp(), settings_db.m_keyObfuscator, false }).first->second.value;
        }

        else
        {
            if( cache_value )
            {
                cache_map.try_emplace(std::move(cache_key),
                                      ImplCache::CacheValue<BaseValueType> { value, get_expiry_timestamp(), settings_db.m_keyObfuscator, false });
            }

            return value;
        }
    }

    return std::optional<ReturnType>();
}


template<typename T>
void SettingsDb::ImplDb::Write(SettingsDb& settings_db, wstring_view key, const T& value, bool cache_value)
{
    ASSERT(std::find_if(m_tables.cbegin(), m_tables.cend(),
                        [&](const std::unique_ptr<ImplTable>& table) { return ( table.get() == settings_db.m_implTable ); }) != m_tables.cend());

    auto get_expiry_timestamp = [&]() -> std::optional<int64_t>
    {
        if( settings_db.m_expirationSeconds.has_value() )
            return GetTimestamp<int64_t>() + *settings_db.m_expirationSeconds;

        return std::nullopt;
    };

    if( cache_value )
    {
        std::map<ImplCache::CacheKey, ImplCache::CacheValue<T>>& cache_map = m_cache->GetCacheMap<T>();
        ImplCache::CacheKey cache_key(this, settings_db.m_implTable, key);
        auto lookup = cache_map.find(cache_key);

        if( lookup != cache_map.end() )
        {
            ImplCache::CacheValue<T>& already_cached_value = lookup->second;
            already_cached_value.value = value;
            already_cached_value.expiry_timestamp = get_expiry_timestamp();
            already_cached_value.key_obfuscator = settings_db.m_keyObfuscator;
            already_cached_value.value_was_written = true;
        }

        else
        {
            m_cache->GetCacheMap<T>().try_emplace(std::move(cache_key),
                                                  ImplCache::CacheValue<T> { value, get_expiry_timestamp(), settings_db.m_keyObfuscator, true });
        }
    }

    else
    {
        WriteToDb(*settings_db.m_implTable, key, value, get_expiry_timestamp(), settings_db.m_keyObfuscator.get());
    }
}


template<typename T>
void SettingsDb::ImplDb::WriteToDb(ImplTable& table, wstring_view key, const T& value, const std::optional<int64_t>& expiry_timestamp,
                                   const KeyObfuscator* key_obfuscator)
{
    table.stmt_write.Bind(1, GetDbKey(key_obfuscator, key))
                    .Bind(2, value);

    if( expiry_timestamp.has_value() )
    {
        table.stmt_write.Bind(3, *expiry_timestamp);
    }

    else
    {
        table.stmt_write.BindNull(3);
    }

    table.stmt_write.Step();
    table.stmt_write.Reset();
}


void SettingsDb::ImplDb::ClearOldValuesAndWriteCachedValues(ImplTable& table)
{
    try
    {
        int64_t current_timestamp = GetTimestamp<int64_t>();

        // wrap everything in a transaction
        if( sqlite3_exec(m_db, SqlStatements::BeginTransaction, nullptr, nullptr, nullptr) != SQLITE_OK )
            throw std::exception();

        // clear old values
        std::wstring delete_sql = FormatTextCS2WS(_T("DELETE FROM `%s` WHERE `expiration` <= ?;"), table.table_name_for_queries.c_str());
        SQLiteStatement stmt_delete(m_db, delete_sql, true);
        stmt_delete.Bind(1, current_timestamp);
        stmt_delete.Step();

        // write out cached values
        auto write_cached_values = [&](const auto& cache_map)
        {
            for( const auto& [cache_key, cache_value] : cache_map )
            {
                if( ( cache_value.value_was_written ) &&
                    ( std::get<0>(cache_key) == this && std::get<1>(cache_key) == &table ) &&
                    ( !cache_value.expiry_timestamp.has_value() || *cache_value.expiry_timestamp > current_timestamp ) )
                {
                    WriteToDb(table, std::get<2>(cache_key), cache_value.value, cache_value.expiry_timestamp, cache_value.key_obfuscator.get());
                }
            }
        };

        write_cached_values(m_cache->GetCacheMap<bool>());
        write_cached_values(m_cache->GetCacheMap<int>());
        write_cached_values(m_cache->GetCacheMap<unsigned int>());
        write_cached_values(m_cache->GetCacheMap<int64_t>());
        write_cached_values(m_cache->GetCacheMap<size_t>());
        write_cached_values(m_cache->GetCacheMap<float>());
        write_cached_values(m_cache->GetCacheMap<double>());
        write_cached_values(m_cache->GetCacheMap<std::wstring>());

        if( sqlite3_exec(m_db, SqlStatements::EndTransaction, nullptr, nullptr, nullptr) != SQLITE_OK )
            throw std::exception();
    }

    catch(...)
    {
        ASSERT(false);
    }
}


std::string SettingsDb::ImplDb::GetDbKey(const KeyObfuscator* key_obfuscator, wstring_view key)
{
    if( key_obfuscator == nullptr )
    {
        return UTF8Convert::WideToUTF8(key);
    }

    else
    {
        ASSERT(*key_obfuscator == KeyObfuscator::Hash);
        return UTF8Convert::WideToUTF8(Hash::Hash(key));
    }
}



// --------------------------------------------------------------------------
// SettingsDb
// --------------------------------------------------------------------------

SettingsDb::SettingsDb(const std::wstring& filename_only, std::wstring settings_name/* = _T("settings")*/,
                       std::optional<int64_t> expiration_seconds/* = std::nullopt*/, std::optional<KeyObfuscator> key_obfuscator/* = std::nullopt*/)
    :   m_implDb(nullptr),
        m_implTable(nullptr),
        m_expirationSeconds(std::move(expiration_seconds)),
        m_keyObfuscator(key_obfuscator.has_value() ? std::make_shared<KeyObfuscator>(*key_obfuscator) : nullptr)
{
    ASSERT(PortableFunctions::PathGetDirectory(filename_only).empty());
    ASSERT(!m_expirationSeconds.has_value() || *m_expirationSeconds > 0);

    if( filename_only.empty() )
        return;

    // allow multiple instances of SettingsDb to share the same database
    static std::map<StringNoCase, std::unique_ptr<ImplDb>> implementations;

    const auto& lookup = implementations.find(filename_only);

    if( lookup != implementations.cend() )
    {
        m_implDb = lookup->second.get();
    }

    else
    {
        // open a new database
        std::wstring full_filename = PortableFunctions::PathAppendToPath(GetAppDataPath(), filename_only);
        sqlite3* db;

        if( sqlite3_open(UTF8Convert::WideToUTF8(full_filename).c_str(), &db) != SQLITE_OK )
            return;

        m_implDb = implementations.try_emplace(filename_only, std::make_unique<ImplDb>(db)).first->second.get();
    }

    // make sure the table name is properly escaped for queries
    m_implTable = m_implDb->OpenTable(SQLiteHelpers::EscapeText(std::move(settings_name)));
}


namespace
{
    std::wstring ToFilename(CSProExecutables::Program program)
    {
        std::optional<std::wstring> module_filename = GetExecutablePath(program);

        if( module_filename.has_value() )
            return PortableFunctions::PathGetFilenameWithoutExtension(*module_filename) + _T(".db");

        return std::wstring();
    }
}


SettingsDb::SettingsDb(CSProExecutables::Program program, std::wstring settings_name/* = _T("settings")*/,
                       std::optional<int64_t> expiration_seconds/* = std::nullopt*/, std::optional<KeyObfuscator> key_obfuscator/* = std::nullopt*/)
    :   SettingsDb(ToFilename(program), std::move(settings_name), std::move(expiration_seconds), std::move(key_obfuscator))
{
}


template<typename T>
std::optional<T> SettingsDb::ReadWorker(wstring_view key, bool cache_value)
{
    if( m_implTable == nullptr )
        return std::nullopt;

    using RealType = std::remove_const_t<std::remove_pointer_t<T>>;

    return m_implDb->Read<T, typename RealType>(*this, key, cache_value);
}


template<typename T>
void SettingsDb::WriteWorker(wstring_view key, const T& value, bool cache_value)
{
    if( m_implTable == nullptr )
        return;

    m_implDb->Write<T>(*this, key, value, cache_value);
}


#define INSTANTIATE(ValueType) template CLASS_DECL_ZUTILO std::optional<const ValueType*> SettingsDb::ReadWorker(wstring_view key, bool cache_value); \
                               template CLASS_DECL_ZUTILO std::optional<ValueType> SettingsDb::ReadWorker(wstring_view key, bool cache_value);        \
                               template CLASS_DECL_ZUTILO void SettingsDb::WriteWorker(wstring_view key, const ValueType& value, bool cache_value);

INSTANTIATE(bool)
INSTANTIATE(int)
INSTANTIATE(unsigned int)
INSTANTIATE(int64_t)
INSTANTIATE(size_t)
INSTANTIATE(float)
INSTANTIATE(double)
INSTANTIATE(std::wstring)
// if any types are added, make sure they are also listed in SettingsDb::ImplDb::ClearOldValuesAndWriteCachedValues
