#include "stdafx.h"
#include "CSProScintillaCtrl.h"


[[nodiscard]] std::string CSProScintillaCtrl::GetText(int length/* = -1*/)
{
    if( length == -1 )
        length = GetTextLength();

    // GetText expects a buffer that will also store the null terminator
    std::string text(length + 1, '\0');
    const int actual_length = __super::GetText(text.size(), text.data());
    text.resize(actual_length);

    return text;
}


[[nodiscard]] std::string CSProScintillaCtrl::GetTargetText()
{
    const int length = __super::GetTargetText(nullptr);

    // GetTargetText expects a buffer that will also store the null terminator
    std::string text(length + 1, '\0');
    const int actual_length = __super::GetTargetText(text.data());
    text.resize(actual_length);

    return text;
}


void CSProScintillaCtrl::SetReadOnlyText(const cs::string_sz text)
{
    SetReadOnly(FALSE);
    SetText(text);
    SetReadOnly(TRUE);
}


[[nodiscard]] std::string CSProScintillaCtrl::GetSelText()
{
    const int length = __super::GetSelText(nullptr);

    // GetSelText expects a buffer that will also store the null terminator
    std::string text(length + 1, '\0');
    const int actual_length = __super::GetSelText(text.data());
    text.resize(actual_length);

    return text;
}
