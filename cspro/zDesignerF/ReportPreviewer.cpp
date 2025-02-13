#include "StdAfx.h"
#include "ReportPreviewer.h"
#include <zEdit2O/ScintillaColorizer.h>
#include <zEngineO/ReportTokenizer.h>
#include <zHtml/SharedHtmlLocalFileServer.h>


struct ReportPreviewer::ReportVirtualFileMappingDetails
{
    SharedHtmlLocalFileServer file_server;
    std::unique_ptr<VirtualFileMapping> virtual_file_mapping;
};


ReportPreviewer::ReportPreviewer(wstring_view report_text, const LogicSettings& logic_settings)
{
    class DesignerReportTokenizer : public ReportTokenizer
    {
    public:
        void OnErrorUnbalancedEscapes(size_t /*line_number*/) override { }
        void OnErrorTokenNotEnded(const ReportToken& /*report_token*/) override { }
    };

    DesignerReportTokenizer report_tokenizer;

    if( !report_tokenizer.Tokenize(report_text, logic_settings) )
        throw CSProException(_T("There are errors that must be fixed before previewing the report. Compile the report to see the errors."));

    // without writing a full blown HTML parser, try to intelligently write out logic to the report:
    // - when in a head or script block, don't write out any logic
    // - when in a tag attribute value, write the logic escaped for HTML and quotes
    // - when elsewhere in a tag, write the logic escaped for HTML
    // - otherwise colorize the logic without formatting

    int lexer_language = Lexers::GetLexer_Logic(logic_settings);

    std::optional<TCHAR> tag_attribute_quote_char;
    TCHAR previous_report_char = 0;
    bool in_tag = false;
    bool building_tag_text = false;
    std::wstring tag_text;
    bool in_head_block = false;
    bool in_script_block = false;

    for( const ReportToken& report_token : report_tokenizer.GetReportTokens() )
    {
        // add logic
        if( report_token.type != ReportToken::Type::ReportText )
        {
            if( in_head_block || in_script_block )
                continue;

            if( tag_attribute_quote_char.has_value() )
            {
                std::wstring html = Encoders::ToHtml(report_token.text);
                SO::Replace(html, _T("\""), _T("&#34;"));
                SO::Replace(html, _T("'"), _T("&#39;"));
                m_reportHtml.append(html);
            }

            else if( in_tag )
            {
                m_reportHtml.append(Encoders::ToHtml(report_token.text));
            }

            else if( !SO::IsWhitespace(report_token.text) )
            {
                ScintillaColorizer colorizer(lexer_language, report_token.text);

                m_reportHtml.append(colorizer.GetHtml(ScintillaColorizer::HtmlProcessorType::ContentOnly));
            }
        }

        // add the report text directly and then update the report characteristics
        else
        {
            m_reportHtml.append(report_token.text);

            for( TCHAR ch : report_token.text )
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

                    auto process_tag = [&](const TCHAR* end_tag, bool& flag)
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

                    process_tag(_T("/head"), in_head_block);
                    process_tag(_T("/script"), in_script_block);
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
                    if( !std::iswspace(ch) )
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
}


ReportPreviewer::~ReportPreviewer()
{
}


std::wstring ReportPreviewer::GetReportUrl(const std::wstring& report_filename)
{
    if( m_reportVirtualFileMappingDetails == nullptr )
    {
        m_reportVirtualFileMappingDetails = std::make_unique<ReportVirtualFileMappingDetails>();

        m_reportVirtualFileMappingDetails->virtual_file_mapping = std::make_unique<VirtualFileMapping>(
            m_reportVirtualFileMappingDetails->file_server.CreateVirtualHtmlFile(PortableFunctions::PathGetDirectory(report_filename),
                [ html = UTF8Convert::WideToUTF8(m_reportHtml) ]()
                {
                    return html;
                }));
    }

    return m_reportVirtualFileMappingDetails->virtual_file_mapping->GetUrl();
}


std::unique_ptr<UriResolver> ReportPreviewer::GetReportUriResolver(std::wstring report_filename)
{
    std::wstring report_url = GetReportUrl(report_filename);
    return UriResolver::CreateUriDomain(report_url, report_url, std::move(report_filename));
}
