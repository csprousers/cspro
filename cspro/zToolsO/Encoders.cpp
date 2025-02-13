#include "StdAfx.h"
#include "Encoders.h"
#include "base64.h"
#include "TextConverter.h"
#include <regex>

using namespace Encoders;

static_assert(std::wstring_view(HexChars).length() == 16);
static_assert(std::wstring_view(EscapeRepresentations).length() == std::wstring_view(EscapeSequences).length());
static_assert(std::wstring_view(JsonEscapeRepresentations).length() == std::wstring_view(JsonEscapeSequences).length());


// --------------------------------------------------------------------------
// HTML
// --------------------------------------------------------------------------

constexpr wstring_view HtmlTag_lt  = _T("&lt;");
constexpr wstring_view HtmlTag_gt  = _T("&gt;");
constexpr wstring_view HtmlTag_amp = _T("&amp;");
constexpr wstring_view HtmlTag_br  = _T("<br />");

#pragma warning(push)
#pragma warning(disable:4701)
#pragma warning(disable:4706)
std::wstring Encoders::ToHtml(const wstring_view text_sv, const bool escape_spaces/* = true*/)
{
    // add each character, escaping a few
    std::wstring html;

    for( const TCHAR ch : text_sv )
    {
        bool newline_read;
        bool tab_read;

        if( ch == '<' )
        {
            html.append(HtmlTag_lt);
        }

        else if( ch == '>' )
        {
            html.append(HtmlTag_gt);
        }

        else if( ch == '&' )
        {
            html.append(HtmlTag_amp);
        }

        else if( escape_spaces && ( ( newline_read = ( ch == '\n' ) ) ||
                                    ( tab_read     = ( ch == '\t' ) ) ||
                                                     ( ch == ' '  ) ) )
        {
            if( newline_read )
            {
                html.append(HtmlTag_br);
            }

            else
            {
                size_t spaces_to_output = tab_read ? 4 : 1;

                // force a non-breaking space on the first character
                TCHAR prev_ch = html.empty() ? ' ' : html.back();

                do
                {
                    // also check against end tag characters
                    if( prev_ch == ' ' || prev_ch == '>' )
                    {
                        html.append(_T("&nbsp;"));
                        prev_ch = ';';
                    }

                    else
                    {
                        html.push_back(' ');
                        prev_ch = ' ';
                    }

                } while( --spaces_to_output > 0 );
            }
        }

        else
        {
            html.push_back(ch);
        }
    }

    return html;
}
#pragma warning(pop)


std::wstring Encoders::ToHtmlTagValue(const wstring_view text_sv)
{
    // encodes to HTML and then escapes quotes and newlines
    std::wstring html = ToHtml(text_sv);
    SO::Replace(html, _T("\""), _T("&quot;"));
    SO::Replace(html, HtmlTag_br, _T("&#013;"));
    return html;
}


std::wstring Encoders::FromHtmlAmpersandEscapes(std::wstring text)
{
    SO::Replace(text, HtmlTag_lt, _T("<"));
    SO::Replace(text, HtmlTag_gt, _T(">"));
    SO::Replace(text, HtmlTag_amp, _T("&"));
    return text;
}


std::wstring Encoders::ToPreformattedTextHtml(const wstring_view title_sv, const wstring_view body_sv)
{
    std::wstring title_html = ToHtml(title_sv, false);
    std::wstring body_html = ToHtml(body_sv, false);

    const std::vector<wstring_view> element_svs =
    {
        _T("<html><head><title>"),
        title_html,
        _T("</title></head><body><pre>"),
        body_html,
        _T("</pre></body></html>")
    };

    size_t total_length = 0;

    for( const wstring_view& element_sv : element_svs )
        total_length += element_sv.length();

    std::wstring html(total_length, '\0');
    TCHAR* html_buffer = html.data();

    for( const wstring_view& element_sv : element_svs )
    {
        _tmemcpy(html_buffer, element_sv.data(), element_sv.length());
        html_buffer += element_sv.length();
    };

    return html;
}



// --------------------------------------------------------------------------
// PERCENT-ENCODING + URI
// --------------------------------------------------------------------------

namespace
{
    inline std::wstring ToPercentEncodingWorker(const wstring_view text_sv, const char* const additional_characters_allowed)
    {
        const std::string utf_text = UTF8Convert::WideToUTF8(text_sv);

        // assume that all characters will be percent encoded
        std::wstring encoded_text(utf_text.size() * 3 + 1, '\0');
        TCHAR* encoded_text_ptr = encoded_text.data();

        for( const char ch : utf_text )
        {
            if( IsPercentEncodingUnreservedCharacter(ch) ||
                ( additional_characters_allowed != nullptr && strchr(additional_characters_allowed, ch) != nullptr ) )
            {
                *(encoded_text_ptr++) = ch;
            }

            else
            {
                *(encoded_text_ptr++) = '%';
                *(encoded_text_ptr++) = HexChars[static_cast<byte>(ch) >> 4];
                *(encoded_text_ptr++) = HexChars[static_cast<byte>(ch) & 0x0F];
            }
        }

        encoded_text.resize(encoded_text_ptr - encoded_text.data());

        return encoded_text;
    }
}


std::wstring Encoders::ToPercentEncoding(const wstring_view text_sv)
{
    return ToPercentEncodingWorker(text_sv, nullptr);
}


std::wstring Encoders::ToUri(const wstring_view text_sv, const bool allow_hash_to_specify_fragment/* = true*/)
{
    // list from https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/encodeURI
    const char* EscapeCharacters = "#;,/?:@&=+$!*'()";
    return ToPercentEncodingWorker(text_sv, allow_hash_to_specify_fragment ? ( EscapeCharacters ) :
                                                                             ( EscapeCharacters + 1 ));
}


std::wstring Encoders::ToUriComponent(const wstring_view text_sv)
{
    // list from https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/encodeURIComponent
    return ToPercentEncodingWorker(text_sv, "_!*'()");
}


std::wstring Encoders::FromPercentEncoding(const wstring_view text_sv, const bool assume_utf8_encoding/* = true*/)
{
    std::vector<std::tuple<const TCHAR*, std::string>> encoded_sections;
    const TCHAR* last_percent_encoded_ptr = nullptr;
    const TCHAR* encoded_text_ptr = text_sv.data();

    // parse the string, decoding any percent encoded sections
    for( size_t length_remaining = text_sv.length(); length_remaining > 0; --length_remaining, ++encoded_text_ptr )
    {
        const TCHAR* first_hex_char;
        const TCHAR* second_hex_char;

        if( length_remaining >= 3 && encoded_text_ptr[0] == '%' &&
            ( first_hex_char = _tcschr(HexChars, std::towlower(encoded_text_ptr[1])) ) != nullptr &&
            ( second_hex_char = _tcschr(HexChars, std::towlower(encoded_text_ptr[2])) ) != nullptr )
        {
            const char decoded_char = static_cast<char>(( ( first_hex_char - HexChars ) << 4 ) | ( second_hex_char - HexChars ));

            // if this follows an encoded section, combine it (which will allow for text
            // that had non-Latin characters percent encoded as UTF-8 to be properly decoded)
            if( last_percent_encoded_ptr == ( encoded_text_ptr - 3 ) )
            {
                std::get<1>(encoded_sections.back()).push_back(decoded_char);
            }

            else
            {
                encoded_sections.emplace_back(encoded_text_ptr, std::string(1, decoded_char));
            }

            last_percent_encoded_ptr = encoded_text_ptr;
        }
    }

    // if nothing was percent encoded, we can simply return the string
    if( encoded_sections.empty() )
        return text_sv;

    // otherwise we have to remove the extra characters and convert the encoded sections
    // from UTF-8 (or ANSI) to wide characters
    const TCHAR* const encoded_text_end_ptr = encoded_text_ptr;

    const size_t max_decoded_text_length = ( encoded_text_end_ptr - text_sv.data() ) - ( 2 * encoded_sections.size() );
    std::wstring decoded_text(max_decoded_text_length, '\0');
    TCHAR* decoded_text_ptr = decoded_text.data();

    encoded_text_ptr = text_sv.data();

    for( const auto& [encoded_section_start_ptr, decoded_chars] : encoded_sections )
    {
        // copy any unencoded text prior to this section
        const size_t unencoded_chars = encoded_section_start_ptr - encoded_text_ptr;
        _tmemcpy(decoded_text_ptr, encoded_text_ptr, unencoded_chars);
        decoded_text_ptr += unencoded_chars;

        // convert the decoded characters as if they were UTF-8 (or ANSI) encoded
        const std::wstring wide_decoded_chars = assume_utf8_encoding ? UTF8Convert::UTF8ToWide(decoded_chars) :
                                                                       TextConverter::WindowsAnsiToWide(decoded_chars);
        _tmemcpy(decoded_text_ptr, wide_decoded_chars.data(), wide_decoded_chars.length());
        decoded_text_ptr += wide_decoded_chars.length();

        encoded_text_ptr = encoded_section_start_ptr + decoded_chars.length() * 3;
    }

    // copy any final unencoded text
    _tmemcpy(decoded_text_ptr, encoded_text_ptr, encoded_text_end_ptr - encoded_text_ptr);

    decoded_text.resize(_tcslen(decoded_text.data()));

    return decoded_text;
}



// --------------------------------------------------------------------------
// CSV (comma) / semicolon
// --------------------------------------------------------------------------

std::unique_ptr<std::wstring> Encoders::ToCsvWorker(const wstring_view text_sv, const TCHAR separator/* = ','*/)
{
    // following RFC 4180: https://tools.ietf.org/html/rfc4180

    // characters that trigger the need to be in quotes: , " CR/LF
    int number_double_quotes = 0;
    bool need_to_delimit = false;

    // calculate the length of the delimited string
    for( const TCHAR ch : text_sv )
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
    auto delimited_text = std::make_unique<std::wstring>(delimited_text_length, '\0');

    TCHAR* delimited_text_buffer = delimited_text->data();

    *(delimited_text_buffer++) = '"';

    for( const TCHAR ch : text_sv )
    {
        if( ch == '"' )
            *(delimited_text_buffer++) = '"';

        *(delimited_text_buffer++) = ch;
    }

    *delimited_text_buffer = '"';

    return delimited_text;
}


std::wstring Encoders::ToCsv(std::wstring text, const TCHAR separator/* = ','*/)
{
    std::unique_ptr<std::wstring> delimited_string = ToCsvWorker(text, separator);
    return ( delimited_string != nullptr ) ? *delimited_string : text;
}



// --------------------------------------------------------------------------
// TSV (tab)
// --------------------------------------------------------------------------

std::unique_ptr<std::wstring> Encoders::ToTsvWorker(const wstring_view text_sv)
{
    // following https://en.wikipedia.org/wiki/Tab-separated_values
    constexpr const TCHAR* CharactersToEscape = _T("\t\r\n\\");
    constexpr const TCHAR* EscapeCharacters   = _T("trn\\");

    int number_characters_to_escape = 0;

    // calculate the length of the delimited string
    for( const TCHAR ch : text_sv )
    {
        if( _tcschr(CharactersToEscape, ch) != nullptr )
            ++number_characters_to_escape;
    }

    // do not delimit unless necessary
    if( number_characters_to_escape == 0 )
        return nullptr;

    // add the escapes
    const size_t delimited_text_length = text_sv.length() + number_characters_to_escape;
    auto delimited_text = std::make_unique<std::wstring>(delimited_text_length, '\0');

    TCHAR* delimited_text_buffer = delimited_text->data();

    for( const TCHAR ch : text_sv )
    {
        const TCHAR* character_to_escape = _tcschr(CharactersToEscape , ch);

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


std::wstring Encoders::ToTsv(std::wstring text)
{
    std::unique_ptr<std::wstring> delimited_string = ToTsvWorker(text);
    return ( delimited_string != nullptr ) ? *delimited_string : text;
}



// --------------------------------------------------------------------------
// File URLs
// --------------------------------------------------------------------------

// although the UrlCreateFromPath and PathCreateFromUrl functions exist on Windows, they don't
// support UTF-8 URLs (if building to support on Windows 7), so we will use our own implementations

std::wstring Encoders::ToFileUrl(std::wstring filename)
{
    return _T("file:///") + ToUri(PortableFunctions::PathToForwardSlash(std::move(filename)));
}


std::optional<std::wstring> Encoders::FromFileUrl(wstring_view file_url_sv)
{
    constexpr std::wstring_view FileUrlStart = _T("file:/");

    file_url_sv = SO::Trim(file_url_sv);

    if( SO::StartsWithNoCase(file_url_sv, FileUrlStart) )
    {
        // allow up to three slashes
        wstring_view url_sv = file_url_sv.data() + FileUrlStart.length();

        for( int i = 0; i < 2 && !url_sv.empty() && url_sv.front() == '/'; ++i )
            url_sv = url_sv.substr(1);

        std::wstring filename = PortableFunctions::PathToNativeSlash(FromPercentEncoding(url_sv));

        // if the file or directory doesn't exist, see if it exists when using encoding using ANSI characters
        if( !PortableFunctions::FileExists(filename) )
        {
            std::wstring filename_from_ansi_encoding = PortableFunctions::PathToNativeSlash(FromPercentEncoding(url_sv, false));

            if( PortableFunctions::FileExists(filename_from_ansi_encoding) )
                return filename_from_ansi_encoding;
        }

        return filename;
    }

    return std::nullopt;
}



// --------------------------------------------------------------------------
// Escaped Text
// --------------------------------------------------------------------------

#ifdef _DEBUG
std::wstring ToEscapedStringWorker(std::wstring text, const bool escape_single_quotes = true)
#else
std::wstring Encoders::ToEscapedString(std::wstring text, const bool escape_single_quotes/* = true*/)
#endif
{
    if( text.empty() )
        return text;

    const TCHAR* escape_representations_to_use = EscapeRepresentations;
    const TCHAR* escape_sequences_to_use = EscapeSequences;

    if( !escape_single_quotes )
    {
        static_assert(EscapeRepresentations[0] == '\'');
        ++escape_representations_to_use;
        ++escape_sequences_to_use;
    }

    std::vector<std::tuple<const TCHAR*, TCHAR>> characters_needing_escaping;

    // parse the text, finding any characters that need to be escaped
    const TCHAR* const input_text_start_ptr = text.c_str();
    const TCHAR* input_text_ptr = input_text_start_ptr;

    for( ; *input_text_ptr != 0; ++input_text_ptr )
    {
        const TCHAR* const escape_representation_pos = _tcschr(escape_representations_to_use, *input_text_ptr);

        if( escape_representation_pos != nullptr )
            characters_needing_escaping.emplace_back(input_text_ptr, escape_sequences_to_use[escape_representation_pos - escape_representations_to_use]);
    }

    // return if there are no characters needing escaping
    if( characters_needing_escaping.empty() )
        return text;

    // otherwise escape the text
    const TCHAR* const input_text_end_ptr = input_text_ptr;
    input_text_ptr = input_text_start_ptr;

    const size_t escaped_text_length = text.length() + characters_needing_escaping.size();
    std::wstring escaped_text(escaped_text_length, '\0');
    TCHAR* escaped_text_ptr = escaped_text.data();

    auto copy_unescaped_text = [&](const TCHAR* copy_up_to_but_not_including_ptr)
    {
        size_t unescaped_chars = copy_up_to_but_not_including_ptr - input_text_ptr;
        _tmemcpy(escaped_text_ptr, input_text_ptr, unescaped_chars);
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
std::wstring FromEscapedStringWorker(std::wstring text)
#else
std::wstring Encoders::FromEscapedString(std::wstring text)
#endif
{
    if( text.empty() )
        return text;

    std::vector<std::tuple<TCHAR*, TCHAR>> characters_needing_unescaping;

    TCHAR* text_ptr = text.data();
    bool last_character_was_an_escape = false;

    for( ; *text_ptr != 0; ++text_ptr )
    {
        if( last_character_was_an_escape )
        {
            const TCHAR escaped_representation = Encoders::GetEscapedRepresentation(*text_ptr);

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

    const TCHAR* current_text_end_ptr = text_ptr;

    for( auto characters_needing_unescaping_itr = characters_needing_unescaping.crbegin();
         characters_needing_unescaping_itr != characters_needing_unescaping.crend();
         ++characters_needing_unescaping_itr )
    {
        // unescape the character
        ASSERT(*std::get<0>(*characters_needing_unescaping_itr) == '\\');
        *std::get<0>(*characters_needing_unescaping_itr) = std::get<1>(*characters_needing_unescaping_itr);

        // shift the text following this character
        TCHAR* const text_following_character_ptr = std::get<0>(*characters_needing_unescaping_itr) + 2;

        memmove(std::get<0>(*characters_needing_unescaping_itr) + 1,
                text_following_character_ptr,
                sizeof(TCHAR) * ( current_text_end_ptr - text_following_character_ptr ));

        --current_text_end_ptr;
    }

    text.resize(unescaped_text_length);

    return text;
}


#ifdef _DEBUG
std::wstring Encoders::ToEscapedString(const std::wstring text, const bool escape_single_quotes/* = true*/)
{
    std::wstring escaped_text = ToEscapedStringWorker(text, escape_single_quotes);
    ASSERT(FromEscapedStringWorker(escaped_text) == text);
    return escaped_text;
}


std::wstring Encoders::FromEscapedString(const std::wstring text)
{
    std::wstring unescaped_text = FromEscapedStringWorker(text);

    // for these checks, also check against unescaped ' or " characters,
    // which don't necessary have to be escaped
    const std::wstring expected_escaped_text = ToEscapedStringWorker(unescaped_text);

    if( expected_escaped_text != text )
    {
        std::wstring expected_escaped_text_in_double_quote_string = expected_escaped_text;
        SO::Replace(expected_escaped_text_in_double_quote_string, _T("\\'"), _T("'"));

        if( expected_escaped_text_in_double_quote_string != text )
        {
            std::wstring expected_escaped_text_in_single_quote_string = expected_escaped_text;
            SO::Replace(expected_escaped_text_in_single_quote_string, _T("\\\""), _T("\""));

            ASSERT(expected_escaped_text_in_single_quote_string == text);
        }
    }

    return unescaped_text;
}
#endif


std::wstring Encoders::ToLogicString(std::wstring text)
{
     return _T('"') + ToEscapedString(std::move(text), false) + _T('"');
}



// --------------------------------------------------------------------------
// JSON
// --------------------------------------------------------------------------

std::wstring Encoders::ToJsonString(const wstring_view text_sv, const bool escape_forward_slashes/* = true*/)
{
    // specification: https://datatracker.ietf.org/doc/html/rfc7159#section-7

    const TCHAR* json_escape_representations_to_use = JsonEscapeRepresentations;
    const TCHAR* json_escape_sequences_to_use = JsonEscapeSequences;

    if( !escape_forward_slashes )
    {
        static_assert(JsonEscapeRepresentations[0] == '/');
        ++json_escape_representations_to_use;
        ++json_escape_sequences_to_use;
    }

    std::vector<std::tuple<const TCHAR*, TCHAR>> characters_needing_escaping;
    size_t json_string_length = 2;

    // parse the text, finding any characters that need to be escaped and calculating the new string length
    const TCHAR* input_text_ptr = text_sv.data();
    const TCHAR* input_text_end_ptr = input_text_ptr + text_sv.length();

    for( ; input_text_ptr != input_text_end_ptr; ++input_text_ptr )
    {
        const TCHAR* const escape_representation_pos = _tcschr(json_escape_representations_to_use, *input_text_ptr);

        if( escape_representation_pos != nullptr )
        {
            characters_needing_escaping.emplace_back(input_text_ptr, json_escape_sequences_to_use[escape_representation_pos - json_escape_representations_to_use]);
            json_string_length += 2;
        }

        else if( *input_text_ptr <= LastControlCharacter )
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
    std::wstring escaped_text(json_string_length, '\0');
    TCHAR* escaped_text_ptr = escaped_text.data();

    *(escaped_text_ptr++) = '"';

    input_text_ptr = text_sv.data();

    auto copy_unescaped_text = [&](const TCHAR* copy_up_to_but_not_including_ptr)
    {
        const size_t unescaped_chars = ( copy_up_to_but_not_including_ptr - input_text_ptr );
        _tmemcpy(escaped_text_ptr, input_text_ptr, unescaped_chars);
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
            const TCHAR ch = *character_needing_escaping_ptr;
            ASSERT(ch <= LastControlCharacter);

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

std::wstring Encoders::ToRegex(const NullTerminatedString text)
{
    const std::wregex special_chars{ LR"([[\]{}()*+?.,\/\^$|])" };
    return std::regex_replace(text.c_str(), special_chars, LR"(\$&)");
}



// --------------------------------------------------------------------------
// Data URL
// --------------------------------------------------------------------------

namespace DataUrl
{
    // information about data URLs: https://developer.mozilla.org/en-US/docs/Web/HTTP/Basics_of_HTTP/Data_URLs
    // data:[<mediatype>][;base64],<data>
    constexpr std::wstring_view DataUrlPrefix         = _T("data:");
    constexpr std::wstring_view DefaultMediaType      = _T("text/plain");
    constexpr std::wstring_view Base64EncodingAndData = _T(";base64,");
    constexpr std::wstring_view Base64Encoding        = Base64EncodingAndData.substr(1, Base64EncodingAndData.length() - 2);
    constexpr wchar_t EncodingPrefix                  = Base64EncodingAndData.front();
    constexpr wchar_t DataPrefix                      = Base64EncodingAndData.back();
}


bool Encoders::IsDataUrl(const wstring_view text_sv)
{
    return SO::StartsWithNoCase(text_sv, DataUrl::DataUrlPrefix);
}


std::tuple<std::unique_ptr<std::vector<std::byte>>, std::wstring> Encoders::FromDataUrl(wstring_view data_url_sv)
{
    if( !IsDataUrl(data_url_sv) )
        return { };

    data_url_sv = data_url_sv.substr(DataUrl::DataUrlPrefix.length());

    const size_t data_prefix_pos = data_url_sv.find(DataUrl::DataPrefix);

    if( data_prefix_pos == wstring_view::npos )
        return { };

    wstring_view mediatype_sv = data_url_sv.substr(0, data_prefix_pos);

    enum class EncodingType { PercentEncoding, Base64 };
    EncodingType encoding_type;

    const size_t encoding_prefix = mediatype_sv.find(DataUrl::EncodingPrefix);

    if( encoding_prefix == wstring_view::npos )
    {
        encoding_type = EncodingType::PercentEncoding;
    }

    else
    {
        // return if an unknown encoding
        if( !SO::StartsWithNoCase(DataUrl::Base64Encoding, mediatype_sv.substr(encoding_prefix + 1)) )
            return { };

        encoding_type = EncodingType::Base64;
        mediatype_sv = mediatype_sv.substr(0, encoding_prefix);
    }

    // if there is no media type, use the default one
    if( SO::IsWhitespace(mediatype_sv) )
        mediatype_sv = DataUrl::DefaultMediaType;

    // decode the data
    const wstring_view data_sv = data_url_sv.substr(data_prefix_pos + 1);
    auto data = std::make_unique<std::vector<std::byte>>();

    if( encoding_type == EncodingType::PercentEncoding )
    {
        const std::wstring data_string = FromPercentEncoding(data_sv);
        *data = UTF8Convert::WideToUTF8Buffer(data_string);
    }

    else
    {
        ASSERT(encoding_type == EncodingType::Base64);
        *data = Base64::Decode<wstring_view, std::vector<std::byte>>(data_sv);
    }

    return std::make_tuple(std::move(data), mediatype_sv);
}


std::wstring Encoders::ToDataUrl(const std::vector<std::byte>& content, const std::wstring& mediatype)
{
    return SO::Concatenate(std::wstring(DataUrl::DataUrlPrefix),
                           mediatype,
                           DataUrl::Base64EncodingAndData,
                           Base64::Encode<std::wstring>(content));
}
