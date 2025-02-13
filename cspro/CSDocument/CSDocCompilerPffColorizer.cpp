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

    std::wstring Colorize(std::wstring text);
    std::wstring ColorizeWord(const std::wstring& text);

private:
    enum class WordType { Heading, AppType, Attribute };
    static std::optional<WordType> GetWordType(const std::wstring& text);
    std::wstring ColorizeWord(WordType word_type, const std::wstring& text);

private:
    static std::unique_ptr<std::map<StringNoCase, WordType>> m_words;
};


std::unique_ptr<std::map<StringNoCase, PffColorizer::WordType>> PffColorizer::m_words;


PffColorizer::PffColorizer()
{
    if( m_words == nullptr )
    {
        m_words = std::make_unique<std::map<StringNoCase, PffColorizer::WordType>>();

        auto add_words = [&](WordType word_type, const std::vector<const TCHAR*>& words)
        {
            for( const TCHAR* word : words )
                m_words->try_emplace(word, word_type);
        };

        add_words(WordType::Heading, PFF::GetHeadingWords());
        add_words(WordType::AppType, PFF::GetAppTypeWords());
        add_words(WordType::Attribute, PFF::GetAttributeWords());
    }
}


std::optional<PffColorizer::WordType> PffColorizer::GetWordType(const std::wstring& text)
{
    ASSERT(m_words != nullptr);

    const auto& lookup = m_words->find(text);

    if( lookup != m_words->cend() )
        return lookup->second;

    return std::nullopt;
}


std::wstring PffColorizer::ColorizeWord(WordType word_type, const std::wstring& text)
{
    if( word_type == WordType::Heading )
    {
        return _T("<font color=\"#008\"><strong>") + Encoders::ToHtml(text) + _T("</strong></font>");
    }

    else if( word_type == WordType::AppType )
    {
        return _T("<strong>") + Encoders::ToHtml(text) + _T("</strong>");
    }

    else
    {
        ASSERT(word_type == WordType::Attribute);
        return _T("<font color=\"#008\">") + Encoders::ToHtml(text) + _T("</font>");
    }
}


std::wstring PffColorizer::Colorize(std::wstring text)
{
    ASSERT(text.find('\r') == std::wstring::npos);

    std::wstring html = _T("<div class=\"code_colorization indent\">");

    auto add_escaped_text = [&](wstring_view text_sv) { html.append(Encoders::ToHtml(text_sv)); };
    size_t last_text_start_block = 0;
    size_t last_word_start_block = 0;
    bool in_word_block = false;
    bool word_block_ends_at_right_bracket = false;
    bool keep_processing = true;

    SO::ConvertTabsToSpaces(text);
    const wstring_view text_sv = text;

    for( size_t i = 0; keep_processing; ++i )
    {
        keep_processing = ( i < text.length() );

        const TCHAR ch = keep_processing ? text[i] : 0;
        const bool newline = ( ch == '\n' );

        if( keep_processing && ch != '=' && !newline && ( !std::iswspace(ch) || word_block_ends_at_right_bracket ) )
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
            const std::wstring word = text_sv.substr(last_word_start_block, i - last_word_start_block);
            const std::optional<WordType> word_type = GetWordType(word);

            const size_t last_text_block_length = i - last_text_start_block - ( word_type.has_value() ? word.length() : 0 );
            const wstring_view pre_word_text_sv = text_sv.substr(last_text_start_block, last_text_block_length);

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
            html.append(_T("<br />"));

            last_text_start_block = i + 1;
        }
    }

    // add any final text at the end
    add_escaped_text(text_sv.substr(last_text_start_block));

    html.append(_T("</div>"));

    return html;
}


std::wstring PffColorizer::ColorizeWord(const std::wstring& text)
{
    std::optional<WordType> word_type = GetWordType(text);

    if( word_type.has_value() )
        return ColorizeWord(*word_type, text);

    throw CSProException(_T("PFF files do not have the word: ") + text);
}



// --------------------------------------------------------------------------
// CSDocCompilerWorker
// --------------------------------------------------------------------------

std::wstring CSDocCompilerWorker::PffEndHandler(const std::wstring& inner_text)
{
    PffColorizer colorizer;
    return colorizer.Colorize(TrimOnlyOneNewlineFromBothEnds(inner_text));
}


std::wstring CSDocCompilerWorker::PffColorEndHandler(const std::wstring& inner_text)
{
    PffColorizer colorizer;
    return colorizer.ColorizeWord(SO::Trim(inner_text));
}
