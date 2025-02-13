#pragma once


inline std::string ToLowerUtf8(wstring_view sv)
{
    return UTF8Convert::WideToUTF8(SO::ToLower(sv));
}
