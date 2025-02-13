#pragma once

#include <zFreqO/zFreqO.h>
#include <zFreqO/FrequencyPrinter.h>
#include <zHtml/HtmlWriter.h>

class CStdioFileUnicode;
class HtmlWriter;


class ZFREQO_API HtmlFrequencyPrinter : public FrequencyPrinter
{
public:
    // print frequencies to the open file
    HtmlFrequencyPrinter(CStdioFileUnicode& file);

    // open the file and print frequencies to it
    HtmlFrequencyPrinter(NullTerminatedString filename);

    // print frequencies to the HtmlWriter stream
    HtmlFrequencyPrinter(HtmlWriter& html_writer, bool writer_header_and_footer);

    ~HtmlFrequencyPrinter();
    
    void StartFrequencyGroup() override;

    void Print(const FrequencyTable& frequency_table) override;

private:
    void PrintHeader();
    void PrintRowsAndTotal(const FrequencyTable& frequency_table);
    void PrintStatistics(const FrequencyTable& frequency_table);

private:
    std::unique_ptr<CStdioFileUnicode> m_ownedFile;
    CStdioFileUnicode* m_file;

    std::unique_ptr<HtmlWriter> m_ownedHtmlWriter;
    HtmlWriter& m_htmlWriter;

    bool m_writeHeaderAndFooter;
    bool m_expectingFrequencyGroupFirstTable = true;
};
