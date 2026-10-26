#include "stdafx.h"
#include "CSProScintillaCtrl.h"


std::string CSProScintillaCtrl::GetText(Sci_Position length/* = -1*/)
{
    if( length == -1 )
        length = GetTextLength();

    // GetText expects a buffer that will also store the null terminator
    std::string text(length + 1, '\0');
    const Sci_Position actual_length = __super::GetText(text.size(), text.data());
    text.resize(actual_length);

    return text;
}


std::string CSProScintillaCtrl::GetTargetText()
{
    const Sci_Position length = __super::GetTargetText(nullptr);

    // GetTargetText expects a buffer that will also store the null terminator
    std::string text(length + 1, '\0');
    const Sci_Position actual_length = __super::GetTargetText(text.data());
    text.resize(actual_length);

    return text;
}


std::string CSProScintillaCtrl::GetSelText()
{
    const Sci_Position length = __super::GetSelText(nullptr);

    // GetSelText expects a buffer that will also store the null terminator
    std::string text(length + 1, '\0');
    const Sci_Position actual_length = __super::GetSelText(text.data());
    text.resize(actual_length);

    return text;
}


void CSProScintillaCtrl::ClearReadOnlyText()
{
    SetReadOnly(FALSE);
    ClearAll();
    SetReadOnly(TRUE);
}


void CSProScintillaCtrl::SetReadOnlyText(const cs::string_sz text)
{
    SetReadOnly(FALSE);
    SetText(text);
    SetReadOnly(TRUE);
}


void CSProScintillaCtrl::AppendReadOnlyText(const std::string_view text_sv)
{
    SetReadOnly(FALSE);
    AppendText(text_sv.length(), text_sv.data());
    SetReadOnly(TRUE);
}
