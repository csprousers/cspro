#pragma once

#include <zMarkdown/zMarkdown.h>
#include <zMarkdown/Markdown.h>
#include <external/md4c/md4c.h>


// --------------------------------------------------------------------------
// Markdown::ParserCallback is a class-based alternative to using MD4C's
// C-style callbacks. Subclasses can throw exceptions and they will
// properly handled.
//
// Markdown::HtmlParserCallback is a subclass that creates HTML based on the
// implementation in md4c-html.c.
// --------------------------------------------------------------------------

class Markdown::ParserCallback
{
public:
    virtual ~ParserCallback() { }

    virtual void EnterBlock(MD_BLOCKTYPE type, void* detail) = 0;
    virtual void LeaveBlock(MD_BLOCKTYPE type, void* detail) = 0;
    virtual void EnterSpan(MD_SPANTYPE type, void* detail) = 0;
    virtual void LeaveSpan(MD_SPANTYPE type, void* detail) = 0;
    virtual void ProcessOutput(MD_TEXTTYPE type, std::string_view text_sv) = 0;
};


class ZMARKDOWN_API Markdown::HtmlParserCallback : public Markdown::ParserCallback
{
public:
    HtmlParserCallback();

    std::string ReleaseHtml() { return std::move(m_html); }

protected:
    void EnterBlock(MD_BLOCKTYPE type, void* detail) override;

    void LeaveBlock(MD_BLOCKTYPE type, void* detail) override;

    void EnterSpan(MD_SPANTYPE type, void* detail) override final;
    virtual void EnterSpanWorker(MD_SPANTYPE type, void* detail);

    void LeaveSpan(MD_SPANTYPE type, void* detail) override final;
    virtual void LeaveSpanWorker(MD_SPANTYPE type, void* detail);

    void ProcessOutput(MD_TEXTTYPE type, std::string_view text_sv) override;

private:
    enum class EscapeType { Verbatim, ForHtml, ForUrl };
    void render(std::string_view text_sv, EscapeType escape_type);
    void render_utf8_codepoint(unsigned codepoint, EscapeType escape_type);
    void render_entity(std::string_view text_sv, EscapeType escape_type);
    void render_attribute(const MD_ATTRIBUTE* attr, EscapeType escape_type);
    void render_open_ol_block(const MD_BLOCK_OL_DETAIL* det);
    void render_open_li_block(const MD_BLOCK_LI_DETAIL* det);
    void render_open_code_block(const MD_BLOCK_CODE_DETAIL* det);
    void render_open_td_block(const MD_CHAR* cell_type, const MD_BLOCK_TD_DETAIL* det);
    void render_open_a_span(const MD_SPAN_A_DETAIL* det);
    void render_open_img_span(const MD_SPAN_IMG_DETAIL* det);
    void render_close_img_span(const MD_SPAN_IMG_DETAIL* det);
    void render_open_wikilink_span(const MD_SPAN_WIKILINK_DETAIL* det);

private:
    std::string m_html;
    int m_imageNestingLevel;
};
