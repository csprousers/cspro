#pragma once

#include <zToolsO/zToolsO.h>


// --------------------------------------------------------------------------
// TC = Text Converter
// --------------------------------------------------------------------------

class TC
{
public:
    // UTF-8 encoding information: https://en.wikipedia.org/wiki/UTF-8
    //  - 1 byte:  0xxxxxxx
    //  - 2 bytes: 110xxxxx 10xxxxxx
    //  - 3 bytes: 1110xxxx 10xxxxxx 10xxxxxx
    //  - 4 bytes: 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx

    // Returns whether the character uses 1 byte (and thus has same value in UTF-8 and wide formats).
    template<typename CT>
    constexpr static bool IsUtf8SingleByte(const CT ch) noexcept;

    // Returns the maximum number of UTF-8 bytes needed to represent any wide character.
    constexpr static size_t MaxUtf8BytesNeededForWideChar() noexcept { return 4; }

    // Returns the number of bytes needed to represent the wide character in UTF-8.
    constexpr static size_t Utf8BytesNeededForWideChar(wchar_t ch) noexcept;

    // Returns the number of bytes represented by the UTF-8 sequence that starts with the character.
    template<typename CT>
    constexpr static size_t Utf8BytesFromFirstByte(CT ch) noexcept;

    // Returns whether a character is in the middle of a UTF-8 sequence (bytes 2 - 4).
    constexpr static bool InMiddleOfUtf8Sequence(char ch) noexcept;

    // Returns the wide character representation of the first character in the UTF-8 string.
    constexpr static wchar_t GetWideCharFromUtf8Sequence(const char* text, size_t utf8_bytes_from_first_byte);
    constexpr static wchar_t GetWideCharFromUtf8Sequence(const char* text);

    // Returns true if the string only uses UTF-8 single byte characters.
    CLASS_DECL_ZTOOLSO static bool UsesOnlyUtf8SingleByteChars(std::string_view text_sv);

    // Returns the UTF-8 representation of a single wide character.
    CLASS_DECL_ZTOOLSO static const std::string& GetUtf8ForWideChar(wchar_t ch);

    // Ensures that the characters at the end of the text string are not part of
    // of an incomplete UTF-8 sequence.
    CLASS_DECL_ZTOOLSO static void EnsureValidEndingUtf8Sequence(std::string& text);

    // Returns a UTF-8 std::string from a buffer of wide text that is guaranteed to
    // only have ASCII characters (0 - 127). The buffer is used during the conversion.
    CLASS_DECL_ZTOOLSO static std::string AsciiToUtf8(wchar_t* wide_ascii_buffer, size_t length);

    // Converts wide text to UTF-8.
    template<typename T>
    static std::string ToUtf8(const T& wide_text);

    // Converts wide text to UTF-8. If wide_length is SIZE_MAX, the string must be null-terminated.
    CLASS_DECL_ZTOOLSO static std::string ToUtf8(const wchar_t* wide_text, size_t wide_length);

    // Converts UTF-8 text to wide.
    template<typename RT = std::wstring, typename T>
    static RT ToWide(const T& utf8_text);

    // Converts UTF-8 text to wide. If utf8_length is SIZE_MAX, the string must be null-terminated.
    template<typename RT = std::wstring>
    CLASS_DECL_ZTOOLSO static RT ToWide(const char* utf8_text, size_t utf8_length);

    // Creates a string from wide text, returning it as either a std::wstring
    // or converting it to UTF-8 and returning it as a std::string.
    template<typename T>
    CLASS_DECL_ZTOOLSO static T CreateFromWide(const wchar_t* wide_text, size_t wide_length);
};



#ifdef WIN_DESKTOP

// --------------------------------------------------------------------------
// WindowsTC = Windows Text Converter
// --------------------------------------------------------------------------

class WindowsTC
{
public:
    // Converts UTF-8 text to/from BSTR.
    CLASS_DECL_ZTOOLSO static BSTR ToBSTR(std::string_view utf8_text_sv);
    CLASS_DECL_ZTOOLSO static std::string FromBSTR(BSTR wide_text);

    // Converts UTF-8 text to/from COleVariant.
    CLASS_DECL_ZTOOLSO static COleVariant ToOleVariant(std::string_view utf8_text_sv);
    CLASS_DECL_ZTOOLSO static std::string FromOleVariant(const COleVariant& wide_text);
};

#endif



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

template<typename CT>
constexpr bool TC::IsUtf8SingleByte(const CT ch) noexcept
{
    if constexpr(std::numeric_limits<CT>::min() < 0)
    {
        if( ch < 0 )
            return false;
    }

    return ( ch <= static_cast<CT>(0x7F) );
}


constexpr size_t TC::Utf8BytesNeededForWideChar(const wchar_t ch) noexcept
{
    ASSERT(ch <= 0x10FFFF);

    return ( ch <= 0x7F )   ? 1 :
           ( ch <= 0x7FF )  ? 2 :
           ( ch <= 0xFFFF ) ? 3 :
                              4;
}


template<typename CT>
constexpr size_t TC::Utf8BytesFromFirstByte(const CT ch) noexcept
{
    return ( static_cast<unsigned char>(ch) < 0x80 ) ? 1 :
           ( static_cast<unsigned char>(ch) < 0xE0 ) ? 2 :
           ( static_cast<unsigned char>(ch) < 0xF0 ) ? 3 :
                                                       4;
}


constexpr bool TC::InMiddleOfUtf8Sequence(const char ch) noexcept
{
    return ( ( ch & 0xC0 ) == 0x80 );
}


constexpr wchar_t TC::GetWideCharFromUtf8Sequence(const char* const text, const size_t utf8_bytes_from_first_byte)
{
    ASSERT(text != nullptr);

    switch( utf8_bytes_from_first_byte )
    {
        case 1: return *text;
        case 2: return ( ( text[0] & 0x1F ) << 6 ) | ( text[1] & 0x3F );
        case 3: return ( ( text[0] & 0x0F ) << 12 ) | ( ( text[1] & 0x3F ) << 6 ) | ( text[2] & 0x3F );
        default:
        case 4: return ( ( text[0] & 0x07 ) << 18 ) | ( (text[1] & 0x3F ) << 12 ) | ( ( text[2] & 0x3F ) << 6 ) | ( text[3] & 0x3F );
    }
}


constexpr wchar_t TC::GetWideCharFromUtf8Sequence(const char* const text)
{
    ASSERT(text != nullptr);
    return GetWideCharFromUtf8Sequence(text, Utf8BytesFromFirstByte(*text));
}


template<typename T>
std::string TC::ToUtf8(const T& wide_text)
{
    if constexpr(std::is_same_v<T, std::wstring> ||
                 std::is_same_v<T, std::wstring_view> ||
                 std::is_same_v<T, wstring_view>)
    {
        return ToUtf8(wide_text.data(), wide_text.length());
    }

    else if constexpr(std::is_same_v<T, CString>)
    {
        return ToUtf8(wide_text.GetString(), wide_text.GetLength());
    }

    else
    {
        static_assert(std::is_same_v<T, const wchar_t*> ||
                      std::is_same_v<T, wchar_t*> ||
                      std::is_array_v<T>); // for wchar_t[]

        return ToUtf8(wide_text, SIZE_MAX);
    }
}


template<typename RT/* = std::wstring*/, typename T>
RT TC::ToWide(const T& utf8_text)
{
    if constexpr(std::is_same_v<T, std::string> ||
                 std::is_same_v<T, std::string_view> ||
                 std::is_same_v<T, cs::string_view_sz>)
    {
        return ToWide<RT>(utf8_text.data(), utf8_text.length());
    }

    else if constexpr(std::is_same_v<T, cs::string_sz>)
    {
        return ToWide<RT>(utf8_text.data(), SIZE_MAX);
    }

    else
    {
        static_assert(std::is_same_v<T, const char*> ||
                      std::is_same_v<T, char*> ||
                      std::is_array_v<T>); // for char[]

        return ToWide<RT>(utf8_text, SIZE_MAX);
    }
}
