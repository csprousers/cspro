#pragma once

#include <zUtilO/zUtilO.h>
#include <zUtilO/SimpleDbMap.h>
#include <zToolsO/StringNoCase.h>


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

    bool Open(std::vector<TableType> table_types, std::wstring common_store_filename = std::wstring());

    void SwitchTable(TableType table_type);
    void SwitchTable(const StringNoCase& table_name, bool make_table_name_valid); // throws on table creation error

    void Close() override;

    bool Clear() override;
    bool Delete(wstring_view key) override;

    bool PutString(wstring_view key, wstring_view value) override;
    std::optional<std::wstring> GetString(wstring_view key) override;

    static std::wstring GetSystemSetting(wstring_view key);

private:
    bool UseGlobalCommonStoreAndCaching() const;

    // instead of accessing the database, system settings will be cached
    void CacheSystemSettings(std::map<size_t, std::wstring>& cached_system_settings);

    std::map<size_t, std::wstring>* GetCurrentCachedSystemSettings(wstring_view key_for_system_setting_check = wstring_view()) const;

private:
    std::vector<TableType> m_tableTypes;
    std::variant<std::monostate, TableType, std::wstring> m_currentTableTypeOrName;
    std::unique_ptr<std::map<StringNoCase, StringNoCase>> m_createdValidTableNames;

    std::unique_ptr<std::map<size_t, std::wstring>> m_cachedSystemSettings;
    std::unique_ptr<CommonStore> m_globalCommonStore;
};
