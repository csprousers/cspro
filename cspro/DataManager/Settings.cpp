#include "StdAfx.h"
#include "Settings.h"
#include "CaseHtmlContentCreatorSettings.h"
#include "CaseJsonContentCreatorSettings.h"
#include "CaseQuestionnaireContentCreatorSettings.h"
#include "CaseTextContentCreatorSettings.h"
#include "DataSourceSettings.h"
#include "ExportDataSettings.h"
#include "ExtractBinaryDataSettings.h"
#include "ExtractNotesSettings.h"


CREATE_JSON_KEY(caseViewHtml)
CREATE_JSON_KEY(caseViewJson)
CREATE_JSON_KEY(caseViewQuestionnaire)
CREATE_JSON_KEY(caseViewText)
CREATE_JSON_KEY(exportData)
CREATE_JSON_KEY(extractBinaryData)
CREATE_JSON_KEY(extractNotes)


namespace
{
    // the main settings will not expire
    constexpr const char* MainSettingsTableName    = "data_manager";
    constexpr const char* MainSettingsDataKey      = "data_sources";

    // data source settings will be persisted for four weeks
    constexpr const char* DataSourcesTableName     = "data_sources";
    constexpr int64_t DataSourcesExpirationSeconds = DateHelper::SecondsInWeek(4);
}


struct Settings::PerDataSourceData
{
    std::shared_ptr<DataSourceSettings> data_source_settings;

    std::shared_ptr<ViewableCaseIteratorSettings> viewable_case_iterator_settings;

    std::shared_ptr<CaseHtmlContentCreatorSettings> case_html_content_creator_settings;
    std::shared_ptr<CaseJsonContentCreatorSettings> case_json_content_creator_settings;
    std::shared_ptr<CaseQuestionnaireContentCreatorSettings> case_questionnaire_content_creator_settings;
    std::shared_ptr<CaseTextContentCreatorSettings> case_text_content_creator_settings;

    std::shared_ptr<ExportDataSettings> export_data_settings;

    std::shared_ptr<ExtractBinaryDataSettings> extract_binary_data_settings;
    std::shared_ptr<ExtractNotesSettings> extract_notes_settings;
};


Settings::Settings()
    :   m_mainSettingsDb(std::make_unique<SettingsDb>(CSProExecutables::Program::DataManager, MainSettingsTableName)),
        m_perDataSourceSettingsDb(std::make_unique<SettingsDb>(CSProExecutables::Program::DataManager, DataSourcesTableName, DataSourcesExpirationSeconds))
{
    // load the initial settings
    GetPerDataSourceData(SO::Empty_string, true);
}


Settings::~Settings()
{
    try
    {
        Save();

    } catch(...) { ASSERT(false); }
}


Settings::PerDataSourceData* Settings::GetPerDataSourceData(const ConnectionString& connection_string, const bool create_default_settings)
{
    return GetPerDataSourceData(connection_string.GetName(DataRepositoryNameType::Full), create_default_settings);
}


Settings::PerDataSourceData* Settings::GetPerDataSourceData(const std::string& settings_key, const bool create_default_settings)
{
    ASSERT(settings_key.empty() == m_perDataSourceData.empty());

    const auto& lookup = std::find_if(m_perDataSourceData.cbegin(), m_perDataSourceData.cend(),
                                      [&](const auto& settings_key_and_data) { return ( std::get<0>(settings_key_and_data) == settings_key ); });

    if( lookup != m_perDataSourceData.cend() )
        return std::get<1>(*lookup).get();

    auto per_data_source_data = std::make_unique<Settings::PerDataSourceData>();

    // load the settings
    const std::string* const settings_json = settings_key.empty() ? m_mainSettingsDb->Read<std::string*>(MainSettingsDataKey) :
                                                                    m_perDataSourceSettingsDb->Read<std::string*>(settings_key);

    if( settings_json != nullptr )
    {
        try
        {
            const JsonNode json_node = Json::Parse(*settings_json);

            per_data_source_data->data_source_settings = std::make_unique<DataSourceSettings>(json_node.Get<DataSourceSettings>(JK::dataSource));

            per_data_source_data->viewable_case_iterator_settings = std::make_unique<ViewableCaseIteratorSettings>(json_node.Get<ViewableCaseIteratorSettings>(JK::caseListing));

            per_data_source_data->case_html_content_creator_settings = std::make_unique<CaseHtmlContentCreatorSettings>(json_node.Get<CaseHtmlContentCreatorSettings>(JK::caseViewHtml));
            per_data_source_data->case_json_content_creator_settings = std::make_unique<CaseJsonContentCreatorSettings>(json_node.Get<CaseJsonContentCreatorSettings>(JK::caseViewJson));
            per_data_source_data->case_questionnaire_content_creator_settings = std::make_unique<CaseQuestionnaireContentCreatorSettings>(json_node.Get<CaseQuestionnaireContentCreatorSettings>(JK::caseViewQuestionnaire));
            per_data_source_data->case_text_content_creator_settings = std::make_unique<CaseTextContentCreatorSettings>(json_node.Get<CaseTextContentCreatorSettings>(JK::caseViewText));

            per_data_source_data->export_data_settings = std::make_unique<ExportDataSettings>(json_node.Get<ExportDataSettings>(JK::exportData));

            per_data_source_data->extract_binary_data_settings = std::make_unique<ExtractBinaryDataSettings>(json_node.Get<ExtractBinaryDataSettings>(JK::extractBinaryData));
            per_data_source_data->extract_notes_settings = std::make_unique<ExtractNotesSettings>(json_node.Get<ExtractNotesSettings>(JK::extractNotes));
        }
        catch(...) { ASSERT(false); }
    }

    else if( !create_default_settings )
    {
        return nullptr;
    }

    EnsurePerDataSourceDataSettings(*per_data_source_data, &PerDataSourceData::data_source_settings);

    EnsurePerDataSourceDataSettings(*per_data_source_data, &PerDataSourceData::viewable_case_iterator_settings);

    EnsurePerDataSourceDataSettings(*per_data_source_data, &PerDataSourceData::case_html_content_creator_settings);
    EnsurePerDataSourceDataSettings(*per_data_source_data, &PerDataSourceData::case_json_content_creator_settings);
    EnsurePerDataSourceDataSettings(*per_data_source_data, &PerDataSourceData::case_questionnaire_content_creator_settings);
    EnsurePerDataSourceDataSettings(*per_data_source_data, &PerDataSourceData::case_text_content_creator_settings);

    EnsurePerDataSourceDataSettings(*per_data_source_data, &PerDataSourceData::export_data_settings);

    EnsurePerDataSourceDataSettings(*per_data_source_data, &PerDataSourceData::extract_binary_data_settings);
    EnsurePerDataSourceDataSettings(*per_data_source_data, &PerDataSourceData::extract_notes_settings);

    return std::get<1>(m_perDataSourceData.emplace_back(settings_key, std::move(per_data_source_data))).get();
}


template<typename T>
void Settings::EnsurePerDataSourceDataSettings(PerDataSourceData& per_data_source_data, std::shared_ptr<T> (PerDataSourceData::*settings))
{
    if( per_data_source_data.*settings != nullptr )
        return;

    // when creating default settings, some settings are not at all copyable
    if constexpr(std::is_same_v<T, DataSourceSettings> ||
                 std::is_same_v<T, CaseQuestionnaireContentCreatorSettings>)
    {
    }

    // other settings are fully or partially copyable from the last available settings
    else if( !m_perDataSourceData.empty() )
    {
        const PerDataSourceData& last_per_data_source_data = *std::get<1>(m_perDataSourceData.back());
        ASSERT(last_per_data_source_data.*settings != nullptr);
        per_data_source_data.*settings = std::make_unique<T>(*(last_per_data_source_data.*settings));

        // handle partially copyable settings
        ResetUniqueToDataSourceSettings(*(per_data_source_data.*settings));

        return;
    }

    // otherwise create default settings
    per_data_source_data.*settings = std::make_unique<T>();

    if constexpr(std::is_same_v<T, ViewableCaseIteratorSettings>)
    {
        // default to key order
        (per_data_source_data.*settings)->SetMethod(CaseIterationMethod::KeyOrder);
    }
}


template<typename T>
void Settings::ResetUniqueToDataSourceSettings(T& settings)
{
    if constexpr(std::is_same_v<T, ExportDataSettings>)
    {
        settings.base_file_path.clear();
    }

    else if constexpr(std::is_same_v<T, ExtractBinaryDataSettings>)
    {
        settings.output_directory.clear();
    }

    else if constexpr(std::is_same_v<T, ExtractNotesSettings>)
    {
        settings.notes_connection_string.Clear();
        settings.notes_dictionary_file_path.clear();
    }
}


template<typename T>
std::shared_ptr<T> Settings::GetSettings(const ConnectionString& connection_string, const bool create_unique_copy_of_settings/* = false*/)
{
    PerDataSourceData* const per_data_source_data = GetPerDataSourceData(connection_string, true);
    ASSERT(per_data_source_data != nullptr);
    std::shared_ptr<T> settings;

    if constexpr(std::is_same_v<T, DataSourceSettings>)
    {
        settings = per_data_source_data->data_source_settings;
    }

    else if constexpr(std::is_same_v<T, ViewableCaseIteratorSettings>)
    {
        settings = per_data_source_data->viewable_case_iterator_settings;
    }

    else if constexpr(std::is_same_v<T, CaseHtmlContentCreatorSettings>)
    {
        settings = per_data_source_data->case_html_content_creator_settings;
    }

    else if constexpr(std::is_same_v<T, CaseJsonContentCreatorSettings>)
    {
        settings = per_data_source_data->case_json_content_creator_settings;
    }

    else if constexpr(std::is_same_v<T, CaseQuestionnaireContentCreatorSettings>)
    {
        settings = per_data_source_data->case_questionnaire_content_creator_settings;
    }

    else if constexpr(std::is_same_v<T, CaseTextContentCreatorSettings>)
    {
        settings = per_data_source_data->case_text_content_creator_settings;
    }

    else if constexpr(std::is_same_v<T, ExportDataSettings>)
    {
        settings = per_data_source_data->export_data_settings;
    }

    else if constexpr(std::is_same_v<T, ExtractBinaryDataSettings>)
    {
        settings = per_data_source_data->extract_binary_data_settings;
    }

    else if constexpr(std::is_same_v<T, ExtractNotesSettings>)
    {
        settings = per_data_source_data->extract_notes_settings;
    }

    else
    {
        static_assert(false);
    }

    ASSERT(settings != nullptr);

    if( create_unique_copy_of_settings )
    {
        settings = std::make_unique<T>(*settings);
        ResetUniqueToDataSourceSettings(*settings);
    }

    return settings;
}

template std::shared_ptr<DataSourceSettings> Settings::GetSettings(const ConnectionString& connection_string, bool create_unique_copy_of_settings);
template std::shared_ptr<ViewableCaseIteratorSettings> Settings::GetSettings(const ConnectionString& connection_string, bool create_unique_copy_of_settings);
template std::shared_ptr<CaseHtmlContentCreatorSettings> Settings::GetSettings(const ConnectionString& connection_string, bool create_unique_copy_of_settings);
template std::shared_ptr<CaseJsonContentCreatorSettings> Settings::GetSettings(const ConnectionString& connection_string, bool create_unique_copy_of_settings);
template std::shared_ptr<CaseQuestionnaireContentCreatorSettings> Settings::GetSettings(const ConnectionString& connection_string, bool create_unique_copy_of_settings);
template std::shared_ptr<CaseTextContentCreatorSettings> Settings::GetSettings(const ConnectionString& connection_string, bool create_unique_copy_of_settings);
template std::shared_ptr<ExportDataSettings> Settings::GetSettings(const ConnectionString& connection_string, bool create_unique_copy_of_settings);
template std::shared_ptr<ExtractBinaryDataSettings> Settings::GetSettings(const ConnectionString& connection_string, bool create_unique_copy_of_settings);
template std::shared_ptr<ExtractNotesSettings> Settings::GetSettings(const ConnectionString& connection_string, bool create_unique_copy_of_settings);


std::shared_ptr<const DataSourceSettings> Settings::GetDataSourceSettingsIfExist(const ConnectionString& connection_string)
{
    const PerDataSourceData* const per_data_source_data = GetPerDataSourceData(connection_string, false);

    return ( per_data_source_data != nullptr ) ? per_data_source_data->data_source_settings :
                                                 nullptr;
}


void Settings::Save()
{
    for( const auto& [settings_key, per_data_source_data] : m_perDataSourceData )
    {
        // the initial settings are ignored and instead the last used settings are saved
        // and will become the initial settings in the next run of the program
        if( settings_key.empty() )
        {
            m_mainSettingsDb->Write(MainSettingsDataKey, ToJson(*std::get<1>(m_perDataSourceData.back())));
        }

        else
        {
            m_perDataSourceSettingsDb->Write(settings_key, ToJson(*per_data_source_data));
        }
    }
}


std::string Settings::ToJson(PerDataSourceData& per_data_source_data)
{
    const std::unique_ptr<JsonStringWriter> json_writer = Json::CreateStringWriter();

    json_writer->BeginObject()
                .Write(JK::dataSource, *per_data_source_data.data_source_settings)
                .Write(JK::caseListing, *per_data_source_data.viewable_case_iterator_settings)
                .Write(JK::caseViewHtml, *per_data_source_data.case_html_content_creator_settings)
                .Write(JK::caseViewJson, *per_data_source_data.case_json_content_creator_settings)
                .Write(JK::caseViewQuestionnaire, *per_data_source_data.case_questionnaire_content_creator_settings)
                .Write(JK::caseViewText, *per_data_source_data.case_text_content_creator_settings)
                .Write(JK::exportData, *per_data_source_data.export_data_settings)
                .Write(JK::extractBinaryData, *per_data_source_data.extract_binary_data_settings)
                .Write(JK::extractNotes, *per_data_source_data.extract_notes_settings)
                .EndObject();

    return json_writer->ReleaseString();
}
