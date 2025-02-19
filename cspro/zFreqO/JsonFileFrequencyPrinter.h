#pragma once

#include <zFreqO/zFreqO.h>
#include <zFreqO/JsonFrequencyPrinter.h>


class ZFREQO_API JsonFileFrequencyPrinter : public JsonFrequencyPrinter
{
public:
    JsonFileFrequencyPrinter(const std::string& file_path);
    ~JsonFileFrequencyPrinter();
};
