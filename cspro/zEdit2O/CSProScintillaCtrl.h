#pragma once

#include <zEdit2O/zEdit2O.h>
#include <zEdit2O/ScintillaCtrl.h>


// CSProScintillaCtrl is the base subclass that all CSPro-related Scintilla controls derive from;
// it has some convenience methods (not put in CScintillaCtrl to make it easier to upgrade to new versions of Scintilla)

class CLASS_DECL_ZEDIT2O CSProScintillaCtrl : public Scintilla::CScintillaCtrl
{
public:
    [[nodiscard]] std::string GetTextUtf8(int length = -1);
    [[nodiscard]] std::wstring GetText(int length = -1);

    [[nodiscard]] std::wstring GetTargetText();

    void SetText(const char* text)        { Scintilla::CScintillaCtrl::SetText(text); }
    void SetText(const std::string& text) { Scintilla::CScintillaCtrl::SetText(text.c_str()); }

    void SetText(wstring_view text_sv);
    void SetReadOnlyText(wstring_view text_sv);

    void AddText(wstring_view text_sv);

    [[nodiscard]] std::wstring GetSelText();
    void ReplaceSel(wstring_view text_sv);
};
