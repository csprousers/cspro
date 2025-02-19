#include "StdAfx.h"
#include "LanguageSettingsPersister.h"


namespace
{
    // language settings will be persisted for a quarter of a year
    constexpr const char* LanguageSettingsTableName     = "language_settings";
    constexpr int64_t LanguageSettingsExpirationSeconds = DateHelper::SecondsInWeek(52 / 4);


    constexpr std::optional<unsigned> JavaScriptModuleTypeToJson(const std::optional<unsigned>& index)
    {
        return index.has_value() ? std::make_optional(*index - ID_RUN_JAVASCRIPT_MODULE_AUTODETECT) :
                                   std::nullopt;
    }


    constexpr std::optional<unsigned> JavaScriptModuleTypeFromJson(const std::optional<unsigned>& index)
    {
        if( index.has_value() )
        {
            const unsigned adjusted_index = *index + ID_RUN_JAVASCRIPT_MODULE_AUTODETECT;

            if( adjusted_index >= ID_RUN_JAVASCRIPT_MODULE_AUTODETECT && adjusted_index <= ID_RUN_JAVASCRIPT_MODULE_MODULE )
                return adjusted_index;
        }

        return std::nullopt;
    }
}


CREATE_JSON_KEY(actionInvokerAbortOnException)
CREATE_JSON_KEY(actionInvokerDisplayResultsAsJson)

CREATE_ENUM_JSON_SERIALIZER(LanguageType,
    { LanguageType::CSProLogic,         "CSPro-Logic" },
    { LanguageType::CSProReportHtml,    "CSPro-Report-HTML" },
    { LanguageType::CSProReport,        "CSPro-Report" },
    { LanguageType::CSProMessages,      "CSPro-Messages" },
    { LanguageType::CSProActionInvoker, "CSPro-Action-Invoker" },
    { LanguageType::CSProHtmlDialog,    "CSPro-HTML-Dialog" },
    { LanguageType::CSProSpecFileJson,  "CSPro-Spec-JSON" },
    { LanguageType::CSProSpecFileIni,   "CSPro-Spec-INI" },
    { LanguageType::Html,               "HTML" },
    { LanguageType::JavaScript,         "JavaScript" },
    { LanguageType::Json,               "JSON" },
    { LanguageType::Sql,                "SQL" },
    { LanguageType::Yaml,               "YAML" },
    { LanguageType::Text,               "text" })


LanguageSettingsPersister::LanguageSettingsPersister()
    :   m_settingsDb(CSProExecutables::Program::CSCode, LanguageSettingsTableName, LanguageSettingsExpirationSeconds, SettingsDb::KeyObfuscator::Hash)
{
}


LanguageSettingsPersister::~LanguageSettingsPersister()
{
    // save the accessed/modified settings
    for( const auto& [file_path, data] : m_data )
    {
        if( file_path.empty() )
            continue;

        try
        {
            const std::unique_ptr<JsonStringWriter> json_writer = Json::CreateStringWriter();

            json_writer->BeginObject()
                        .WriteIfHasValue(JK::languageType, data.language_type);

            if( data.action_invoker_json_results_and_exception_flags.has_value() )
            {
                json_writer->Write(JK::actionInvokerDisplayResultsAsJson, std::get<0>(*data.action_invoker_json_results_and_exception_flags))
                            .Write(JK::actionInvokerAbortOnException, std::get<1>(*data.action_invoker_json_results_and_exception_flags));
            }

            json_writer->WriteIfHasValue(JK::logicSettings, data.logic_settings)
                        .WriteIfHasValue(JK::module, JavaScriptModuleTypeToJson(data.javascript_module_type))
                        .EndObject();

            m_settingsDb.Write(file_path, json_writer->GetString());
        }
        catch(...) { ASSERT(false); }
    }
}


LanguageSettingsPersister::Data* LanguageSettingsPersister::GetData(const std::string& file_path)
{
    // see if the data has already been read
    const auto& lookup = m_data.find(file_path);

    if( lookup != m_data.cend() )
        return &lookup->second;

    // if not, try to read it from the settings database, ignoring any JSON errors
    try
    {
        const std::string* const json_text = m_settingsDb.Read<std::string*>(file_path);

        if( json_text != nullptr )
        {
            const JsonNode json_node = Json::Parse(*json_text);

            return &m_data.try_emplace(file_path, Data { json_node.GetOptional<LanguageType>(JK::languageType),
                                                         json_node.Contains(JK::actionInvokerDisplayResultsAsJson) ? std::make_optional(std::tuple<bool, bool>(json_node.Get<bool>(JK::actionInvokerDisplayResultsAsJson),
                                                                                                                                                               json_node.Get<bool>(JK::actionInvokerAbortOnException))) : std::nullopt,
                                                         json_node.GetOptional<LogicSettings>(JK::logicSettings),
                                                         JavaScriptModuleTypeFromJson(json_node.GetOptional<unsigned>(JK::module)) }).first->second;
        }
    }
    catch(...) { }

    return nullptr;
}


LanguageSettingsPersister::Data& LanguageSettingsPersister::GetOrCreateData(const std::string& file_path)
{
    Data* const data = GetData(file_path);

    if( data != nullptr )
        return *data;

    return m_data.try_emplace(file_path, Data { }).first->second;
}


std::optional<LanguageType> LanguageSettingsPersister::GetLanguageType(const std::string& file_path)
{
    const Data* const data = GetData(file_path);

    return ( data != nullptr ) ? data->language_type :
                                 std::nullopt;
}


std::optional<std::tuple<bool, bool>> LanguageSettingsPersister::GetActionInvokerJsonResultsAndExceptionFlags(const std::string& file_path)
{
    const Data* const data = GetData(file_path);

    return ( data != nullptr ) ? data->action_invoker_json_results_and_exception_flags :
                                 std::nullopt;
}


std::optional<LogicSettings> LanguageSettingsPersister::GetLogicSettings(const std::string& file_path)
{
    const Data* const data = GetData(file_path);

    return ( data != nullptr ) ? data->logic_settings :
                                 std::nullopt;
}


std::optional<unsigned> LanguageSettingsPersister::GetJavaScriptModuleType(const std::string& file_path)
{
    const Data* const data = GetData(file_path);

    return ( data != nullptr ) ? data->javascript_module_type :
                                 std::nullopt;
}


void LanguageSettingsPersister::Remember(const std::string& file_path,
                                         const LanguageType* const language_type,
                                         const std::tuple<bool, bool>* const action_invoker_json_results_and_exception_flags,
                                         const LogicSettings* const logic_settings,
                                         const unsigned* const javascript_module_type)
{
    Data& data = GetOrCreateData(file_path);

    if( language_type != nullptr )
        data.language_type = *language_type;

    if( action_invoker_json_results_and_exception_flags != nullptr )
        data.action_invoker_json_results_and_exception_flags = *action_invoker_json_results_and_exception_flags;

    if( logic_settings != nullptr )
        data.logic_settings = *logic_settings;

    if( javascript_module_type != nullptr )
        data.javascript_module_type = *javascript_module_type;
}
