#include "StdAfx.h"
#include "CommonStore.h"
#include <zSql/SQLiteHelpers.h>
#include <zSql/TableNamer.h>


namespace
{
    constexpr const char* SystemSettingPrefix = "CSEntry.";

    std::string GetGlobalCommonStoreFilename()
    {
        return Path::Combine(GetAppDataPath(), "CommonStore.db");
    }

    std::vector<CommonStore*> CommonStores;
    std::unique_ptr<std::map<std::string, std::string>> GlobalCachedSystemSettings;
}


CommonStore::CommonStore()
{
}


CommonStore::~CommonStore()
{
    Close();
}


bool CommonStore::Open(std::vector<TableType> table_types, std::string common_store_file_path/* = std::string()*/)
{
    m_tableTypes = std::move(table_types);
    ASSERT(!m_tableTypes.empty());

    const bool accessing_user_settings = ( std::find(m_tableTypes.cbegin(), m_tableTypes.cend(), TableType::UserSettings) != m_tableTypes.cend() );

    if( common_store_file_path.empty() )
    {
        common_store_file_path = GetGlobalCommonStoreFilename();
    }

    // if a specific common store file is specified, open it, but also open the global common store if it exists
    // and when accessing user settings
    else if( accessing_user_settings && PortableFunctions::FileIsRegular(GetGlobalCommonStoreFilename()) )
    {
        m_globalCommonStore = std::make_unique<CommonStore>();

        if( !m_globalCommonStore->Open({ TableType::UserSettings }) )
            m_globalCommonStore.reset();
    }

    // all tables will be created with string values
    std::vector<std::tuple<std::string, ValueType>> table_names_and_value_types;

    for( const TableType table_type : m_tableTypes )
    {
        const char* const table_name = ( table_type == TableType::UserSettings )    ? "UserSettings" :
                                       ( table_type == TableType::ConfigVariables ) ? "Configurations" :
                                                                                      "PersistentVariables";
        table_names_and_value_types.emplace_back(table_name, ValueType::String);
    }

    if( !SimpleDbMap::Open(std::move(common_store_file_path), table_names_and_value_types) )
        return false;

    m_currentTableTypeOrName = m_tableTypes.front();

    if( accessing_user_settings )
        CommonStores.emplace_back(this);

    return true;
}


void CommonStore::SwitchTable(const TableType table_type)
{
    if( std::holds_alternative<TableType>(m_currentTableTypeOrName) && table_type == std::get<TableType>(m_currentTableTypeOrName) )
        return;

    const size_t index = std::distance(m_tableTypes.cbegin(), std::find(m_tableTypes.cbegin(), m_tableTypes.cend(), table_type));
    ASSERT(index < m_tableDetails.size());
    m_currentTable = m_tableDetails[index].get();
    m_currentTableTypeOrName = table_type;
}


void CommonStore::SwitchTable(std::string_view table_name_sv, const bool make_table_name_valid)
{
    auto get_valid_table_name = [&]() -> const std::string&
    {
        if( m_createdValidTableNames == nullptr )
        {
            m_createdValidTableNames = std::make_unique<std::vector<std::tuple<std::string, std::string>>>();
        }

        else
        {
            for( const auto& [this_table_name, this_valid_table_name] : *m_createdValidTableNames )
            {
                if( SO::EqualsNoCase(this_table_name, table_name_sv) )
                    return this_valid_table_name;
            }
        }

        return std::get<1>(m_createdValidTableNames->emplace_back(std::string(table_name_sv), Sqlite::CreateValidTableName(std::string(table_name_sv))));
    };

    if( make_table_name_valid )
        table_name_sv = get_valid_table_name();

    if( std::holds_alternative<std::string>(m_currentTableTypeOrName) && SO::EqualsNoCase(table_name_sv, std::get<std::string>(m_currentTableTypeOrName)) )
        return;

    // the table may already be open
    const auto& table_lookup = std::find_if(m_tableDetails.cbegin(), m_tableDetails.cend(),
        [&](const std::unique_ptr<TableDetails>& table_details)
        {
            return SO::EqualsNoCase(table_name_sv, table_details->table_name);
        });

    m_currentTable = ( table_lookup != m_tableDetails.cend() ) ? table_lookup->get() :
                                                                 CreateTableIfNotExists(std::string(table_name_sv), ValueType::String);
    m_currentTableTypeOrName = m_currentTable->table_name;
}


void CommonStore::Close()
{
    CommonStores.erase(std::remove(CommonStores.begin(), CommonStores.end(), this), CommonStores.end());

    SimpleDbMap::Close();

    m_cachedSystemSettings.reset();
    m_globalCommonStore.reset();
}


bool CommonStore::Clear()
{
    std::map<std::string, std::string>* current_cached_system_settings = GetCurrentCachedSystemSettings();

    if( current_cached_system_settings != nullptr )
        current_cached_system_settings->clear();

    return SimpleDbMap::Clear();
}


bool CommonStore::Delete(const std::string& key)
{
    std::map<std::string, std::string>* current_cached_system_settings = GetCurrentCachedSystemSettings(key);

    if( current_cached_system_settings != nullptr )
        current_cached_system_settings->erase(key);

    return SimpleDbMap::Delete(key);
}


bool CommonStore::PutString(const std::string& key, const std::string& value)
{
    // if this is a system setting and the settings have already been cached, add or clear this setting
    std::map<std::string, std::string>* current_cached_system_settings = GetCurrentCachedSystemSettings(key);

    if( current_cached_system_settings != nullptr )
    {
        if( value.empty() )
        {
            current_cached_system_settings->erase(key);
        }

        else
        {
            (*current_cached_system_settings)[key] = value;
        }
    }

    return SimpleDbMap::PutString(key, value);
}


std::optional<std::string> CommonStore::GetString(const std::string& key)
{
    std::optional<std::string> value = SimpleDbMap::GetString(key);

    if( !value.has_value() && UseGlobalCommonStoreAndCaching() && m_globalCommonStore != nullptr )
        value = m_globalCommonStore->GetString(key);

    return value;
}


bool CommonStore::UseGlobalCommonStoreAndCaching() const
{
    return ( std::holds_alternative<TableType>(m_currentTableTypeOrName) && std::get<TableType>(m_currentTableTypeOrName) == TableType::UserSettings );
}


std::map<std::string, std::string>* CommonStore::GetCurrentCachedSystemSettings(std::string_view key_for_system_setting_check_sv/* = std::string_view()*/) const
{
    std::map<std::string, std::string>* current_cached_system_settings = nullptr;

    // caching is only used for user settings
    if( ( UseGlobalCommonStoreAndCaching() ) &&
        ( key_for_system_setting_check_sv.empty() || SO::StartsWith(key_for_system_setting_check_sv, SystemSettingPrefix) ) )
    {
        current_cached_system_settings = ( m_globalCommonStore == nullptr ) ? GlobalCachedSystemSettings.get() :
                                                                              m_cachedSystemSettings.get();
    }

    return current_cached_system_settings;
}


void CommonStore::CacheSystemSettings(std::map<std::string, std::string>& cached_system_settings)
{
    ASSERT(UseGlobalCommonStoreAndCaching());

    const std::string sql = FormatText("SELECT `Key`, `Value` FROM `%s` WHERE `Key` LIKE '%s%%';",
                                       m_currentTable->table_name.c_str(), SystemSettingPrefix);
    SQLiteStatement iterator_stmt(m_db, sql);

    while( iterator_stmt.Step() == SQLITE_ROW )
        cached_system_settings[iterator_stmt.GetColumn<std::string>(0)] = iterator_stmt.GetColumn<std::string>(1);
}


std::string CommonStore::GetSystemSetting(const std::string& key)
{
    ASSERT(SO::StartsWith(key, SystemSettingPrefix));

    CommonStore* current_common_store = CommonStores.empty() ? nullptr :
                                                               CommonStores.back();

    // if the global common store settings haven't been cached yet, cache the settings
    if( GlobalCachedSystemSettings == nullptr )
    {
        GlobalCachedSystemSettings = std::make_unique<std::map<std::string, std::string>>();

        // if the global common store is already open, use it directly
        if( current_common_store != nullptr )
        {
            CommonStore& global_common_store = ( current_common_store->m_globalCommonStore != nullptr ) ? *current_common_store->m_globalCommonStore :
                                                                                                          *current_common_store;
            global_common_store.SwitchTable(TableType::UserSettings);
            global_common_store.CacheSystemSettings(*GlobalCachedSystemSettings);
        }

        // otherwise, open the global common store
        else if( PortableFunctions::FileIsRegular(GetGlobalCommonStoreFilename()) )
        {
            CommonStore temp_global_common_store;

            if( temp_global_common_store.Open({ TableType::UserSettings }) )
                temp_global_common_store.CacheSystemSettings(*GlobalCachedSystemSettings);
        }
    }


    const std::map<std::string, std::string>* current_cached_system_settings = nullptr;

    // if no common store is open, or the global common store is open, use the global settings
    if( current_common_store == nullptr || current_common_store->m_globalCommonStore == nullptr )
    {
        current_cached_system_settings = GlobalCachedSystemSettings.get();
    }

    else
    {
        if( current_common_store->m_cachedSystemSettings == nullptr )
        {
            // add the global settings and then access the database and get all settings for this common store
            current_common_store->m_cachedSystemSettings = std::make_unique<std::map<std::string, std::string>>(*GlobalCachedSystemSettings);
            current_common_store->CacheSystemSettings(*current_common_store->m_cachedSystemSettings);
        }

        current_cached_system_settings = current_common_store->m_cachedSystemSettings.get();
    }

    // check for the setting
    if( !current_cached_system_settings->empty() )
    {
        const auto& map_search = current_cached_system_settings->find(key);

        if( map_search != current_cached_system_settings->cend() )
            return map_search->second;
    }

    return std::string();
}
