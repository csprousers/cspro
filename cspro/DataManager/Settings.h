#pragma once

#include <zUtilO/SettingsDb.h>

class CaseHtmlContentCreatorSettings;
struct CaseJsonContentCreatorSettings;
class CaseQuestionnaireContentCreatorSettings;
struct CaseTextContentCreatorSettings;
class DataSourceSettings;
struct ExportDataSettings;
struct ExtractBinaryDataSettings;
struct ExtractNotesSettings;
class SettingsDb;


class Settings
{
    struct PerDataSourceData;

public:
    Settings();
    ~Settings();

    // --------------------------------------------------------------------------
    // Data Source and Case settings
    // --------------------------------------------------------------------------

    // GetSettings returns a non-null pointer to the requested settings.
    // If settings don't exist for the connection string, then default, or a copy of the last used, settings are returned.
    // By default, the settings are held by this class and then saved at the end of the program run, but if
    // create_unique_copy_of_settings is true, the settings returned will be unique and will not be saved at the end.
    template<typename T>
    std::shared_ptr<T> GetSettings(const ConnectionString& connection_string, bool create_unique_copy_of_settings = false);

    // Returns the DataSourceSettings if they exist, returning null otherwise.
    std::shared_ptr<const DataSourceSettings> GetDataSourceSettingsIfExist(const ConnectionString& connection_string);

private:
    PerDataSourceData* GetPerDataSourceData(const ConnectionString& connection_string, bool create_default_settings);
    PerDataSourceData* GetPerDataSourceData(const std::string& settings_key, bool create_default_settings);

    template<typename T>
    void EnsurePerDataSourceDataSettings(PerDataSourceData& per_data_source_data, std::shared_ptr<T> (PerDataSourceData::*settings));

    void Save();

    static std::string ToJson(PerDataSourceData& per_data_source_data);

private:
    std::unique_ptr<SettingsDb> m_mainSettingsDb;
    std::unique_ptr<SettingsDb> m_perDataSourceSettingsDb;

    // the first entry, with a blank string, will be the initial settings
    std::vector<std::tuple<std::string, std::unique_ptr<PerDataSourceData>>> m_perDataSourceData;
};
