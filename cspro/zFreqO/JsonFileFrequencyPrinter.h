#pragma once

#include <zFreqO/zFreqO.h>
#include <zFreqO/JsonFrequencyPrinter.h>


class ZFREQO_API JsonFileFrequencyPrinter : public JsonFrequencyPrinter
{
public:
    JsonFileFrequencyPrinter(const std::wstring& filename);
    ~JsonFileFrequencyPrinter();

private:
    std::unique_ptr<JsonWriter> m_ownedJsonWriter;
};
