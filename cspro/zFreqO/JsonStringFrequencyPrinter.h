#pragma once

#include <zFreqO/zFreqO.h>
#include <zFreqO/JsonFrequencyPrinter.h>


class ZFREQO_API JsonStringFrequencyPrinter : public JsonFrequencyPrinter
{
public:
    JsonStringFrequencyPrinter(std::string& json_frequency_text);
    ~JsonStringFrequencyPrinter();

private:
    std::unique_ptr<JsonWriter> m_ownedJsonWriter;
};
