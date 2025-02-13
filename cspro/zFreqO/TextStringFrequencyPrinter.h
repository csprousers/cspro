#pragma once

#include <zFreqO/TextFrequencyPrinter.h>


class TextStringFrequencyPrinter : public TextFrequencyPrinter
{
public:
    TextStringFrequencyPrinter(int listing_width)
        :   TextFrequencyPrinter(FormatType::IgnorePageLength, listing_width)
    {
    }

    const std::wstring& GetText() const
    {
        return m_text;
    }

protected:
    void WriteLine(NullTerminatedString line) override
    {
        SO::AppendWithSeparator(m_text, line, '\n');
    }

private:
    std::wstring m_text;
};
