#include "stdafx.h"
#include "JsonStringFrequencyPrinter.h"


JsonStringFrequencyPrinter::JsonStringFrequencyPrinter(std::string& json_frequency_text)
    :   m_ownedJsonWriter(Json::CreateStringWriter(json_frequency_text, JsonFormattingOptions::Compact))
{
    ASSERT(json_frequency_text.empty());

    m_jsonWriter = m_ownedJsonWriter.get();

    m_jsonWriter->BeginArray();
}


JsonStringFrequencyPrinter::~JsonStringFrequencyPrinter()
{
    m_jsonWriter->EndArray();
}
