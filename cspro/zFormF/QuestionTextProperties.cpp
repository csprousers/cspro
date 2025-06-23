#include "StdAfx.h"
#include "QuestionTextProperties.h"
#include <zToolsO/WinSettings.h>
#include <zJson/Json.h>


CREATE_JSON_KEY(compilationInterval)
CREATE_JSON_KEY(eolAnnotationErrors)


std::shared_ptr<QuestionTextProperties> QuestionTextProperties::m_properties;


std::shared_ptr<const QuestionTextProperties> QuestionTextProperties::Get()
{
    if( m_properties == nullptr )
    {
        try
        {
            const std::string json_text = WinSettings::Read<std::string>(WinSettings::Type::QuestionText);
            m_properties = std::make_unique<QuestionTextProperties>(Json::FromJson<QuestionTextProperties>(json_text));
        }

        catch(...)
        {
            m_properties = std::make_unique<QuestionTextProperties>();
        }
    }

    return m_properties;
}


void QuestionTextProperties::Set(QuestionTextProperties properties)
{
    WinSettings::Write(WinSettings::Type::QuestionText, Json::ToJson(properties));

    // update the global properties object
    if( m_properties != nullptr )
        *m_properties = std::move(properties);
}


QuestionTextProperties QuestionTextProperties::CreateFromJson(const JsonNode& json_node)
{
    return
    {
        json_node.Get<CapiText::Format>(JK::format),
        json_node.Get<unsigned int>(JK::compilationInterval),
        json_node.Get<bool>(JK::eolAnnotationErrors)
    };
}


void QuestionTextProperties::WriteJson(JsonWriter& json_writer) const
{
    json_writer.BeginObject()
               .Write(JK::format, default_capi_text_format)
               .Write(JK::compilationInterval, automatic_compilation_seconds)
               .Write(JK::eolAnnotationErrors, errors_use_end_of_line_annotations)
               .EndObject();
}
