#include "StdAfx.h"
#include "PortableColor.h"


namespace
{
    struct ColorTableEntry
    {
        const char* name;
        const uint32_t colorint;
    };

    const ColorTableEntry ColorTable[] =
    {
        { "AliceBlue",            0xfff0f8ff },
        { "AntiqueWhite",         0xfffaebd7 },
        { "Aqua",                 0xff00ffff },
        { "Aquamarine",           0xff7fffd4 },
        { "Azure",                0xfff0ffff },
        { "Beige",                0xfff5f5dc },
        { "Bisque",               0xffffe4c4 },
        { "Black",                0xff000000 },
        { "BlanchedAlmond",       0xffffebcd },
        { "Blue",                 0xff0000ff },
        { "BlueViolet",           0xff8a2be2 },
        { "Brown",                0xffa52a2a },
        { "BurlyWood",            0xffdeb887 },
        { "CadetBlue",            0xff5f9ea0 },
        { "Chartreuse",           0xff7fff00 },
        { "Chocolate",            0xffd2691e },
        { "Coral",                0xffff7f50 },
        { "CornflowerBlue",       0xff6495ed },
        { "Cornsilk",             0xfffff8dc },
        { "Crimson",              0xffdc143c },
        { "Cyan",                 0xff00ffff },
        { "DarkBlue",             0xff00008b },
        { "DarkCyan",             0xff008b8b },
        { "DarkGoldenRod",        0xffb8860b },
        { "DarkGray",             0xffa9a9a9 },
        { "DarkGrey",             0xffa9a9a9 },
        { "DarkGreen",            0xff006400 },
        { "DarkKhaki",            0xffbdb76b },
        { "DarkMagenta",          0xff8b008b },
        { "DarkOliveGreen",       0xff556b2f },
        { "Darkorange",           0xffff8c00 },
        { "DarkOrchid",           0xff9932cc },
        { "DarkRed",              0xff8b0000 },
        { "DarkSalmon",           0xffe9967a },
        { "DarkSeaGreen",         0xff8fbc8f },
        { "DarkSlateBlue",        0xff483d8b },
        { "DarkSlateGray",        0xff2f4f4f },
        { "DarkSlateGrey",        0xff2f4f4f },
        { "DarkTurquoise",        0xff00ced1 },
        { "DarkViolet",           0xff9400d3 },
        { "DeepPink",             0xffff1493 },
        { "DeepSkyBlue",          0xff00bfff },
        { "DimGray",              0xff696969 },
        { "DimGrey",              0xff696969 },
        { "DodgerBlue",           0xff1e90ff },
        { "FireBrick",            0xffb22222 },
        { "FloralWhite",          0xfffffaf0 },
        { "ForestGreen",          0xff228b22 },
        { "Fuchsia",              0xffff00ff },
        { "Gainsboro",            0xffdcdcdc },
        { "GhostWhite",           0xfff8f8ff },
        { "Gold",                 0xffffd700 },
        { "GoldenRod",            0xffdaa520 },
        { "Gray",                 0xff808080 },
        { "Grey",                 0xff808080 },
        { "Green",                0xff008000 },
        { "GreenYellow",          0xffadff2f },
        { "HoneyDew",             0xfff0fff0 },
        { "HotPink",              0xffff69b4 },
        { "IndianRed",            0xffcd5c5c },
        { "Indigo",               0xff4b0082 },
        { "Ivory",                0xfffffff0 },
        { "Khaki",                0xfff0e68c },
        { "Lavender",             0xffe6e6fa },
        { "LavenderBlush",        0xfffff0f5 },
        { "LawnGreen",            0xff7cfc00 },
        { "LemonChiffon",         0xfffffacd },
        { "LightBlue",            0xffadd8e6 },
        { "LightCoral",           0xfff08080 },
        { "LightCyan",            0xffe0ffff },
        { "LightGoldenRodYellow", 0xfffafad2 },
        { "LightGray",            0xffd3d3d3 },
        { "LightGrey",            0xffd3d3d3 },
        { "LightGreen",           0xff90ee90 },
        { "LightPink",            0xffffb6c1 },
        { "LightSalmon",          0xffffa07a },
        { "LightSeaGreen",        0xff20b2aa },
        { "LightSkyBlue",         0xff87cefa },
        { "LightSlateGray",       0xff778899 },
        { "LightSlateGrey",       0xff778899 },
        { "LightSteelBlue",       0xffb0c4de },
        { "LightYellow",          0xffffffe0 },
        { "Lime",                 0xff00ff00 },
        { "LimeGreen",            0xff32cd32 },
        { "Linen",                0xfffaf0e6 },
        { "Magenta",              0xffff00ff },
        { "Maroon",               0xff800000 },
        { "MediumAquaMarine",     0xff66cdaa },
        { "MediumBlue",           0xff0000cd },
        { "MediumOrchid",         0xffba55d3 },
        { "MediumPurple",         0xff9370d8 },
        { "MediumSeaGreen",       0xff3cb371 },
        { "MediumSlateBlue",      0xff7b68ee },
        { "MediumSpringGreen",    0xff00fa9a },
        { "MediumTurquoise",      0xff48d1cc },
        { "MediumVioletRed",      0xffc71585 },
        { "MidnightBlue",         0xff191970 },
        { "MintCream",            0xfff5fffa },
        { "MistyRose",            0xffffe4e1 },
        { "Moccasin",             0xffffe4b5 },
        { "NavajoWhite",          0xffffdead },
        { "Navy",                 0xff000080 },
        { "OldLace",              0xfffdf5e6 },
        { "Olive",                0xff808000 },
        { "OliveDrab",            0xff6b8e23 },
        { "Orange",               0xffffa500 },
        { "OrangeRed",            0xffff4500 },
        { "Orchid",               0xffda70d6 },
        { "PaleGoldenRod",        0xffeee8aa },
        { "PaleGreen",            0xff98fb98 },
        { "PaleTurquoise",        0xffafeeee },
        { "PaleVioletRed",        0xffd87093 },
        { "PapayaWhip",           0xffffefd5 },
        { "PeachPuff",            0xffffdab9 },
        { "Peru",                 0xffcd853f },
        { "Pink",                 0xffffc0cb },
        { "Plum",                 0xffdda0dd },
        { "PowderBlue",           0xffb0e0e6 },
        { "Purple",               0xff800080 },
        { "Red",                  0xffff0000 },
        { "RosyBrown",            0xffbc8f8f },
        { "RoyalBlue",            0xff4169e1 },
        { "SaddleBrown",          0xff8b4513 },
        { "Salmon",               0xfffa8072 },
        { "SandyBrown",           0xfff4a460 },
        { "SeaGreen",             0xff2e8b57 },
        { "SeaShell",             0xfffff5ee },
        { "Sienna",               0xffa0522d },
        { "Silver",               0xffc0c0c0 },
        { "SkyBlue",              0xff87ceeb },
        { "SlateBlue",            0xff6a5acd },
        { "SlateGray",            0xff708090 },
        { "SlateGrey",            0xff708090 },
        { "Snow",                 0xfffffafa },
        { "SpringGreen",          0xff00ff7f },
        { "SteelBlue",            0xff4682b4 },
        { "Tan",                  0xffd2b48c },
        { "Teal",                 0xff008080 },
        { "Thistle",              0xffd8bfd8 },
        { "Tomato",               0xffff6347 },
        { "Turquoise",            0xff40e0d0 },
        { "Violet",               0xffee82ee },
        { "Wheat",                0xfff5deb3 },
        { "White",                0xffffffff },
        { "WhiteSmoke",           0xfff5f5f5 },
        { "Yellow",               0xffffff00 },
        { "YellowGreen",          0xff9acd32 },
    };


    constexpr uint32_t RGBA_to_ColorInt(const uint32_t red, const uint32_t green, const uint32_t blue, const uint32_t alpha)
    {
        return ( alpha << 24 ) |
               ( red   << 16 ) |
               ( green <<  8 ) |
               ( blue  <<  0 );
    }

    constexpr uint32_t RGBA_to_ColorInt(const uint32_t rgba)
    {
        return ( ( rgba & 0xffffff00 ) >>  8 ) |
               ( ( rgba & 0x000000ff ) << 24 );
    }

    constexpr uint32_t COLORREF_to_ColorInt(const COLORREF colorref)
    {
        return RGBA_to_ColorInt(GetRValue(colorref), GetGValue(colorref), GetBValue(colorref), 0xff);
    }

    constexpr COLORREF ColorInt_to_COLORREF(const uint32_t colorint)
    {
        return RGB(( colorint >> 16 ) & 0xff,
                   ( colorint >>  8 ) & 0xff,
                   ( colorint >>  0 ) & 0xff);
    }
}


const PortableColor PortableColor::Black = FromRGB(0, 0, 0);
const PortableColor PortableColor::White = FromRGB(255, 255, 255);


PortableColor::PortableColor(const uint32_t colorint, const COLORREF colorref)
    :   m_colorint(colorint),
        m_colorref(colorref)
{
}


PortableColor::PortableColor() // black
    :   PortableColor(RGBA_to_ColorInt(0, 0, 0, 0xff), RGB(0, 0, 0))
{
}


PortableColor PortableColor::FromColorInt(const uint32_t colorint)
{
    return PortableColor(colorint, ColorInt_to_COLORREF(colorint));
}


PortableColor PortableColor::FromCOLORREF(const COLORREF colorref)
{
    return PortableColor(COLORREF_to_ColorInt(colorref), colorref);
}


PortableColor PortableColor::FromRGB(const uint32_t red, const uint32_t green, const uint32_t blue)
{
    return FromColorInt(RGBA_to_ColorInt(red, green, blue, 0xff));
}


std::optional<PortableColor> PortableColor::FromString(const cs::string_view_sz text_sv)
{
    // process colors specified with a # sign
    if( text_sv.length() >= 4 && text_sv.front() == '#' )
    {
        const bool using_shorthand = ( text_sv.length() == 4 || text_sv.length() == 5 );
        const bool alpha_specified = ( text_sv.length() == 5 || text_sv.length() == 9 );

        uint32_t rgba = strtoul(text_sv.c_str() + 1, nullptr, 16);

        // duplicate the hex codes if necessary; e.g., #fab -> #ffaabb
        if( using_shorthand )
        {
            const char nibble1 = ( rgba & 0xf000 ) >> 12;
            const char nibble2 = ( rgba & 0x0f00 ) >> 8;
            const char nibble3 = ( rgba & 0x00f0 ) >> 4;
            const char nibble4 = ( rgba & 0x000f );

            rgba = ( ( ( nibble1 << 4 ) | nibble1 ) << 24 ) |
                   ( ( ( nibble2 << 4 ) | nibble2 ) << 16 ) |
                   ( ( ( nibble3 << 4 ) | nibble3 ) <<  8 ) |
                   ( ( ( nibble4 << 4 ) | nibble4 ) <<  0 );
        }

        // add the alpha channel if necessary
        if( !alpha_specified )
            rgba = ( rgba << 8 ) | 0xff;

        return FromColorInt(RGBA_to_ColorInt(rgba));
    }


    // process colors specified using rgb(...)
    else if( SO::StartsWithNoCase(text_sv, "rgb") )
    {
        const std::string_view color_text_sv = SO::GetTextBetweenCharacters(text_sv.substr(3), '(', ')');
        const std::vector<std::string> colors = SO::SplitString(color_text_sv, ',');

        if( colors.size() == 3 )
            return FromRGB(atoi(colors[0].c_str()), atoi(colors[1].c_str()), atoi(colors[2].c_str()));
    }


    // process colors specified by color name
    else
    {
        for( size_t i = 0; i < _countof(ColorTable); ++i )
        {
            if( SO::EqualsNoCase(text_sv, ColorTable[i].name) )
                return FromColorInt(ColorTable[i].colorint);
        }
    }

    return std::nullopt;
}


std::string PortableColor::ToString(const bool use_color_names, const bool include_alpha_channel) const
{
    if( use_color_names )
    {
        for( size_t i = 0; i < _countof(ColorTable); ++i )
        {
            if( ColorTable[i].colorint == m_colorint )
                return ColorTable[i].name;
        }
    }

    std::string text = FormatText("#%02x%02x%02x", static_cast<unsigned int>(GetRValue(m_colorref)),
                                                   static_cast<unsigned int>(GetGValue(m_colorref)),
                                                   static_cast<unsigned int>(GetBValue(m_colorref)));

    if( include_alpha_channel )
        text.append(FormatText("%02x", static_cast<unsigned int>(( m_colorint >> 24 ) & 0xff)));

    return text;
}


PortableColor PortableColor::CreateFromJson(const JsonNode& json_node)
{
    const std::string text = json_node.Get<std::string>();
    std::optional<PortableColor> portable_color = FromString(text);

    if( !portable_color.has_value() )
        throw JsonParseException("The color '%s' is not valid", text.c_str());

    return *portable_color;    
}


void PortableColor::WriteJson(JsonWriter& json_writer) const
{
    json_writer.Write(ToString(false));
}


void PortableColor::serialize(Serializer& ar)
{
    ar & m_colorint;

    if( ar.IsLoading() )
        m_colorref = ColorInt_to_COLORREF(m_colorint);
}
