#include "StdAfx.h"
#include "Utf8.h"
#include "Utf8Convert.h"
#include <mutex>


// --------------------------------------------------------------------------
// TC = Text Converter
// --------------------------------------------------------------------------

bool TC::UsesOnlyUtf8SingleByteChars(const std::string_view text_sv)
{
    for( const char ch: text_sv )
    {
        if( !IsUtf8SingleByte(ch) )
            return false;
    }

    return true;
}


const std::string& TC::GetUtf8ForWideChar(const wchar_t ch)
{
    // cache the conversions
    static std::map<wchar_t, std::unique_ptr<std::string>> wide_char_to_utf8_map;

    const auto& lookup = wide_char_to_utf8_map.find(ch);

    if( lookup != wide_char_to_utf8_map.cend() )
    {
        return *lookup->second;
    }

    // if not found, generate the UTF-8 string
    else
    {
        static std::mutex map_mutex;
        const std::lock_guard<std::mutex> lock(map_mutex);
        const auto& insert = wide_char_to_utf8_map.try_emplace(ch, std::make_unique<std::string>(ToUtf8(std::wstring_view(&ch, 1))));
        ASSERT(TC::Utf8BytesNeededForWideChar(ch) == insert.first->second->length());
        return *insert.first->second;
    }
}


void TC::EnsureValidEndingUtf8Sequence(std::string& text)
{
    if( text.empty() || TC::IsUtf8SingleByte(text.back()) )
        return;

    auto text_ritr = text.crbegin();
    const auto& text_crend = text.crend();

    while( InMiddleOfUtf8Sequence(*text_ritr) )
    {
        ++text_ritr;

        if( text_ritr == text_crend )
        {
            ASSERT(false);
            return;
        }
    }

    // + 1 added for the character at the beginning of the sequence
    const size_t utf8_sequence_length_present = 1 + text_ritr - text.crbegin();

    // if the complete sequence is not present, remove the text starting with this character
    if( utf8_sequence_length_present != Utf8BytesFromFirstByte(*text_ritr) )
        text.resize(text.length() - utf8_sequence_length_present);
}


std::string TC::AsciiToUtf8(wchar_t* wide_ascii_buffer, const size_t length)
{
    char* utf8_buffer_start = reinterpret_cast<char*>(wide_ascii_buffer);
    char* utf8_buffer_itr = utf8_buffer_start;
    const wchar_t* const wide_ascii_buffer_end = wide_ascii_buffer + length;

    while( wide_ascii_buffer < wide_ascii_buffer_end )
    {
        ASSERT(Utf8BytesNeededForWideChar(*wide_ascii_buffer) == 1);

        *utf8_buffer_itr = static_cast<char>(*wide_ascii_buffer);

        ++wide_ascii_buffer;
        ++utf8_buffer_itr;
    }

    ASSERT(length == static_cast<size_t>(utf8_buffer_itr - utf8_buffer_start));

    return std::string(utf8_buffer_start, length);
}


std::string TC::ToUtf8(const wchar_t* const wide_text, const size_t wide_length)
{
    static_assert(static_cast<int>(SIZE_MAX) == -1);

#ifdef WIN_DESKTOP
    int utf8_length = WideCharToMultiByte(CP_UTF8, 0, wide_text, static_cast<int>(wide_length), nullptr, 0, nullptr, nullptr);

    // don't count the terminating null character
    if( wide_length == SIZE_MAX )
        --utf8_length;

    std::string utf8_text;
    utf8_text.resize(utf8_length, '\0');

    WideCharToMultiByte(CP_UTF8, 0, wide_text, static_cast<int>(wide_length), utf8_text.data(), utf8_length, nullptr, nullptr);

    return utf8_text;

#else
    return UTF8Convert::WideToUTF8(wide_text, static_cast<int>(wide_length));

#endif
}


template<typename RT/* = std::wstring*/>
RT TC::ToWide(const char* const utf8_text, const size_t utf8_length)
{
    static_assert(static_cast<int>(SIZE_MAX) == -1);

#ifdef WIN_DESKTOP
    int wide_length = MultiByteToWideChar(CP_UTF8, 0, utf8_text, static_cast<int>(utf8_length), nullptr, 0);

    // don't count the terminating null character
    if( utf8_length == SIZE_MAX )
        --wide_length;

    if constexpr(std::is_same_v<RT, std::wstring>)
    {
        RT wide_text(wide_length, '\0');
        MultiByteToWideChar(CP_UTF8, 0, utf8_text, static_cast<int>(utf8_length), wide_text.data(), wide_length);
        return wide_text;
    }

    else
    {
        static_assert(constexpr(std::is_same_v<RT, CString>));
        RT wide_text;
        MultiByteToWideChar(CP_UTF8, 0, utf8_text, static_cast<int>(utf8_length), wide_text.GetBufferSetLength(wide_length), wide_length);
        wide_text.ReleaseBuffer(wide_length);
        return wide_text;
    }

#else
    return UTF8Convert::UTF8ToWide<RT>(utf8_text, static_cast<int>(utf8_length));

#endif
}

template CLASS_DECL_ZTOOLSO std::wstring TC::ToWide(const char* utf8_text, size_t utf8_length);
template CLASS_DECL_ZTOOLSO CString TC::ToWide(const char* utf8_text, size_t utf8_length);


template<typename T>
T TC::CreateFromWide(const wchar_t* const wide_text, const size_t wide_length)
{
    if constexpr(std::is_same_v<T, std::wstring>)
    {
        return std::wstring(wide_text, wide_length);
    }

    else
    {
        return ToUtf8(wide_text, wide_length);
    }
}

template CLASS_DECL_ZTOOLSO std::wstring TC::CreateFromWide(const wchar_t* wide_text, size_t wide_length);
template CLASS_DECL_ZTOOLSO std::string TC::CreateFromWide(const wchar_t* wide_text, size_t wide_length);


#ifdef WIN_DESKTOP

BSTR WindowsTC::ToBSTR(const std::string_view utf8_text_sv)
{
    const int wide_length = MultiByteToWideChar(CP_UTF8, 0, utf8_text_sv.data(), static_cast<int>(utf8_text_sv.length()), nullptr, 0);
    BSTR wide_text = SysAllocStringLen(nullptr, wide_length);
    MultiByteToWideChar(CP_UTF8, 0, utf8_text_sv.data(), static_cast<int>(utf8_text_sv.length()), wide_text, wide_length);
    return wide_text;
}


std::string WindowsTC::FromBSTR(const BSTR wide_text)
{
    const UINT wide_length = SysStringLen(wide_text);
    return ( wide_length != 0 ) ? TC::ToUtf8(static_cast<const wchar_t*>(wide_text), wide_length) :
                                  std::string();
}


COleVariant WindowsTC::ToOleVariant(const std::string_view utf8_text_sv)
{
    const int wide_length = MultiByteToWideChar(CP_UTF8, 0, utf8_text_sv.data(), static_cast<int>(utf8_text_sv.length()), nullptr, 0);
    COleVariant wide_text;
    wide_text.Clear();
    wide_text.vt = VT_BSTR;
    wide_text.bstrVal = SysAllocStringLen(nullptr, wide_length);
    MultiByteToWideChar(CP_UTF8, 0, utf8_text_sv.data(), static_cast<int>(utf8_text_sv.length()), wide_text.bstrVal, wide_length);
    return wide_text;
}


std::string WindowsTC::FromOleVariant(const COleVariant& wide_text)
{
    return ( wide_text.vt == VT_BSTR ) ? TC::ToUtf8(wide_text.bstrVal) :
                                         ReturnProgrammingError(std::string());
}

#endif // WIN_DESKTOP
