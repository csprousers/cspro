#pragma once

#ifdef WIN_DESKTOP

#include <zFreqO/zFreqO.h>
#include <zFreqO/FrequencyPrinter.h>

class TableFrequencyPrinterWorker;


class ZFREQO_API TableFrequencyPrinter : public FrequencyPrinter
{
public:
    TableFrequencyPrinter(std::string file_path);
    ~TableFrequencyPrinter();
    
    void StartFrequencyGroup() override { }

    void Print(const FrequencyTable& frequency_table) override;

private:
    std::string m_filePath;
    std::unique_ptr<TableFrequencyPrinterWorker> m_worker;
};

#endif
