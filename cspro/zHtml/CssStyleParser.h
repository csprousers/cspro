#pragma once

#include <zHtml/zHtml.h>

/// <summary>
/// Extract attributes from a CSS style string like "font-family: Arial; font-size: 10;"
/// </summary>

class ZHTML_API CssStyleParser
{
public:
    static std::optional<std::wstring> Attribute(NullTerminatedString attribute_name, NullTerminatedString css);

    static std::optional<std::wstring> FontName(NullTerminatedString css);

    static std::optional<int> FontSize(NullTerminatedString css);

    static bool Bold(NullTerminatedString css);

    static bool Italic(NullTerminatedString css);

    static bool Underline(NullTerminatedString css);

    static std::optional<COLORREF> TextColor(NullTerminatedString css);

    static LOGFONT ToLogfont(NullTerminatedString css);
};
