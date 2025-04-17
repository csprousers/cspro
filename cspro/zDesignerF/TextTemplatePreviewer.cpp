#include "StdAfx.h"
#include "TextTemplatePreviewer.h"
#include <zEdit2O/ScintillaColorizer.h>
#include <zEngineO/TextTemplateTokenizer.h>
#include <zHtml/SharedHtmlLocalFileServer.h>
#include <zViewO/MarkdownViewInput.h>


class ReportPreviewer::DesignerReportTokenizer : public ReportTokenizer
{
public:
    void OnErrorUnbalancedEscapes(size_t /*line_number*/) override { }
    void OnErrorTokenNotEnded(const ReportToken& /*report_token*/) override { }
};


struct ReportPreviewer::ReportVirtualFileMappingDetails
{
    SharedHtmlLocalFileServer file_server;
    std::unique_ptr<VirtualFileMapping> virtual_file_mapping;
};


ReportPreviewer::ReportPreviewer(std::string report_file_path, const std::string_view report_text_sv,
                                 const LogicSettings& logic_settings, const char* const action/* = "previewing"*/)
    :   m_reportFilePath(std::move(report_file_path)),
        m_lexerLanguage(Lexers::GetLexer_Logic(logic_settings))
{
    DesignerReportTokenizer report_tokenizer;

    if( !report_tokenizer.Tokenize(report_text_sv, logic_settings) )
        throw CSProException("There are errors that must be fixed before %s the report. Compile the report to see the errors.", action);

    const FileExtensionAnalyzer report_extension_analyser(m_reportFilePath);
    ASSERT(report_extension_analyser.IsTypeHtmlOrDerivable());

    m_reportHtml = ( report_extension_analyser.IsTypeHtml() ) ? CreateHtmlForHtml(report_tokenizer.GetReportTokens()) :
                                                                CreateHtmlForMarkdown(report_tokenizer.GetReportTokens());
}


ReportPreviewer::~ReportPreviewer()
{
}


std::string ReportPreviewer::CreateHtmlForHtml(const std::vector<ReportToken>& report_tokens) const
{
    // without writing a full blown HTML parser, try to intelligently write out logic to the report:
    // - when in a head or script block, don't write out any logic
    // - when in a tag attribute value, write the logic escaped for HTML and quotes
    // - when elsewhere in a tag, write the logic escaped for HTML
    // - otherwise colorize the logic without formatting

    std::optional<char> tag_attribute_quote_char;
    char previous_report_char = 0;
    bool in_tag = false;
    bool building_tag_text = false;
    std::string tag_text;
    bool in_head_block = false;
    bool in_script_block = false;

    std::string report_html;

    for( const ReportToken& report_token : report_tokens )
    {
        // add logic
        if( report_token.type != ReportToken::Type::ReportText )
        {
            if( in_head_block || in_script_block )
                continue;

            if( tag_attribute_quote_char.has_value() )
            {
                std::string html = Encoders::ToHtml(report_token.text);
                SO::Replace(html, "\"", "&#34;");
                SO::Replace(html, "'", "&#39;");
                report_html.append(html);
            }

            else if( in_tag )
            {
                report_html.append(Encoders::ToHtml(report_token.text));
            }

            else if( !SO::IsWhitespace(report_token.text) )
            {
                ScintillaColorizer colorizer(m_lexerLanguage, report_token.text);

                report_html.append(colorizer.GetHtml(ScintillaColorizer::HtmlProcessorType::ContentOnly));
            }
        }

        // add the report text directly and then update the report characteristics
        else
        {
            report_html.append(report_token.text);

            for( const char ch : report_token.text )
            {
                // in a tag attribute waiting for the end quote
                if( tag_attribute_quote_char.has_value() )
                {
                    if( ch == *tag_attribute_quote_char && previous_report_char != '\\' )
                        tag_attribute_quote_char.reset();
                }

                // in a tag starting a quote
                else if( in_tag && is_quotemark(ch) )
                {
                    tag_attribute_quote_char = ch;
                }

                // ending a tag
                else if( in_tag && ch == '>' )
                {
                    in_tag = false;
                    building_tag_text = false;

                    auto process_tag = [&](const char* const end_tag, bool& flag)
                    {
                        if( SO::EqualsNoCase(tag_text, end_tag) )
                        {
                            flag = false;
                        }

                        else if( SO::EqualsNoCase(tag_text, end_tag + 1) )
                        {
                            flag = true;
                        }
                    };

                    process_tag("/head", in_head_block);
                    process_tag("/script", in_script_block);
                }

                // starting a tag
                else if( !in_tag && ch == '<' )
                {
                    in_tag = true;
                    building_tag_text = true;
                    tag_text.clear();
                }

                // building the tag text
                else if( building_tag_text )
                {
                    if( !std::isspace(ch) )
                    {
                        tag_text.push_back(ch);
                    }

                    else if( !tag_text.empty() )
                    {
                        building_tag_text = false;
                    }
                }

                previous_report_char = ch;
            }
        }
    }

    return report_html;
}


std::string ReportPreviewer::CreateHtmlForMarkdown(const std::vector<ReportToken>& report_tokens) const
{
    std::string markdown;

    for( const ReportToken& report_token : report_tokens )
    {
        // add Markdown
        if( report_token.type == ReportToken::Type::ReportText )
        {
            markdown.append(report_token.text);
        }

        // add logic
        else
        {
            ScintillaColorizer colorizer(m_lexerLanguage, report_token.text);
            const std::string html = colorizer.GetHtml(ScintillaColorizer::HtmlProcessorType::ContentOnly);
            ASSERT(!html.empty() && html.front() == '<' && html.back() == '>');
            markdown.append(html);
        }
    }

    return MarkdownViewInput::ToViewableHtml(m_reportFilePath, markdown);
}


std::string ReportPreviewer::GetReportUrl()
{
    if( m_reportVirtualFileMappingDetails == nullptr )
    {
        m_reportVirtualFileMappingDetails = std::make_unique<ReportVirtualFileMappingDetails>();

        m_reportVirtualFileMappingDetails->virtual_file_mapping = std::make_unique<VirtualFileMapping>(
            m_reportVirtualFileMappingDetails->file_server.CreateVirtualHtmlFile(PortableFunctions::PathGetDirectory(m_reportFilePath),
                [&]()
                {
                    return m_reportHtml;
                }));
    }

    return m_reportVirtualFileMappingDetails->virtual_file_mapping->GetUrl();
}


std::unique_ptr<UriResolver> ReportPreviewer::GetReportUriResolver()
{
    const std::string report_url = GetReportUrl();
    return UriResolver::CreateUriDomain(report_url, report_url, m_reportFilePath);
}
