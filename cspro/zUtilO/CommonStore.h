#pragma once

#include <zUtilO/zUtilO.h>
#include <zUtilO/SimpleDbMap.h>
#include <zToolsO/CaseInsensitiveComparer.h>


// --------------------------------------------------------------------------
// The CommonStore is used for:
//
// - the settings used by the loadsetting/savesetting functions
// - system settings used on Android
// - config variables
// - persistent variables
// - settings stored as part of the Action Invoker (prefixed with CS_)
// --------------------------------------------------------------------------

class CLASS_DECL_ZUTILO CommonStore : public SimpleDbMap
{
public:
    enum class TableType { UserSettings, ConfigVariables, PersistentVariables };

    CommonStore() noexcept;
    ~CommonStore() noexcept;

    bool Open(const std::vector<TableType>& table_types,
              std::string common_store_file_path = std::string()) noexcept;

    void SwitchTable(TableType table_type) noexcept;

    // An exception is thrown when there is an error creating a table.
    void SwitchTable(cs::string_sz table_name, bool make_table_name_valid);

    void Close() noexcept override;

    bool Clear() noexcept override;
    bool Delete(const std::string& key) noexcept override;

    bool PutString(const std::string& key, const std::string& value) noexcept override;
    std::optional<std::string> GetString(const std::string& key) noexcept override;

    static std::string GetSystemSetting(const std::string& key) noexcept;

private:
    static constexpr const char* ToString(TableType table_type);

    static std::string GetGlobalCommonStoreFilename();
    bool UseGlobalCommonStoreAndCaching() const noexcept;

    // instead of accessing the database, system settings will be cached
    void CacheSystemSettings(std::map<std::string, std::string>& cached_system_settings) noexcept;

    std::map<std::string, std::string>* GetCurrentCachedSystemSettings(std::string_view key_for_system_setting_check_sv = std::string_view()) const noexcept;

private:
    std::map<std::string, std::string, cs::case_insensitive_less> m_createdValidTableNames; // table name -> valid table name

    std::unique_ptr<std::map<std::string, std::string>> m_cachedSystemSettings;
    std::unique_ptr<CommonStore> m_globalCommonStore;
};
