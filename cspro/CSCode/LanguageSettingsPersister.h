#pragma once

#include <CSCode/LanguageSettings.h>
#include <zToolsO/CaseInsensitiveComparer.h>
#include <zUtilO/SettingsDb.h>


// --------------------------------------------------------------------------
// LanguageSettingsPersister
//
// a class to manage the language type and CSPro settings related to files;
// this class ensures that, if settings are manually overridden, that the
// user does not have to override them again the next time they open the file
// --------------------------------------------------------------------------

class LanguageSettingsPersister
{
public:
    LanguageSettingsPersister();
    ~LanguageSettingsPersister();

    std::optional<LanguageType> GetLanguageType(const std::string& file_path);
    std::optional<std::tuple<bool, bool>> GetActionInvokerJsonResultsAndExceptionFlags(const std::string& file_path);
    std::optional<LogicSettings> GetLogicSettings(const std::string& file_path);
    std::optional<unsigned> GetJavaScriptModuleType(const std::string& file_path);

    void Remember(const std::string& file_path,
                  const LanguageType* language_type,
                  const std::tuple<bool, bool>* action_invoker_json_results_and_exception_flags,
                  const LogicSettings* logic_settings,
                  const unsigned* javascript_module_type);

private:
    struct Data
    {
        std::optional<LanguageType> language_type;
        std::optional<std::tuple<bool, bool>> action_invoker_json_results_and_exception_flags;
        std::optional<LogicSettings> logic_settings;
        std::optional<unsigned> javascript_module_type;
    };

    Data* GetData(const std::string& file_path);
    Data& GetOrCreateData(const std::string& file_path);
        
private:
    SettingsDb m_settingsDb;
    std::map<std::string, Data, cs::case_insensitive_less> m_data;
};
