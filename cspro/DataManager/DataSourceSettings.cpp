#include "StdAfx.h"
#include "DataSourceSettings.h"


CREATE_JSON_KEY(caseListingWidth)
CREATE_JSON_KEY(caseViewIndex)


DataSourceSettings::DataSourceSettings()
    :   m_dictionaryFileModifiedTime(0),
        m_defaultCasePageCommandId(ID_VIEW_CASE_HTML)
{
}


bool DataSourceSettings::HasUsableDictionaryFilePath() const
{
    try
    {
        return ( !m_dictionaryFilePath.empty() &&
                 m_dictionaryFileModifiedTime == PortableFunctions::FileModifiedTime<true>(m_dictionaryFilePath) );
    }
    catch(...) { return false; }
}


void DataSourceSettings::SetDictionaryFilePath(std::string dictionary_file_path)
{
    ASSERT(PortableFunctions::FileIsRegular(dictionary_file_path));

    m_dictionaryFilePath = std::move(dictionary_file_path);
    m_dictionaryFileModifiedTime = PortableFunctions::FileModifiedTime(m_dictionaryFilePath);
}


DataSourceSettings DataSourceSettings::CreateFromJson(const JsonNode& json_node)
{
    DataSourceSettings data_source_settings;

    const JsonNode dictionary_json_node = json_node.GetOrEmpty(JK::dictionary);

    if( !dictionary_json_node.IsEmpty() )
    {
        data_source_settings.m_dictionaryFilePath = dictionary_json_node.GetAbsolutePath(JK::path);
        data_source_settings.m_dictionaryFileModifiedTime = dictionary_json_node.GetDate(JK::modifiedTime);
    }

    data_source_settings.m_languageName = json_node.GetOrConstruct<std::string>(JK::language);

    data_source_settings.m_caseListingWidth = json_node.GetOptional<int>(JK::caseListingWidth);

    static_assert(ID_VIEW_CASE_HTML + 1 == ID_VIEW_CASE_JSON &&
                  ID_VIEW_CASE_HTML + 2 == ID_VIEW_CASE_TEXT &&
                  ID_VIEW_CASE_HTML + 3 == ID_VIEW_CASE_QUESTIONNAIRE);
    data_source_settings.m_defaultCasePageCommandId = std::min<UINT>(ID_VIEW_CASE_QUESTIONNAIRE,
                                                                     ID_VIEW_CASE_HTML + json_node.Get<UINT>(JK::caseViewIndex));

    return data_source_settings;
}


void DataSourceSettings::WriteJson(JsonWriter& json_writer) const
{
    json_writer.BeginObject();

    if( !m_dictionaryFilePath.empty() )
    {
        json_writer.BeginObject(JK::dictionary)
                   .WritePath(JK::path, m_dictionaryFilePath)
                   .WriteDate(JK::modifiedTime, m_dictionaryFileModifiedTime)
                   .EndObject();
    }

    json_writer.WriteIfNotBlank(JK::language, m_languageName);

    json_writer.WriteIfHasValue(JK::caseListingWidth, m_caseListingWidth);

    json_writer.Write(JK::caseViewIndex, m_defaultCasePageCommandId - ID_VIEW_CASE_HTML);

    json_writer.EndObject();
}
