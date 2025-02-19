#pragma once

#include <zFreqO/TextFrequencyPrinter.h>


class TextStringFrequencyPrinter : public TextFrequencyPrinter
{
public:
    TextStringFrequencyPrinter(const int listing_width)
        :   TextFrequencyPrinter(FormatType::IgnorePageLength, listing_width)
    {
    }

    const std::string& GetText() const
    {
        return m_text;
    }

protected:
    void WriteLine(const std::string_view line_sv) override
    {
        SO::AppendWithSeparator(m_text, line_sv, '\n');
    }

private:
    std::string m_text;
};
