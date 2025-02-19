#pragma once

#include <zToolsO/zToolsO.h>


class CLASS_DECL_ZTOOLSO WinClipboard
{
public:
    static bool HasText()  { return IsClipboardFormatAvailable(CF_TEXT); }
    static bool HasHtml()  { return IsClipboardFormatAvailable(m_htmlFormat); }
    static bool HasImage() { return IsClipboardFormatAvailable(CF_BITMAP); }

    static void PutTextWithFormat(unsigned format, CWnd* pWnd, wstring_view text_sv, bool clear = true);
    static void PutTextWithFormat(unsigned format, CWnd* pWnd, std::string_view text_sv, bool clear = true);

    template<typename T = std::wstring> // can also return std::string
    static T GetTextWithFormat(unsigned format, CWnd* pWnd = nullptr);

    static void PutText(CWnd* pWnd, wstring_view text_sv, bool clear = true)     { PutTextWithFormat(_tCF_TEXT, pWnd, text_sv, clear); }
    static void PutText(CWnd* pWnd, std::string_view text_sv, bool clear = true) { PutTextWithFormat(_tCF_TEXT, pWnd, text_sv, clear); }

    template<typename T = std::wstring> // can also return std::string
    static T GetText(CWnd* pWnd = nullptr) { return GetTextWithFormat<T>(_tCF_TEXT, pWnd); }

    static void PutHtml(std::string html, bool clear = true);
    static std::wstring GetHtml(CWnd* pWnd);

    static HBITMAP GetImage(CWnd* pWnd);

private:
    static const int m_htmlFormat;
};
