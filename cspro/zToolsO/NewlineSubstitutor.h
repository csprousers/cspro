#pragma once

#include <zToolsO/zToolsO.h>
#include <zToolsO/StringOperations.h>


// for places where a newline is not handled, this class provides functionality
// to turn newlines into spaces or into the Unicode newline control character: ␤

class NewlineSubstitutor
{
public:
    static constexpr TCHAR UnicodeNL = _T('␤');

    static std::wstring NewlineToChar(std::wstring text, TCHAR ch)       { return Substitute<std::wstring>(std::move(text), '\n', ch); }
    static std::string  NewlineToChar(const std::string& text, TCHAR ch) { return UTF8_TODO::GetUtf8(NewlineToChar(UTF8_TODO::GetWide(text), ch)); }
    static std::wstring NewlineToSpace(std::wstring text)                { return Substitute<std::wstring>(std::move(text), '\n', ' '); }
    static CString      NewlineToSpace(CString text)                     { return Substitute<CString     >(std::move(text), '\n', ' '); }
    static std::string  NewlineToSpace(const std::string& text)          { return UTF8_TODO::GetUtf8(NewlineToSpace(UTF8_TODO::GetWide(text))); }
    static std::wstring NewlineToUnicodeNL(std::wstring text)            { return Substitute<std::wstring>(std::move(text), '\n', UnicodeNL); }
    static CString      NewlineToUnicodeNL(CString text)                 { return Substitute<CString     >(std::move(text), '\n', UnicodeNL); }
    static std::string  NewlineToUnicodeNL(const std::string& text)      { return UTF8_TODO::GetUtf8(NewlineToUnicodeNL(UTF8_TODO::GetWide(text))); }

    static std::wstring& MakeNewlineToChar(std::wstring& text, TCHAR ch) { return Substitute<std::wstring&>(text, '\n', ch); }
    static std::wstring& MakeNewlineToSpace(std::wstring& text)          { return Substitute<std::wstring&>(text, '\n', ' '); }
    static std::string&  MakeNewlineToSpace(std::string& text)           { text = NewlineToSpace(text); return text; }
    static CString&      MakeNewlineToSpace(CString& text)               { return Substitute<CString&     >(text, '\n', ' '); }
    static std::wstring& MakeNewlineToUnicodeNL(std::wstring& text)      { return Substitute<std::wstring&>(text, '\n', UnicodeNL); }
    static CString&      MakeNewlineToUnicodeNL(CString& text)           { return Substitute<CString&     >(text, '\n', UnicodeNL); }

    static std::wstring CharToNewline(std::wstring text, TCHAR ch)       { return Substitute<std::wstring>(std::move(text), ch, '\n'); }
    static CString      CharToNewline(CString text, TCHAR ch)            { return Substitute<CString     >(std::move(text), ch, '\n'); }
    static std::wstring UnicodeNLToNewline(std::wstring text)            { return Substitute<std::wstring>(std::move(text), UnicodeNL, '\n'); }
    static CString      UnicodeNLToNewline(CString text)                 { return Substitute<CString     >(std::move(text), UnicodeNL, '\n'); }
    static std::string  UnicodeNLToNewline(const std::string& text)      { return UTF8_TODO::GetUtf8(UnicodeNLToNewline(UTF8_TODO::GetWide(text))); }

    static std::wstring& MakeCharToNewline(std::wstring& text, TCHAR ch) { return Substitute<std::wstring&>(text, ch, '\n'); }
    static CString&      MakeCharToNewline(CString& text, TCHAR ch)      { return Substitute<CString&     >(text, ch, '\n'); }
    static std::wstring& MakeUnicodeNLToNewline(std::wstring& text)      { return Substitute<std::wstring&>(text, UnicodeNL, '\n'); }
    static CString&      MakeUnicodeNLToNewline(CString& text)           { return Substitute<CString&     >(text, UnicodeNL, '\n'); }
    static std::string&  MakeUnicodeNLToNewline(std::string& text)       { text = UTF8_TODO::GetUtf8(UnicodeNLToNewline(UTF8_TODO::GetWide(text))); return text; }

    CLASS_DECL_ZTOOLSO static SharableString& MakeNewlineToSpace(SharableString& sharable_string);

private:
    template<typename T>
    static T Substitute(T text, TCHAR old_char, TCHAR new_char);
};



template<typename T>
T NewlineSubstitutor::Substitute(T text, TCHAR old_char, TCHAR new_char)
{
    ASSERT(wstring_view(text).find('\r') == wstring_view::npos);

    if constexpr(std::is_same_v<T, CString> || std::is_same_v<T, CString&>)
    {
        text.Replace(old_char, new_char);
    }

    else
    {
        std::replace(text.begin(), text.end(), old_char, new_char);
    }

    return static_cast<T>(text);
}
