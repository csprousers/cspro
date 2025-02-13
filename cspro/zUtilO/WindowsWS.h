#pragma once

#include <zUtilO/DataExchange.h>


// --------------------------------------------------------------------------
// functions for interacting with Windows functions with std::wstring objects
// --------------------------------------------------------------------------

class WindowsWS
{
public:
    // HWND-based methods
    // --------------------------------------------------------------------------
    static void GetWindowText(HWND hWnd, std::wstring& text);
    static std::wstring GetWindowText(HWND hWnd);
    static void SetWindowText(HWND hWnd, const std::wstring& text);

    static void GetDlgItemText(HWND hDlg, int nIDDlgItem, std::wstring& text);
    static std::wstring GetDlgItemText(HWND hDlg, int nIDDlgItem);
    static void SetDlgItemText(HWND hDlg, int nIDDlgItem, const std::wstring& text);


    // CWnd / CDialog wrappers that call the HWND methods
    // --------------------------------------------------------------------------
    static std::wstring GetWindowText(const CWnd* pWnd);
    static void SetWindowText(CWnd* pWnd, const std::wstring& text);

    static std::wstring GetDlgItemText(CDialog* pDlg, int nIDDlgItem);
    static void SetDlgItemText(CDialog* pDlg, int nIDDlgItem, const std::wstring& text);


    // string resource functions
    // --------------------------------------------------------------------------
    static std::wstring LoadString(UINT id);
    static std::wstring AfxFormatString1(UINT id, NullTerminatedString text);
};



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

inline void WindowsWS::GetWindowText(HWND hWnd, std::wstring& text)
{
    const int text_length = GetWindowTextLength(hWnd);
    text.resize(text_length);
    ASSERT80(text[text_length] == 0);
    ::GetWindowText(hWnd, text.data(), text_length + 1);
    ASSERT80(text.empty() || text.back() != 0);
}


inline std::wstring WindowsWS::GetWindowText(HWND hWnd)
{
    std::wstring text;
    GetWindowText(hWnd, text);
    return text;
}


inline std::wstring WindowsWS::GetWindowText(const CWnd* pWnd)
{
    return GetWindowText(pWnd->GetSafeHwnd());
}


inline void WindowsWS::SetWindowText(HWND hWnd, const std::wstring& text)
{
    ::SetWindowText(hWnd, text.c_str());
}


inline void WindowsWS::SetWindowText(CWnd* pWnd, const std::wstring& text)
{
    SetWindowText(pWnd->GetSafeHwnd(), text);
}


inline void WindowsWS::GetDlgItemText(HWND hDlg, const int nIDDlgItem, std::wstring& text)
{
    HWND hWnd = GetDlgItem(hDlg, nIDDlgItem);

    if( hWnd != nullptr )
    {
        GetWindowText(hWnd, text);
    }

    else
    {
        ASSERT(false);
        text.clear();
    }
}


inline std::wstring WindowsWS::GetDlgItemText(HWND hDlg, const int nIDDlgItem)
{
    std::wstring text;
    GetDlgItemText(hDlg, nIDDlgItem, text);
    return text;
}


inline std::wstring WindowsWS::GetDlgItemText(CDialog* pDlg, const int nIDDlgItem)
{
    return GetDlgItemText(pDlg->GetSafeHwnd(), nIDDlgItem);
}


inline void WindowsWS::SetDlgItemText(HWND hDlg, const int nIDDlgItem, const std::wstring& text)
{
    HWND hWnd = GetDlgItem(hDlg, nIDDlgItem);

    if( hWnd != nullptr )
    {
        SetWindowText(hWnd, text);
    }

    else
    {
        ASSERT(false);
    }
}


inline void WindowsWS::SetDlgItemText(CDialog* pDlg, const int nIDDlgItem, const std::wstring& text)
{
    SetDlgItemText(pDlg->GetSafeHwnd(), nIDDlgItem, text);
}


inline std::wstring WindowsWS::LoadString(const UINT id)
{
    const TCHAR* buffer;
    const int length = ::LoadString(nullptr, id, reinterpret_cast<LPWSTR>(&buffer), 0);

    return ( length != 0 ) ? std::wstring(buffer, length) :
                             std::wstring();
}


inline std::wstring WindowsWS::AfxFormatString1(const UINT id, const NullTerminatedString text)
{
    CString formatted_string;
    ::AfxFormatString1(formatted_string, id, text.c_str());
    return CS2WS(formatted_string);
}



// --------------------------------------------------------------------------
// AfxMessageBox overrides
// --------------------------------------------------------------------------

inline int AfxMessageBox(const std::wstring& text, const UINT nType = MB_OK, const UINT nIDHelp = 0)
{
    return AfxMessageBox(text.c_str(), nType, nIDHelp);
}


inline int AfxMessageBox(const CString& text, const UINT nType = MB_OK, const UINT nIDHelp = 0)
{
    return AfxMessageBox(text.GetString(), nType, nIDHelp);
}


inline int AfxMessageBox(const NullTerminatedString text, const UINT nType = MB_OK, const UINT nIDHelp = 0)
{
    return AfxMessageBox(text.c_str(), nType, nIDHelp);
}
