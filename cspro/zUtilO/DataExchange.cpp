#include "StdAfx.h"
#include "DataExchange.h"
#include "ConnectionString.h"
#include "WindowsWS.h"


// --------------------------------------------------
// DDX_Text
// --------------------------------------------------

void DDX_Text(CDataExchange* pDX, int nIDC, std::wstring& text, bool trim_string_on_save/* = false*/)
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


void DDX_Text(CDataExchange* pDX, int nIDC, ConnectionString& connection_string)
{
    std::wstring text;

    if( !pDX->m_bSaveAndValidate && connection_string.IsDefined() )
        text = connection_string.ToString();

    DDX_Text(pDX, nIDC, text);

    if( pDX->m_bSaveAndValidate )
        connection_string = ConnectionString(std::move(text));
}



// --------------------------------------------------
// DDX_CBStringExact
// --------------------------------------------------

void DDX_CBString(CDataExchange* pDX, int nIDC, std::wstring& text)
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


void DDX_CBStringExact(CDataExchange* pDX, int nIDC, std::wstring& text)
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
		int i = (int)::SendMessage(hWndCtrl, CB_FINDSTRINGEXACT, static_cast<WPARAM>(-1), reinterpret_cast<LPARAM>(text.c_str()));

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
