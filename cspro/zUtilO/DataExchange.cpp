#include "StdAfx.h"
#include "DataExchange.h"
#include "SyncConnectionString.h"
#include "WindowsUtf8.h"
#include "WindowsWS.h"


// --------------------------------------------------------------------------
// DDX_Check
// --------------------------------------------------------------------------

void DDX_Check(CDataExchange* const pDX, const int nIDC, bool& value)
{
    int dlg_value;

    if( pDX->m_bSaveAndValidate )
    {
        DDX_Check(pDX, nIDC, dlg_value);
        ASSERT(dlg_value != BST_INDETERMINATE);
        value = ( dlg_value == BST_CHECKED );
    }

    else
    {
        dlg_value = value ? BST_CHECKED : BST_UNCHECKED;
        DDX_Check(pDX, nIDC, dlg_value);
    }
}



// --------------------------------------------------------------------------
// DDX_Text
// --------------------------------------------------------------------------

void DDX_Text(CDataExchange* const pDX, const int nIDC, std::wstring& text, const bool trim_string_on_save/* = false*/)
{
    HWND hWndCtrl = pDX->PrepareEditCtrl(nIDC);

    if( pDX->m_bSaveAndValidate )
    {
        WindowsWS::GetWindowText(hWndCtrl, text);

        if( trim_string_on_save )
            SO::MakeTrim(text);
    }

    else
    {
        SetWindowText(hWndCtrl, text.c_str());
    }
}


void DDX_Text(CDataExchange* const pDX, const int nIDC, std::string& text, const bool trim_string_on_save/* = false*/)
{
    HWND hWndCtrl = pDX->PrepareEditCtrl(nIDC);

    if( pDX->m_bSaveAndValidate )
    {
        WindowsUtf8::GetText(hWndCtrl, text);

        if( trim_string_on_save )
            SO::MakeTrim(text);
    }

    else
    {
        WindowsUtf8::SetText(hWndCtrl, text);
    }
}


void DDX_TextOnlyLF(CDataExchange* const pDX, const int nIDC, std::string& text, const bool trim_string_on_save/* = false*/)
{
    if( pDX->m_bSaveAndValidate )
    {
        DDX_Text(pDX, nIDC, text, trim_string_on_save);
        SO::MakeNewlineLF(text);
    }

    else if( text.find('\n') != std::string::npos )
    {
        std::string text_copy = text;
        SO::MakeNewlineCRLF(text_copy);
        DDX_Text(pDX, nIDC, text_copy, trim_string_on_save);
    }

    else
    {
        DDX_Text(pDX, nIDC, text, trim_string_on_save);
    }
}


void DDX_Text(CDataExchange* const pDX, const int nIDC, ConnectionString& connection_string)
{
    HWND hWndCtrl = pDX->PrepareEditCtrl(nIDC);

    if( pDX->m_bSaveAndValidate )
    {
        connection_string = ConnectionString(WindowsUtf8::GetText(hWndCtrl));
    }

    else
    {
        WindowsUtf8::SetText(hWndCtrl, connection_string.IsDefined() ? connection_string.ToString() :
                                                                       std::string());
    }
}


void DDX_Text(CDataExchange* const pDX, const int nIDC, SyncConnectionString& sync_connection_string)
{
    HWND hWndCtrl = pDX->PrepareEditCtrl(nIDC);

    if( pDX->m_bSaveAndValidate )
    {
        sync_connection_string = SyncConnectionString(WindowsUtf8::GetText(hWndCtrl));
    }

    else
    {
        WindowsUtf8::SetText(hWndCtrl, sync_connection_string.ToString());
    }
}



// --------------------------------------------------------------------------
// DDX_CBString + DDX_CBStringExact
// --------------------------------------------------------------------------

void DDX_CBString(CDataExchange* const pDX, const int nIDC, std::wstring& text)
{
    HWND hWndCtrl;
    pDX->m_pDlgWnd->GetDlgItem(nIDC, &hWndCtrl);

    if( ( GetWindowLong(hWndCtrl, GWL_STYLE) & CBS_DROPDOWNLIST ) != CBS_DROPDOWNLIST )
    {
        pDX->PrepareEditCtrl(nIDC);
    }

    else
    {
        pDX->PrepareCtrl(nIDC);
    }

    if( pDX->m_bSaveAndValidate )
    {
        // just get current edit item text (or drop list static)
        int text_length = GetWindowTextLength(hWndCtrl);

        if( text_length > 0 )
        {
            // get known length
            text.resize(text_length);
            ASSERT80(text[text_length] == 0);
            GetWindowText(hWndCtrl, text.data(), text_length + 1);
            ASSERT80(text.empty() || text.back() != 0);
        }

        else
        {
            // for drop lists GetWindowTextLength does not work - assume
            // max of 255 characters
            constexpr size_t DefaultLength = 255;
            text.resize(DefaultLength);
            ASSERT80(text[DefaultLength] == 0);
            GetWindowText(hWndCtrl, text.data(), DefaultLength + 1);
            text.resize(_tcslen(text.data()));
        }
    }

    else
    {
        // set current selection based on model string
        if( SendMessage(hWndCtrl, CB_SELECTSTRING, static_cast<WPARAM>(-1), reinterpret_cast<LPARAM>(text.c_str())) == CB_ERR )
        {
            // just set the edit text (will be ignored if DROPDOWNLIST)
            SetWindowText(hWndCtrl, text.c_str());
        }
    }
}


void DDX_CBStringExact(CDataExchange* const pDX, const int nIDC, std::wstring& text)
{
    HWND hWndCtrl;
    pDX->m_pDlgWnd->GetDlgItem(nIDC, &hWndCtrl);

    if( ( GetWindowLong(hWndCtrl, GWL_STYLE) & CBS_DROPDOWNLIST ) != CBS_DROPDOWNLIST )
    {
        pDX->PrepareEditCtrl(nIDC);
    }

    else
    {
        pDX->PrepareCtrl(nIDC);
    }

    if( pDX->m_bSaveAndValidate )
    {
        DDX_CBString(pDX, nIDC, text);
    }

    else
    {
        // set current selection based on data string
        const int i = static_cast<int>(::SendMessage(hWndCtrl, CB_FINDSTRINGEXACT, static_cast<WPARAM>(-1), reinterpret_cast<LPARAM>(text.c_str())));

        if( i < 0 )
        {
            // just set the edit text (will be ignored if DROPDOWNLIST)
            SetWindowText(hWndCtrl, text.c_str());
        }

        else
        {
            // select it
            SendMessage(hWndCtrl, CB_SETCURSEL, i, 0);
        }
    }
}


void DDX_CBStringExact(CDataExchange* const pDX, const int nIDC, std::string& text)
{
    std::wstring wide_text = UTF8_TODO::GetWide(text);
    DDX_CBStringExact(pDX, nIDC, wide_text);
    text = UTF8_TODO::GetUtf8(wide_text);
}


// --------------------------------------------------------------------------
// DDV_MaxChars
// --------------------------------------------------------------------------

void DDV_MaxChars(CDataExchange* const pDX, std::string& text, const int nChars)
{
    // this is copied from the MFC source code with the exception of the SO::WideLength call

    ASSERT(nChars >= 1);        // allow them something
    if (pDX->m_bSaveAndValidate && static_cast<int>(SO::WideLength(text)) > nChars)
    {
        TCHAR szT[32];
        _stprintf_s(szT, _countof(szT), _T("%d"), nChars);
        CString prompt;
        AfxFormatString1(prompt, AFX_IDP_PARSE_STRING_SIZE, szT);
        AfxMessageBox(prompt, MB_ICONEXCLAMATION, AFX_IDP_PARSE_STRING_SIZE);
        prompt.Empty(); // exception prep
        pDX->Fail();
    }
    else if (pDX->m_idLastControl != 0 && pDX->m_bEditLastControl)
    {
        HWND hWndLastControl;
        pDX->m_pDlgWnd->GetDlgItem(pDX->m_idLastControl, &hWndLastControl);
        // limit the control max-chars automatically
        // send messages for both an edit control and a combobox control--one will
        // be understood and one will be disregarded, but this is the only way to
        // ensure that the characters will be limited for both kinds of controls.
        ::SendMessage(hWndLastControl, EM_SETLIMITTEXT, nChars, 0);
        ::SendMessage(hWndLastControl, CB_LIMITTEXT, nChars, 0);
    }
}
