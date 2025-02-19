#include "stdafx.h"
#include "JsonStringFrequencyPrinter.h"


JsonStringFrequencyPrinter::JsonStringFrequencyPrinter(std::string& json_frequency_text)
    :   JsonFrequencyPrinter(Json::CreateStringWriter(json_frequency_text, JsonFormattingOptions::Compact))
{
    ASSERT(json_frequency_text.empty());

    m_jsonWriter->BeginArray();
}


JsonStringFrequencyPrinter::~JsonStringFrequencyPrinter()
{
    m_jsonWriter->EndArray();
}
