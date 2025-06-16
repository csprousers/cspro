#include "StdAfx.h"
#include "CaseJsonContentCreator.h"
#include "CaseJsonContentCreatorSettings.h"
#include "ViewOptionsHelper.h"
#include <zEditO/ScintillaColorizer.h>
#include <zCaseO/CaseJsonSerializer.h>


// --------------------------------------------------------------------------
// CaseJsonContentCreatorSettings
// --------------------------------------------------------------------------

CaseJsonContentCreatorSettings CaseJsonContentCreatorSettings::CreateFromJson(const JsonNode& json_node)
{
    return CaseJsonContentCreatorSettings
    {
        ( json_node.GetFromStringOptions(CSProperty::jsonFormat, { CSValue::compact, CSValue::pretty }) == 0 ),
        json_node.Get<bool>(CSProperty::verbose),
        json_node.Get<bool>(CSProperty::writeBlankValues),
        json_node.Get<bool>(CSProperty::writeLabels),
        ( json_node.GetFromStringOptions(CSProperty::binaryDataFormat, { CSValue::dataUrl, CSValue::suppress }) == 0 )
    };
}


void CaseJsonContentCreatorSettings::WriteJson(JsonWriter& json_writer) const
{
    json_writer.BeginObject()
               .Write(CSProperty::jsonFormat, json_format_compact ? CSValue::compact : CSValue::pretty)
               .Write(CSProperty::verbose, verbose)
               .Write(CSProperty::writeBlankValues, write_blank_values)
               .Write(CSProperty::writeLabels, write_labels)
               .Write(CSProperty::binaryDataFormat, binary_data_urls ? CSValue::dataUrl : CSValue::suppress)
               .EndObject();
}



// --------------------------------------------------------------------------
// CaseJsonContentCreator_CaseJsonWriterSerializerHelper
// --------------------------------------------------------------------------

class CaseJsonContentCreator_CaseJsonWriterSerializerHelper : public CaseJsonWriterSerializerHelper
{
public:
    CaseJsonContentCreator_CaseJsonWriterSerializerHelper(std::shared_ptr<const CaseJsonContentCreatorSettings> case_json_content_creator_settings)
        :   m_caseJsonContentCreatorSettings(std::move(case_json_content_creator_settings))
    {
        ASSERT(m_caseJsonContentCreatorSettings != nullptr);
    }

    void UpdateOptions()
    {
        SetVerbose(m_caseJsonContentCreatorSettings->verbose);
        SetWriteBlankValues(m_caseJsonContentCreatorSettings->write_blank_values);
        SetWriteLabels(m_caseJsonContentCreatorSettings->write_labels);

        if( m_caseJsonContentCreatorSettings->binary_data_urls )
        {
            // the default behavior is to write binary data as a data URL
            ClearBinaryDataWriter();
        }

        else
        {
            SetBinaryDataWriter(
                [&](JsonWriter& /*json_writer*/, const BinaryCaseItem& /*binary_case_item*/, const CaseItemIndex& /*index*/)
                {
                    // suppress writing binary data
                });
        }
    }

    bool GetWriteVectorClock() const override
    {
        return GetVerbose();
    }

private:
    std::shared_ptr<const CaseJsonContentCreatorSettings> m_caseJsonContentCreatorSettings;
};



// --------------------------------------------------------------------------
// CaseJsonContentCreator
// --------------------------------------------------------------------------

CaseJsonContentCreator::CaseJsonContentCreator(CaseHoldingDoc& case_holding_doc)
    :   CaseContentCreatorBase(case_holding_doc),
        m_caseJsonContentCreatorSettings(case_holding_doc.GetSettings<CaseJsonContentCreatorSettings>()),
        m_caseJsonWriterSerializerHelper(std::make_unique<CaseJsonContentCreator_CaseJsonWriterSerializerHelper>(m_caseJsonContentCreatorSettings))
{
}


CaseJsonContentCreator::~CaseJsonContentCreator()
{
}


const wchar_t* CaseJsonContentCreator::GetSaveTitle() const
{
    return L"Save Case JSON";
}


std::vector<const char*> CaseJsonContentCreator::GetSaveFormats() const
{
    return { FileExtensions::Json,
             FileExtensions::HTML };
}


UINT CaseJsonContentCreator::GetViewOptionsMenuResourceId() const
{
    return IDR_VIEW_CASE_JSON;
}


bool CaseJsonContentCreator::ProcessViewOptionsMenu(const std::variant<UINT, CCmdUI*> data)
{
    switch( ViewOptionsHelper::GetCommandId(data) )
    {
        case ID_VIEW_OPTIONS_JSON_COMPACT_JSON:       return ViewOptionsHelper::HandleBooleanCheck(data, m_caseJsonContentCreatorSettings->json_format_compact);
        case ID_VIEW_OPTIONS_JSON_VERBOSE:            return ViewOptionsHelper::HandleBooleanCheck(data, m_caseJsonContentCreatorSettings->verbose);
        case ID_VIEW_OPTIONS_JSON_WRITE_LABELS:       return ViewOptionsHelper::HandleBooleanCheck(data, m_caseJsonContentCreatorSettings->write_labels);
        case ID_VIEW_OPTIONS_JSON_WRITE_BLANK_VALUES: return ViewOptionsHelper::HandleBooleanCheck(data, m_caseJsonContentCreatorSettings->write_blank_values);
        case ID_VIEW_OPTIONS_JSON_WRITE_DATA_URLS:    return ViewOptionsHelper::HandleBooleanCheck(data, m_caseJsonContentCreatorSettings->binary_data_urls);
        default:                                      return ViewOptionsHelper::HandleUnknownCommand(data);
    }
}


SharableString CaseJsonContentCreator::GetTextContentWorker()
{
    ASSERT(m_dataCase != nullptr);

    const JsonFormattingOptions formatting_options = m_caseJsonContentCreatorSettings->json_format_compact ? JsonFormattingOptions::Compact :
                                                                                                             JsonFormattingOptions::PrettySpacing;

    const std::unique_ptr<JsonStringWriter> json_writer = Json::CreateStringWriter(formatting_options);

    assert_cast<CaseJsonContentCreator_CaseJsonWriterSerializerHelper&>(*m_caseJsonWriterSerializerHelper).UpdateOptions();
    const auto case_json_writer_serializer_holder = json_writer->GetSerializerHelper().Register(m_caseJsonWriterSerializerHelper);

    json_writer->Write(*m_dataCase);

    return json_writer->ReleaseSharableString();
}


SharableString CaseJsonContentCreator::GetHtmlContentWorker(bool /*embed_resources = false*/)
{
    // colorize the JSON
    ScintillaColorizer colorizer(SCLEX_JSON, CaseJsonContentCreator::GetTextContentWorker().GetString());
    return colorizer.GetHtml(ScintillaColorizer::HtmlProcessorType::FullHtml);
}
