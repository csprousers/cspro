#pragma once

#include <zToolsO/Encoders.h>
#include <zUtilO/Interapp.h>
#include <sstream>


constexpr const TCHAR* DEFAULT_HTML_HEADER = _T("<!doctype html>\n<html lang=\"en\">\n<head>\n<meta charset=\"utf-8\">\n");


class HtmlWriter
{
public:
    virtual ~HtmlWriter()
    {
    }

    HtmlWriter(std::wostream& stream)
        :   m_stream(stream)
    {
    }

    HtmlWriter& operator<<(const TCHAR* text)
    {
        m_stream << text;
        return *this;
    }

    HtmlWriter& operator<<(const CString& text)
    {
        return operator<<(Encoders::ToHtml(text).c_str());
    }

    HtmlWriter& operator<<(const std::wstring& text)
    {
        return operator<<(Encoders::ToHtml(text).c_str());
    }

    void WriteDefaultHeader(wstring_view title, NullTerminatedString css)
    {
        m_stream << DEFAULT_HTML_HEADER
                 << _T("<title>") << Encoders::ToHtml(title).c_str() << _T("</title>");

        if( !css.empty() )
            m_stream << _T("<style>\n\n") << css.c_str() << _T("</style>");

        m_stream << _T("</head>");
    }

    void WriteDefaultHeader(wstring_view title, Html::CSS css)
    {
        WriteDefaultHeader(title, Html::GetCSS(css));
    }

private:
    std::wostream& m_stream;
};


class HtmlStringWriter : public HtmlWriter
{
private:
    HtmlStringWriter(std::unique_ptr<std::wostringstream> string_stream)
        :   HtmlWriter(*string_stream),
            m_stringStream(std::move(string_stream))
    {
    }

public:
    HtmlStringWriter()
        :   HtmlStringWriter(std::make_unique<std::wostringstream>())
    {
    }

    std::wstring str() const
    {
        return m_stringStream->str();
    }

    void clear()
    {
        m_stringStream->str(std::wstring());
        m_stringStream->clear();
    }

private:
    std::unique_ptr<std::wostringstream> m_stringStream;
};
