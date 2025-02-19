#pragma once

#include <zFreqO/zFreqO.h>
#include <zFreqO/FrequencyPrinter.h>

class HtmlWriter;
namespace FileIO { class TextFile; }


class ZFREQO_API HtmlFrequencyPrinter : public FrequencyPrinter
{
public:
    // open the file and print frequencies to it
    HtmlFrequencyPrinter(std::string file_path);

    // print frequencies to a HtmlWriter
    HtmlFrequencyPrinter(HtmlWriter& html_writer, bool writer_header_and_footer);

    ~HtmlFrequencyPrinter();
    
    void StartFrequencyGroup() override;

    void Print(const FrequencyTable& frequency_table) override;

private:
    void PrintHeader();
    void PrintRowsAndTotal(const FrequencyTable& frequency_table);
    void PrintStatistics(const FrequencyTable& frequency_table);

private:
    std::unique_ptr<FileIO::TextFile> m_textFile;
    cs::shared_or_raw_ptr<HtmlWriter> m_htmlWriter;

    const bool m_writeHeaderAndFooter;
    bool m_expectingFrequencyGroupFirstTable;
};
