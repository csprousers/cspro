#include "StdAfx.h"
#include "CSDocCompilerWorker.h"
#include <zAppO/PFF.h>


// --------------------------------------------------------------------------
// PffColorizer
// --------------------------------------------------------------------------

class PffColorizer
{
public:
    PffColorizer();

    std::string Colorize(std::string text);
    std::string ColorizeWord(const std::string& text);

private:
    enum class WordType { Heading, AppType, Attribute };
    static std::optional<WordType> GetWordType(const std::string& text);
    std::string ColorizeWord(WordType word_type, const std::string& text);

private:
    static std::unique_ptr<std::map<std::string, WordType>> m_words;
};


std::unique_ptr<std::map<std::string, PffColorizer::WordType>> PffColorizer::m_words;


PffColorizer::PffColorizer()
{
    if( m_words == nullptr )
    {
        m_words = std::make_unique<std::map<std::string, PffColorizer::WordType>>();

        auto add_words = [&](const WordType word_type, const std::vector<const char*>& words)
        {
            for( const char* const word : words )
                m_words->try_emplace(word, word_type);
        };

        add_words(WordType::Heading, PFF::GetHeadingWords());
        add_words(WordType::AppType, PFF::GetAppTypeWords());
        add_words(WordType::Attribute, PFF::GetAttributeWords());
    }
}


std::optional<PffColorizer::WordType> PffColorizer::GetWordType(const std::string& text)
{
    ASSERT(m_words != nullptr);

    const auto& lookup = m_words->find(text);

    if( lookup != m_words->cend() )
        return lookup->second;

    return std::nullopt;
}


std::string PffColorizer::ColorizeWord(const WordType word_type, const std::string& text)
{
    if( word_type == WordType::Heading )
    {
        return SO::Concatenate("<font color=\"#008\"><strong>", Encoders::ToHtml(text), "</strong></font>");
    }

    else if( word_type == WordType::AppType )
    {
        return SO::Concatenate("<strong>", Encoders::ToHtml(text), "</strong>");
    }

    else
    {
        ASSERT(word_type == WordType::Attribute);
        return SO::Concatenate("<font color=\"#008\">", Encoders::ToHtml(text), "</font>");
    }
}


std::string PffColorizer::Colorize(std::string text)
{
    ASSERT(text.find('\r') == std::string::npos);

    std::string html = "<div class=\"code_colorization indent\">";

    auto add_escaped_text = [&](const std::string_view text_sv) { html.append(Encoders::ToHtml(text_sv)); };
    size_t last_text_start_block = 0;
    size_t last_word_start_block = 0;
    bool in_word_block = false;
    bool word_block_ends_at_right_bracket = false;
    bool keep_processing = true;

    SO::ConvertTabsToSpaces(text);
    const std::string_view text_sv = text;

    for( size_t i = 0; keep_processing; ++i )
    {
        keep_processing = ( i < text.length() );

        const char ch = keep_processing ? text[i] : 0;
        const bool newline = ( ch == '\n' );

        if( keep_processing && ch != '=' && !newline && ( !std::isspace(ch) || word_block_ends_at_right_bracket ) )
        {
            if( !in_word_block )
            {
                in_word_block = true;
                last_word_start_block = i;
                word_block_ends_at_right_bracket = ( ch == '[' );
            }

            else if( word_block_ends_at_right_bracket && ch == ']' )
            {
                word_block_ends_at_right_bracket = false;
            }

            continue;
        }

        if( in_word_block )
        {
            const std::string word(text_sv.substr(last_word_start_block, i - last_word_start_block));
            const std::optional<WordType> word_type = GetWordType(word);

            const size_t last_text_block_length = i - last_text_start_block - ( word_type.has_value() ? word.length() : 0 );
            const std::string_view pre_word_text_sv = text_sv.substr(last_text_start_block, last_text_block_length);

            if( !pre_word_text_sv.empty()  && ( !keep_processing || newline || word_type.has_value() ) )
            {
                add_escaped_text(pre_word_text_sv);
                last_text_start_block = i;
            }

            if( word_type.has_value() )
            {
                html.append(ColorizeWord(*word_type, word));
                last_text_start_block = i;
            }

            in_word_block = false;
        }

        if( newline )
        {
            add_escaped_text(text_sv.substr(last_text_start_block, i - last_text_start_block));
            html.append("<br>");

            last_text_start_block = i + 1;
        }
    }

    // add any final text at the end
    add_escaped_text(text_sv.substr(last_text_start_block));

    html.append("</div>");

    return html;
}


std::string PffColorizer::ColorizeWord(const std::string& text)
{
    const std::optional<WordType> word_type = GetWordType(text);

    if( word_type.has_value() )
        return ColorizeWord(*word_type, text);

    throw CSProException("PFF files do not have the word: " + text);
}



// --------------------------------------------------------------------------
// CSDocCompilerWorker
// --------------------------------------------------------------------------

std::string CSDocCompilerWorker::PffEndHandler(const std::string& inner_text)
{
    PffColorizer colorizer;
    return colorizer.Colorize(TrimOnlyOneNewlineFromBothEnds(inner_text));
}


std::string CSDocCompilerWorker::PffColorEndHandler(const std::string& inner_text)
{
    PffColorizer colorizer;
    return colorizer.ColorizeWord(std::string(SO::Trim(inner_text)));
}
