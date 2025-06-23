#pragma once

#include <zCapiO/CapiText.h>


struct QuestionTextProperties
{
    CapiText::Format default_capi_text_format  = CapiText::Format::Html;
    unsigned int automatic_compilation_seconds = 1;
    bool errors_use_end_of_line_annotations    = false;

    // Returns a non-null pointer to the global settings.
    static std::shared_ptr<const QuestionTextProperties> Get();

    // Sets the global settings.
    static void Set(QuestionTextProperties properties);

    // serialization
    static QuestionTextProperties CreateFromJson(const JsonNode& json_node);
    void WriteJson(JsonWriter& json_writer) const;

private:
    static std::shared_ptr<QuestionTextProperties> m_properties;
};
