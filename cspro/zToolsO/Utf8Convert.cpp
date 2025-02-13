#include "StdAfx.h"
#include "Utf8Convert.h"
#include "TextConverter.h"

#ifndef WIN32
#include <zPlatformO/MobileStringConversion.h>
#endif


template<typename T/* = std::wstring*/>
T UTF8Convert::UTF8ToWide(const char* utf_string, int utf8_length/* = -1*/)
{
#ifdef WIN32
    if( utf_string == nullptr )
        return T();

    int wide_length = MultiByteToWideChar(CP_UTF8, 0, utf_string, utf8_length, nullptr, 0);

    // don't count the terminating null character
    if( utf8_length == -1 )
        --wide_length;

    if constexpr(std::is_same_v<T, std::wstring>)
    {
        T wide_string(wide_length, '\0');
        MultiByteToWideChar(CP_UTF8, 0, utf_string, utf8_length, wide_string.data(), wide_length);
        return wide_string;
    }

    else
    {
        static_assert(constexpr(std::is_same_v<T, CString>));
        T wide_string;
        MultiByteToWideChar(CP_UTF8, 0, utf_string, utf8_length, wide_string.GetBufferSetLength(wide_length), wide_length);
        wide_string.ReleaseBuffer(wide_length);
        return wide_string;
    }

#else
    return UTF8ToWideAndroid(utf_string, utf8_length);

#endif
}

template CLASS_DECL_ZTOOLSO std::wstring UTF8Convert::UTF8ToWide(const char* utf_string, int utf8_length/* = -1*/);
template CLASS_DECL_ZTOOLSO CString UTF8Convert::UTF8ToWide(const char* utf_string, int utf8_length/* = -1*/);


#ifdef WIN32

namespace
{
    template<typename T>
    inline T WideToMultiByte(UINT CodePage, const wchar_t* wide_string, int wide_length)
    {
        if( wide_string == nullptr )
            return T();

        int multi_byte_length = WideCharToMultiByte(CodePage, 0, wide_string, wide_length, nullptr, 0, nullptr, nullptr);

        // don't count the terminating null character
        if( wide_length == -1 )
            --multi_byte_length;

        T multi_byte_string;
        multi_byte_string.resize(multi_byte_length);

        static_assert(sizeof(char) == sizeof(std::byte));
        WideCharToMultiByte(CodePage, 0, wide_string, wide_length, reinterpret_cast<char*>(multi_byte_string.data()), multi_byte_length, nullptr, nullptr);

        return multi_byte_string;
    }
}

#endif


std::string UTF8Convert::WideToUTF8(const wchar_t* wide_string, int wide_length/* = -1*/)
{
#ifdef WIN32
    return WideToMultiByte<std::string>(CP_UTF8, wide_string, wide_length);
#else
    return WideToUTF8Android(wide_string, wide_length);
#endif
}


std::vector<std::byte> UTF8Convert::WideToUTF8Buffer(std::wstring_view str)
{
#ifdef WIN32
    return WideToMultiByte<std::vector<std::byte>>(CP_UTF8, str.data(), static_cast<int>(str.length()));

#else
    static_assert(sizeof(char) == sizeof(std::byte));

    std::string utf8_string = WideToUTF8(str);
    const char* utf8_string_begin = utf8_string.data();
    const char* utf8_string_end = utf8_string_begin + utf8_string.length();

    return std::vector<std::byte>(reinterpret_cast<const std::byte*>(utf8_string_begin),
                                  reinterpret_cast<const std::byte*>(utf8_string_end));
#endif
}



#ifndef WIN32
int UTF8Convert::EncodedCharsBufferToWideBuffer(Encoding encoding, const char* paBuffer, size_t iaLength, TCHAR* pwBuffer, size_t iwBufferSize)
{
    return ( encoding == Encoding::Utf8 ) ? UTF8BufferToWideBufferAndroid(paBuffer,iaLength,pwBuffer,iwBufferSize) :
                                            TextConverter::WindowsAnsiToWideBuffer(paBuffer, iaLength, pwBuffer, iwBufferSize);
}

int UTF8Convert::WideBufferToUTF8Buffer(const TCHAR* pwBuffer, size_t iwLength, char* paBuffer, size_t iaBufferSize)
{
    return WideBufferToUTF8BufferAndroid(pwBuffer, iwLength, paBuffer, iaBufferSize);
}

int UTF8Convert::UTF8BufferToWideBuffer(const char* paBuffer, size_t iaBufferSize, TCHAR* pwBuffer, size_t iwLength)
{
    return UTF8BufferToWideBufferAndroid(paBuffer, iaBufferSize, pwBuffer, iwLength);
}
#endif
