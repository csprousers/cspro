#pragma once

#include <zUtilO/zUtilO.h>


// NameShortener is used to shorten CSPro names for storage in fixed-width locations of limited length
// such as TextRepositoryNotesFile's storage of field names.
//
// The shortening routine replaces CSPro name characters with Unicode characters, prepending the name
// with a shortening indicator: '!'.
//
// For example: "ABC" -> "!ڒ".

class CLASS_DECL_ZUTILO NameShortener
{
    static constexpr char ShorteningIndicator         = '!';
    static constexpr size_t WidesCharPerShortenedChar = 3;

public:
    static std::string Shorten(std::string cspro_name, size_t max_wide_length);
    static std::string Unshorten(std::string shortened_name);
};
