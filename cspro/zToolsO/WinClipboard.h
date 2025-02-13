#pragma once

#include <zToolsO/zToolsO.h>


class CLASS_DECL_ZTOOLSO WinClipboard
{
public:
    static bool HasText()  { return IsClipboardFormatAvailable(CF_TEXT); }
    static bool HasHtml()  { return IsClipboardFormatAvailable(m_htmlFormat); }
    static bool HasImage() { return IsClipboardFormatAvailable(CF_BITMAP); }

    static void PutTextWithFormat(unsigned format, CWnd* pWnd, wstring_view text, bool clear = true);

    static std::wstring GetTextWithFormat(unsigned format, CWnd* pWnd = nullptr);

    static void PutText(CWnd* pWnd, wstring_view text, bool clear = true)
    {
        PutTextWithFormat(_tCF_TEXT, pWnd, text, clear);
    }

    static std::wstring GetText(CWnd* pWnd = nullptr)
    {
        return GetTextWithFormat(_tCF_TEXT, pWnd);
    }

    static void PutHtml(wstring_view html_text, bool clear = true);
    static std::wstring GetHtml(CWnd* pWnd);

    static HBITMAP GetImage(CWnd* pWnd);

private:
    static const int m_htmlFormat;
};
