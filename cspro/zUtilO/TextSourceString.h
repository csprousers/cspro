#pragma once

#include <zUtilO/TextSource.h>


class TextSourceString : public TextSource
{
public:
    TextSourceString(std::string file_path, SharableString text)
        :   TextSource(std::move(file_path)),
            m_text(std::move(text))
    {
    }

    const std::string& GetText() const override
    {
        return *m_text;
    }

    SharableString GetTextAsSharableString() const
    {
        return m_text;
    }

    int64_t GetModifiedIteration() const override
    {
        return 0;
    }

private:
    SharableString m_text;
};
