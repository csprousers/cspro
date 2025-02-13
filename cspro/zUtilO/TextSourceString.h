#pragma once

#include <zUtilO/TextSource.h>


class TextSourceString : public TextSource
{
public:
    TextSourceString(std::wstring filename, std::wstring text)
        :   TextSource(std::move(filename)),
            m_text(std::move(text))
    {
    }

    const std::wstring& GetText() const override
    {
        return m_text;
    }

    int64_t GetModifiedIteration() const override
    {
        return 0;
    }

private:
    const std::wstring m_text;
};
