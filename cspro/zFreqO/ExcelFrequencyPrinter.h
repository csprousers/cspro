#pragma once

#include <zFreqO/zFreqO.h>
#include <zFreqO/FrequencyPrinter.h>

class ExcelFrequencyPrinterWorker;


class ZFREQO_API ExcelFrequencyPrinter : public FrequencyPrinter
{
public:
    ExcelFrequencyPrinter(NullTerminatedString filename);
    ~ExcelFrequencyPrinter();
    
    void StartFrequencyGroup() override { }

    void Print(const FrequencyTable& frequency_table) override;

private:
    std::unique_ptr<ExcelFrequencyPrinterWorker> m_worker;
};
