#pragma once

#include <zUtilO/zUtilO.h>

#ifdef WIN32
#include <wingdi.h>
#else
#include <zPlatformO/PortableWindowsGUIDefines.h>
#endif

class Serializer;


class CLASS_DECL_ZUTILO PortableFont
{
public:
    static const PortableFont TextDefault;
    static const PortableFont FieldDefault;

    PortableFont();
    PortableFont(const LOGFONT& logfont);

    bool operator==(const PortableFont& rhs) const { return m_index == rhs.m_index; }
    bool operator!=(const PortableFont& rhs) const { return !operator==(rhs); }

    operator LOGFONT() const { return GetLOGFONT(); }

    const LOGFONT& GetLOGFONT() const;
    CFont& GetCFont() const;

    CString GetDescription() const;

    bool IsArabic() const;


    // serialization
    void BuildFromPre80String(wstring_view text);
    CString GetPre80String() const;

    void serialize(Serializer& ar);


private:
    size_t m_index;
};
