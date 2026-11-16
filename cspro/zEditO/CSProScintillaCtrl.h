#pragma once

#include <zEditO/zEditO.h>
#include <zEditO/ScintillaCtrl.h>


// --------------------------------------------------------------------------
// CSProScintillaCtrl
//
// This is the base subclass that all CSPro-related Scintilla controls derive
// from. It has some convenience methods not put in CScintillaCtrl to make
// it easier to upgrade to new versions of Scintilla.
// --------------------------------------------------------------------------

class CLASS_DECL_ZEDITO CSProScintillaCtrl : public Scintilla::CScintillaCtrl
{
public:
    [[nodiscard]] std::string GetText(Sci_Position length = -1);

    [[nodiscard]] std::string GetTargetText();

    void SetText(cs::string_sz text) { __super::SetText(text.c_str()); }

    void AddText(std::string_view text_sv) { __super::AddText(text_sv.length(), text_sv.data()); }

    [[nodiscard]] std::string GetSelText();

    // Turns off the control's read-only mode, clears the text, and then reenables read-only mode.
    void ClearReadOnlyText();

    // Turns off the control's read-only mode, sets the text, and then reenables read-only mode.
    void SetReadOnlyText(cs::string_sz text);

    // Turns off the control's read-only mode, appends the text, and then reenables read-only mode.
    void AppendReadOnlyText(std::string_view text_sv);
};
