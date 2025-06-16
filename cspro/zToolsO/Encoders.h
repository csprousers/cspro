#pragma once

#include <zToolsO/zToolsO.h>
#include <zToolsO/EscapesAndLogicOperators.h>
#include <zToolsO/Tools.h>


class Encoders
{
public:
    static constexpr const char* DecimalChars = "0123456789";
    static constexpr const char* HexChars     = "0123456789abcdef";

    static constexpr const char* JsonEscapeRepresentations = "/\"\\\b\f\n\r\t";
    static constexpr const char* JsonEscapeSequences       = "/\"\\bfnrt";

    static constexpr unsigned char LastControlCharacter = 0x1F;

    static constexpr std::string_view FileUrlPrefix_sv = "file:///";

    // additional escapes defined in EscapesAndLogicOperators.h

    // the ...Worker functions return null values when the text does not need to be encoded


    // --- HTML -----------------------------------------------------------------

    CLASS_DECL_ZTOOLSO static std::unique_ptr<std::string> ToHtmlWorker(std::string_view text_sv, bool escape_spaces = true);
    template<typename T> static std::string ToHtml(T&& text, bool escape_spaces = true);

    CLASS_DECL_ZTOOLSO static std::string ToHtmlTagValue(std::string_view text_sv);
    CLASS_DECL_ZTOOLSO static std::string FromHtmlAmpersandEscapes(std::string text);
    CLASS_DECL_ZTOOLSO static std::string ToPreformattedTextHtml(std::string_view title_sv, std::string_view body_sv);


    // --- Markdown -------------------------------------------------------------

    CLASS_DECL_ZTOOLSO static std::unique_ptr<std::string> ToMarkdownWorker(std::string_view text_sv);
    template<typename T> static std::string ToMarkdown(T&& text);


    // --- PERCENT-ENCODING + URI------------------------------------------------

    static constexpr bool IsPercentEncodingUnreservedCharacter(int ch);

    CLASS_DECL_ZTOOLSO static std::unique_ptr<std::string> ToPercentEncodingWorker(std::string_view text_sv);
    template<typename T> static std::string ToPercentEncoding(T&& text);

    template<typename T = std::string> // can also return std::vector<std::byte>
    static T FromPercentEncoding(std::string_view text_sv);

    CLASS_DECL_ZTOOLSO static std::unique_ptr<std::string> ToUriWorker(std::string_view text_sv, bool allow_hash_to_specify_fragment = true);
    template<typename T> static std::string ToUri(T&& text, bool allow_hash_to_specify_fragment = true);

    CLASS_DECL_ZTOOLSO static std::unique_ptr<std::string> ToUriComponentWorker(std::string_view text_sv);
    template<typename T> static std::string ToUriComponent(T&& text);

    CLASS_DECL_ZTOOLSO static std::string ToUriPath(std::string_view text_sv);

    CLASS_DECL_ZTOOLSO static std::string FromUrlQueryString(std::string_view text_sv);


    // --- HEX ------------------------------------------------------------------

    template<typename T>
    CLASS_DECL_ZTOOLSO static T ToHexValue(std::string_view text_sv);


    // --- COMMA + SEMICOLON + TAB DELIMITED ------------------------------------

    CLASS_DECL_ZTOOLSO static std::unique_ptr<std::string> ToCsvWorker(std::string_view text_sv, char separator = ',');
    template<typename T> static std::string ToCsv(T&& text);

    CLASS_DECL_ZTOOLSO static std::unique_ptr<std::string> ToTsvWorker(std::string_view text_sv);


    // --- FILE URLS ------------------------------------------------------------

    CLASS_DECL_ZTOOLSO static std::string ToFileUrl(std::string file_path);

    // returns std::nullopt if not a valid file URL
    CLASS_DECL_ZTOOLSO static std::optional<std::string> FromFileUrl(std::string_view file_url_sv);


    // --- ESCAPED TEXT ---------------------------------------------------------

    // the escape functions work with the most common escape sequences for ' " \ as well as
    // most others from https://en.cppreference.com/w/cpp/language/escape:
    // \a audible bell   \b backspace         \f form feed
    // \n new line       \r carriage return   \t horizontal tab   \v vertical tab

    // returns the escaped character's representation, or 0 on error; e.g., 'n' returns '\n'
    static char GetEscapedRepresentation(int escape_sequence);

    CLASS_DECL_ZTOOLSO static std::string ToEscapedString(std::string text, bool escape_single_quotes = true);
    CLASS_DECL_ZTOOLSO static std::string FromEscapedString(std::string text);

    // escapes text for use in CSPro logic strings, surrounding the text with double quotes
    CLASS_DECL_ZTOOLSO static std::string ToLogicString(std::string text);


    // --- JSON -----------------------------------------------------------------

    // escapes text for use in JSON, surrounding the text with double quotes
    CLASS_DECL_ZTOOLSO static std::string ToJsonString(std::string_view text_sv, bool escape_forward_slashes = true);


    // --- REGEX ----------------------------------------------------------------
    //
    // converts text to regex literal by escaping regex special characters.
    CLASS_DECL_ZTOOLSO static std::string ToRegex(cs::string_sz text);


    // --- DATA URL -------------------------------------------------------------

    // returns whether the text begins with the data URL prefix
    CLASS_DECL_ZTOOLSO static bool IsDataUrl(std::string_view text_sv);

    // returns whether the text begins with a data URL prefix or http: or https:
    CLASS_DECL_ZTOOLSO static bool IsDataOrHttpUrl(std::string_view text_sv);

    // decodes a data URL into its data and mediatype values; the data pointer will be null on error
    CLASS_DECL_ZTOOLSO static std::tuple<std::unique_ptr<std::vector<std::byte>>, std::string> FromDataUrl(std::string_view data_url_sv);

    // encodes binary data to a data URL encoded with Base64; mediatype can be blank
    CLASS_DECL_ZTOOLSO static std::string ToDataUrl(const std::vector<std::byte>& content, std::string_view mediatype_sv);


private:
    CLASS_DECL_ZTOOLSO static std::unique_ptr<std::string> ToHtmlMarkdownWorker(std::string_view text_sv, std::string_view escape_chars_sv, bool escape_spaces);

    CLASS_DECL_ZTOOLSO static std::unique_ptr<std::string> ToPercentEncodingWorker(std::string_view text_sv, const char* additional_characters_allowed);

    template<typename T, bool ProcessPlusAsSpace>
    static T FromPercentEncodingWorker(std::string_view text_sv);
};



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

template<typename T>
std::string Encoders::ToHtml(T&& text, const bool escape_spaces/* = true*/)
{
    const std::unique_ptr<std::string> encoded_text = ToHtmlWorker(text, escape_spaces);

    if( encoded_text != nullptr )
        return std::move(*encoded_text);

    return std::string(std::forward<T>(text));
}


template<typename T>
std::string Encoders::ToMarkdown(T&& text)
{
    const std::unique_ptr<std::string> encoded_text = ToMarkdownWorker(text);

    if( encoded_text != nullptr )
        return std::move(*encoded_text);

    return std::string(std::forward<T>(text));
}


constexpr bool Encoders::IsPercentEncodingUnreservedCharacter(const int ch)
{
    // unreserved characters list from https://en.wikipedia.org/wiki/Percent-encoding
    return ( is_tokch(ch) || ch == '-' || ch == '.' || ch == '~' );
}


template<typename T>
std::string Encoders::ToPercentEncoding(T&& text)
{
    const std::unique_ptr<std::string> encoded_text = ToPercentEncodingWorker(text);

    if( encoded_text != nullptr )
        return std::move(*encoded_text);

    return std::string(std::forward<T>(text));
}


template<typename T>
std::string Encoders::ToUri(T&& text, const bool allow_hash_to_specify_fragment/* = true*/)
{
    const std::unique_ptr<std::string> encoded_text = ToUriWorker(text, allow_hash_to_specify_fragment);

    if( encoded_text != nullptr )
        return std::move(*encoded_text);

    return std::string(std::forward<T>(text));
}


template<typename T>
std::string Encoders::ToUriComponent(T&& text)
{
    const std::unique_ptr<std::string> encoded_text = ToUriComponentWorker(text);

    if( encoded_text != nullptr )
        return std::move(*encoded_text);

    return std::string(std::forward<T>(text));
}


template<typename T>
std::string Encoders::ToCsv(T&& text)
{
    const std::unique_ptr<std::string> encoded_text = ToCsvWorker(text);

    if( encoded_text != nullptr )
        return std::move(*encoded_text);

    return std::string(std::forward<T>(text));
}


inline char Encoders::GetEscapedRepresentation(const int escape_sequence)
{
    const char* const escape_sequences_pos = strchr(EncoderEscapes::Sequences, escape_sequence);

    return ( escape_sequences_pos == nullptr ) ? 0 :
                                                 EncoderEscapes::Representations[escape_sequences_pos - EncoderEscapes::Sequences];
}
