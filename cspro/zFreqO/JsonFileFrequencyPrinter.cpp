#include "stdafx.h"
#include "JsonFileFrequencyPrinter.h"


JsonFileFrequencyPrinter::JsonFileFrequencyPrinter(const std::wstring& filename)
    :   m_ownedJsonWriter(Json::CreateFileWriter(filename))
{
    m_jsonWriter = m_ownedJsonWriter.get();

    m_jsonWriter->BeginObject()
                 .BeginArray(JK::frequencies);
}


JsonFileFrequencyPrinter::~JsonFileFrequencyPrinter()
{
    m_jsonWriter->EndArray()
                 .EndObject();
}
