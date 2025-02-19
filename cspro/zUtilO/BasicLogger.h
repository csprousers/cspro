#pragma once

#include <zUtilO/zUtilO.h>


// this logger can store either a single string or can be used to
// store many lines of logged text (with an optional color specified)

class CLASS_DECL_ZUTILO BasicLogger
{
public:
    enum class Color { Black, Red, DarkBlue, SlateBlue };

    std::string ToString() const;
    std::string ToHtml() const;

    bool IsEmpty() const { return m_spans.empty(); }

    BasicLogger& operator=(std::string text);

    void Append(Color color, std::string text);
    void Append(std::string text) { Append(Color::Black, std::move(text)); }

    void AppendLine(Color color, std::string text);
    void AppendLine(std::string text = std::string()) { AppendLine(Color::Black, std::move(text)); }

    template<typename... Args>
    void AppendFormat(Color color, const char* formatter, Args const&... args);

    template<typename... Args>
    void AppendFormat(const char* formatter, Args const&... args) { AppendFormat(Color::Black, formatter, args...); }

    template<typename... Args>
    void AppendFormatLine(Color color, const char* formatter, Args const&... args);

    template<typename... Args>
    void AppendFormatLine(const char* formatter, Args const&... args) { AppendFormatLine(Color::Black, formatter, args...); }

private:
    struct Span
    {
        BasicLogger::Color color;
        std::string text;
    };

    std::vector<Span> m_spans;
};


// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

inline BasicLogger& BasicLogger::operator=(std::string text)
{
    m_spans.clear();
    Append(std::move(text));
    return *this;
}


inline void BasicLogger::Append(const Color color, std::string text)
{
    m_spans.emplace_back(Span { color, std::move(text) });
}


inline void BasicLogger::AppendLine(const Color color, std::string text)
{
    Append(color, std::move(text));
    Append(color, "\n");
}


template<typename... Args>
void BasicLogger::AppendFormat(Color color, const char* formatter, Args const&... args)
{
    Append(color, FormatText(formatter, args...));
}


template<typename... Args>
void BasicLogger::AppendFormatLine(Color color, const char* formatter, Args const&... args)
{
    AppendLine(color, FormatText(formatter, args...));
}
