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

    const std::lock_guard<std::mutex> lock(FontsMutex);

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
    const std::lock_guard<std::mutex> lock(FontsMutex);
    const FontDetails& font_details = *Fonts[m_index];

    return font_details.logfont;
}


CFont& PortableFont::GetCFont() const
{
    const std::lock_guard<std::mutex> lock(FontsMutex);
    FontDetails& font_details = *Fonts[m_index];

    if( font_details.cfont == nullptr )
    {
        font_details.cfont = std::make_unique<CFont>();
        font_details.cfont->CreateFontIndirect(&font_details.logfont);
    }

    return *font_details.cfont;
}


std::string PortableFont::GetDescription() const
{
    // Given a LOGFONT structure, this function returns a string describing its face name,
    // point size, and whether or not the font is bold, italic, underline, or strikeout.
    // csc 01 Dec 00
    const LOGFONT& logfont = GetLOGFONT();

    if( logfont.lfHeight == 0 )
    {
        ASSERT(false);
        return "<no font information available>";
    }

#ifdef WIN32
    std::string description = TC::ToUtf8(logfont.lfFaceName);
#else
    std::string description = TC::ToUtf8(TwoByteCharToWide(logfont.lfFaceName, _countof(logfont.lfFaceName)));
#endif

#ifdef WIN_DESKTOP
    CClientDC dc(AfxGetMainWnd());
    dc.SetMapMode(MM_TEXT);
    const int iLogPixels = dc.GetDeviceCaps(LOGPIXELSY);

    int iPointSize = MulDiv(72, logfont.lfHeight, iLogPixels);
    iPointSize *= ( iPointSize < 0 ) ? -1 : 1;

    description.append(FormatText(", %d point", iPointSize));
#endif

    if( logfont.lfWeight > FW_NORMAL )
        description.append(", bold");

    if( logfont.lfItalic )
        description.append(", italic");

    if( logfont.lfUnderline )
        description.append(", underline");

    if( logfont.lfStrikeOut )
        description.append(", strikeout");

    // JH 7/05 - display script if arabic or russian
    if( IsArabic() )
    {
        description.append(", Arabic");
    }

    else if( logfont.lfCharSet == RUSSIAN_CHARSET )
    {
        description.append(", Cyrillic");
    }

    return description;
}


bool PortableFont::IsArabic() const
{
    const LOGFONT& logfont = GetLOGFONT();
    return ( logfont.lfCharSet == ARABIC_CHARSET );
}


void PortableFont::BuildFromPre80String(const std::string& text)
{
    // use the text default if the string is not long enough
    if( SO::WideLength(text) < 65 )
    {
        *this = TextDefault;
        return;
    }

    const char* text_itr = text.c_str();

    auto get_int = [&]()
    {
        const int value = atoi(text_itr);
        text_itr += 5;
        return value;
    };

    LOGFONT logfont
    {
        get_int(),
        get_int(),
        get_int(),
        get_int(),
        get_int(),
        static_cast<BYTE>(get_int()),
        static_cast<BYTE>(get_int()),
        static_cast<BYTE>(get_int()),
        static_cast<BYTE>(get_int()),
        static_cast<BYTE>(get_int()),
        static_cast<BYTE>(get_int()),
        static_cast<BYTE>(get_int()),
        static_cast<BYTE>(get_int())
    };

    const std::wstring wide_face_name = TC::ToWide(text_itr);

#ifdef WIN32
    SO::CopyToFixedBuffer(logfont.lfFaceName, wide_face_name);

#else
    const size_t face_name_length = std::min(wide_face_name.length(), _countof(LOGFONT::lfFaceName) - 1);

    for( size_t i = 0; i < face_name_length; ++i )
        logfont.lfFaceName[i] = wide_face_name[i];

    logfont.lfFaceName[face_name_length] = 0;
#endif

    *this = PortableFont(logfont);
}


std::string PortableFont::GetPre80String() const
{
    const LOGFONT& logfont = GetLOGFONT();

    return FormatText("%04d %04d %04d %04d %04d %04d %04d %04d %04d %04d %04d %04d %04d %s",
                      static_cast<int>(logfont.lfHeight), static_cast<int>(logfont.lfWidth),
                      static_cast<int>(logfont.lfEscapement), static_cast<int>(logfont.lfOrientation),
                      static_cast<int>(logfont.lfWeight), static_cast<int>(logfont.lfItalic),
                      static_cast<int>(logfont.lfUnderline), static_cast<int>(logfont.lfStrikeOut),
                      static_cast<int>(logfont.lfCharSet), static_cast<int>(logfont.lfOutPrecision),
                      static_cast<int>(logfont.lfClipPrecision), static_cast<int>(logfont.lfQuality),
                      static_cast<int>(logfont.lfPitchAndFamily),
#ifdef WIN32
                      TC::ToUtf8(logfont.lfFaceName).c_str());
#else
                      ReturnProgrammingError("<Font Name>"));
#endif
}


void PortableFont::serialize(Serializer& ar)
{
    if( ar.IsSaving() )
    {
        ar.Write(GetLOGFONT());
    }

    else
    {
        *this = ar.Read<LOGFONT>();
    }
}
