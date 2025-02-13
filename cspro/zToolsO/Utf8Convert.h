#pragma once

#include <zToolsO/zToolsO.h>
#include <zToolsO/Tools.h>


// Utility class for converting between character encodings

namespace UTF8Convert
{
    /// <summary>
    /// Convert from UTF-8 encoded string to wide character (2 bytes per char) string.
    /// Only need to provide stringLen if pUTF8String is not null-terminated.
    /// </summary>
    template<typename T = std::wstring>
    CLASS_DECL_ZTOOLSO T UTF8ToWide(const char* utf_string, int utf8_length = -1);

    template<typename T = std::wstring>
    T UTF8ToWide(const unsigned char* utf_string, int utf8_length = -1)
    {
        return UTF8ToWide<T>(reinterpret_cast<const char*>(utf_string), utf8_length);
    }

    template<typename T = std::wstring>
    T UTF8ToWide(std::string_view str)
    {
        return UTF8ToWide<T>(str.data(), static_cast<int>(str.length()));
    }

    /// <summary>
    /// Convert wide character (2 bytes per char) string to UTF-8 encoded string.
    /// Only need to provide wide_length if wide_string is not null-terminated.
    /// </summary>
    CLASS_DECL_ZTOOLSO std::string WideToUTF8(const wchar_t* wide_string, int wide_length = -1);

    inline std::string WideToUTF8(std::wstring_view str)
    {
        return WideToUTF8(str.data(), static_cast<int>(str.length()));
    }

    /// <summary>
    /// Convert wide character (2 bytes per char) string to UTF-8 encoded text buffer.
    /// </summary>
    CLASS_DECL_ZTOOLSO std::vector<std::byte> WideToUTF8Buffer(std::wstring_view str);


#ifdef WIN32
    /// <summary>
    /// Convert multibyte character (ANSI or UTF-8) buffer to a wide character buffer.
    /// </summary>
    inline int EncodedCharsBufferToWideBuffer(Encoding eEncoding, const char* paBuffer, size_t iaLength, TCHAR* pwBuffer, size_t iwBufferSize)
    {
        return MultiByteToWideChar(( eEncoding == Encoding::Utf8 ) ? CP_UTF8 : CP_ACP, 0, paBuffer, iaLength, pwBuffer, iwBufferSize);
    }

    /// <summary>
    /// Convert wide character buffer to a UTF-8 buffer.
    /// </summary>
    inline int WideBufferToUTF8Buffer(const TCHAR* pwBuffer, size_t iwLength, char* paBuffer, size_t iaBufferSize)
    {
        return WideCharToMultiByte(CP_UTF8, 0, pwBuffer, iwLength, paBuffer, iaBufferSize, NULL, NULL);
    }

    /// <summary>
    /// Convert UTF-8 character buffer to a wide character buffer.
    /// </summary>
    inline int UTF8BufferToWideBuffer(const char* paBuffer, size_t iaBufferSize, TCHAR* pwBuffer, size_t iwLength)
    {
        return MultiByteToWideChar(CP_UTF8, 0, paBuffer, iaBufferSize, pwBuffer, iwLength);
    }

#else
    // CR_TODO ... improve Android performance for these methods
    CLASS_DECL_ZTOOLSO int EncodedCharsBufferToWideBuffer(Encoding eEncoding, const char* paBuffer, size_t iaLength, TCHAR* pwBuffer, size_t iwBufferSize);
    CLASS_DECL_ZTOOLSO int WideBufferToUTF8Buffer(const TCHAR* pwBuffer, size_t iwLength, char* paBuffer, size_t iaBufferSize);
    CLASS_DECL_ZTOOLSO int UTF8BufferToWideBuffer(const char* paBuffer, size_t iaBufferSize, TCHAR* pwBuffer, size_t iwLength);

#endif

    /// <summary>
    /// Template helper to get a string of a certain type.
    /// </summary>
    template<typename RT, typename ST>
    RT GetString(ST&& str)
    {
        static_assert(std::is_same_v<RT, std::string> ||
                      std::is_same_v<RT, std::wstring>);

        if constexpr(std::is_same_v<RT, std::remove_reference_t<ST>>)
        {
            return std::forward<ST>(str);
        }

        else if constexpr(( std::is_same_v<RT, std::string> && std::is_same_v<std::remove_cv_t<std::remove_reference_t<ST>>, std::string_view> ) ||
                          ( std::is_same_v<RT, std::wstring> && std::is_same_v<std::remove_cv_t<std::remove_reference_t<ST>>, wstring_view> ))
        {
            return RT(str);
        }

        else if constexpr(std::is_same_v<RT, std::wstring>)
        {
            return UTF8ToWide(str.data(), static_cast<int>(str.length()));
        }

        else
        {
            return WideToUTF8(str.data(), static_cast<int>(str.length()));
        }
    }
};
