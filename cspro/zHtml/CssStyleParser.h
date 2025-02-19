#pragma once

#include <zHtml/zHtml.h>


// --------------------------------------------------------------------------
// CssStyleParser
//
// This class extracts attributes from a CSS style string like:
//     "font-family: Arial; font-size: 10;"
// --------------------------------------------------------------------------

class ZHTML_API CssStyleParser
{
public:
    static std::optional<std::string> Attribute(cs::string_sz attribute_name, cs::string_sz css);

    static std::optional<std::string> FontName(cs::string_sz css);

    static std::optional<int> FontSize(cs::string_sz css);

    static bool Bold(cs::string_sz css);

    static bool Italic(cs::string_sz css);

    static bool Underline(cs::string_sz css);

    static std::optional<COLORREF> TextColor(cs::string_sz css);

    static LOGFONT ToLogfont(cs::string_sz css);
};
