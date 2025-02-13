#include "stdafx.h"
#include "CssStyleParser.h"
#include <regex>


std::optional<std::wstring> CssStyleParser::Attribute(NullTerminatedString attribute_name, NullTerminatedString css)
{
    std::wregex re(FormatText(_T("%s:\\s*([^;]*);"), attribute_name.c_str()));
    std::wcmatch match;
    if (std::regex_search(css.c_str(), match, re))
        return match.str(1);
    else
        return {};
}


std::optional<std::wstring> CssStyleParser::FontName(NullTerminatedString css)
{
    return Attribute(_T("font-family"), css);
}


std::optional<int> CssStyleParser::FontSize(NullTerminatedString css)
{
    std::optional<std::wstring> size = Attribute(_T("font-size"), css);
    return size ? std::stoi(*size) : std::optional<int>{};
}


bool CssStyleParser::Bold(NullTerminatedString css)
{
    return Attribute(_T("font-weight"), css) == _T("bold");
}


bool CssStyleParser::Italic(NullTerminatedString css)
{
    return Attribute(_T("font-style"), css) == _T("italic");
}


bool CssStyleParser::Underline(NullTerminatedString css)
{
    return Attribute(_T("text-decoration"), css) == _T("underline");
}


std::optional<COLORREF> CssStyleParser::TextColor(NullTerminatedString css)
{
    std::optional<std::wstring> color_string = Attribute(_T("color"), css);
    if (color_string) {
        std::optional<PortableColor> portable_color = PortableColor::FromString(*color_string);
        if (portable_color)
            return portable_color->ToCOLORREF();
    }
    return std::nullopt;
}


LOGFONT CssStyleParser::ToLogfont(NullTerminatedString css)
{
    LOGFONT lf;
    memset(&lf, 0, sizeof(LOGFONT));
    std::optional<int> font_size = FontSize(css);
    CDC dc_screen;
    dc_screen.Attach(::GetDC(NULL));
    lf.lfHeight = -MulDiv(font_size ? *font_size : 12, dc_screen.GetDeviceCaps(LOGPIXELSY), 72);
    std::optional<std::wstring> font_name = FontName(css);
    if (font_name && font_name->length() < LF_FACESIZE )
        _tcscpy(lf.lfFaceName, font_name->c_str());
    lf.lfItalic = Italic(css);
    lf.lfWeight = Bold(css) ? FW_BOLD : FW_REGULAR;
    lf.lfUnderline = Underline(css);
    return lf;
}
