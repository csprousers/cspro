#include "StdAfx.h"
#include "TextTemplatePreviewer.h"
#include <zEdit2O/ScintillaColorizer.h>
#include <zEngineO/TextTemplateTokenizer.h>
#include <zHtml/SharedHtmlLocalFileServer.h>
#include <zViewO/MarkdownViewInput.h>


class TextTemplatePreviewer::DesignerTextTemplateTokenizer : public TextTemplateTokenizer
{
public:
    DesignerTextTemplateTokenizer() : TextTemplateTokenizer(true) { }

    void OnErrorUnbalancedEscapes(size_t /*line_number*/) override { }
    void OnErrorTokenNotEnded(const TextTemplateToken& /*token*/) override { }
};


struct TextTemplatePreviewer::VirtualFileMappingDetails
{
    SharedHtmlLocalFileServer file_server;
    std::unique_ptr<VirtualFileMapping> virtual_file_mapping;
};


TextTemplatePreviewer::TextTemplatePreviewer(std::string text_template_file_path, const std::string_view text_template_sv,
                                             const LogicSettings& logic_settings, const char* const action/* = "previewing"*/)
    :   m_textTemplateFilePath(std::move(text_template_file_path)),
        m_lexerLanguage(Lexers::GetLexer_Logic(logic_settings))
{
    DesignerTextTemplateTokenizer text_template_tokenizer;

    if( !text_template_tokenizer.Tokenize(text_template_sv, logic_settings) )
        throw CSProException("There are errors that must be fixed before %s the text template. Compile the text template to see the errors.", action);

    const FileExtensionAnalyzer extension_analyser(m_textTemplateFilePath);
    ASSERT(extension_analyser.IsTypeHtmlOrDerivable());

    m_html = extension_analyser.IsTypeHtml() ? CreateHtmlForHtml(text_template_tokenizer.GetTokens()) :
                                               CreateHtmlForMarkdown(text_template_tokenizer.GetTokens());
}


TextTemplatePreviewer::~TextTemplatePreviewer()
{
}


std::string TextTemplatePreviewer::CreateHtmlForHtml(const std::vector<TextTemplateToken>& tokens) const
{
    // without writing a full blown HTML parser, try to intelligently write out logic to the text template:
    // - when in a head or script block, don't write out any logic
    // - when in a tag attribute value, write the logic escaped for HTML and quotes
    // - when elsewhere in a tag, write the logic escaped for HTML
    // - otherwise colorize the logic without formatting

    std::optional<char> tag_attribute_quote_char;
    char previous_source_char = 0;
    bool in_tag = false;
    bool building_tag_text = false;
    std::string tag_text;
    bool in_head_block = false;
    bool in_script_block = false;

    std::string html;

    for( const TextTemplateToken& token : tokens )
    {
        // add logic
        if( token.type != TextTemplateToken::Type::DirectText )
        {
            if( in_head_block || in_script_block )
                continue;

            if( tag_attribute_quote_char.has_value() )
            {
                std::string this_html = Encoders::ToHtml(token.text);
                SO::Replace(this_html, "\"", "&#34;");
                SO::Replace(this_html, "'", "&#39;");
                html.append(this_html);
            }

            else if( in_tag )
            {
                html.append(Encoders::ToHtml(token.text));
            }

            else if( !SO::IsWhitespace(token.text) )
            {
                ScintillaColorizer colorizer(m_lexerLanguage, token.text);

                html.append(colorizer.GetHtml(ScintillaColorizer::HtmlProcessorType::ContentOnly));
            }
        }

        // add the text template's text directly and then update the report characteristics
        else
        {
            html.append(token.text);

            for( const char ch : token.text )
            {
                // in a tag attribute waiting for the end quote
                if( tag_attribute_quote_char.has_value() )
                {
                    if( ch == *tag_attribute_quote_char && previous_source_char != '\\' )
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

                previous_source_char = ch;
            }
        }
    }

    return html;
}


std::string TextTemplatePreviewer::CreateHtmlForMarkdown(const std::vector<TextTemplateToken>& tokens) const
{
    std::string markdown;

    for( const TextTemplateToken& token : tokens )
    {
        // add Markdown
        if( token.type == TextTemplateToken::Type::DirectText )
        {
            markdown.append(token.text);
        }

        // add logic
        else
        {
            ScintillaColorizer colorizer(m_lexerLanguage, token.text);
            const std::string html = colorizer.GetHtml(ScintillaColorizer::HtmlProcessorType::ContentOnly);
            ASSERT(!html.empty() && html.front() == '<' && html.back() == '>');

            // logic like ~~~"**"~~~ is colored like <span style="color:Fuchsia;">"**"</span>
            // which is a problem because Markdown syntax is processed within span-level tags (but not block-level tags),
            // so we must escape the content in between each of the tags
            size_t last_processed_end_tag_pos = 0;
            size_t start_tag_pos = 0;

            while( start_tag_pos < html.length() )
            {
                size_t end_tag_pos;

                start_tag_pos = html.find('<', start_tag_pos);

                if( ( start_tag_pos == std::string::npos ) ||
                    ( ( end_tag_pos = html.find('>', start_tag_pos + 1)) ) == std::string::npos )
                {
                    throw ProgrammingErrorException();
                }

                // append the logic (e.g., "**")
                if( start_tag_pos > last_processed_end_tag_pos )
                {
                    std::string logic = html.substr(last_processed_end_tag_pos + 1, start_tag_pos - last_processed_end_tag_pos - 1);

                    // the logic may have named entities such as &nbsp; so convert these back to their original values
                    // so that they are properly escaped for Markdown
                    logic = Encoders::FromHtmlAmpersandEscapes(std::move(logic));

                    markdown.append(Encoders::ToMarkdown(logic));
                }

                // append the tag
                markdown.append(html.substr(start_tag_pos, end_tag_pos - start_tag_pos + 1));

                last_processed_end_tag_pos = end_tag_pos;

                start_tag_pos = end_tag_pos + 1;
            }

            ASSERT(start_tag_pos == html.length());
        }
    }

    return MarkdownViewInput::ToViewableHtml(m_textTemplateFilePath, markdown);
}


std::string TextTemplatePreviewer::GetUrl()
{
    if( m_virtualFileMappingDetails == nullptr )
    {
        m_virtualFileMappingDetails = std::make_unique<VirtualFileMappingDetails>();

        m_virtualFileMappingDetails->virtual_file_mapping = std::make_unique<VirtualFileMapping>(
            m_virtualFileMappingDetails->file_server.CreateVirtualHtmlFile(PortableFunctions::PathGetDirectory(m_textTemplateFilePath),
                [&]()
                {
                    return m_html;
                }));
    }

    return m_virtualFileMappingDetails->virtual_file_mapping->GetUrl();
}


std::unique_ptr<UriResolver> TextTemplatePreviewer::GetUriResolver()
{
    const std::string url = GetUrl();
    return UriResolver::CreateUriDomain(url, url, m_textTemplateFilePath);
}
