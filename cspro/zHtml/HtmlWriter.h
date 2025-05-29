#pragma once

#include <zToolsO/Encoders.h>
#include <zUtilO/Interapp.h>
#include <sstream>


// --------------------------------------------------------------------------
// HtmlWriter: a class that facilicates writing HTML to a stream
//
// HtmlStringWriter: a HtmlWriter that writes to a string
//
// when using both:
//     - write text that does not need to be encoded, or has already been
//       encoded, using operator<<(const char*) or WriteRaw
//
//    - write text that has to be encoded using operator<<(std::string_view)
//      or WriteEncoded
//
// other methods that can be used:
//    - WriteDefaultHeader
//    - WriteTagValue
//    - WriteNewline
// --------------------------------------------------------------------------

class HtmlWriter
{
public:
    static constexpr std::string_view DefaultHeader_sv = "<!doctype html>\n<html lang=\"en\">\n<head>\n<meta charset=\"utf-8\">\n";

    HtmlWriter(cs::non_null_shared_or_raw_ptr<std::ostream> stream);
    virtual ~HtmlWriter() { }

    HtmlWriter& WriteRaw(std::string_view text_sv);
    HtmlWriter& operator<<(const char* text);

    HtmlWriter& WriteEncoded(std::string_view text_sv);
    HtmlWriter& operator<<(std::string_view text_sv) { return WriteEncoded(text_sv); }

    HtmlWriter& WriteDefaultHeader(std::string_view title_sv, std::string_view css_sv);
    HtmlWriter& WriteDefaultHeader(std::string_view title_sv, Html::CSS css);

    HtmlWriter& WriteTagValue(std::string_view text_sv) { return WriteRaw(Encoders::ToHtmlTagValue(text_sv)); }

    HtmlWriter& WriteNewline() { return WriteRaw("<br>"); }

protected:
    cs::non_null_shared_or_raw_ptr<std::ostream> m_stream;
};


class HtmlStringWriter : public HtmlWriter
{
public:
    using stream_type = std::ostringstream;

    HtmlStringWriter() : HtmlWriter(std::make_unique<stream_type>()) { }

    std::string str() const { return static_cast<stream_type*>(m_stream.get())->str(); }

    size_t size();
    size_t length() { return size(); }
};



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

inline HtmlWriter::HtmlWriter(cs::non_null_shared_or_raw_ptr<std::ostream> stream)
    :   m_stream(std::move(stream))
{
}


inline HtmlWriter& HtmlWriter::WriteRaw(const std::string_view text_sv)
{
    m_stream->write(text_sv.data(), text_sv.length());

    return *this;
}


inline HtmlWriter& HtmlWriter::operator<<(const char* text)
{
    *m_stream << text;

    return *this;
}


inline HtmlWriter& HtmlWriter::WriteEncoded(const std::string_view text_sv)
{
    const std::unique_ptr<const std::string> encoded_html = Encoders::ToHtmlWorker(text_sv);

    return WriteRaw(( encoded_html != nullptr ) ? std::string_view(*encoded_html) :
                                                  text_sv);
}


inline HtmlWriter& HtmlWriter::WriteDefaultHeader(const std::string_view title_sv, const std::string_view css_sv)
{
    WriteRaw(DefaultHeader_sv);

    *m_stream << "<title>" << title_sv << "</title>";

    if( !css_sv.empty() )
    {
        *m_stream << "<style>\n\n";
        WriteRaw(css_sv);
        *m_stream << "</style>";
    }

    *m_stream << "</head>";

    return *this;
}


inline HtmlWriter& HtmlWriter::WriteDefaultHeader(const std::string_view title_sv, const Html::CSS css)
{
    return WriteDefaultHeader(title_sv, Html::GetCSS(css));
}


inline size_t HtmlStringWriter::size()
{
    m_stream->seekp(0, std::ios::end);
    return static_cast<size_t>(m_stream->tellp());
}
