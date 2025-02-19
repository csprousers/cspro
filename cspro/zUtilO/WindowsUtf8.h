#pragma once

#ifdef WIN_DESKTOP

#include <zUtilO/zUtilO.h>


// --------------------------------------------------------------------------
// functions for interacting with Windows functions with std::string objects
// representing UTF-8 text
// --------------------------------------------------------------------------

class CLASS_DECL_ZUTILO WindowsUtf8
{
public:
    // HWND-based methods
    // --------------------------------------------------------------------------
    static void GetText(HWND hWnd, std::string& text) { text = GetText(hWnd); }
    static std::string GetText(HWND hWnd);
    static void SetText(HWND hWnd, std::string_view text_sv);

    static void GetText(HWND hDlg, int nIDDlgItem, std::string& text)        { GetText(GetDlgItem(hDlg, nIDDlgItem), text); }
    static std::string GetText(HWND hDlg, int nIDDlgItem)                    { return GetText(GetDlgItem(hDlg, nIDDlgItem)); }
    static void SetText(HWND hDlg, int nIDDlgItem, std::string_view text_sv) { SetText(GetDlgItem(hDlg, nIDDlgItem), text_sv); }
    

    // CWnd / CDialog wrappers that call the HWND methods
    // --------------------------------------------------------------------------
    static void GetText(const CWnd* pWnd, std::string& text)        { GetText(pWnd->GetSafeHwnd(), text); }
    static std::string GetText(const CWnd* pWnd)                    { return GetText(pWnd->GetSafeHwnd()); }
    static void SetText(const CWnd* pWnd, std::string_view text_sv) { SetText(pWnd->GetSafeHwnd(), text_sv); }

    static void GetText(const CWnd* pDlg, int nIDDlgItem, std::string& text)        { GetText(pDlg->GetSafeHwnd(), nIDDlgItem, text); }
    static std::string GetText(const CWnd* pDlg, int nIDDlgItem)                    { return GetText(pDlg->GetSafeHwnd(), nIDDlgItem); }
    static void SetText(const CWnd* pDlg, int nIDDlgItem, std::string_view text_sv) { SetText(pDlg->GetSafeHwnd(), nIDDlgItem, text_sv); }
};

#endif // WIN_DESKTOP
