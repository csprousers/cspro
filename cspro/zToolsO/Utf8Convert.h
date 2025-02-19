#pragma once

#include <zToolsO/zToolsO.h>
#include <zToolsO/Tools.h>


// Utility class for converting between character encodings

class UTF8Convert
{
public:
    /// <summary>
    /// Convert from UTF-8 encoded string to wide character (2 bytes per char) string.
    /// Only need to provide stringLen if pUTF8String is not null-terminated.
    /// </summary>
    template<typename T = std::wstring>
    static T UTF8ToWide(const char* utf8_text, int utf8_length = -1)
    {
        return UTF8ToWideWorker<T>(utf8_text, utf8_length);
    }

    static std::wstring UTF8ToWide(const unsigned char* utf8_text, int utf8_length = -1)
    {
        return UTF8ToWideWorker<std::wstring>(reinterpret_cast<const char*>(utf8_text), utf8_length);
    }

    template<typename T = std::wstring, typename ST>
    static T UTF8ToWide(const ST& utf8_text)
    {
        return UTF8ToWideWorker<T>(utf8_text.data(), static_cast<int>(utf8_text.length()));
    }


    /// <summary>
    /// Convert wide character (2 bytes per char) string to UTF-8 encoded string.
    /// Only need to provide wide_length if wide_string is not null-terminated.
    /// </summary>
    CLASS_DECL_ZTOOLSO static std::string WideToUTF8(const wchar_t* wide_string, int wide_length = -1);

    static std::string WideToUTF8(std::wstring_view str)
    {
        return WideToUTF8(str.data(), static_cast<int>(str.length()));
    }


#ifdef WIN32
    /// <summary>
    /// Convert multibyte character (ANSI or UTF-8) buffer to a wide character buffer.
    /// </summary>
    static int EncodedCharsBufferToWideBuffer(Encoding eEncoding, const char* paBuffer, size_t iaLength, TCHAR* pwBuffer, size_t iwBufferSize)
    {
        return MultiByteToWideChar(( eEncoding == Encoding::Utf8 ) ? CP_UTF8 : CP_ACP, 0, paBuffer, static_cast<int>(iaLength), pwBuffer, static_cast<int>(iwBufferSize));
    }

    /// <summary>
    /// Convert wide character buffer to a UTF-8 buffer.
    /// </summary>
    static int WideBufferToUTF8Buffer(const TCHAR* pwBuffer, size_t iwLength, char* paBuffer, size_t iaBufferSize)
    {
        return WideCharToMultiByte(CP_UTF8, 0, pwBuffer, static_cast<int>(iwLength), paBuffer, static_cast<int>(iaBufferSize), NULL, NULL);
    }

    /// <summary>
    /// Convert UTF-8 character buffer to a wide character buffer.
    /// </summary>
    static int UTF8BufferToWideBuffer(const char* paBuffer, size_t iaBufferSize, TCHAR* pwBuffer, size_t iwLength)
    {
        return MultiByteToWideChar(CP_UTF8, 0, paBuffer, static_cast<int>(iaBufferSize), pwBuffer, static_cast<int>(iwLength));
    }

#else
    // CR_TODO ... improve Android performance for these methods
    CLASS_DECL_ZTOOLSO static int EncodedCharsBufferToWideBuffer(Encoding eEncoding, const char* paBuffer, size_t iaLength, TCHAR* pwBuffer, size_t iwBufferSize);
    CLASS_DECL_ZTOOLSO static int WideBufferToUTF8Buffer(const TCHAR* pwBuffer, size_t iwLength, char* paBuffer, size_t iaBufferSize);
    CLASS_DECL_ZTOOLSO static int UTF8BufferToWideBuffer(const char* paBuffer, size_t iaBufferSize, TCHAR* pwBuffer, size_t iwLength);

#endif

    /// <summary>
    /// Template helper to get a string of a certain type.
    /// </summary>
    template<typename RT, typename ST>
    static RT GetString(ST&& str)
    {
        static_assert(std::is_same_v<RT, std::string> ||
                      std::is_same_v<RT, std::wstring>);

        if constexpr(std::is_same_v<RT, std::remove_reference_t<ST>>)
        {
            return std::forward<ST>(str);
        }

        else if constexpr(( std::is_same_v<RT, std::string> && std::is_same_v<std::remove_cvref_t<ST>, std::string_view> ) ||
                          ( std::is_same_v<RT, std::wstring> && std::is_same_v<std::remove_cvref_t<ST>, wstring_view> ))
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

private:
    template<typename T>
    CLASS_DECL_ZTOOLSO static T UTF8ToWideWorker(const char* utf8_text, int utf8_length);
};
