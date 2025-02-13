#include "StdAfx.h"
#include "NoteEditDlg.h"


NoteEditDlg::NoteEditDlg(std::wstring title, std::wstring note)
    :   m_title(std::move(title)),
        m_note(std::move(note))
{
}


const TCHAR* NoteEditDlg::GetDialogName()
{
    return _T("note-edit");
}


std::wstring NoteEditDlg::GetJsonArgumentsText()
{
    return Json::CreateObjectString(
        {
            { JK::title, m_title },
            { JK::note,  m_note }
        });
}


void NoteEditDlg::ProcessJsonResults(const JsonNode<wchar_t>& json_results)
{
    m_note = json_results.Get<std::wstring>(JK::note);
}
