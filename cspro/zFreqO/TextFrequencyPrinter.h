#pragma once

#include <zFreqO/zFreqO.h>
#include <zFreqO/FrequencyPrinter.h>


class ZFREQO_API TextFrequencyPrinter : public FrequencyPrinter
{
protected:
    enum class FormatType { IgnorePageLength, UsePageLength, UsePageLengthAddFormFeedBeforeFirstFrequency };

    TextFrequencyPrinter(FormatType format_type, int listing_width);

public:
    void StartFrequencyGroup() override;

    void Print(const FrequencyTable& frequency_table) override;

protected:
    virtual void WriteLine(NullTerminatedString line) = 0;

private:
    FormatType m_formatType;
    int m_lineLength;
    int m_pageNumber;
    int m_lineNumber;
    bool m_expectingPrinterFirstTable;
    bool m_expectingFrequencyGroupFirstTable;
};
