#include "StdAfx.h"
#include "PortableFont.h"
#include <mutex>


namespace
{
    struct FontDetails
    {
        const LOGFONT logfont;
        std::unique_ptr<CFont> cfont;

        FontDetails(LOGFONT logfont_)
            :   logfont(std::move(logfont_))
        {
        }
    };

    std::vector<std::unique_ptr<FontDetails>> Fonts;
    std::mutex FontsMutex;
}


const PortableFont PortableFont::TextDefault(LOGFONT{ -13, 0, 0, 0, 700, FALSE, FALSE, 0, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, FF_DONTCARE, { 'A', 'r', 'i', 'a', 'l', 0 } });
const PortableFont PortableFont::FieldDefault(LOGFONT{ 18, 0, 0, 0, 600, FALSE, FALSE, 0, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, FF_DONTCARE, { 'C', 'o', 'u', 'r', 'i', 'e', 'r', ' ', 'N', 'e', 'w', 0 } });


PortableFont::PortableFont()
    :   PortableFont(PortableFont::TextDefault)
{
}


PortableFont::PortableFont(const LOGFONT& logfont)
    :   m_index(0)
{
    ASSERT(logfont.lfHeight != 0);

    std::lock_guard<std::mutex> lock(FontsMutex);

    // find an existing font...
    for( ; m_index < Fonts.size(); ++m_index )
    {
        if( memcmp(&Fonts[m_index]->logfont, &logfont, sizeof(LOGFONT)) == 0 )
            return;
    }

    // ...or add a new one
    Fonts.emplace_back(std::make_unique<FontDetails>(logfont));
}


const LOGFONT& PortableFont::GetLOGFONT() const
{
    std::lock_guard<std::mutex> lock(FontsMutex);
    const FontDetails& font_details = *Fonts[m_index];

    return font_details.logfont;
}


CFont& PortableFont::GetCFont() const
{
    std::lock_guard<std::mutex> lock(FontsMutex);
    FontDetails& font_details = *Fonts[m_index];

    if( font_details.cfont == nullptr )
    {
        font_details.cfont = std::make_unique<CFont>();
        font_details.cfont->CreateFontIndirect(&font_details.logfont);
    }

    return *font_details.cfont;
}


CString PortableFont::GetDescription() const
{
    // Given a LOGFONT structure, this function returns a string describing its face name,
    // point size, and whether or not the font is bold, italic, underline, or strikeout.
    // csc 01 Dec 00
    const LOGFONT& logfont = GetLOGFONT();

    if( logfont.lfHeight == 0 )
    {
        ASSERT(false);
        return _T("<no font information available>");
    }

#ifdef WIN32
    CString description = logfont.lfFaceName;
#else
    CString description = TwoByteCharToWide(logfont.lfFaceName, _countof(logfont.lfFaceName));
#endif

#ifdef WIN_DESKTOP
    CClientDC dc(AfxGetMainWnd());
    dc.SetMapMode(MM_TEXT);
    int iLogPixels = dc.GetDeviceCaps(LOGPIXELSY);

    int iPointSize = MulDiv(72, logfont.lfHeight, iLogPixels);
    iPointSize *= ( iPointSize < 0 ) ? -1 : 1;

    description.AppendFormat(_T(", %d point"), iPointSize);
#endif

    if( logfont.lfWeight > FW_NORMAL )
        description.Append(_T(", bold"));

    if( logfont.lfItalic )
        description.Append(_T(", italic"));

    if( logfont.lfUnderline )
        description.Append(_T(", underline"));

    if( logfont.lfStrikeOut )
        description.Append(_T(", strikeout"));

    // JH 7/05 - display script if arabic or russian
    if( IsArabic() )
    {
        description.Append(_T(", Arabic"));
    }

    else if( logfont.lfCharSet == RUSSIAN_CHARSET )
    {
        description.Append(_T(", Cyrillic"));
    }

    return description;
}


bool PortableFont::IsArabic() const
{
    const LOGFONT& logfont = GetLOGFONT();
    return ( logfont.lfCharSet == ARABIC_CHARSET );
}


void PortableFont::BuildFromPre80String(wstring_view text)
{
    // use the text default if the string is not long enough
    if( text.length() < 65 )
    {
        *this = TextDefault;
        return;
    }

    auto get_int = [&]()
    {
        int value = _ttoi(text.data());
        text = text.substr(5);
        return value;
    };

    LOGFONT logfont
    {
        get_int(),
        get_int(),
        get_int(),
        get_int(),
        get_int(),
        (BYTE)get_int(),
        (BYTE)get_int(),
        (BYTE)get_int(),
        (BYTE)get_int(),
        (BYTE)get_int(),
        (BYTE)get_int(),
        (BYTE)get_int(),
        (BYTE)get_int()
    };

    size_t face_name_length = std::min(text.length(), _countof(LOGFONT::lfFaceName) - 1);

#ifdef WIN32
    _tcsncpy(logfont.lfFaceName, text.data(), face_name_length);
#else
    for( size_t i = 0; i < face_name_length; ++i )
        logfont.lfFaceName[i] = text[i];
#endif

    logfont.lfFaceName[face_name_length] = 0;

    *this = PortableFont(logfont);
}


CString PortableFont::GetPre80String() const
{
    const LOGFONT& logfont = GetLOGFONT();

    return FormatText(_T("%04d %04d %04d %04d %04d %04d %04d %04d %04d %04d %04d %04d %04d %s"),
                      logfont.lfHeight, logfont.lfWidth, logfont.lfEscapement, logfont.lfOrientation,
                      logfont.lfWeight, logfont.lfItalic, logfont.lfUnderline, logfont.lfStrikeOut,
                      logfont.lfCharSet, logfont.lfOutPrecision, logfont.lfClipPrecision, logfont.lfQuality,
                      logfont.lfPitchAndFamily, logfont.lfFaceName);
}


void PortableFont::serialize(Serializer& ar)
{
    if( ar.IsSaving() )
    {
        ar << GetLOGFONT();
    }

    else
    {
        LOGFONT logfont;
        ar >> logfont;
        *this = logfont;
    }
}
