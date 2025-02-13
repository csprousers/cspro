#include "stdafx.h"
#include "CSProScintillaCtrl.h"


[[nodiscard]] std::string CSProScintillaCtrl::GetTextUtf8(int length/* = -1*/)
{
    if( length == -1 )
        length = GetTextLength();

    // GetText expects a buffer that will also store the null terminator
    std::string utf8_text(length + 1, '\0');
    const int actual_length = __super::GetText(utf8_text.size(), utf8_text.data());
    utf8_text.resize(actual_length);

    return utf8_text;
}


[[nodiscard]] std::wstring CSProScintillaCtrl::GetText(const int length/* = -1*/)
{
    return UTF8Convert::UTF8ToWide(GetTextUtf8(length));
}


[[nodiscard]] std::wstring CSProScintillaCtrl::GetTargetText()
{
    const int length = __super::GetTargetText(nullptr);

    // GetTargetText expects a buffer that will also store the null terminator
    auto buffer = std::make_unique_for_overwrite<char[]>(length + 1);
    __super::GetTargetText(buffer.get());

    return UTF8Convert::UTF8ToWide(buffer.get(), length);
}


void CSProScintillaCtrl::SetText(const wstring_view text_sv)
{
    const std::string utf8_text = UTF8Convert::WideToUTF8(text_sv);

    __super::SetText(utf8_text.c_str());
}


void CSProScintillaCtrl::SetReadOnlyText(const wstring_view text_sv)
{
    SetReadOnly(FALSE);
    SetText(text_sv);
    SetReadOnly(TRUE);
}


void CSProScintillaCtrl::AddText(const wstring_view text_sv)
{
    const std::string utf8_text = UTF8Convert::WideToUTF8(text_sv);

    __super::AddText(utf8_text.length(), utf8_text.c_str());
}


[[nodiscard]] std::wstring CSProScintillaCtrl::GetSelText()
{
    const int length = __super::GetSelText(nullptr);

    // GetSelText expects a buffer that will also store the null terminator
    auto buffer = std::make_unique_for_overwrite<char[]>(length + 1);
    __super::GetSelText(buffer.get());

    return UTF8Convert::UTF8ToWide(buffer.get(), length);
}


void CSProScintillaCtrl::ReplaceSel(const wstring_view text_sv)
{
    const std::string utf8_text = UTF8Convert::WideToUTF8(text_sv);

    __super::ReplaceSel(utf8_text.c_str());
}
