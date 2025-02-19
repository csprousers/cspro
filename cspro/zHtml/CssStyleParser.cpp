#include "stdafx.h"
#include "CssStyleParser.h"
#include <regex>


std::optional<std::string> CssStyleParser::Attribute(const cs::string_sz attribute_name, const cs::string_sz css)
{
    std::regex re(FormatText("%s:\\s*([^;]*);", attribute_name.c_str()));
    std::cmatch match;

    if( std::regex_search(css.c_str(), match, re) )
        return match.str(1);

    return std::nullopt;
}


std::optional<std::string> CssStyleParser::FontName(const cs::string_sz css)
{
    return Attribute("font-family", css);
}


std::optional<int> CssStyleParser::FontSize(const cs::string_sz css)
{
    const std::optional<std::string> size = Attribute("font-size", css);

    return size.has_value() ? std::make_optional(std::stoi(*size)) :
                              std::nullopt;
}


bool CssStyleParser::Bold(const cs::string_sz css)
{
    return ( Attribute("font-weight", css) == "bold" );
}


bool CssStyleParser::Italic(const cs::string_sz css)
{
    return ( Attribute("font-style", css) == "italic" );
}


bool CssStyleParser::Underline(const cs::string_sz css)
{
    return ( Attribute("text-decoration", css) == "underline" );
}


std::optional<COLORREF> CssStyleParser::TextColor(const cs::string_sz css)
{
    const std::optional<std::string> color_string = Attribute("color", css);

    if( color_string.has_value() )
    {
        const std::optional<PortableColor> portable_color = PortableColor::FromString(*color_string);

        if( portable_color.has_value() )
            return portable_color->ToCOLORREF();
    }

    return std::nullopt;
}


LOGFONT CssStyleParser::ToLogfont(const cs::string_sz css)
{
    const std::optional<int> font_size = FontSize(css);
    const std::optional<std::string> font_name = FontName(css);

    CDC dc_screen;
    dc_screen.Attach(::GetDC(NULL));

    LOGFONT lf;
    memset(&lf, 0, sizeof(LOGFONT));

    lf.lfHeight = -MulDiv(font_size.value_or(12), dc_screen.GetDeviceCaps(LOGPIXELSY), 72);

    if( font_name.has_value() )
        SO::CopyToFixedBuffer(lf.lfFaceName, TC::ToWide(*font_name));

    lf.lfItalic = Italic(css);
    lf.lfWeight = Bold(css) ? FW_BOLD : FW_REGULAR;
    lf.lfUnderline = Underline(css);

    return lf;
}
