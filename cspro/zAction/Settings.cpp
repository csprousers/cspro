#include "stdafx.h"
#include <zUtilO/CommonStore.h>


std::shared_ptr<CommonStore> ActionInvoker::Runtime::SwitchToProperSettingsTable(const JsonNode& json_node, bool& using_UserSettings_table)
{
    std::shared_ptr<CommonStore> common_store = ObjectTransporter::GetCommonStore();
    ASSERT(common_store != nullptr && common_store->IsOpen());

    constexpr const char* UserSettingsTableName = "UserSettings";
    constexpr const char* CustomTablePrefix     = "CS_";

    using_UserSettings_table = true;

    if( json_node.Contains(JK::source) )
    {
        const std::string_view table_name_sv = json_node.Get<std::string_view>(JK::source);

        if( !SO::EqualsNoCase(table_name_sv, UserSettingsTableName) )
        {
            using_UserSettings_table = false;
            common_store->SwitchTable(SO::Concatenate(CustomTablePrefix, table_name_sv), true);
        }
    }

    if( using_UserSettings_table )
        common_store->SwitchTable(CommonStore::TableType::UserSettings);

    return common_store;
}


ActionInvoker::Result ActionInvoker::Runtime::Settings_getValue(const JsonNode& json_node, Caller& /*caller*/)
{
    bool using_UserSettings_table;
    const std::shared_ptr<CommonStore> common_store = SwitchToProperSettingsTable(json_node, using_UserSettings_table);

    const std::string key = json_node.Get<std::string>(JK::key);

    std::optional<std::string> value = common_store->GetString(key);

    if( value.has_value() )
    {
        // the loadsetting/savesetting table does not store values in JSON format
        if( using_UserSettings_table )
            return Result::String(std::move(*value));

        return Result::FromJsonNode(std::move(*value));
    }

    if( json_node.Contains(JK::value) )
        return Result::FromJsonNode(json_node.Get(JK::value));

    throw CSProException("No setting exists with the key '%s'.", key.c_str());
}


ActionInvoker::Result ActionInvoker::Runtime::Settings_putValue(const JsonNode& json_node, Caller& /*caller*/)
{
    bool using_UserSettings_table;
    const std::shared_ptr<CommonStore> common_store = SwitchToProperSettingsTable(json_node, using_UserSettings_table);

    const std::string key = json_node.Get<std::string>(JK::key);

    // the loadsetting/savesetting table does not store values in JSON format, so store
    // everything as a string, only allowing the types that would work in CSPro logic
    if( using_UserSettings_table )
    {
        const JsonNode value_node = json_node.Get(JK::value);

        if( !value_node.IsString() && !value_node.IsNumber() )
            throw CSProException("Only strings and numbers can be stored when using the 'UserSettings' source.");

        common_store->PutString(key, value_node.Get<std::string>());
    }

    else
    {
        common_store->PutString(key, json_node.Get(JK::value).GetNodeAsString());
    }    

    return Result::Undefined();
}
