#include "StdAfx.h"
#include "CSDocCompilerWorker.h"
#include "HtmlTags.h"
#include <zMarkdown/ParserCallback.h>


// --------------------------------------------------------------------------
// CSDocCompilerWorker::MarkdownCreator
//
// This class makes modifications to match the CSPro Document syntax.
// --------------------------------------------------------------------------

class CSDocCompilerWorker::MarkdownCreator : public Markdown::HtmlParserCallback
{
public:
    MarkdownCreator(CSDocCompilerWorker& worker);

protected:
    void EnterBlock(MD_BLOCKTYPE type, void* detail) override;
    void LeaveBlock(MD_BLOCKTYPE type, void* detail) override;
    void EnterSpanWorker(MD_SPANTYPE type, void* detail) override;
    void LeaveSpanWorker(MD_SPANTYPE type, void* detail) override;
    void ProcessOutputCode(std::string_view text_sv) override;

private:
    void EnterSpanH();
    void LeaveSpanH(unsigned h_level);

    void EnterSpanA(const MD_SPAN_A_DETAIL* detail);

    void EnterSpanImg(const MD_SPAN_IMG_DETAIL* detail);
    void LeaveSpanImg(const MD_SPAN_IMG_DETAIL* detail);

    void EnterBlockCode(const MD_BLOCK_CODE_DETAIL* detail);
    void LeaveBlockCode();

private:
    CSDocCompilerWorker& m_worker;
    std::optional<size_t> m_htmlPosAtTagStartH;
    std::optional<std::tuple<std::string, std::string>> m_codeDetails; // language name, code block
};


CSDocCompilerWorker::MarkdownCreator::MarkdownCreator(CSDocCompilerWorker& worker)
    :   m_worker(worker)
{
}


void CSDocCompilerWorker::MarkdownCreator::EnterBlock(const MD_BLOCKTYPE type, void* const detail)
{
    switch( type )
    {
        case MD_BLOCK_H:
        {
            EnterSpanH();
            break;
        }

        case MD_BLOCK_CODE:
        {
            EnterBlockCode(static_cast<const MD_BLOCK_CODE_DETAIL*>(detail));
            break;
        }

        default:
        {
            HtmlParserCallback::EnterBlock(type, detail);
            break;
        }
    }
}


void CSDocCompilerWorker::MarkdownCreator::LeaveBlock(const MD_BLOCKTYPE type, void* const detail)
{
    switch( type )
    {
        case MD_BLOCK_H:
        {
            LeaveSpanH(static_cast<const MD_BLOCK_H_DETAIL*>(detail)->level);
            break;
        }

        case MD_BLOCK_CODE:
        {
            LeaveBlockCode();
            break;
        }

        default:
        {
            HtmlParserCallback::LeaveBlock(type, detail);
            break;
        }
    }
}


void CSDocCompilerWorker::MarkdownCreator::EnterSpanWorker(const MD_SPANTYPE type, void* const detail)
{
    switch( type )
    {
        case MD_SPAN_EM:
        {
            m_html.append(HT::Italic[0]);
            break;
        }

        case MD_SPAN_STRONG:
        {
            m_html.append(HT::Bold[0]);
            break;
        }

        case MD_SPAN_A:
        {
            EnterSpanA(static_cast<const MD_SPAN_A_DETAIL*>(detail));
            break;
        }

        case MD_SPAN_IMG:
        {
            EnterSpanImg(static_cast<const MD_SPAN_IMG_DETAIL*>(detail));
            break;
        }

        default:
        {
            HtmlParserCallback::EnterSpanWorker(type, detail);
            break;
        }
    }
}


void CSDocCompilerWorker::MarkdownCreator::LeaveSpanWorker(const MD_SPANTYPE type, void* const detail)
{
    switch( type )
    {
        case MD_SPAN_EM:
        {
            m_html.append(HT::Italic[1]);
            break;
        }

        case MD_SPAN_STRONG:
        {
            m_html.append(HT::Bold[1]);
            break;
        }

        case MD_SPAN_IMG:
        {
            LeaveSpanImg(static_cast<const MD_SPAN_IMG_DETAIL*>(detail));
            break;
        }

        default:
        {
            HtmlParserCallback::LeaveSpanWorker(type, detail);
            break;
        }
    }
}


void CSDocCompilerWorker::MarkdownCreator::ProcessOutputCode(const std::string_view text_sv)
{
    if( m_codeDetails.has_value() )
    {
        // code block
        std::get<1>(*m_codeDetails).append(text_sv);
    }

    else
    {
        // inline code
        HtmlParserCallback::ProcessOutputCode(text_sv);
    }
}


void CSDocCompilerWorker::MarkdownCreator::EnterSpanH()
{
    if( m_htmlPosAtTagStartH.has_value() )
        throw ProgrammingErrorException();

    m_htmlPosAtTagStartH = m_html.size();
}


void CSDocCompilerWorker::MarkdownCreator::LeaveSpanH(const unsigned h_level)
{
    if( !m_htmlPosAtTagStartH.has_value() )
        throw ProgrammingErrorException();

    // treat # as the title
    if( h_level == 1 )
    {
        const std::string title_html = m_html.substr(*m_htmlPosAtTagStartH);
        std::string raw_title = Encoders::FromHtmlAmpersandEscapes(title_html);

        m_worker.TitleStartHandler({ });

        m_html.replace(m_html.begin() + *m_htmlPosAtTagStartH, m_html.end(),
                       m_worker.CreateTitleHtml(std::move(raw_title), title_html));
    }

    // treat ## as a subheader
    else if( h_level == 2 )
    {
        m_html.insert(*m_htmlPosAtTagStartH, HT::Subheader[0])
              .append(HT::Subheader[1]);
    }

    // use <h3...h6> for the rest of the levels
    else
    {
        std::string tag = FormatText("<h%d>", static_cast<int>(h_level));
        m_html.insert(*m_htmlPosAtTagStartH, tag);

        tag.insert(1, 1, '/');
        m_html.append(tag);
    }

    m_htmlPosAtTagStartH.reset();
}


void CSDocCompilerWorker::MarkdownCreator::EnterSpanA(const MD_SPAN_A_DETAIL* const detail)
{
    const size_t html_pos = m_html.length();
    render_attribute(&detail->href, EscapeType::ForUrl);

    std::string url = m_html.substr(html_pos);
    m_html.erase(html_pos);

    m_html.append(m_worker.CreateLinkStartHtml(std::move(url), false));

    if( detail->title.text != nullptr )
    {
        m_html.append(" title=\"");
        render_attribute(&detail->title, EscapeType::ForHtmlOrTag);
        m_html.push_back('"');
    }

    m_html.append(">");
}


void CSDocCompilerWorker::MarkdownCreator::EnterSpanImg(const MD_SPAN_IMG_DETAIL* const detail)
{
    const size_t html_pos = m_html.length();
    render_attribute(&detail->src, EscapeType::ForUrl);

    const std::string image_path = m_html.substr(html_pos);
    m_html.erase(html_pos);

    m_html.append(m_worker.CreateImageStartHtml(image_path));
}


void CSDocCompilerWorker::MarkdownCreator::LeaveSpanImg(const MD_SPAN_IMG_DETAIL* const detail)
{
    if( detail->title.text != nullptr )
    {
        m_html.append("\" title=\"");
        render_attribute(&detail->title, EscapeType::ForHtmlOrTag);
    }

    m_html.append("\">");
}


void CSDocCompilerWorker::MarkdownCreator::EnterBlockCode(const MD_BLOCK_CODE_DETAIL* const detail)
{
    if( m_codeDetails.has_value() )
        throw ProgrammingErrorException();

    std::string& language_name = std::get<0>(m_codeDetails.emplace());

    if( detail->lang.text != nullptr )
    {
        const size_t html_pos = m_html.length();
        render_attribute(&detail->lang, EscapeType::ForHtmlOrTag);

        language_name = m_html.substr(html_pos);
        m_html.erase(html_pos);
    }

    // a blank language name will mean CSPro
    if( SO::EqualsNoCase(language_name, "CSPro") )
        language_name.clear();

    if( !language_name.empty() )
        m_worker.ColorStartHandler(language_name);

    m_html.append(HT::ParagraphDiv_sv[0]);
}


void CSDocCompilerWorker::MarkdownCreator::LeaveBlockCode()
{
    if( !m_codeDetails.has_value() )
        throw ProgrammingErrorException();

    const auto& [language_name, code] = *m_codeDetails;

    m_html.append(language_name.empty() ? m_worker.LogicEndHandler(code) :
                                          m_worker.ColorEndHandler(code));
    m_codeDetails.reset();

    m_html.append(HT::ParagraphDiv_sv[1]);
}



// --------------------------------------------------------------------------
// CSDocCompilerWorker
// --------------------------------------------------------------------------

std::string CSDocCompilerWorker::MarkdownEndHandler(const std::string& inner_text)
{
    MarkdownCreator markdown_creator(*this);
    Markdown::Parse(markdown_creator, inner_text);
    return markdown_creator.ReleaseHtml();
}
