#include "StdAfx.h"
#include "TextInputDlg.h"


CREATE_JSON_KEY(allowEmptyText)
CREATE_JSON_KEY(initialValue)
CREATE_JSON_KEY(multiline)
CREATE_JSON_KEY(numeric)
CREATE_JSON_KEY(password)
CREATE_JSON_KEY(textInput)
CREATE_JSON_KEY(uppercase)


TextInputDlg::TextInputDlg()
    :   m_numeric(false),
        m_password(false),
        m_uppercase(false),
        m_multiline(false),
        m_requireInput(false)
{
}


const TCHAR* TextInputDlg::GetDialogName()
{
    return _T("text-input");
}


std::wstring TextInputDlg::GetJsonArgumentsText()
{
    return Json::CreateObjectString(
        {
            { JK::title,          m_title },
            { JK::initialValue,   m_initialValue },
            { JK::numeric,        m_numeric },
            { JK::password,       m_password },
            { JK::uppercase,      m_uppercase },
            { JK::multiline,      m_multiline },
            { JK::allowEmptyText, !m_requireInput }
        });
}


void TextInputDlg::ProcessJsonResults(const JsonNode<wchar_t>& json_results)
{
    m_textInput = json_results.Get<std::wstring>(JK::textInput);
}
