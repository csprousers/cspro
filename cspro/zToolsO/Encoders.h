#pragma once

#include <zToolsO/zToolsO.h>
#include <zToolsO/EscapesAndLogicOperators.h>
#include <zToolsO/Tools.h>


namespace Encoders
{
    constexpr const TCHAR* HexChars = _T("0123456789abcdef");

    constexpr const TCHAR* JsonEscapeRepresentations = _T("/\"\\\b\f\n\r\t");
    constexpr const TCHAR* JsonEscapeSequences       = _T("/\"\\bfnrt");

    constexpr TCHAR LastControlCharacter = 0x1F;

    // additional escapes defined in EscapesAndLogicOperators.h


    CLASS_DECL_ZTOOLSO std::wstring ToHtml(wstring_view text_sv, bool escape_spaces = true);
    CLASS_DECL_ZTOOLSO std::wstring ToHtmlTagValue(wstring_view text_sv);
    CLASS_DECL_ZTOOLSO std::wstring FromHtmlAmpersandEscapes(std::wstring text);
    CLASS_DECL_ZTOOLSO std::wstring ToPreformattedTextHtml(wstring_view title_sv, wstring_view body_sv);


    CLASS_DECL_ZTOOLSO std::wstring ToPercentEncoding(wstring_view text_sv);
    CLASS_DECL_ZTOOLSO std::wstring FromPercentEncoding(wstring_view text_sv, bool assume_utf8_encoding = true);

    constexpr bool IsPercentEncodingUnreservedCharacter(TCHAR ch);

    CLASS_DECL_ZTOOLSO std::wstring ToUri(wstring_view text_sv, bool allow_hash_to_specify_fragment = true);
    CLASS_DECL_ZTOOLSO std::wstring ToUriComponent(wstring_view text_sv);


    CLASS_DECL_ZTOOLSO std::unique_ptr<std::wstring> ToCsvWorker(wstring_view text_sv, TCHAR separator = ',');
    CLASS_DECL_ZTOOLSO std::wstring ToCsv(std::wstring text, TCHAR separator = ',');

    CLASS_DECL_ZTOOLSO std::unique_ptr<std::wstring> ToTsvWorker(wstring_view text_sv);
    CLASS_DECL_ZTOOLSO std::wstring ToTsv(std::wstring text);


    CLASS_DECL_ZTOOLSO std::wstring ToFileUrl(std::wstring filename);
    CLASS_DECL_ZTOOLSO std::optional<std::wstring> FromFileUrl(wstring_view file_url_sv);


    // the escape functions work with the most common escape sequences for ' " \ as well as
    // most others from https://en.cppreference.com/w/cpp/language/escape:
    // \a audible bell   \b backspace         \f form feed
    // \n new line       \r carriage return   \t horizontal tab   \v vertical tab

    // returns the escaped character's representation, or 0 on error; e.g., 'n' returns '\n'
    TCHAR GetEscapedRepresentation(TCHAR escape_sequence);

    CLASS_DECL_ZTOOLSO std::wstring ToEscapedString(std::wstring text, bool escape_single_quotes = true);
    CLASS_DECL_ZTOOLSO std::wstring FromEscapedString(std::wstring text);

    // escapes text for use in CSPro logic strings, surrounding the text with double quotes
    CLASS_DECL_ZTOOLSO std::wstring ToLogicString(std::wstring text);


    // escapes text for use in JSON, surrounding the text with double quotes
    CLASS_DECL_ZTOOLSO std::wstring ToJsonString(wstring_view text_sv, bool escape_forward_slashes = true);


    // converts text to regex literal by escaping regex special characters.
    CLASS_DECL_ZTOOLSO std::wstring ToRegex(NullTerminatedString text);


    // returns whether the text begins with the data URL prefix
    CLASS_DECL_ZTOOLSO bool IsDataUrl(wstring_view text_sv);

    // decodes a data URL into its data and mediatype values; the data pointer will be null on error
    CLASS_DECL_ZTOOLSO std::tuple<std::unique_ptr<std::vector<std::byte>>, std::wstring> FromDataUrl(wstring_view data_url_sv);

    // encodes binary data to a data URL encoded with Base64; mediatype can be blank
    CLASS_DECL_ZTOOLSO std::wstring ToDataUrl(const std::vector<std::byte>& content, const std::wstring& mediatype);
}



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

constexpr bool Encoders::IsPercentEncodingUnreservedCharacter(TCHAR ch)
{
    // unreserved characters list from https://en.wikipedia.org/wiki/Percent-encoding
    return ( is_tokch(ch) || ch == '-' || ch == '.' || ch == '~' );
}


inline TCHAR Encoders::GetEscapedRepresentation(TCHAR escape_sequence)
{
    const TCHAR* escape_sequences_pos = _tcschr(EscapeSequences, escape_sequence);
    return ( escape_sequences_pos == nullptr ) ? 0 : EscapeRepresentations[escape_sequences_pos - EscapeSequences];
}
