#include "stdafx.h"
#include "JsonFileFrequencyPrinter.h"


JsonFileFrequencyPrinter::JsonFileFrequencyPrinter(const std::string& file_path)
    :   JsonFrequencyPrinter(Json::CreateFileWriter(UTF8_TODO::GetWide(file_path)))
{
    m_jsonWriter->BeginObject()
                 .BeginArray(JK::frequencies);
}


JsonFileFrequencyPrinter::~JsonFileFrequencyPrinter()
{
    m_jsonWriter->EndArray()
                 .EndObject();
}
