#include "StdAfx.h"
#include "NoteEditDlg.h"


NoteEditDlg::NoteEditDlg(std::string title, SharableString note)
    :   m_title(std::move(title)),
        m_note(std::move(note))
{
}


std::string NoteEditDlg::GetDialogName()
{
    return "note-edit";
}


SharableString NoteEditDlg::GetJsonArgumentsText()
{
    return Json::CreateObjectString(
        {
            { JK::title, m_title },
            { JK::note,  m_note }
        });
}


void NoteEditDlg::ProcessJsonResults(const JsonNode& json_results)
{
    m_note = json_results.Get<std::string>(JK::note);
}
