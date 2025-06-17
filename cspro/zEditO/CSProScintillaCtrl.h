#pragma once

#include <zEditO/zEditO.h>
#include <zEditO/ScintillaCtrl.h>


// CSProScintillaCtrl is the base subclass that all CSPro-related Scintilla controls derive from;
// it has some convenience methods (not put in CScintillaCtrl to make it easier to upgrade to new versions of Scintilla)

class CLASS_DECL_ZEDITO CSProScintillaCtrl : public Scintilla::CScintillaCtrl
{
public:
    [[nodiscard]] std::string GetText(int length = -1);

    [[nodiscard]] std::string GetTargetText();

    void SetText(cs::string_sz text) { __super::SetText(text.c_str()); }

    void SetReadOnlyText(cs::string_sz text);

    void AddText(std::string_view text_sv) { __super::AddText(text_sv.length(), text_sv.data()); }

    [[nodiscard]] std::string GetSelText();
};
