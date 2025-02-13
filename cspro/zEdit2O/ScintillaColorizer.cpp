#include "stdafx.h"
#include "ScintillaColorizer.h"
#include "LexerProperties.h"
#include <zToolsO/Encoders.h>
#include <zUtilO/PortableColor.h>
#include <zHtml/HtmlWriter.h>
#include <zScintilla/CSPro/CSPro.h>


// --------------------------------------------------------------------------
// Color and style analysis routines
// --------------------------------------------------------------------------

ScintillaColorizer::ScintillaColorizer(Scintilla::CScintillaCtrl& scintilla_ctrl, Sci_Position start_pos, Sci_Position end_pos)
    :   m_styleColorMap(LexerProperties::GetColors(scintilla_ctrl.GetLexer()))
{
    Sci_Position text_length = end_pos - start_pos;
    ASSERT(start_pos >= 0 && text_length >= 0 && end_pos <= scintilla_ctrl.GetLength());

    if( text_length == 0 )
        return;

    // colorize all of the text because text not currently on the screen may not have been colorized
    scintilla_ctrl.Colourise(0, end_pos);

    // get the styled text, which comes in a buffer with two ending nulls
    auto styled_text = std::make_unique_for_overwrite<char[]>(text_length * 2 + 2);

    Scintilla::TextRange text_range;
    text_range.chrg.cpMin = start_pos;
    text_range.chrg.cpMax = end_pos;
    text_range.lpstrText = styled_text.get();
    scintilla_ctrl.GetStyledText(&text_range);

    GenerateEntities(std::move(styled_text));
}


ScintillaColorizer::ScintillaColorizer(int lexer_language, std::string_view text_sv)
    :   m_styleColorMap(LexerProperties::GetColors(lexer_language))
{
    GenerateEntities(CSProScintilla::GetStyledText(lexer_language, LexerProperties::GetKeywords(lexer_language), text_sv));
}


ScintillaColorizer::ScintillaColorizer(int lexer_language, wstring_view text_sv)
    :   ScintillaColorizer(lexer_language, UTF8Convert::WideToUTF8(text_sv))
{
}


void ScintillaColorizer::GenerateEntities(std::unique_ptr<char[]> chars_and_styles)
{
    // the styled text is encoded as UTF-8, so combine the entities by style and then
    // convert each styled section to wide characters
    char* chars_and_styles_itr = chars_and_styles.get();

    // return on a blank string
    if( *chars_and_styles_itr == 0 )
    {
        ASSERT(chars_and_styles_itr[1] == 0);
        return;
    }

    char* entity_start_pos = chars_and_styles_itr++;
    char current_entity_style = *(chars_and_styles_itr++);

    // we need to keep track of the line position to properly handle conversions of tabs to spaces
    int position_in_line = 0;

    auto add_entity = [&]()
    {
        ASSERT(entity_start_pos < chars_and_styles_itr);

        // overwrite the styles, which are no longer needed, to lump all the UTF-8 characters together
        char* destination_char_itr = entity_start_pos + 1;

        for( char* source_char_itr = entity_start_pos + 2; source_char_itr < chars_and_styles_itr; ++destination_char_itr,
                                                                                                   source_char_itr += 2 )
        {
            *destination_char_itr = *source_char_itr;
        }

        Entity& entity = m_entities.emplace_back(Entity
            {
                UTF8Convert::UTF8ToWide(entity_start_pos, destination_char_itr - entity_start_pos),
                current_entity_style
            });

        // convert tabs to spaces
        position_in_line = SO::ConvertTabsToSpaces(entity.text, position_in_line);
    };

    // process the text styles
    for( ; *chars_and_styles_itr != 0; chars_and_styles_itr += 2 )
    {
        // if the style changed, process the last entity
        if( current_entity_style != chars_and_styles_itr[1] )
        {
            add_entity();
            entity_start_pos = chars_and_styles_itr;
            current_entity_style = chars_and_styles_itr[1];
        }
    }

    // add the last entity
    ASSERT(chars_and_styles_itr[0] == 0 && chars_and_styles_itr[1] == 0);

    add_entity();
}


COLORREF ScintillaColorizer::GetStyleColor(char style)
{
    const auto& style_lookup = m_styleColorMap.find(style);

    return ( style_lookup != m_styleColorMap.cend() ) ? style_lookup->second :
                                                        RGB(0, 0, 0);
}


const TCHAR* ScintillaColorizer::GetHtmlColor(COLORREF color)
{
    static std::map<COLORREF, std::wstring> color_to_name_map;
    const auto& color_name_lookup = color_to_name_map.find(color);

    if( color_name_lookup != color_to_name_map.cend() )
        return color_name_lookup->second.c_str();

    return color_to_name_map.try_emplace(color, PortableColor::FromCOLORREF(color).ToString()).first->second.c_str();
}



// --------------------------------------------------------------------------
// HTML generation
// --------------------------------------------------------------------------

namespace
{
    template<typename ET>
    std::wstring CreateHtml(ScintillaColorizer& colorizer, ScintillaColorizer::HtmlProcessor& html_processor,
                            const std::vector<ET>& entities)
    {
        std::wstringstream output;
        std::optional<COLORREF> current_color;

        auto end_current_color = [&]()
        {
            if( current_color.has_value() )
            {
                output << _T("</span>");
                current_color.reset();
            }
        };

        html_processor.WriteHtmlHeader(output);

        for( const auto& entity : entities )
        {
            // set the color if it has changed from the previous entity or if this entity has tags
            COLORREF color = colorizer.GetStyleColor(entity.style);

            bool set_color = ( !current_color.has_value() || current_color != color );
            bool end_color = false;

            if constexpr(std::is_same_v<ET, ScintillaColorizer::ExtendedEntity>)
            {
                if( !entity.entity_spanning_tags.empty() || !entity.entity_specific_tags.empty() )
                {
                    set_color = true;
                    end_color = true;
                }
            }

            // end the current color
            if( set_color )
                end_current_color();

            // write out any pre-color (entity-spanning) start tags
            if constexpr(std::is_same_v<ET, ScintillaColorizer::ExtendedEntity>)
            {
                for( const auto& [start_tag, end_tag] : entity.entity_spanning_tags )
                    output << start_tag.c_str();
            }

            // set the color
            if( set_color )
            {
                output << _T("<span style=\"color:") << colorizer.GetHtmlColor(color) << _T(";\">");
                current_color = color;
            }

            // write out any post-color (entity-specific) start tags
            if constexpr(std::is_same_v<ET, ScintillaColorizer::ExtendedEntity>)
            {
                for( const auto& [start_tag, end_tag] : entity.entity_specific_tags )
                    output << start_tag.c_str();
            }

            // write out the entity
            output << Encoders::ToHtml(entity.text).c_str();

            // write out any post-color (entity-specific) end tags
            if constexpr(std::is_same_v<ET, ScintillaColorizer::ExtendedEntity>)
            {
                for( auto itr = entity.entity_specific_tags.crbegin(); itr != entity.entity_specific_tags.crend(); ++itr )
                    output << std::get<1>(*itr).c_str();
            }

            // end the color if necessary
            if( end_color )
                end_current_color();

            // write out any pre-color (entity-spanning) end tags
            if constexpr(std::is_same_v<ET, ScintillaColorizer::ExtendedEntity>)
            {
                for( auto itr = entity.entity_spanning_tags.crbegin(); itr != entity.entity_spanning_tags.crend(); ++itr )
                    output << std::get<1>(*itr).c_str();
            }
        }

        end_current_color();

        html_processor.WriteHtmlFooter(output);

        return output.str();
    }


    class DefaultHtmlProcessor : public ScintillaColorizer::HtmlProcessor
    {
    public:
        DefaultHtmlProcessor(ScintillaColorizer::HtmlProcessorType html_processor_type)
            :   m_type(html_processor_type)
        {
        }

        void WriteHtmlHeader(std::wstringstream& output) const override
        {
            if( m_type == ScintillaColorizer::HtmlProcessorType::FullHtml )
            {
                output << DEFAULT_HTML_HEADER
                       << _T("<!--cspro-->\n")
                          _T("<title>CSPro</title>")
                          _T("</head>\n")
                          _T("<body>\n")
                          _T("<div");
            }

            else if( m_type == ScintillaColorizer::HtmlProcessorType::SpanOnly )
            {
                output << _T("<span");
            }

            if( m_type != ScintillaColorizer::HtmlProcessorType::ContentOnly )
            {
                output << _T(" style=\"word-wrap:break-word; margin:0px; padding:0px; border:0px; background-color:#ffffff; font-family: Consolas, monaco, monospace; font-size:10pt;\">");
            }
        }

        void WriteHtmlFooter(std::wstringstream& output) const override
        {
            if( m_type == ScintillaColorizer::HtmlProcessorType::FullHtml )
            {
                output << _T("</div>\n")
                          _T("</body>\n")
                          _T("</html>");
            }

            else if( m_type == ScintillaColorizer::HtmlProcessorType::SpanOnly )
            {
                output << _T("</span>");
            }
        }

    private:
        ScintillaColorizer::HtmlProcessorType m_type;
    };
}


std::wstring ScintillaColorizer::GetHtml(std::variant<HtmlProcessorType, HtmlProcessor*> html_processor_or_type)
{
    if( std::holds_alternative<HtmlProcessorType>(html_processor_or_type) )
    {
        // use the default HTML processor if one was not supplied
        DefaultHtmlProcessor default_html_processor(std::get<HtmlProcessorType>(html_processor_or_type));
        return CreateHtml(*this, default_html_processor, m_entities);
    }

    else
    {
        HtmlProcessor* html_processor = std::get<HtmlProcessor*>(html_processor_or_type);
        ASSERT(html_processor != nullptr);

        // run the HTML generation either on entities or extended entities
        std::vector<ExtendedEntity> extended_entities = html_processor->GetExtendedEntities(m_entities);

        return extended_entities.empty() ? CreateHtml(*this, *html_processor, m_entities):
                                           CreateHtml(*this, *html_processor, extended_entities);
    }
}



// --------------------------------------------------------------------------
// CSPro Users code generation
// --------------------------------------------------------------------------

std::wstring ScintillaColorizer::GetCSProUsersForumCode()
{
    std::wstringstream output;
    std::optional<COLORREF> current_color;

    auto end_current_color = [&]()
    {
        if( current_color.has_value() )
        {
            output << _T("[/color]");
            current_color.reset();
        }
    };

    output << _T("[cspro]");

    for( const Entity& entity : m_entities )
    {
        // set the color if there are non-whitespace characters and the color has changed from the previous color
        COLORREF color = GetStyleColor(entity.style);

        if( !SO::IsWhitespace(entity.text) && ( !current_color.has_value() || *current_color != color ) )
        {
            end_current_color();

            // only set the color if it not the default black color
            if( color != RGB(0, 0, 0) )
            {
                output << _T("[color=") << GetHtmlColor(color) << _T("]");
                current_color = color;
            }
        }

        // replace consecutive spaces with an actual space character
        std::wstring modified_text = entity.text;
        ASSERT(modified_text.find('\t') == std::wstring::npos);

        SO::RecursiveReplace(modified_text, _T("  "), _T("[sp][/sp] "));

        output << modified_text.c_str();
    }

    end_current_color();

    output << _T("[/cspro]");

    return output.str();
}


namespace
{
    struct CSProUsersBlogHtmlProcessor : public ScintillaColorizer::HtmlProcessor
    {
        void WriteHtmlHeader(std::wstringstream& output) const override
        {
            output << _T("<div style=\"margin: 0px; padding: 1em; ")
                      _T("border-radius: 3px; ")
                      _T("line-height: 1.5; ")
                      _T("font-family: 'Inconsolata', monospace; font-size: 10pt; ")
                      _T("color: rgb(51, 51, 51); ")
                      _T("background-color: rgb(232, 232, 232);\">\n");
        }

        void WriteHtmlFooter(std::wstringstream& output) const override
        {
            output << _T("\n</div>\n");
        }
    };
}


std::wstring ScintillaColorizer::GetCSProUsersBlogCode()
{
    CSProUsersBlogHtmlProcessor html_processor;
    return GetHtml(&html_processor);
}
