#pragma once

#include <zUtilO/zUtilO.h>


// this logger can store either a single string or can be used to
// store many lines of logged text (with an optional color specified)

class CLASS_DECL_ZUTILO BasicLogger
{
public:
    enum class Color { Black, Red, DarkBlue, SlateBlue };


    std::wstring ToString() const;

    std::wstring ToHtml() const;

    bool IsEmpty() const
    {
        return m_spans.empty();
    }


    BasicLogger& operator=(std::wstring text)
    {
        m_spans.clear();
        Append(std::move(text));
        return *this;
    }

    void Append(Color color, std::wstring text)
    {
        m_spans.emplace_back(Span { color, std::move(text) });
    }

    void Append(std::wstring text)
    {
        Append(Color::Black, std::move(text));
    }


    void AppendLine(Color color, std::wstring text)
    {
        Append(color, std::move(text));
        Append(color, _T("\n"));
    }

    void AppendLine(std::wstring text = std::wstring())
    {
        AppendLine(Color::Black, std::move(text));
    }


    template<typename... Args>
    void Format(const TCHAR* formatter, Args const&... args)
    {
        operator=(FormatTextCS2WS(formatter, args...));
    }

    template<typename... Args>
    void AppendFormat(Color color, const TCHAR* formatter, Args const&... args)
    {
        Append(color, FormatTextCS2WS(formatter, args...));
    }

    template<typename... Args>
    void AppendFormat(const TCHAR* formatter, Args const&... args)
    {
        AppendFormat(Color::Black, formatter, args...);
    }

    template<typename... Args>
    void AppendFormatLine(Color color, const TCHAR* formatter, Args const&... args)
    {
        AppendLine(color, FormatTextCS2WS(formatter, args...));
    }

    template<typename... Args>
    void AppendFormatLine(const TCHAR* formatter, Args const&... args)
    {
        AppendFormatLine(Color::Black, formatter, args...);
    }

private:
    struct Span
    {
        BasicLogger::Color color;
        std::wstring text;
    };

    std::vector<Span> m_spans;
};
