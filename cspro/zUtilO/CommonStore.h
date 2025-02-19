#pragma once

#include <zUtilO/zUtilO.h>
#include <zUtilO/SimpleDbMap.h>


// the CommonStore is used for:
// - the settings used by the loadsetting/savesetting functions
// - system settings used on Android
// - config variables
// - persistent variables
// - settings stored as part of the Action Invoker (prefixed with CS_)


class CLASS_DECL_ZUTILO CommonStore : public SimpleDbMap
{
public:
    enum class TableType { UserSettings, ConfigVariables, PersistentVariables };

    CommonStore();
    ~CommonStore();

    bool Open(std::vector<TableType> table_types, std::string common_store_file_path = std::string());

    void SwitchTable(TableType table_type);
    void SwitchTable(std::string_view table_name_sv, bool make_table_name_valid); // throws on table creation error

    void Close() override;

    bool Clear() override;
    bool Delete(const std::string& key) override;

    bool PutString(const std::string& key, const std::string& value) override;
    std::optional<std::string> GetString(const std::string& key) override;

    static std::string GetSystemSetting(const std::string& key);

private:
    bool UseGlobalCommonStoreAndCaching() const;

    // instead of accessing the database, system settings will be cached
    void CacheSystemSettings(std::map<std::string, std::string>& cached_system_settings);

    std::map<std::string, std::string>* GetCurrentCachedSystemSettings(std::string_view key_for_system_setting_check_sv = std::string_view()) const;

private:
    std::vector<TableType> m_tableTypes;
    std::variant<std::monostate, TableType, std::string> m_currentTableTypeOrName;
    std::unique_ptr<std::vector<std::tuple<std::string, std::string>>> m_createdValidTableNames; // table name -> valid table name

    std::unique_ptr<std::map<std::string, std::string>> m_cachedSystemSettings;
    std::unique_ptr<CommonStore> m_globalCommonStore;
};
