#include "stdafx.h"
#include "MessageEditCtrl.h"


void MessageEditCtrl::InitializeControl()
{
    SetAccelerators(IDR_MESSAGE_EDIT);
    SetCtxMenuId(IDR_MESSAGE_EDIT);

    // see which version of messages is being used
    int lexer_language = Lexers::GetLexer_MessageDefault();
    WindowsDesktopMessage::Send(UWM::Edit::GetLexerLanguage, this, &lexer_language);

    BaseInitControl(lexer_language);

    SetMargins(0);
}
