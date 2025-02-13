#pragma once

#include <zToolsO/zToolsO.h>


class TextConverter
{
public:
    // --------------------------------------------------------------------------
    // ANSI (Windows code page 1252) <---> wide characters
    // --------------------------------------------------------------------------

    // ST can be std::string or std::string_view
    template<typename ST>
    static std::wstring WindowsAnsiToWide(const ST& ansi_string);
    static std::wstring WindowsAnsiToWide(const char* ansi_string);
    static std::wstring WindowsAnsiToWide(const char* non_null_ansi_string, size_t ansi_length);

    static int WindowsAnsiToWideBuffer(const char* non_null_ansi_string, size_t ansi_length,
                                       wchar_t* non_null_wide_buffer, size_t wide_buffer_length);

    // ST can be std::wstring or wstring_view
    template<typename ST>
    static std::string WideToWindowsAnsi(const ST& wide_string);
    static std::string WideToWindowsAnsi(const wchar_t* wide_string);
    static std::string WideToWindowsAnsi(const wchar_t* non_null_wide_string, size_t wide_length);


private:
    CLASS_DECL_ZTOOLSO static std::wstring WindowsAnsiToWideWorker(const char* non_null_ansi_string, size_t ansi_length);
    CLASS_DECL_ZTOOLSO static int WindowsAnsiToWideBufferWorker(const char* non_null_ansi_string, wchar_t* non_null_wide_buffer, size_t length);
    CLASS_DECL_ZTOOLSO static std::string WideToWindowsAnsiWorker(const wchar_t* non_null_wide_string, size_t wide_length);

#ifdef WIN32
    template<unsigned code_page, bool length_is_known>
    static std::wstring MultiByteToWide(const char* non_null_multi_byte_string, int multi_byte_length);

    template<unsigned code_page>
    static int MultiByteToWideBuffer(const char* non_null_multi_byte_string, size_t multi_byte_length,
                                     wchar_t* non_null_wide_buffer, size_t wide_buffer_length);

    template<unsigned code_page, bool length_is_known>
    static std::string WideToMultiByte(const wchar_t* non_null_wide_string, int wide_length);
#endif
};



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

#ifdef WIN32

template<unsigned code_page, bool length_is_known>
std::wstring TextConverter::MultiByteToWide(const char* const non_null_multi_byte_string, const int multi_byte_length)
{
    ASSERT(non_null_multi_byte_string != nullptr && multi_byte_length >= -1);
    ASSERT(length_is_known == ( multi_byte_length != -1 ));

    int wide_length = MultiByteToWideChar(code_page, 0, non_null_multi_byte_string, multi_byte_length, nullptr, 0);

    // when the input length is not known, MultiByteToWideChar counts the null terminator in the output length
    if constexpr(!length_is_known)
        --wide_length;

    std::wstring wide_string(wide_length, '\0');

    MultiByteToWideBuffer<code_page>(non_null_multi_byte_string, multi_byte_length, wide_string.data(), wide_length);

    return wide_string;
}


template<unsigned code_page>
static int TextConverter::MultiByteToWideBuffer(const char* const non_null_multi_byte_string, const size_t multi_byte_length,
                                                 wchar_t* const non_null_wide_buffer, const size_t wide_buffer_length)
{
    ASSERT(non_null_multi_byte_string != nullptr && non_null_wide_buffer != nullptr);
    ASSERT(multi_byte_length != SIZE_MAX && wide_buffer_length != SIZE_MAX);

    return MultiByteToWideChar(code_page, 0, non_null_multi_byte_string, multi_byte_length, non_null_wide_buffer, wide_buffer_length);
}


template<unsigned code_page, bool length_is_known>
std::string TextConverter::WideToMultiByte(const wchar_t* const non_null_wide_string, const int wide_length)
{
    ASSERT(non_null_wide_string != nullptr && wide_length >= -1);
    ASSERT(length_is_known == ( wide_length != -1 ));

    int multi_byte_length = WideCharToMultiByte(code_page, 0, non_null_wide_string, wide_length, nullptr, 0, nullptr, nullptr);

    // when the input length is not known, WideCharToMultiByte counts the null terminator in the output length
    if constexpr(!length_is_known)
        --multi_byte_length;

    std::string multi_byte_string(multi_byte_length, '\0');

    WideCharToMultiByte(code_page, 0, non_null_wide_string, wide_length, multi_byte_string.data(), multi_byte_length, nullptr, nullptr);

    return multi_byte_string;
}

#endif // WIN32


template<typename ST>
std::wstring TextConverter::WindowsAnsiToWide(const ST& ansi_string)
{
    return WindowsAnsiToWide(ansi_string.data(), ansi_string.length());
}


inline std::wstring TextConverter::WindowsAnsiToWide(const char* const ansi_string)
{
    if( ansi_string == nullptr )
        return std::wstring();

#ifdef WIN32
    return MultiByteToWide<CP_ACP, false>(ansi_string, -1);
#else
    return WindowsAnsiToWideWorker(ansi_string, strlen(ansi_string));
#endif
}


inline std::wstring TextConverter::WindowsAnsiToWide(const char* const non_null_ansi_string, const size_t ansi_length)
{
    ASSERT(non_null_ansi_string != nullptr);

#ifdef WIN32
    return MultiByteToWide<CP_ACP, true>(non_null_ansi_string, static_cast<int>(ansi_length));
#else
    return WindowsAnsiToWideWorker(non_null_ansi_string, ansi_length);
#endif
}


inline int TextConverter::WindowsAnsiToWideBuffer(const char* non_null_ansi_string, const size_t ansi_length,
                                                  wchar_t* const non_null_wide_buffer, const size_t wide_buffer_length)
{
#ifdef WIN32
    return MultiByteToWideBuffer<CP_ACP>(non_null_ansi_string, ansi_length, non_null_wide_buffer, wide_buffer_length);
#else
    return WindowsAnsiToWideBufferWorker(non_null_ansi_string, non_null_wide_buffer, std::min(ansi_length, wide_buffer_length));
#endif
}


template<typename ST>
std::string TextConverter::WideToWindowsAnsi(const ST& wide_string)
{
    return WideToWindowsAnsi(wide_string.data(), wide_string.length());
}


inline std::string TextConverter::WideToWindowsAnsi(const wchar_t* const wide_string)
{
    if( wide_string == nullptr )
        return std::string();

#ifdef WIN32
    return WideToMultiByte<CP_ACP, false>(wide_string, -1);
#else
    return WideToWindowsAnsiWorker(wide_string, wcslen(wide_string));
#endif
}


inline std::string TextConverter::WideToWindowsAnsi(const wchar_t* const non_null_wide_string, const size_t wide_length)
{
    ASSERT(non_null_wide_string != nullptr);

#ifdef WIN32
    return WideToMultiByte<CP_ACP, true>(non_null_wide_string, static_cast<int>(wide_length));
#else
    return WideToWindowsAnsiWorker(non_null_wide_string, wide_length);
#endif
}
