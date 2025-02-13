// the license below applies to the original source code, which has been
// modified to support the template approach
#include "StdAfx.h"
#include "base64.h"

/* 
   base64.cpp and base64.h

   base64 encoding and decoding with C++.

   Version: 1.01.00

   Copyright (C) 2004-2017 René Nyffenegger

   This source code is provided 'as-is', without any express or implied
   warranty. In no event will the author be held liable for any damages
   arising from the use of this software.

   Permission is granted to anyone to use this software for any purpose,
   including commercial applications, and to alter it and redistribute it
   freely, subject to the following restrictions:

   1. The origin of this source code must not be misrepresented; you must not
      claim that you wrote the original source code. If you use this source code
      in a product, an acknowledgment in the product documentation would be
      appreciated but is not required.

   2. Altered source versions must be plainly marked as such, and must not be
      misrepresented as being the original source code.

   3. This notice may not be removed or altered from any source distribution.

   René Nyffenegger rene.nyffenegger@adp-gmbh.ch

*/


namespace
{
    const std::string  base64_chars_string  =    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    const std::wstring base64_chars_wstring = _T("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/");
}

template<typename StringType>
const auto& base64_chars()
{
    if constexpr(std::is_same_v<StringType, std::wstring> ||
                 std::is_same_v<StringType, wstring_view> )
    {
        return base64_chars_wstring;
    }

    else
    {
        return base64_chars_string;
    }
}


template<typename CharType>
bool inline is_base64(CharType c)
{
  return (isalnum(c) || (c == '+') || (c == '/'));
}


template<typename StringType>
StringType base64_encode(unsigned char const* bytes_to_encode, size_t in_len)
{
  StringType ret;
  int i = 0;
  int j = 0;
  unsigned char char_array_3[3];
  unsigned char char_array_4[4];

  while (in_len--) {
    char_array_3[i++] = *(bytes_to_encode++);
    if (i == 3) {
      char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
      char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
      char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
      char_array_4[3] = char_array_3[2] & 0x3f;

      for(i = 0; (i <4) ; i++)
        ret += base64_chars<StringType>()[char_array_4[i]];
      i = 0;
    }
  }

  if (i)
  {
    for(j = i; j < 3; j++)
      char_array_3[j] = '\0';

    char_array_4[0] = ( char_array_3[0] & 0xfc) >> 2;
    char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
    char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);

    for (j = 0; (j < i + 1); j++)
      ret += base64_chars<StringType>()[char_array_4[j]];

    while((i++ < 3))
      ret += '=';

  }

  return ret;
}



namespace
{
    template<typename ReturnType, typename CharType>
    void AppendChar(ReturnType& return_value, CharType ch)
    {
        if constexpr(std::is_same_v<ReturnType, std::vector<std::byte>>)
        {
            ASSERT(static_cast<wchar_t>(ch) == static_cast<wchar_t>(static_cast<std::byte>(ch)));
            return_value.emplace_back(static_cast<std::byte>(ch));
        }

        else
        {
            return_value += ch;
        }
    }
}

template<typename ReturnType, typename StringType, typename CharType>
ReturnType base64_decode(const StringType& encoded_string)
{
  size_t in_len = encoded_string.size();
  int i = 0;
  int j = 0;
  int in_ = 0;
  CharType char_array_4[4];
  CharType char_array_3[3];
  ReturnType ret;

  while (in_len-- && ( encoded_string[in_] != '=') && is_base64(encoded_string[in_])) {
    char_array_4[i++] = encoded_string[in_]; in_++;
    if (i ==4) {
      for (i = 0; i <4; i++)
        char_array_4[i] = base64_chars<StringType>().find(char_array_4[i]) & 0xff;

      char_array_3[0] = ( char_array_4[0] << 2       ) + ((char_array_4[1] & 0x30) >> 4);
      char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
      char_array_3[2] = ((char_array_4[2] & 0x3) << 6) +   char_array_4[3];

      for (i = 0; (i < 3); i++) {
          AppendChar(ret, char_array_3[i]);
      }
      i = 0;
    }
  }

  if (i) {
    for (j = 0; j < i; j++)
      char_array_4[j] = base64_chars<StringType>().find(char_array_4[j]) & 0xff;

    char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
    char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);

    for (j = 0; (j < i - 1); j++) {
        AppendChar(ret, char_array_3[j]);
    }
  }

  return ret;
}



template<> CLASS_DECL_ZTOOLSO std::string Base64::Encode<std::string>(const void* contents, size_t size)
{
    return base64_encode<std::string>(reinterpret_cast<const unsigned char*>(contents), size);
}

template<> CLASS_DECL_ZTOOLSO std::wstring Base64::Encode<std::wstring>(const void* contents, size_t size)
{
    return base64_encode<std::wstring>(reinterpret_cast<const unsigned char*>(contents), size);
}


template<> CLASS_DECL_ZTOOLSO std::string Base64::Decode(const std::string& encoded_string)
{
    return base64_decode<std::string, std::string, unsigned char>(encoded_string);
}

template<> CLASS_DECL_ZTOOLSO std::wstring Base64::Decode(const std::wstring& encoded_string)
{
    return base64_decode<std::wstring, std::wstring, wchar_t>(encoded_string);
}

template<> CLASS_DECL_ZTOOLSO std::vector<std::byte> Base64::Decode(const std::wstring& encoded_string)
{
    return base64_decode<std::vector<std::byte>, std::wstring, wchar_t>(encoded_string);
}

template<> CLASS_DECL_ZTOOLSO std::vector<std::byte> Base64::Decode(const wstring_view& encoded_string)
{
    return base64_decode<std::vector<std::byte>, wstring_view, wchar_t>(encoded_string);
}

template<> CLASS_DECL_ZTOOLSO CString Base64::Decode(const CString& encoded_string)
{
    return WS2CS(Decode<std::wstring>(CS2WS(encoded_string)));
}
