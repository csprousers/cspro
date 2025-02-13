#include "StdAfx.h"
#include "TextEditView.h"
#include <zToolsO/WinSettings.h>


IMPLEMENT_DYNCREATE(TextEditView, CLogicView)

BEGIN_MESSAGE_MAP(TextEditView, CLogicView)
    ON_COMMAND(ID_EDIT_FIND_NEXT, OnFindNext)
    ON_COMMAND(ID_EDIT_WORD_WRAP, OnWordWrap)
    ON_UPDATE_COMMAND_UI(ID_EDIT_WORD_WRAP, OnUpdateWordWrap)
END_MESSAGE_MAP()


void TextEditView::OnInitialUpdate()
{
    __super::OnInitialUpdate();

    TextEditDoc& text_edit_doc = GetTextEditDoc();
    CLogicCtrl* logic_ctrl = GetLogicCtrl();

    const std::wstring initial_text = text_edit_doc.OnViewInitialUpdate(*this);

    // set the proper lexer
    logic_ctrl->InitLogicControl(true, true, text_edit_doc.GetLexerLanguage());

    // set the text
    SetTextAndSetSavePoint(initial_text);
    logic_ctrl->EmptyUndoBuffer();

    // enable word wrap (if previously turned on, or by default)
    logic_ctrl->SetWrapVisualFlags(Scintilla::WrapVisualFlag::Margin);

    if( WinSettings::Read<DWORD>(WinSettings::Type::WordWrap, 1) == 1 )
        logic_ctrl->SetWrapMode(Scintilla::Wrap::WhiteSpace);
}


void TextEditView::OnActivateView(BOOL bActivate, CView* pActivateView, CView* pDeactiveView)
{
    if( !bActivate )
    {
        TextEditDoc& text_edit_doc = GetTextEditDoc();
        text_edit_doc.OnViewDeactivate();
    }

    __super::OnActivateView(bActivate, pActivateView, pDeactiveView);
}


void TextEditView::SetTextAndSetSavePoint(const std::wstring& text)
{
    CLogicCtrl* logic_ctrl = GetLogicCtrl();

    logic_ctrl->SetText(text);
    logic_ctrl->SetSavePoint();
}


void TextEditView::OnSavePointReached(Scintilla::NotificationData* /*pSCNotification*/)
{
    TextEditDoc& text_edit_doc = GetTextEditDoc();
    text_edit_doc.SetModifiedFlag(FALSE);
}


void TextEditView::OnSavePointLeft(Scintilla::NotificationData* /*pSCNotification*/)
{
    TextEditDoc& text_edit_doc = GetTextEditDoc();
    text_edit_doc.SetModifiedFlag(TRUE);
}


bool TextEditView::OnHandleHelp()
{
    HtmlHelp(reinterpret_cast<DWORD_PTR>(nullptr), HH_DISPLAY_TOPIC);
    return true;
}


void TextEditView::OnFindNext()
{
    SendMessage(WM_COMMAND, ID_EDIT_REPEAT);
}


void TextEditView::OnWordWrap()
{
    CLogicCtrl* logic_ctrl = GetLogicCtrl();
    const bool turn_on_word_wrap = ( logic_ctrl->GetWrapMode() == Scintilla::Wrap::None );

    logic_ctrl->SetWrapMode(turn_on_word_wrap ? Scintilla::Wrap::WhiteSpace : Scintilla::Wrap::None);

    WinSettings::Write<DWORD>(WinSettings::Type::WordWrap, turn_on_word_wrap ? 1 : 0);
}


void TextEditView::OnUpdateWordWrap(CCmdUI* pCmdUI)
{
    pCmdUI->SetCheck(GetLogicCtrl()->GetWrapMode() != Scintilla::Wrap::None);
}
