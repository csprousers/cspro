#include "StdAfx.h"
#include "PortableColor.h"


namespace
{
    struct ColorTableEntry
    {
        const TCHAR* name;
        const uint32_t colorint;
    };

    ColorTableEntry ColorTable[] =
    {
        { _T("AliceBlue"),            0xfff0f8ff },
        { _T("AntiqueWhite"),         0xfffaebd7 },
        { _T("Aqua"),                 0xff00ffff },
        { _T("Aquamarine"),           0xff7fffd4 },
        { _T("Azure"),                0xfff0ffff },
        { _T("Beige"),                0xfff5f5dc },
        { _T("Bisque"),               0xffffe4c4 },
        { _T("Black"),                0xff000000 },
        { _T("BlanchedAlmond"),       0xffffebcd },
        { _T("Blue"),                 0xff0000ff },
        { _T("BlueViolet"),           0xff8a2be2 },
        { _T("Brown"),                0xffa52a2a },
        { _T("BurlyWood"),            0xffdeb887 },
        { _T("CadetBlue"),            0xff5f9ea0 },
        { _T("Chartreuse"),           0xff7fff00 },
        { _T("Chocolate"),            0xffd2691e },
        { _T("Coral"),                0xffff7f50 },
        { _T("CornflowerBlue"),       0xff6495ed },
        { _T("Cornsilk"),             0xfffff8dc },
        { _T("Crimson"),              0xffdc143c },
        { _T("Cyan"),                 0xff00ffff },
        { _T("DarkBlue"),             0xff00008b },
        { _T("DarkCyan"),             0xff008b8b },
        { _T("DarkGoldenRod"),        0xffb8860b },
        { _T("DarkGray"),             0xffa9a9a9 },
        { _T("DarkGrey"),             0xffa9a9a9 },
        { _T("DarkGreen"),            0xff006400 },
        { _T("DarkKhaki"),            0xffbdb76b },
        { _T("DarkMagenta"),          0xff8b008b },
        { _T("DarkOliveGreen"),       0xff556b2f },
        { _T("Darkorange"),           0xffff8c00 },
        { _T("DarkOrchid"),           0xff9932cc },
        { _T("DarkRed"),              0xff8b0000 },
        { _T("DarkSalmon"),           0xffe9967a },
        { _T("DarkSeaGreen"),         0xff8fbc8f },
        { _T("DarkSlateBlue"),        0xff483d8b },
        { _T("DarkSlateGray"),        0xff2f4f4f },
        { _T("DarkSlateGrey"),        0xff2f4f4f },
        { _T("DarkTurquoise"),        0xff00ced1 },
        { _T("DarkViolet"),           0xff9400d3 },
        { _T("DeepPink"),             0xffff1493 },
        { _T("DeepSkyBlue"),          0xff00bfff },
        { _T("DimGray"),              0xff696969 },
        { _T("DimGrey"),              0xff696969 },
        { _T("DodgerBlue"),           0xff1e90ff },
        { _T("FireBrick"),            0xffb22222 },
        { _T("FloralWhite"),          0xfffffaf0 },
        { _T("ForestGreen"),          0xff228b22 },
        { _T("Fuchsia"),              0xffff00ff },
        { _T("Gainsboro"),            0xffdcdcdc },
        { _T("GhostWhite"),           0xfff8f8ff },
        { _T("Gold"),                 0xffffd700 },
        { _T("GoldenRod"),            0xffdaa520 },
        { _T("Gray"),                 0xff808080 },
        { _T("Grey"),                 0xff808080 },
        { _T("Green"),                0xff008000 },
        { _T("GreenYellow"),          0xffadff2f },
        { _T("HoneyDew"),             0xfff0fff0 },
        { _T("HotPink"),              0xffff69b4 },
        { _T("IndianRed"),            0xffcd5c5c },
        { _T("Indigo"),               0xff4b0082 },
        { _T("Ivory"),                0xfffffff0 },
        { _T("Khaki"),                0xfff0e68c },
        { _T("Lavender"),             0xffe6e6fa },
        { _T("LavenderBlush"),        0xfffff0f5 },
        { _T("LawnGreen"),            0xff7cfc00 },
        { _T("LemonChiffon"),         0xfffffacd },
        { _T("LightBlue"),            0xffadd8e6 },
        { _T("LightCoral"),           0xfff08080 },
        { _T("LightCyan"),            0xffe0ffff },
        { _T("LightGoldenRodYellow"), 0xfffafad2 },
        { _T("LightGray"),            0xffd3d3d3 },
        { _T("LightGrey"),            0xffd3d3d3 },
        { _T("LightGreen"),           0xff90ee90 },
        { _T("LightPink"),            0xffffb6c1 },
        { _T("LightSalmon"),          0xffffa07a },
        { _T("LightSeaGreen"),        0xff20b2aa },
        { _T("LightSkyBlue"),         0xff87cefa },
        { _T("LightSlateGray"),       0xff778899 },
        { _T("LightSlateGrey"),       0xff778899 },
        { _T("LightSteelBlue"),       0xffb0c4de },
        { _T("LightYellow"),          0xffffffe0 },
        { _T("Lime"),                 0xff00ff00 },
        { _T("LimeGreen"),            0xff32cd32 },
        { _T("Linen"),                0xfffaf0e6 },
        { _T("Magenta"),              0xffff00ff },
        { _T("Maroon"),               0xff800000 },
        { _T("MediumAquaMarine"),     0xff66cdaa },
        { _T("MediumBlue"),           0xff0000cd },
        { _T("MediumOrchid"),         0xffba55d3 },
        { _T("MediumPurple"),         0xff9370d8 },
        { _T("MediumSeaGreen"),       0xff3cb371 },
        { _T("MediumSlateBlue"),      0xff7b68ee },
        { _T("MediumSpringGreen"),    0xff00fa9a },
        { _T("MediumTurquoise"),      0xff48d1cc },
        { _T("MediumVioletRed"),      0xffc71585 },
        { _T("MidnightBlue"),         0xff191970 },
        { _T("MintCream"),            0xfff5fffa },
        { _T("MistyRose"),            0xffffe4e1 },
        { _T("Moccasin"),             0xffffe4b5 },
        { _T("NavajoWhite"),          0xffffdead },
        { _T("Navy"),                 0xff000080 },
        { _T("OldLace"),              0xfffdf5e6 },
        { _T("Olive"),                0xff808000 },
        { _T("OliveDrab"),            0xff6b8e23 },
        { _T("Orange"),               0xffffa500 },
        { _T("OrangeRed"),            0xffff4500 },
        { _T("Orchid"),               0xffda70d6 },
        { _T("PaleGoldenRod"),        0xffeee8aa },
        { _T("PaleGreen"),            0xff98fb98 },
        { _T("PaleTurquoise"),        0xffafeeee },
        { _T("PaleVioletRed"),        0xffd87093 },
        { _T("PapayaWhip"),           0xffffefd5 },
        { _T("PeachPuff"),            0xffffdab9 },
        { _T("Peru"),                 0xffcd853f },
        { _T("Pink"),                 0xffffc0cb },
        { _T("Plum"),                 0xffdda0dd },
        { _T("PowderBlue"),           0xffb0e0e6 },
        { _T("Purple"),               0xff800080 },
        { _T("Red"),                  0xffff0000 },
        { _T("RosyBrown"),            0xffbc8f8f },
        { _T("RoyalBlue"),            0xff4169e1 },
        { _T("SaddleBrown"),          0xff8b4513 },
        { _T("Salmon"),               0xfffa8072 },
        { _T("SandyBrown"),           0xfff4a460 },
        { _T("SeaGreen"),             0xff2e8b57 },
        { _T("SeaShell"),             0xfffff5ee },
        { _T("Sienna"),               0xffa0522d },
        { _T("Silver"),               0xffc0c0c0 },
        { _T("SkyBlue"),              0xff87ceeb },
        { _T("SlateBlue"),            0xff6a5acd },
        { _T("SlateGray"),            0xff708090 },
        { _T("SlateGrey"),            0xff708090 },
        { _T("Snow"),                 0xfffffafa },
        { _T("SpringGreen"),          0xff00ff7f },
        { _T("SteelBlue"),            0xff4682b4 },
        { _T("Tan"),                  0xffd2b48c },
        { _T("Teal"),                 0xff008080 },
        { _T("Thistle"),              0xffd8bfd8 },
        { _T("Tomato"),               0xffff6347 },
        { _T("Turquoise"),            0xff40e0d0 },
        { _T("Violet"),               0xffee82ee },
        { _T("Wheat"),                0xfff5deb3 },
        { _T("White"),                0xffffffff },
        { _T("WhiteSmoke"),           0xfff5f5f5 },
        { _T("Yellow"),               0xffffff00 },
        { _T("YellowGreen"),          0xff9acd32 },
    };


    inline uint32_t RGBA_to_ColorInt(uint32_t red, uint32_t green, uint32_t blue, uint32_t alpha)
    {
        return ( alpha << 24 ) |
               ( red   << 16 ) |
               ( green <<  8 ) |
               ( blue  <<  0 );
    }

    inline uint32_t RGBA_to_ColorInt(uint32_t rgba)
    {
        return ( ( rgba & 0xffffff00 ) >>  8 ) |
               ( ( rgba & 0x000000ff ) << 24 );
    }

    inline uint32_t COLORREF_to_ColorInt(COLORREF colorref)
    {
        return RGBA_to_ColorInt(GetRValue(colorref), GetGValue(colorref), GetBValue(colorref), 0xff);
    }

    inline COLORREF ColorInt_to_COLORREF(uint32_t colorint)
    {
        return RGB(( colorint >> 16 ) & 0xff,
                   ( colorint >>  8 ) & 0xff,
                   ( colorint >>  0 ) & 0xff);
    }
}


const PortableColor PortableColor::Black = FromRGB(0, 0, 0);
const PortableColor PortableColor::White = FromRGB(255, 255, 255);


PortableColor::PortableColor(uint32_t colorint, COLORREF colorref)
    :   m_colorint(colorint),
        m_colorref(colorref)
{
}

PortableColor::PortableColor() // black
    :   PortableColor(RGBA_to_ColorInt(0, 0, 0, 0xff), RGB(0, 0, 0))
{
}


PortableColor PortableColor::FromColorInt(uint32_t colorint)
{
    return PortableColor(colorint, ColorInt_to_COLORREF(colorint));
}

PortableColor PortableColor::FromCOLORREF(COLORREF colorref)
{
    return PortableColor(COLORREF_to_ColorInt(colorref), colorref);
}

PortableColor PortableColor::FromRGB(uint32_t red, uint32_t green, uint32_t blue)
{
    return FromColorInt(RGBA_to_ColorInt(red, green, blue, 0xff));
}


std::optional<PortableColor> PortableColor::FromString(NullTerminatedStringView text)
{
    // process colors specified with a # sign
    if( text.length() >= 4 && text.front() == '#' )
    {
        bool using_shorthand = ( text.length() == 4 || text.length() == 5 );
        bool alpha_specified = ( text.length() == 5 || text.length() == 9 );

        uint32_t rgba = _tcstoul(text.c_str() + 1, nullptr, 16);

        // duplicate the hex codes if necessary; e.g., #fab -> #ffaabb
        if( using_shorthand )
        {
            char nibble1 = ( rgba & 0xf000 ) >> 12;
            char nibble2 = ( rgba & 0x0f00 ) >> 8;
            char nibble3 = ( rgba & 0x00f0 ) >> 4;
            char nibble4 = ( rgba & 0x000f );

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
    else if( SO::StartsWithNoCase(text, _T("rgb")) )
    {
        wstring_view color_text = SO::GetTextBetweenCharacters(text.substr(3), '(', ')');
        std::vector<std::wstring> colors = SO::SplitString(color_text, ',');

        if( colors.size() == 3 )
            return FromRGB(_ttoi(colors[0].c_str()), _ttoi(colors[1].c_str()), _ttoi(colors[2].c_str()));
    }


    // process colors specified by color name
    else
    {
        for( size_t i = 0; i < _countof(ColorTable); ++i )
        {
            if( SO::EqualsNoCase(text, ColorTable[i].name) )
                return FromColorInt(ColorTable[i].colorint);
        }
    }

    return std::nullopt;
}


std::wstring PortableColor::ToString(bool use_color_names, bool include_alpha_channel) const
{
    if( use_color_names )
    {
        for( size_t i = 0; i < _countof(ColorTable); ++i )
        {
            if( ColorTable[i].colorint == m_colorint )
                return ColorTable[i].name;
        }
    }

    std::wstring text = FormatTextCS2WS(_T("#%02x%02x%02x"), GetRValue(m_colorref), GetGValue(m_colorref), GetBValue(m_colorref));

    if( include_alpha_channel )
        text.append(FormatTextCS2WS(_T("%02x"), ( m_colorint >> 24 ) & 0xff));

    return text;
}


PortableColor PortableColor::CreateFromJson(const JsonNode<wchar_t>& json_node)
{
    std::wstring text = json_node.Get<std::wstring>();
    std::optional<PortableColor> portable_color = FromString(text);

    if( !portable_color.has_value() )
        throw JsonParseException(_T("The color '%s' is not valid"), text.c_str());

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
