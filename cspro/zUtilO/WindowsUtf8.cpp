#include "StdAfx.h"
#include "WindowsUtf8.h"


std::string WindowsUtf8::GetText(HWND hWnd)
{
    const int wide_text_length = ( hWnd != nullptr ) ? GetWindowTextLength(hWnd) : 0;

    if( wide_text_length == 0 )
        return std::string();

    const int wide_text_length_with_null_terminator = wide_text_length + 1;

    auto wide_buffer = std::make_unique_for_overwrite<wchar_t[]>(wide_text_length_with_null_terminator);
    GetWindowText(hWnd, wide_buffer.get(), wide_text_length_with_null_terminator);

    return TC::ToUtf8(wide_buffer.get(), wide_text_length);
}


void WindowsUtf8::SetText(HWND hWnd, const std::string_view text_sv)
{
    if( hWnd != nullptr )
        SetWindowText(hWnd, TC::ToWide(text_sv).c_str());
}
