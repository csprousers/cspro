#include "StdAfx.h"
#include "CommonStore.h"
#include <SQLite/SQLiteHelpers.h>
#include <SQLite/TableNamer.h>


namespace
{
    constexpr const TCHAR* SystemSettingPrefix = _T("CSEntry.");

    std::wstring GetGlobalCommonStoreFilename()
    {
        return PortableFunctions::PathAppendToPath(GetAppDataPath(), _T("CommonStore.db"));
    }

    std::vector<CommonStore*> CommonStores;
    std::unique_ptr<std::map<size_t, std::wstring>> GlobalCachedSystemSettings;
}


CommonStore::CommonStore()
{
}


CommonStore::~CommonStore()
{
    Close();
}


bool CommonStore::Open(std::vector<TableType> table_types, std::wstring common_store_filename/* = std::wstring()*/)
{
    m_tableTypes = std::move(table_types);
    ASSERT(!m_tableTypes.empty());

    bool accessing_user_settings = ( std::find(m_tableTypes.cbegin(), m_tableTypes.cend(), TableType::UserSettings) != m_tableTypes.cend() );

    if( common_store_filename.empty() )
    {
        common_store_filename = GetGlobalCommonStoreFilename();
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
    std::vector<std::tuple<std::wstring, ValueType>> table_names_and_value_types;

    for( TableType table_type : m_tableTypes )
    {
        const TCHAR* const table_name =
            ( table_type == TableType::UserSettings )    ? _T("UserSettings") : 
            ( table_type == TableType::ConfigVariables ) ? _T("Configurations") :
                                                           _T("PersistentVariables");
        table_names_and_value_types.emplace_back(table_name, ValueType::String);
    }

    if( !SimpleDbMap::Open(common_store_filename, table_names_and_value_types) )
        return false;

    m_currentTableTypeOrName = m_tableTypes.front();

    if( accessing_user_settings )
        CommonStores.emplace_back(this);

    return true;
}


void CommonStore::SwitchTable(TableType table_type)
{
    if( std::holds_alternative<TableType>(m_currentTableTypeOrName) && table_type == std::get<TableType>(m_currentTableTypeOrName) )
        return;

    size_t index = std::distance(m_tableTypes.cbegin(), std::find(m_tableTypes.cbegin(), m_tableTypes.cend(), table_type));
    ASSERT(index < m_tableDetails.size());
    m_currentTable = m_tableDetails[index].get();
    m_currentTableTypeOrName = table_type;
}


void CommonStore::SwitchTable(const StringNoCase& table_name, bool make_table_name_valid)
{
    auto get_valid_table_name = [&]() -> const StringNoCase&
    {
        if( m_createdValidTableNames == nullptr )
            m_createdValidTableNames = std::make_unique<std::map<StringNoCase, StringNoCase>>();

        const auto& valid_table_name_lookup = m_createdValidTableNames->find(table_name);
        return ( valid_table_name_lookup != m_createdValidTableNames->cend() ) ?
               valid_table_name_lookup->second :
               m_createdValidTableNames->try_emplace(table_name, UTF8Convert::UTF8ToWide(CreateSQLiteValidTableName(UTF8Convert::WideToUTF8(table_name)))).first->second;
    };

    const StringNoCase& actual_table_name = make_table_name_valid ? get_valid_table_name() :
                                                                    table_name;

    if( std::holds_alternative<std::wstring>(m_currentTableTypeOrName) && SO::EqualsNoCase(actual_table_name, std::get<std::wstring>(m_currentTableTypeOrName)) )
        return;

    // the table may already be open
    const auto& table_lookup = std::find_if(m_tableDetails.cbegin(), m_tableDetails.cend(),
        [&](const std::unique_ptr<TableDetails>& table_details)
        {
            return SO::EqualsNoCase(actual_table_name, table_details->table_name);
        });

    m_currentTable = ( table_lookup != m_tableDetails.cend() ) ? table_lookup->get() :
                                                                 CreateTableIfNotExists(actual_table_name, ValueType::String);
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
    auto* current_cached_system_settings = GetCurrentCachedSystemSettings();

    if( current_cached_system_settings != nullptr )
        current_cached_system_settings->clear();

    return SimpleDbMap::Clear();
}


bool CommonStore::Delete(wstring_view key)
{
    auto* current_cached_system_settings = GetCurrentCachedSystemSettings(key);

    if( current_cached_system_settings != nullptr )
        current_cached_system_settings->erase(key.hash_code());

    return SimpleDbMap::Delete(key);
}


bool CommonStore::PutString(wstring_view key, wstring_view value)
{
    // if this is a system setting and the settings have already been cached, add or clear this setting
    auto* current_cached_system_settings = GetCurrentCachedSystemSettings(key);

    if( current_cached_system_settings != nullptr )
    {
        if( value.empty() )
        {
            current_cached_system_settings->erase(key.hash_code());
        }

        else
        {
            (*current_cached_system_settings)[key.hash_code()] = value;
        }
    }

    return SimpleDbMap::PutString(key, value);
}


std::optional<std::wstring> CommonStore::GetString(wstring_view key)
{
    std::optional<std::wstring> value = SimpleDbMap::GetString(key);

    if( !value.has_value() && UseGlobalCommonStoreAndCaching() && m_globalCommonStore != nullptr )
        value = m_globalCommonStore->GetString(key);

    return value;
}


bool CommonStore::UseGlobalCommonStoreAndCaching() const
{
    return ( std::holds_alternative<TableType>(m_currentTableTypeOrName) && std::get<TableType>(m_currentTableTypeOrName) == TableType::UserSettings );
}


std::map<size_t, std::wstring>* CommonStore::GetCurrentCachedSystemSettings(wstring_view key_for_system_setting_check/* = wstring_view()*/) const
{
    std::map<size_t, std::wstring>* current_cached_system_settings = nullptr;

    // caching is only used for user settings
    if( ( UseGlobalCommonStoreAndCaching() ) &&
        ( key_for_system_setting_check.empty() || SO::StartsWith(key_for_system_setting_check, SystemSettingPrefix) ) )
    {
        current_cached_system_settings = ( m_globalCommonStore == nullptr ) ? GlobalCachedSystemSettings.get() :
                                                                              m_cachedSystemSettings.get();
    }

    return current_cached_system_settings;
}


void CommonStore::CacheSystemSettings(std::map<size_t, std::wstring>& cached_system_settings)
{
    ASSERT(UseGlobalCommonStoreAndCaching());

    SQLiteStatement iterator_stmt(m_db, FormatText(_T("SELECT `Key`, `Value` FROM `%s` WHERE `Key` LIKE '%s%%';"),
        m_currentTable->table_name.c_str(), SystemSettingPrefix));

    while( iterator_stmt.Step() == SQLITE_ROW )
    {
        std::wstring key = iterator_stmt.GetColumn<std::wstring>(0);
        cached_system_settings[wstring_view(key).hash_code()] = iterator_stmt.GetColumn<std::wstring>(1);
    }
}


std::wstring CommonStore::GetSystemSetting(wstring_view key)
{
    ASSERT(SO::StartsWith(key, SystemSettingPrefix));

    CommonStore* current_common_store = CommonStores.empty() ? nullptr : CommonStores.back();

    // if the global common store settings haven't been cached yet, cache the settings
    if( GlobalCachedSystemSettings == nullptr )
    {
        GlobalCachedSystemSettings = std::make_unique<std::map<size_t, std::wstring>>();

        // if the global common store is already open, use it directly
        if( current_common_store != nullptr )
        {
            CommonStore* global_common_store = ( current_common_store->m_globalCommonStore != nullptr ) ?
                current_common_store->m_globalCommonStore.get() : current_common_store;

            global_common_store->SwitchTable(TableType::UserSettings);
            global_common_store->CacheSystemSettings(*GlobalCachedSystemSettings);
        }

        // otherwise, open the global common store
        else if( PortableFunctions::FileIsRegular(GetGlobalCommonStoreFilename()) )
        {
            CommonStore temp_global_common_store;

            if( temp_global_common_store.Open({ TableType::UserSettings }) )
                temp_global_common_store.CacheSystemSettings(*GlobalCachedSystemSettings);
        }
    }


    const std::map<size_t, std::wstring>* current_cached_system_settings = nullptr;

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
            current_common_store->m_cachedSystemSettings = std::make_unique<std::map<size_t, std::wstring>>(*GlobalCachedSystemSettings);
            current_common_store->CacheSystemSettings(*current_common_store->m_cachedSystemSettings);
        }

        current_cached_system_settings = current_common_store->m_cachedSystemSettings.get();
    }

    // check for the setting
    if( !current_cached_system_settings->empty() )
    {
        const auto& map_search = current_cached_system_settings->find(key.hash_code());

        if( map_search != current_cached_system_settings->cend() )
            return map_search->second;
    }

    return std::wstring();
}
