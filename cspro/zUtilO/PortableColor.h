#pragma once

#include <zUtilO/zUtilO.h>

template<typename CharType> class JsonNode;
class JsonWriter;
class Serializer;


class CLASS_DECL_ZUTILO PortableColor
{
private:
    PortableColor(uint32_t colorint, COLORREF colorref);

public:
    static const PortableColor Black;
    static const PortableColor White;

    PortableColor(); // black

    static PortableColor FromColorInt(uint32_t colorint);
    static PortableColor FromCOLORREF(COLORREF colorref);
    static PortableColor FromRGB(uint32_t red, uint32_t green, uint32_t blue);

    COLORREF ToCOLORREF() const { return m_colorref; }
    int ToColorInt() const      { return (int)m_colorint; }

    bool operator==(const PortableColor& rhs_pc) const { return ( m_colorint == rhs_pc.m_colorint ); }
    bool operator!=(const PortableColor& rhs_pc) const { return ( m_colorint != rhs_pc.m_colorint ); }

    static std::optional<PortableColor> FromString(NullTerminatedStringView text);

    static PortableColor FromString(NullTerminatedStringView text, const PortableColor& default_color)
    {
        return FromString(text).value_or(default_color);
    }

    std::wstring ToStringRGBA(bool use_color_names = false) const
    {
        return ToString(use_color_names, true);
    }

    std::wstring ToStringRGB(bool use_color_names = false) const
    {
        return ToString(use_color_names, false);
    }

    /// <summary>
    /// By default, the HTML color name is returned when possible; if not, then #rrggbb is returned
    /// if the alpha channel is set to 0xff, or #rrggbbaa otherwise.
    /// </summary>
    std::wstring ToString(bool use_color_names = true) const
    {
        return ( ( m_colorint & 0xff000000 ) == 0xff000000 ) ? ToStringRGB(use_color_names) :
                                                               ToStringRGBA(use_color_names);
    }

    // serialization
    static PortableColor CreateFromJson(const JsonNode<wchar_t>& json_node);
    void WriteJson(JsonWriter& json_writer) const;

    void serialize(Serializer& ar);

private:
    std::wstring ToString(bool use_color_names, bool include_alpha_channel) const;

private:
    uint32_t m_colorint; // 32-bit AARRGGBB
    COLORREF m_colorref; // 24-bit BBGGRR
};
