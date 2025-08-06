#pragma once

#include <zCapiO/zCapiO.h>

enum class EncodeType;


class CLASS_DECL_ZCAPIO CapiText
{
public:
    enum class Type { Question, Help };

    enum class Format { Html, ReportHtml, ReportMarkdown };
    static constexpr const char* FormatTexts[] = { "HTML", "HTML-Report", "Markdown-Report" };

    CapiText(SharableString text = SharableString(), Format format = Format::Html);

    // Returns the raw HTML or Markdown.
    const SharableString& GetText() const { return m_text; }

    // Returns the format.
    Format GetFormat() const { return m_format; }

    // Returns the encoding type for the format.
    EncodeType GetEncodeType() const;

    // Returns true if the format supports text template logic escapes: <? ... ?>
    bool FormatSupportsLogicEscapes() const { return ( m_format != Format::Html ); }

    // Returns the text as HTML. Markdown will be converted HTML.
    SharableString GetHtml() const { return GetHtml(m_text); }
    SharableString GetHtml(const SharableString& text) const;

    // Returns the text as HTML. Tilde text template delimiters in Markdown will be escaped
    // prior to the conversion to HTML so that ~~ is not treated as strikethrough.
    // For example:
    //   Markdown input:                           **~~P03_SEX~~**
    //   GetHtml:                                  <strong><del>P03_SEX</del></strong>
    //   GetHtmlWithEscapedTextTemplateDelimiters: <strong>&#126;&#126;P03_SEX&#126;&#126;</strong>
    SharableString GetHtmlWithEscapedTextTemplateDelimiters() const;

    // Returns or sets the program index for evaluating the question text.
    int GetProgramIndex() const             { return m_programIndex; }
    void SetProgramIndex(int program_index) { m_programIndex = program_index; }

    void WriteJson(JsonWriter& json_writer) const;
    void serialize(Serializer& ar);

private:
    SharableString m_text;
    Format m_format;
    int m_programIndex;
};


DECLARE_ENUM_JSON_SERIALIZER_CLASS(CapiText::Format, CLASS_DECL_ZCAPIO)



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

inline CapiText::CapiText(SharableString text/* = SharableString()*/, const Format format/* = Format::Html*/)
    :   m_text(std::move(text)),
        m_format(format),
        m_programIndex(-1)
{
}
