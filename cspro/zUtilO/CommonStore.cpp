#include "StdAfx.h"
#include "CommonStore.h"
#include <zSql/DB.h>
#include <zSql/TableNamer.h>


namespace
{
    constexpr const char* SystemSettingPrefix = "CSEntry.";

    std::vector<CommonStore*> CommonStores;
    std::unique_ptr<std::map<std::string, std::string>> GlobalCachedSystemSettings;
}


CommonStore::CommonStore() noexcept
{
}


CommonStore::~CommonStore() noexcept
{
    Close();
}


constexpr const char* CommonStore::ToString(const TableType table_type)
{
    return ( table_type == TableType::UserSettings )        ? "UserSettings":
           ( table_type == TableType::ConfigVariables )     ? "Configurations" :
         /*( table_type == TableType::PersistentVariables )*/ "PersistentVariables";
}


bool CommonStore::Open(const std::vector<TableType>& table_types,
                       std::string common_store_file_path/* = std::string()*/) noexcept
{
    ASSERT(!table_types.empty());

    const bool accessing_user_settings = ( std::find(table_types.cbegin(), table_types.cend(), TableType::UserSettings) != table_types.cend() );

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

    for( const TableType table_type : table_types )
        table_names_and_value_types.emplace_back(ToString(table_type), ValueType::String);

    if( !SimpleDbMap::Open(std::move(common_store_file_path), table_names_and_value_types) )
        return false;

    if( accessing_user_settings )
        CommonStores.emplace_back(this);

    return true;
}


void CommonStore::SwitchTable(const TableType table_type) noexcept
{
    try
    {
        SimpleDbMap::SwitchTable(ToString(table_type), std::nullopt);
    }
    catch(...) { ASSERT(false); }
}


void CommonStore::SwitchTable(const cs::string_sz table_name, const bool make_table_name_valid)
{
    const char* actual_table_name = table_name.c_str();

    if( make_table_name_valid )
    {
        auto lookup = m_createdValidTableNames.find(actual_table_name);

        if( lookup == m_createdValidTableNames.cend() )
        {
            std::string table_name_str(actual_table_name);
            std::string valid_table_name = Sqlite::CreateValidTableName(table_name_str);
            lookup = m_createdValidTableNames.try_emplace(std::move(table_name_str), std::move(valid_table_name)).first;
        }

        actual_table_name = lookup->second.c_str();
    }

    SimpleDbMap::SwitchTable(actual_table_name, ValueType::String);
}


void CommonStore::Close() noexcept
{
    try
    {
        CommonStores.erase(std::remove(CommonStores.begin(), CommonStores.end(), this), CommonStores.end());

        SimpleDbMap::Close();

        m_cachedSystemSettings.reset();
        m_globalCommonStore.reset();
    }
    catch(...) { ASSERT(false); }
}


bool CommonStore::Clear() noexcept
{
    std::map<std::string, std::string>* current_cached_system_settings = GetCurrentCachedSystemSettings();

    if( current_cached_system_settings != nullptr )
        current_cached_system_settings->clear();

    return SimpleDbMap::Clear();
}


bool CommonStore::Delete(const std::string& key) noexcept
{
    std::map<std::string, std::string>* current_cached_system_settings = GetCurrentCachedSystemSettings(key);

    if( current_cached_system_settings != nullptr )
        current_cached_system_settings->erase(key);

    return SimpleDbMap::Delete(key);
}


bool CommonStore::PutString(const std::string& key, const std::string& value) noexcept
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


std::optional<std::string> CommonStore::GetString(const std::string& key) noexcept
{
    std::optional<std::string> value = SimpleDbMap::GetString(key);

    if( !value.has_value() && m_globalCommonStore != nullptr && UseGlobalCommonStoreAndCaching() )
        value = m_globalCommonStore->GetString(key);

    return value;
}


std::string CommonStore::GetGlobalCommonStoreFilename()
{
    return Path::Combine(GetAppDataPath(), "CommonStore.db");
}


bool CommonStore::UseGlobalCommonStoreAndCaching() const noexcept
{
    return ( GetCurrentTableName() == ToString(TableType::UserSettings) );
}


std::map<std::string, std::string>* CommonStore::GetCurrentCachedSystemSettings(const std::string_view key_for_system_setting_check_sv/* = std::string_view()*/) const noexcept
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


void CommonStore::CacheSystemSettings(std::map<std::string, std::string>& cached_system_settings) noexcept
{
    ASSERT(UseGlobalCommonStoreAndCaching());

    try
    {
        const std::string sql = FormatText(
            "SELECT `Key`, `Value` "
            "FROM `%s` "
            "WHERE `Key` LIKE '%s%%';",
            GetCurrentTableName().c_str(),
            SystemSettingPrefix
        );

        Sqlite::Statement iterator_stmt = GetDb().PrepareStatement(sql);

        while( iterator_stmt.Step() == Sqlite::Result::Row )
            cached_system_settings[iterator_stmt.GetColumn<std::string>(0)] = iterator_stmt.GetColumn<std::string>(1);
    }

    catch(...) { ASSERT(false); }
}


std::string CommonStore::GetSystemSetting(const std::string& key) noexcept
{
    ASSERT(SO::StartsWith(key, SystemSettingPrefix));

    CommonStore* const current_common_store = CommonStores.empty() ? nullptr :
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
