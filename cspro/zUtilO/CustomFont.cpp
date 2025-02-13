#include "StdAfx.h"
#include "CustomFont.h"


// --------------------------------------------------------------------------
// UserDefinedFontWrapper
// --------------------------------------------------------------------------

class UserDefinedFontWrapper
{
public:
    UserDefinedFontWrapper(std::unique_ptr<CFont> font)
        :   m_font(std::move(font))
    {
    }

    CFont* GetFont() { return m_font.get(); }

private:
    std::unique_ptr<CFont> m_font;
};



// --------------------------------------------------------------------------
// UserDefinedFonts
// --------------------------------------------------------------------------

bool UserDefinedFonts::IsFontDefined(FontType font_type) const
{
    return ( GetFont(font_type) != nullptr );
}


CFont* UserDefinedFonts::GetFont(FontType font_type) const
{
    ASSERT(font_type != FontType::All);
    size_t index = GetFontTypeIndex(font_type);
    return ( index < m_fonts.size() && m_fonts[index] != nullptr ) ? m_fonts[index]->GetFont() : nullptr;
}


bool UserDefinedFonts::SetFont(FontType font_type, const std::wstring& font_name, int font_size, bool is_bold, bool is_italics)
{
    auto new_font = std::make_unique<CFont>();

    BOOL create_font_success = new_font->CreateFont(-1 * font_size, 0, 0, 0,
                                                    is_bold ? FW_BOLD : FW_NORMAL,
                                                    is_italics,
                                                    FALSE, 0, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, FF_DONTCARE,
                                                    font_name.c_str());

    if( create_font_success == FALSE || AfxGetMainWnd() == nullptr )
        return false;

    // see if the font requested can actually be created
    CClientDC hDC(AfxGetMainWnd());

    CFont* current_font = hDC.GetCurrentFont();
    hDC.SelectObject(new_font.get());

    TEXTMETRIC tm;
    TCHAR szFaceName[LF_FACESIZE];

    GetTextMetrics(hDC, &tm);
    GetTextFace(hDC, _countof(szFaceName), szFaceName);

    hDC.SelectObject(current_font); // restore the font

    if( !SO::EqualsNoCase(font_name, szFaceName) ||
        ( tm.tmWeight != ( is_bold ? FW_BOLD : FW_NORMAL ) ) ||
        ( ( tm.tmItalic != 0 ) != is_italics ) )
    {
        DeleteObject(new_font.get());
        return false;
    }

    // set the font (or fonts)
    auto font_wrapper = std::make_shared<UserDefinedFontWrapper>(std::move(new_font));

    size_t start_index;
    size_t end_index;

    if( font_type == FontType::All )
    {
        start_index = GetFontTypeIndex(FontType::ErrMsg);
        end_index = GetFontTypeIndex(FontType::NumberPad);
    }

    else
    {
        start_index = GetFontTypeIndex(font_type);
        end_index = start_index;
    }

    size_t necessary_vector_size = end_index + 1;

    if( m_fonts.size() < necessary_vector_size )
        m_fonts.resize(necessary_vector_size);

    for( size_t index = start_index; index <= end_index; ++index )
    {
        if( index != GetFontTypeIndex(FontType::All) )
            m_fonts[index] = font_wrapper;
    }

    return true;
}


void UserDefinedFonts::ResetFont(FontType font_type)
{
    size_t start_index;
    size_t end_index;

    if( font_type == FontType::All )
    {
        start_index = GetFontTypeIndex(FontType::ErrMsg);
        end_index = m_fonts.size() - 1;
    }

    else
    {
        start_index = GetFontTypeIndex(font_type);
        end_index = start_index;
    }

    for( size_t index = start_index; index < m_fonts.size() && index <= end_index; ++index )
        m_fonts[index].reset();
}
