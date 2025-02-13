#pragma once

#ifdef WIN_DESKTOP

#include <zFreqO/zFreqO.h>
#include <zFreqO/FrequencyPrinter.h>

class TableFrequencyPrinterWorker;


class ZFREQO_API TableFrequencyPrinter : public FrequencyPrinter
{
public:
    TableFrequencyPrinter(std::wstring filename);
    ~TableFrequencyPrinter();
    
    void StartFrequencyGroup() override { }

    void Print(const FrequencyTable& frequency_table) override;

private:
    std::wstring m_filename;
    std::unique_ptr<TableFrequencyPrinterWorker> m_worker;
};

#endif
