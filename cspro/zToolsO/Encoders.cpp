#include "StdAfx.h"
#include "Encoders.h"
#include "base64.h"
#include "TextConverter.h"
#include <zHtml/HtmlWriter.h>
#include <regex>


static_assert(std::string_view(Encoders::HexChars).length() == 16);
static_assert(std::string_view(EncoderEscapes::Representations).length() == std::string_view(EncoderEscapes::Sequences).length());
static_assert(std::string_view(Encoders::JsonEscapeRepresentations).length() == std::string_view(Encoders::JsonEscapeSequences).length());


// --------------------------------------------------------------------------
// HTML
// --------------------------------------------------------------------------

constexpr std::string_view MarkdownEscapeChars_sv = " \n\t<>&\\`*_{}[]()#+-.!|";
constexpr std::string_view HtmlEscapeChars_sv     = MarkdownEscapeChars_sv.substr(0, 6);
constexpr std::string_view HtmlTag_lt_sv          = "&lt;";
constexpr std::string_view HtmlTag_gt_sv          = "&gt;";
constexpr std::string_view HtmlTag_amp_sv         = "&amp;";
constexpr std::string_view HtmlTag_nbsp_sv        = "&nbsp;";
constexpr std::string_view HtmlTag_br_sv          = "<br>";


std::unique_ptr<std::string> Encoders::ToHtmlMarkdownWorker(const std::string_view text_sv, std::string_view escape_chars_sv, const bool escape_spaces)
{
    constexpr size_t IndexSpace         = 0;
    constexpr size_t IndexNewline       = 1;
    constexpr size_t IndexTab           = 2;
    constexpr size_t IndexLessThan      = 3;
    constexpr size_t IndexGreaterThan   = 4;
    constexpr size_t IndexAmpersand     = 5;

    constexpr size_t IndexFirstNonSpace = 3;

    ASSERT81(escape_chars_sv.length() > IndexAmpersand &&
             escape_chars_sv[IndexSpace] == ' ' &&
             escape_chars_sv[IndexNewline] == '\n' &&
             escape_chars_sv[IndexTab] == '\t' &&
             escape_chars_sv[IndexLessThan] == '<' &&
             escape_chars_sv[IndexGreaterThan] == '>' &&
             escape_chars_sv[IndexAmpersand] == '&');

    if( !escape_spaces )
        escape_chars_sv.remove_prefix(IndexFirstNonSpace);

    const auto& text_sv_cbegin = text_sv.cbegin();
    const auto& text_sv_cend = text_sv.cend();
    auto text_sv_itr = text_sv_cbegin;

    // the escaped_text object will only be created when characters must be escaped
    std::unique_ptr<std::string> escaped_text;

    auto is_previous_char_space = [&]()
    {
        // force a non-breaking space on the first character, which will allow chained calls to be properly spaced
        // e.g., "abc " " xyz" -> "abc &nbsp;xyz"
        if( text_sv_itr == text_sv_cbegin )
            return true;

        const char prev_ch = ( escaped_text != nullptr ) ? escaped_text->back() :
                                                           *( text_sv_itr - 1 );

        // also treat end tags as space characters so that a string like "a\n b" is encoded
        // with an escaped space following the newline
        return ( prev_ch == ' ' || prev_ch == '>' );
    };

    // escape a few characters
    for( ; text_sv_itr != text_sv_cend; ++text_sv_itr )
    {
        const char ch = *text_sv_itr;
        size_t escape_index = escape_chars_sv.find(ch);

        if( !escape_spaces && escape_index != std::string_view::npos )
            escape_index += IndexFirstNonSpace;

        // space characters will be escaped only when preceeded by another space character
        if( ( escape_index == std::string_view::npos ) ||
            ( escape_index == IndexSpace ) && !is_previous_char_space() )
        {
            // the character does not need to be escaped, but if already escaping characters, add it to escaped_text
            if( escaped_text != nullptr )
                escaped_text->push_back(ch);

            continue;
        }

        // at this point, all remaining characters to be processed are escaped
        if( escaped_text == nullptr )
        {
            escaped_text = std::make_unique<std::string>(text_sv_cbegin, text_sv_itr);
            escaped_text->reserve(text_sv.length());
        }

        // ' ' escaped to &nbsp;
        if( escape_index == IndexSpace )
        {
            escaped_text->append(HtmlTag_nbsp_sv);
        }

        // \n escaped to <br>
        else if( escape_index == IndexNewline )
        {
            escaped_text->append(HtmlTag_br_sv);
        }

        // < escaped to &lt;
        else if( escape_index == IndexLessThan )
        {
            escaped_text->append(HtmlTag_lt_sv);
        }

        // > escaped to &gt;
        else if( escape_index == IndexGreaterThan )
        {
            escaped_text->append(HtmlTag_gt_sv);
        }

        // & escaped to &amp;
        else if( escape_index == IndexAmpersand )
        {
            escaped_text->append(HtmlTag_amp_sv);
        }

        // \t escaped to four spaces, to "&nbsp; &nbsp; " or " &nbsp; &nbsp;"
        else if( escape_index == IndexTab )
        {
            constexpr std::string_view TabEscape_sv = " &nbsp; &nbsp; ";
            constexpr size_t TabEscapeLength = TabEscape_sv.length() - 1;

            escaped_text->append(is_previous_char_space() ? ( TabEscape_sv.data() + 1 ) : TabEscape_sv.data(), TabEscapeLength);
        }

        // Markdown escapes
        else
        {
            ASSERT(escape_index <= MarkdownEscapeChars_sv.length());
            escaped_text->push_back('\\');
            escaped_text->push_back(ch);
        }
    }

    return escaped_text;
}


std::unique_ptr<std::string> Encoders::ToHtmlWorker(const std::string_view text_sv, const bool escape_spaces/* = true*/)
{
    return ToHtmlMarkdownWorker(text_sv, HtmlEscapeChars_sv, escape_spaces);
}


std::string Encoders::ToHtmlTagValue(const std::string_view text_sv)
{
    // encodes to HTML and then escapes quotes and newlines
    std::string html = ToHtml(text_sv);

    SO::Replace(html, "\"", "&quot;");
    SO::Replace(html, HtmlTag_br_sv, "&#013;");

    return html;
}


std::string Encoders::FromHtmlAmpersandEscapes(std::string text)
{
    const size_t first_ampersand_pos = text.find('&');

    if( first_ampersand_pos != std::string::npos )
    {
        SO::Replace(text, HtmlTag_lt_sv, "<", first_ampersand_pos);
        SO::Replace(text, HtmlTag_gt_sv, ">", first_ampersand_pos);
        SO::Replace(text, HtmlTag_nbsp_sv, " ", first_ampersand_pos);
        SO::Replace(text, HtmlTag_amp_sv, "&", first_ampersand_pos);
    }

    return text;
}


std::string Encoders::ToPreformattedTextHtml(const std::string_view title_sv, const std::string_view body_sv)
{
    return SO::Concatenate(HtmlWriter::DefaultHeader_sv,
                           "<title>",
                           ToHtml(title_sv, false),
                           "</title>\n</head>\n<body>\n<pre>",
                           ToHtml(body_sv, false),
                           "</pre>\n</body>\n</html>");
}



// --------------------------------------------------------------------------
// Markdown
// --------------------------------------------------------------------------

std::unique_ptr<std::string> Encoders::ToMarkdownWorker(const std::string_view text_sv)
{
    return ToHtmlMarkdownWorker(text_sv, MarkdownEscapeChars_sv, true);
}



// --------------------------------------------------------------------------
// PERCENT-ENCODING + URI
// --------------------------------------------------------------------------

std::unique_ptr<std::string> Encoders::ToPercentEncodingWorker(const std::string_view text_sv, const char* const additional_characters_allowed)
{
    const auto& text_sv_cbegin = text_sv.cbegin();
    const auto& text_sv_cend = text_sv.cend();
    auto text_sv_itr = text_sv_cbegin;

    // the encoded_text object will only be created when characters must be escaped
    std::unique_ptr<std::string> encoded_text;

    for( ; text_sv_itr != text_sv_cend; ++text_sv_itr )
    {
        const char ch = *text_sv_itr;

        if( Encoders::IsPercentEncodingUnreservedCharacter(ch) ||
            ( additional_characters_allowed != nullptr && strchr(additional_characters_allowed, ch) != nullptr ) )
        {
            // the character should not be escaped, but if already escaping characters, add it
            if( encoded_text != nullptr )
                encoded_text->push_back(ch);
        }

        // handle a character that needs escaping
        else
        {
            if( encoded_text == nullptr )
            {
                encoded_text = std::make_unique<std::string>(text_sv_cbegin, text_sv_itr);
                encoded_text->reserve(text_sv.length());
            }

            encoded_text->push_back('%');
            encoded_text->push_back(Encoders::HexChars[static_cast<byte>(ch) >> 4]);
            encoded_text->push_back(Encoders::HexChars[static_cast<byte>(ch) & 0x0F]);
        }
    }

    return encoded_text;
}


std::unique_ptr<std::string> Encoders::ToPercentEncodingWorker(const std::string_view text_sv)
{
    return ToPercentEncodingWorker(text_sv, nullptr);
}


std::unique_ptr<std::string> Encoders::ToUriWorker(const std::string_view text_sv, const bool allow_hash_to_specify_fragment/* = true*/)
{
    // list from https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/encodeURI
    constexpr const char* AdditionalCharactersAllowed = "#;,/?:@&=+$!*'()";
    return ToPercentEncodingWorker(text_sv, allow_hash_to_specify_fragment ? ( AdditionalCharactersAllowed ) :
                                                                             ( AdditionalCharactersAllowed + 1 ));
}


std::unique_ptr<std::string> Encoders::ToUriComponentWorker(const std::string_view text_sv)
{
    // list from https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/encodeURIComponent
    return ToPercentEncodingWorker(text_sv, "_!*'()");
}


std::string Encoders::ToUriPath(const std::string_view text_sv)
{
    // this list is modified from what ToUriWorker uses to remove:
    //     - ? because that signifies the start of a query string
    //     - # because that signifies a fragment
    constexpr const char* AdditionalCharactersAllowed = ";,/:@&=+$!*'()";
    const std::unique_ptr<std::string> encoded_text = ToPercentEncodingWorker(text_sv, AdditionalCharactersAllowed);

    if( encoded_text != nullptr )
        return std::move(*encoded_text);

    return std::string(text_sv);
}


template<typename T, bool ProcessPlusAsSpace>
T Encoders::FromPercentEncodingWorker(const std::string_view text_sv)
{
    T decoded_text;
    decoded_text.reserve(text_sv.size());

    const char* encoded_text_ptr = text_sv.data();
    size_t length_remaining = text_sv.length();

    // parse the string, decoding any percent encoded sections
    while( length_remaining > 0 )
    {
        const char* first_hex_char;
        const char* second_hex_char;

        char ch = encoded_text_ptr[0];

        if( ch == '%' && length_remaining >= 3 &&
            ( first_hex_char = strchr(HexChars, std::tolower(encoded_text_ptr[1])) ) != nullptr &&
            ( second_hex_char = strchr(HexChars, std::tolower(encoded_text_ptr[2])) ) != nullptr )
        {
            ch = static_cast<char>(( ( first_hex_char - HexChars ) << 4 ) | ( second_hex_char - HexChars ));

            length_remaining -= 3;
            encoded_text_ptr += 3;
        }

        else
        {
            if constexpr(ProcessPlusAsSpace)
            {
                if( ch == '+' )
                    ch = ' ';
            }

            --length_remaining;
            ++encoded_text_ptr;
        }

        decoded_text.push_back(static_cast<typename T::value_type>(ch));
    }

    return decoded_text;
}


template<typename T/* = std::string*/>
T Encoders::FromPercentEncoding(const std::string_view text_sv)
{
    return FromPercentEncodingWorker<T, false>(text_sv);
}

template CLASS_DECL_ZTOOLSO std::string Encoders::FromPercentEncoding(std::string_view text_sv);
template CLASS_DECL_ZTOOLSO std::vector<std::byte> Encoders::FromPercentEncoding(std::string_view text_sv);


std::string Encoders::FromUrlQueryString(const std::string_view text_sv)
{
    return FromPercentEncodingWorker<std::string, true>(text_sv);
}



// --------------------------------------------------------------------------
// HEX
// --------------------------------------------------------------------------

template<typename T>
T Encoders::ToHexValue(const std::string_view text_sv)
{
    T value = 0;

    for( const char ch : text_sv )
    {
        value *= 16;

        value += ( ch >= '0' && ch <= '9' ) ? ( ch - '0' ) :
                 ( ch >= 'A' && ch <= 'Z' ) ? ( ch - 'A' + 10 ) :
                                              ( ch - 'a' + 10 );
    }

    return value;
}

template CLASS_DECL_ZTOOLSO unsigned Encoders::ToHexValue(std::string_view text_sv);



// --------------------------------------------------------------------------
// CSV (comma) / semicolon
// --------------------------------------------------------------------------

std::unique_ptr<std::string> Encoders::ToCsvWorker(const std::string_view text_sv, char separator/* = ','*/)
{
    // following RFC 4180: https://tools.ietf.org/html/rfc4180

    // characters that trigger the need to be in quotes: , " CR/LF
    size_t number_double_quotes = 0;
    bool need_to_delimit = false;

    // calculate the length of the delimited string
    for( const char ch : text_sv )
    {
        if( ch == '"' )
        {
            ++number_double_quotes;
            need_to_delimit = true;
        }

        else if( !need_to_delimit )
        {
            need_to_delimit = ( ch == separator || is_crlf(ch) );
        }
    }

    // do not delimit unless necessary
    if( !need_to_delimit )
        return nullptr;

    // delimit the string, surrounding the entire string in double quotes
    const size_t delimited_text_length = text_sv.length() + 2 + number_double_quotes;
    auto delimited_text = std::make_unique<std::string>(delimited_text_length, '\0');

    char* delimited_text_buffer = delimited_text->data();

    *(delimited_text_buffer++) = '"';

    for( const char ch : text_sv )
    {
        if( ch == '"' )
            *(delimited_text_buffer++) = '"';

        *(delimited_text_buffer++) = ch;
    }

    *delimited_text_buffer = '"';

    return delimited_text;
}



// --------------------------------------------------------------------------
// TSV (tab)
// --------------------------------------------------------------------------

std::unique_ptr<std::string> Encoders::ToTsvWorker(const std::string_view text_sv)
{
    // following https://en.wikipedia.org/wiki/Tab-separated_values
    constexpr const char* CharactersToEscape = "\t\r\n\\";
    constexpr const char* EscapeCharacters   = "trn\\";
    static_assert(std::string_view(CharactersToEscape).length() == std::string_view(EscapeCharacters).length());

    size_t number_characters_to_escape = 0;

    // calculate the length of the delimited string
    for( const char ch : text_sv )
    {
        if( strchr(CharactersToEscape, ch) != nullptr )
            ++number_characters_to_escape;
    }

    // do not delimit unless necessary
    if( number_characters_to_escape == 0 )
        return nullptr;

    // add the escapes
    const size_t delimited_text_length = text_sv.length() + number_characters_to_escape;
    auto delimited_text = std::make_unique<std::string>(delimited_text_length, '\0');

    char* delimited_text_buffer = delimited_text->data();

    for( const char ch : text_sv )
    {
        const char* character_to_escape = strchr(CharactersToEscape , ch);

        if( character_to_escape != nullptr )
        {
            *(delimited_text_buffer++) = '\\';
            *(delimited_text_buffer++) = EscapeCharacters[character_to_escape - CharactersToEscape];
        }

        else
        {
            *(delimited_text_buffer++) = ch;
        }
    }

    return delimited_text;
}



// --------------------------------------------------------------------------
// File URLs
// --------------------------------------------------------------------------

// although the UrlCreateFromPath and PathCreateFromUrl functions exist on Windows, they don't
// support UTF-8 URLs (if building to support on Windows 7), so we will use our own implementations

std::string Encoders::ToFileUrl(std::string file_path)
{
    return SO::Concatenate(FileUrlPrefix_sv, ToUri(PortableFunctions::PathToForwardSlash(std::move(file_path))));
}


std::optional<std::string> Encoders::FromFileUrl(std::string_view file_url_sv)
{
    constexpr std::string_view FileUrlStart_sv = "file:/";

    file_url_sv = SO::Trim(file_url_sv);

    if( SO::StartsWithNoCase(file_url_sv, FileUrlStart_sv) )
    {
        // allow up to three slashes
        std::string_view url_sv = file_url_sv.data() + FileUrlStart_sv.length();

        for( int i = 0; i < 2 && !url_sv.empty() && url_sv.front() == '/'; ++i )
            url_sv.remove_prefix(1);

        std::string file_path = PortableFunctions::PathToNativeSlash(FromPercentEncoding(url_sv));

        // if the file or directory doesn't exist, see if it exists when using encoding using ANSI characters
        if( !PortableFunctions::FileExists(file_path) )
        {
            std::string file_path_from_ansi_encoding = TextConverter::AnsiToUtf8(file_path);

            if( PortableFunctions::FileExists(file_path_from_ansi_encoding) )
                return file_path_from_ansi_encoding;
        }

        return file_path;
    }

    return std::nullopt;
}



// --------------------------------------------------------------------------
// Escaped Text
// --------------------------------------------------------------------------

#ifdef _DEBUG
std::string ToEscapedStringWorker(std::string text, const bool escape_single_quotes = true)
#else
std::string Encoders::ToEscapedString(std::string text, const bool escape_single_quotes/* = true*/)
#endif
{
    if( text.empty() )
        return text;

    const char* escape_representations_to_use = EncoderEscapes::Representations;
    const char* escape_sequences_to_use = EncoderEscapes::Sequences;

    if( !escape_single_quotes )
    {
        static_assert(EncoderEscapes::Representations[0] == '\'');
        ++escape_representations_to_use;
        ++escape_sequences_to_use;
    }

    std::vector<std::tuple<const char*, char>> characters_needing_escaping; // position / character

    // parse the text, finding any characters that need to be escaped
    const char* const input_text_start_ptr = text.c_str();
    const char* input_text_ptr = input_text_start_ptr;

    for( ; *input_text_ptr != '\0'; ++input_text_ptr )
    {
        const char* const escape_representation_pos = strchr(escape_representations_to_use, *input_text_ptr);

        if( escape_representation_pos != nullptr )
            characters_needing_escaping.emplace_back(input_text_ptr, escape_sequences_to_use[escape_representation_pos - escape_representations_to_use]);
    }

    // return if there are no characters needing escaping
    if( characters_needing_escaping.empty() )
        return text;

    // otherwise escape the text
    const char* const input_text_end_ptr = input_text_ptr;
    input_text_ptr = input_text_start_ptr;

    const size_t escaped_text_length = text.length() + characters_needing_escaping.size();
    std::string escaped_text(escaped_text_length, '\0');
    char* escaped_text_ptr = escaped_text.data();

    auto copy_unescaped_text = [&](const char* const copy_up_to_but_not_including_ptr)
    {
        const size_t unescaped_chars = copy_up_to_but_not_including_ptr - input_text_ptr;
        memcpy(escaped_text_ptr, input_text_ptr, unescaped_chars);
        escaped_text_ptr += unescaped_chars;
    };

    for( const auto& [character_needing_escaping_ptr, escape_sequence]  : characters_needing_escaping )
    {
        // copy any unescaped text prior to this character
        copy_unescaped_text(character_needing_escaping_ptr);

        // add the escape
        *(escaped_text_ptr++) = '\\';
        *(escaped_text_ptr++) = escape_sequence;

        input_text_ptr = character_needing_escaping_ptr + 1;
    }

    // copy any final unescaped text
    copy_unescaped_text(input_text_end_ptr);

    return escaped_text;
}


#ifdef _DEBUG
std::string FromEscapedStringWorker(std::string text)
#else
std::string Encoders::FromEscapedString(std::string text)
#endif
{
    if( text.empty() )
        return text;

    std::vector<std::tuple<char*, char>> characters_needing_unescaping;

    char* text_ptr = text.data();
    bool last_character_was_an_escape = false;

    for( ; *text_ptr != '\0'; ++text_ptr )
    {
        if( last_character_was_an_escape )
        {
            const char escaped_representation = Encoders::GetEscapedRepresentation(*text_ptr);

            if( escaped_representation != 0 )
                characters_needing_unescaping.emplace_back(text_ptr - 1, escaped_representation);

            last_character_was_an_escape = false;
        }

        else
        {
            last_character_was_an_escape = ( *text_ptr == '\\' );
        }
    }

    // return if there are no characters needing unescaping
    if( characters_needing_unescaping.empty() )
        return text;

    // otherwise unescape the text in place from the back to the front
    const size_t unescaped_text_length = text.length() - characters_needing_unescaping.size();

    const char* current_text_end_ptr = text_ptr;

    for( auto characters_needing_unescaping_itr = characters_needing_unescaping.crbegin();
         characters_needing_unescaping_itr != characters_needing_unescaping.crend();
         ++characters_needing_unescaping_itr )
    {
        // unescape the character
        ASSERT(*std::get<0>(*characters_needing_unescaping_itr) == '\\');
        *std::get<0>(*characters_needing_unescaping_itr) = std::get<1>(*characters_needing_unescaping_itr);

        // shift the text following this character
        char* const text_following_character_ptr = std::get<0>(*characters_needing_unescaping_itr) + 2;

        memmove(std::get<0>(*characters_needing_unescaping_itr) + 1,
                text_following_character_ptr,
                current_text_end_ptr - text_following_character_ptr);

        --current_text_end_ptr;
    }

    text.resize(unescaped_text_length);

    return text;
}


#ifdef _DEBUG
std::string Encoders::ToEscapedString(const std::string text, const bool escape_single_quotes/* = true*/)
{
    std::string escaped_text = ToEscapedStringWorker(text, escape_single_quotes);
    ASSERT(FromEscapedStringWorker(escaped_text) == text);
    return escaped_text;
}


std::string Encoders::FromEscapedString(const std::string text)
{
    std::string unescaped_text = FromEscapedStringWorker(text);

    // for these checks, also check against unescaped ' or " characters,
    // which don't necessary have to be escaped
    const std::string expected_escaped_text = ToEscapedStringWorker(unescaped_text);

    if( expected_escaped_text != text )
    {
        std::string expected_escaped_text_in_double_quote_string = expected_escaped_text;
        SO::Replace(expected_escaped_text_in_double_quote_string, "\\'", "'");

        if( expected_escaped_text_in_double_quote_string != text )
        {
            std::string expected_escaped_text_in_single_quote_string = expected_escaped_text;
            SO::Replace(expected_escaped_text_in_single_quote_string, "\\\"", "\"");

            ASSERT(expected_escaped_text_in_single_quote_string == text);
        }
    }

    return unescaped_text;
}
#endif


std::string Encoders::ToLogicString(std::string text)
{
    return '"' + ToEscapedString(std::move(text), false) + '"';
}



// --------------------------------------------------------------------------
// JSON
// --------------------------------------------------------------------------

std::string Encoders::ToJsonString(const std::string_view text_sv, const bool escape_forward_slashes/* = true*/)
{
    // specification: https://datatracker.ietf.org/doc/html/rfc7159#section-7

    const char* json_escape_representations_to_use = JsonEscapeRepresentations;
    const char* json_escape_sequences_to_use = JsonEscapeSequences;

    if( !escape_forward_slashes )
    {
        static_assert(JsonEscapeRepresentations[0] == '/');
        ++json_escape_representations_to_use;
        ++json_escape_sequences_to_use;
    }

    std::vector<std::tuple<const char*, char>> characters_needing_escaping;
    size_t json_string_length = 2;

    // parse the text, finding any characters that need to be escaped and calculating the new string length
    const char* input_text_ptr = text_sv.data();
    const char* const input_text_end_ptr = input_text_ptr + text_sv.length();

    for( ; input_text_ptr != input_text_end_ptr; ++input_text_ptr )
    {
        const char* const escape_representation_pos = strchr(json_escape_representations_to_use, *input_text_ptr);

        if( escape_representation_pos != nullptr )
        {
            characters_needing_escaping.emplace_back(input_text_ptr, json_escape_sequences_to_use[escape_representation_pos - json_escape_representations_to_use]);
            json_string_length += 2;
        }

        else if( static_cast<unsigned char>(*input_text_ptr) <= LastControlCharacter )
        {
            characters_needing_escaping.emplace_back(input_text_ptr, '\0');
            json_string_length += 6;
        }

        else
        {
            ++json_string_length;
        }
    }

    // escape the text
    std::string escaped_text(json_string_length, '\0');
    char* escaped_text_ptr = escaped_text.data();

    *(escaped_text_ptr++) = '"';

    input_text_ptr = text_sv.data();

    auto copy_unescaped_text = [&](const char* const copy_up_to_but_not_including_ptr)
    {
        const size_t unescaped_chars = ( copy_up_to_but_not_including_ptr - input_text_ptr );
        memcpy(escaped_text_ptr, input_text_ptr, unescaped_chars);
        escaped_text_ptr += unescaped_chars;
    };

    for( const auto& [character_needing_escaping_ptr, escape_sequence] : characters_needing_escaping )
    {
        // copy any unescaped text prior to this character
        copy_unescaped_text(character_needing_escaping_ptr);

        *(escaped_text_ptr++) = '\\';

        // add a two-character sequence...
        if( escape_sequence != 0 )
        {
            *(escaped_text_ptr++) = escape_sequence;
        }

        // ...or a six-character sequence
        else
        {
            const char ch = *character_needing_escaping_ptr;
            ASSERT(static_cast<unsigned char>(ch) <= LastControlCharacter);

            *(escaped_text_ptr++) = 'u';
            *(escaped_text_ptr++) = '0';
            *(escaped_text_ptr++) = '0';
            *(escaped_text_ptr++) = HexChars[static_cast<byte>(ch) >> 4];
            *(escaped_text_ptr++) = HexChars[static_cast<byte>(ch) & 0x0F];
        }

        input_text_ptr = character_needing_escaping_ptr + 1;
    }

    // copy any final unescaped text
    copy_unescaped_text(input_text_end_ptr);

    *escaped_text_ptr = '"';
    ASSERT(*( escaped_text_ptr - ( json_string_length - 1 ) ) == '"');

    return escaped_text;
}



// --------------------------------------------------------------------------
// RegEx
// --------------------------------------------------------------------------

std::string Encoders::ToRegex(const cs::string_sz text)
{
    const std::regex special_chars{ R"([[\]{}()*+?.,\/\^$|])" };
    return std::regex_replace(text.c_str(), special_chars, R"(\$&)");
}



// --------------------------------------------------------------------------
// Data URL
// --------------------------------------------------------------------------

namespace DataUrl
{
    // information about data URLs: https://developer.mozilla.org/en-US/docs/Web/HTTP/Basics_of_HTTP/Data_URLs
    // data:[<mediatype>][;base64],<data>
    constexpr std::string_view DataUrlPrefix_sv         = "data:";
    constexpr std::string_view DefaultMediaType_sv      = "text/plain";
    constexpr std::string_view Base64EncodingAndData_sv = ";base64,";
    constexpr std::string_view Base64Encoding_sv        = Base64EncodingAndData_sv.substr(1, Base64EncodingAndData_sv.length() - 2);
    constexpr char EncodingPrefix                       = Base64EncodingAndData_sv.front();
    constexpr char DataPrefix                           = Base64EncodingAndData_sv.back();
}


bool Encoders::IsDataUrl(const std::string_view text_sv)
{
    return SO::StartsWithNoCase(text_sv, DataUrl::DataUrlPrefix_sv);
}


bool Encoders::IsDataOrHttpUrl(std::string_view text_sv)
{
    constexpr std::string_view HttpUrlPrefix_sv = "http";

    if( SO::StartsWithNoCase(text_sv, HttpUrlPrefix_sv) )
    {
        text_sv.remove_prefix(HttpUrlPrefix_sv.length());

        return ( SO::StartsWithNoCase(text_sv, "s:") ||
                 SO::StartsWithNoCase(text_sv, ":") );
    }

    else
    {
        return IsDataUrl(text_sv);
    }
}


std::tuple<std::unique_ptr<std::vector<std::byte>>, std::string> Encoders::FromDataUrl(std::string_view data_url_sv)
{
    if( !IsDataUrl(data_url_sv) )
        return { };

    data_url_sv = data_url_sv.substr(DataUrl::DataUrlPrefix_sv.length());

    const size_t data_prefix_pos = data_url_sv.find(DataUrl::DataPrefix);

    if( data_prefix_pos == std::string_view::npos )
        return { };

    std::string_view mediatype_sv = data_url_sv.substr(0, data_prefix_pos);

    enum class EncodingType { PercentEncoding, Base64 };
    EncodingType encoding_type;

    const size_t encoding_prefix = mediatype_sv.find(DataUrl::EncodingPrefix);

    if( encoding_prefix == std::string_view::npos )
    {
        encoding_type = EncodingType::PercentEncoding;
    }

    else
    {
        // return if an unknown encoding
        if( !SO::StartsWithNoCase(DataUrl::Base64Encoding_sv, mediatype_sv.substr(encoding_prefix + 1)) )
            return { };

        encoding_type = EncodingType::Base64;
        mediatype_sv = mediatype_sv.substr(0, encoding_prefix);
    }

    // if there is no media type, use the default one
    if( SO::IsWhitespace(mediatype_sv) )
        mediatype_sv = DataUrl::DefaultMediaType_sv;

    // decode the data
    const std::string_view data_sv = data_url_sv.substr(data_prefix_pos + 1);
    std::optional<std::vector<std::byte>> data;

    if( encoding_type == EncodingType::PercentEncoding )
    {
        data = FromPercentEncoding<std::vector<std::byte>>(data_sv);
    }

    else
    {
        ASSERT(encoding_type == EncodingType::Base64);
        data = Base64::DecodeToBuffer(data_sv);
    }

    return std::make_tuple(std::make_unique<std::vector<std::byte>>(std::move(*data)), std::string(mediatype_sv));
}


std::string Encoders::ToDataUrl(const std::vector<std::byte>& content, const std::string_view mediatype_sv)
{
    return SO::Concatenate(DataUrl::DataUrlPrefix_sv, mediatype_sv, DataUrl::Base64EncodingAndData_sv) +
           Base64::Encode(content);
}
