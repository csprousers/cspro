#pragma once

#include <zUtilO/zUtilO.h>

class UserDefinedFontWrapper;


class CLASS_DECL_ZUTILO UserDefinedFonts
{
public:
    enum class FontType { ErrMsg = 1, ValueSets, Userbar, Notes, All, NumberPad };

    bool IsFontDefined(FontType font_type) const;
    CFont* GetFont(FontType font_type) const;
    bool SetFont(FontType font_type, const std::wstring& font_name, int font_size, bool is_bold, bool is_italics);
    void ResetFont(FontType font_type);

private:
    static constexpr size_t GetFontTypeIndex(FontType font_type) { return static_cast<size_t>(font_type) - 1; }

private:
    std::vector<std::shared_ptr<UserDefinedFontWrapper>> m_fonts;
};
