#include "StdAfx.h"
#include "TextTemplatePreviewer.h"
#include <zEditO/ScintillaColorizer.h>
#include <zHtml/SharedHtmlLocalFileServer.h>
#include <zMarkdown/Markdown.h>
#include <zCapiO/CapiText.h>
#include <zEngineO/Nodes/TextTemplate.h>


namespace
{
    constexpr bool HighlightLogicAndShowDelimitersInOutput = true;
}


struct TextTemplatePreviewer::ConstructionData
{
    const LogicSettings& logic_settings;
    int lexer_language;
    const char* action;
    std::unique_ptr<ErrorSuppressingTextTemplateTokenizer> text_template_tokenizer;
};


struct TextTemplatePreviewer::VirtualFileMappingDetails
{
    SharedHtmlLocalFileServer file_server;
    std::unique_ptr<VirtualFileMapping> virtual_file_mapping;
};


TextTemplatePreviewer::TextTemplatePreviewer(const EncodeType encode_type, const LogicSettings& logic_settings,
                                             const std::string_view text_template_sv, std::optional<std::string> text_template_file_path,
                                             const char* const action/* = "previewing"*/)
    :   m_textTemplateFilePath(std::move(text_template_file_path))
{
    ASSERT(action != nullptr);

    ConstructionData data
    {
        logic_settings,
        Lexers::GetLexer_Logic(logic_settings),
        action
    };

    TokenizeTemplate(data, text_template_sv);

    m_html = ( encode_type == EncodeType::Html ) ? ProcessHtml(data) :
                                                   ProcessMarkdown(data);
}


TextTemplatePreviewer::TextTemplatePreviewer(const std::string& text_template_file_path, const std::string_view text_template_sv,
                                             const LogicSettings& logic_settings, const char* const action/* = "previewing"*/)
    :   TextTemplatePreviewer(GetEncodeType(text_template_file_path), logic_settings,
                              text_template_sv, text_template_file_path, action)
{
}


TextTemplatePreviewer::TextTemplatePreviewer(const CapiText& capi_text, const LogicSettings& logic_settings)
    :   TextTemplatePreviewer(capi_text.GetEncodeType(), logic_settings,
                              capi_text.GetText().GetString(), std::nullopt)
{
}


TextTemplatePreviewer::~TextTemplatePreviewer()
{
}


EncodeType TextTemplatePreviewer::GetEncodeType(const std::string& text_template_file_path)
{
    const FileExtensionAnalyzer extension_analyser(text_template_file_path);
    ASSERT(extension_analyser.IsTypeHtmlOrDerivable());

    return extension_analyser.IsTypeHtml() ? EncodeType::Html :
                                             EncodeType::Markdown;
}


void TextTemplatePreviewer::TokenizeTemplate(ConstructionData& data, const std::string_view text_template_sv)
{
    data.text_template_tokenizer = std::make_unique<ErrorSuppressingTextTemplateTokenizer>(true);

    if( !data.text_template_tokenizer->Tokenize(text_template_sv, data.logic_settings) )
        throw CSProException("There are errors that must be fixed before %s the text template. Compile the text template to see the errors.", data.action);
}


void TextTemplatePreviewer::AppendColorizedLogic(std::string& html, const TextTemplateToken::Type type, const std::string& colorized_tag_html)
{
    if constexpr(HighlightLogicAndShowDelimitersInOutput)
    {
        // the background color is the color used by the cspro-capi-fill class at 37.5% opacity
        constexpr const char* SpanStart = "<span style=\"font-family: Consolas, monaco, monospace; "
                                                        "background-color: #e7f6f660\">";

        const std::tuple<const char*, const char*> delimiters = TextTemplateToken::GetEscapedDelimiters(type);

        html.append(SpanStart)
            .append(std::get<0>(delimiters))
            .append(colorized_tag_html)
            .append(std::get<1>(delimiters))
            .append("</span>");
    }

    else
    {
        html.append(colorized_tag_html);
    }
}


std::string TextTemplatePreviewer::ProcessHtml(ConstructionData& data)
{
    // if there are no fills or logic, there is no reason to process this further
    if( data.text_template_tokenizer->IsOnlyDirectTextUsed() )
    {
        ASSERT(data.text_template_tokenizer->GetTokens().size() == 1);
        return data.text_template_tokenizer->GetTokens().front().text;
    }

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

    for( const TextTemplateToken& token : data.text_template_tokenizer->GetTokens() )
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
                ScintillaColorizer colorizer(data.lexer_language, token.text);
                AppendColorizedLogic(html, token.type, colorizer.GetHtml(ScintillaColorizer::HtmlProcessorType::ContentOnly));
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


std::string TextTemplatePreviewer::ProcessMarkdown(ConstructionData& data) const
{
    std::string result;
    bool result_is_already_html;

    // if there are no fills or logic, there is no reason to process this further
    if( data.text_template_tokenizer->IsOnlyDirectTextUsed() )
    {
        ASSERT(data.text_template_tokenizer->GetTokens().size() == 1);
        result = data.text_template_tokenizer->GetTokens().front().text;
        result_is_already_html = false;
    }

    else
    {
        // when the Markdown contains tags, it will be converted to HTML
        // and passed to ProcessHtml so that rules like not writing out logic
        // in a head or script block are followed
        if( data.text_template_tokenizer->DirectTextContains('<') )
        {
            result = ProcessMarkdownWithHtmlTagSupport(data);
            result_is_already_html = true;
        }

        // otherwise use the quicker Markdown-specific converter
        else
        {
            result = ProcessMarkdownWithNoHtmlTags(data);
            result_is_already_html = false;
        }
    }

    // potentially create a HTML document
    if( m_textTemplateFilePath.has_value() )
    {
        const std::string title = Path::GetFilenameWithoutExtension(*m_textTemplateFilePath);
        CssProvider css_provider(Html::CSS::Markdown, false);

        return result_is_already_html ? Markdown::ToHtmlDocumentFromConvertedMarkdown(title, result, &css_provider) :
                                        Markdown::ToHtmlDocument(title, result, &css_provider);
    }

    // otherwise convert Markdown to HTML when necessary
    else if( !result_is_already_html )
    {
        return Markdown::ToHtml(result);
    }

    else
    {
        return result;
    }
}


std::string TextTemplatePreviewer::ProcessMarkdownWithNoHtmlTags(ConstructionData& data)
{
    ASSERT(!data.text_template_tokenizer->IsOnlyDirectTextUsed());

    std::string markdown;

    for( const TextTemplateToken& token : data.text_template_tokenizer->GetTokens() )
    {
        // add Markdown
        if( token.type == TextTemplateToken::Type::DirectText )
        {
            markdown.append(token.text);
        }

        // add logic
        else
        {
            ScintillaColorizer colorizer(data.lexer_language, token.text);
            const std::string html = colorizer.GetHtml(ScintillaColorizer::HtmlProcessorType::ContentOnly);
            ASSERT(!html.empty() && html.front() == '<' && html.back() == '>');

            // logic like ~~~"**"~~~ is colored like <span style="color:Fuchsia;">"**"</span>
            // which is a problem because Markdown syntax is processed within span-level tags (but not block-level tags),
            // so we must escape the content in between each of the tags
            std::string escaped_html;
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

                // add the logic (e.g., "**")
                if( start_tag_pos > last_processed_end_tag_pos )
                {
                    std::string logic = html.substr(last_processed_end_tag_pos + 1, start_tag_pos - last_processed_end_tag_pos - 1);

                    // the logic may have named entities such as &nbsp; so convert these back to their original values
                    // so that they are properly escaped for Markdown
                    logic = Encoders::FromHtmlAmpersandEscapes(std::move(logic));

                    escaped_html.append(Encoders::ToMarkdown(logic));
                }

                // add the tag
                escaped_html.append(html.substr(start_tag_pos, end_tag_pos - start_tag_pos + 1));

                last_processed_end_tag_pos = end_tag_pos;

                start_tag_pos = end_tag_pos + 1;
            }

            ASSERT(start_tag_pos == html.length());

            AppendColorizedLogic(markdown, token.type, escaped_html);
        }
    }

    return markdown;
}


std::string TextTemplatePreviewer::ProcessMarkdownWithHtmlTagSupport(ConstructionData& data)
{
    ASSERT(!data.text_template_tokenizer->IsOnlyDirectTextUsed());

    // build Markdown with all fills and logic replaced with the replacement text
    std::string html = data.text_template_tokenizer->ConvertDirectText(
        [&](std::string& direct_text)
        {
            direct_text = Markdown::ToHtml(direct_text);
        });

    // now tokenize and process this constructed HTML
    TokenizeTemplate(data, html);

    return ProcessHtml(data);
}


std::string TextTemplatePreviewer::GetUrl()
{
    ASSERT(m_textTemplateFilePath.has_value());

    if( m_virtualFileMappingDetails == nullptr )
    {
        m_virtualFileMappingDetails = std::make_unique<VirtualFileMappingDetails>();

        m_virtualFileMappingDetails->virtual_file_mapping = std::make_unique<VirtualFileMapping>(
            m_virtualFileMappingDetails->file_server.CreateVirtualHtmlFile(PortableFunctions::PathGetDirectory(ValueOrDefault(m_textTemplateFilePath)),
                [&]()
                {
                    return m_html;
                }));
    }

    return m_virtualFileMappingDetails->virtual_file_mapping->GetUrl();
}


std::unique_ptr<UriResolver> TextTemplatePreviewer::GetUriResolver()
{
    ASSERT(m_textTemplateFilePath.has_value());

    const std::string url = GetUrl();
    return UriResolver::CreateUriDomain(url, url, ValueOrDefault(m_textTemplateFilePath));
}
