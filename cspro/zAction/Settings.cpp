#include "stdafx.h"
#include <zUtilO/CommonStore.h>


CommonStore& ActionInvoker::Runtime::SwitchToProperSettingsTable(const JsonNode<wchar_t>& json_node, bool& using_UserSettings_table)
{
    std::shared_ptr<CommonStore> common_store = ObjectTransporter::GetCommonStore();
    ASSERT(common_store != nullptr && common_store->IsOpen());

    constexpr const TCHAR* UserSettingsTableName = _T("UserSettings");
    constexpr const TCHAR* CustomTablePrefix     = _T("CS_");

    using_UserSettings_table = true;

    if( json_node.Contains(JK::source) )
    {
        const wstring_view table_name_sv = json_node.Get<wstring_view>(JK::source);

        if( !SO::EqualsNoCase(table_name_sv, UserSettingsTableName) )
        {
            using_UserSettings_table = false;
            common_store->SwitchTable(CustomTablePrefix + std::wstring(table_name_sv), true);
        }
    }

    if( using_UserSettings_table )
        common_store->SwitchTable(CommonStore::TableType::UserSettings);

    return *common_store;
}


ActionInvoker::Result ActionInvoker::Runtime::Settings_getValue(const JsonNode<wchar_t>& json_node, Caller& /*caller*/)
{
    bool using_UserSettings_table;
    CommonStore& common_store = SwitchToProperSettingsTable(json_node, using_UserSettings_table);

    const wstring_view key_sv = json_node.Get<wstring_view>(JK::key);

    std::optional<std::wstring> value = common_store.GetString(key_sv);

    if( value.has_value() )
    {
        // the loadsetting/savesetting table does not store values in JSON format
        if( using_UserSettings_table )
            return Result::String(*value);

        return Result::FromJsonNode(*value);
    }

    if( json_node.Contains(JK::value) )
        return Result::FromJsonNode(json_node.Get(JK::value));

    throw CSProException(_T("No setting exists with the key '%s'."), std::wstring(key_sv).c_str());
}


ActionInvoker::Result ActionInvoker::Runtime::Settings_putValue(const JsonNode<wchar_t>& json_node, Caller& /*caller*/)
{
    bool using_UserSettings_table;
    CommonStore& common_store = SwitchToProperSettingsTable(json_node, using_UserSettings_table);

    const wstring_view key_sv = json_node.Get<wstring_view>(JK::key);

    // the loadsetting/savesetting table does not store values in JSON format, so store
    // everything as a string, only allowing the types that would work in CSPro logic
    if( using_UserSettings_table )
    {
        const auto& value_node = json_node.Get(JK::value);

        if( !value_node.IsString() && !value_node.IsNumber() )
            throw CSProException("Only strings and numbers can be stored when using the \"UserSettings\" source.");

        common_store.PutString(key_sv, value_node.Get<std::wstring>());
    }

    else
    {
        common_store.PutString(key_sv, json_node.Get(JK::value).GetNodeAsString());
    }    

    return Result::Undefined();
}
