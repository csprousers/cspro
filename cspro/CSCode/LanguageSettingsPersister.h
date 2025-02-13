#pragma once

#include <CSCode/LanguageSettings.h>
#include <zUtilO/SettingsDb.h>


// --------------------------------------------------------------------------
// LanguageSettingsPersister
//
// a class to manage the language type and CSPro settings related to files;
// this class ensures that, if settings are manually overriden, that the
// user does not have to override them again the next time they open the file
// --------------------------------------------------------------------------

class LanguageSettingsPersister
{
public:
    LanguageSettingsPersister();
    ~LanguageSettingsPersister();

    std::optional<LanguageType> GetLanguageType(const std::wstring& filename);
    std::optional<std::tuple<bool, bool>> GetActionInvokerJsonResultsAndExceptionFlags(const std::wstring& filename);
    std::optional<LogicSettings> GetLogicSettings(const std::wstring& filename);
    std::optional<unsigned> GetJavaScriptModuleType(const std::wstring& filename);

    void Remember(const std::wstring& filename,
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

    Data* GetData(const std::wstring& filename);
    Data& GetOrCreateData(const std::wstring& filename);
        
private:
    SettingsDb m_settingsDb;
    std::map<StringNoCase, Data> m_data;
};
