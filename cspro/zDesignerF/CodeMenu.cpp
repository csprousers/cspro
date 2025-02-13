#include "StdAfx.h"
#include "CodeMenu.h"
#include "PathAdjusterDlg.h"
#include "StringEncoderDlg.h"


void CodeMenu::ReplaceCodeMenuPopups(CMenu* pPopupMenu, int document_lexer_language, int view_lexer_language)
{
    ASSERT(pPopupMenu->GetMenuItemID(0) == FirstCodeMenuItemId);

    if( Lexers::UsesCSProLogic(document_lexer_language) )
        WindowHelpers::ReplaceMenuItemWithPopupMenu(pPopupMenu, ID_CODE_DEPRECATION_WARNINGS, zDesignerFDLL.hModule, IDR_DEPRECATION_WARNINGS);

    if( Lexers::IsCSProLogic(view_lexer_language) )
        WindowHelpers::ReplaceMenuItemWithPopupMenu(pPopupMenu, ID_CODE_FOLDING, zDesignerFDLL.hModule, IDR_CODE_FOLDING);
}


void CodeMenu::OnPasteStringLiteral(CLogicCtrl& logic_ctrl)
{
    std::wstring clipboard_text = WinClipboard::GetText();
    SO::Remove(clipboard_text, '\r');

    if( clipboard_text.empty() )
        return;

    const int lexer_language = logic_ctrl.GetLexer();
    std::wstring escaped_text;

    if( Lexers::UsesCSProLogic(lexer_language) || lexer_language == SCLEX_CSPRO_MESSAGE_V8_0 )
    {
        const bool escape_string_literals = Lexers::IsNotV0(lexer_language);
        const bool split_newlines = ( escape_string_literals &&
                                      lexer_language != SCLEX_CSPRO_MESSAGE_V8_0 &&
                                      WinSettings::Read<DWORD>(WinSettings::Type::CodeLogicSplitNewlines, 1) == 1 );

        Logic::StringEscaper logic_string_escaper(escape_string_literals);

        escaped_text = split_newlines ? logic_string_escaper.EscapeStringWithSplitNewlines(std::move(clipboard_text)) :
                                        logic_string_escaper.EscapeString(std::move(clipboard_text));
    }

    else
    {
        ASSERT(lexer_language == SCLEX_JSON || lexer_language == SCLEX_JAVASCRIPT);
        const bool escape_forward_slashes = ( WinSettings::Read<DWORD>(WinSettings::Type::CodeEscapeJsonForwardSlashes, 0) == 1);
        escaped_text = Encoders::ToJsonString(clipboard_text, escape_forward_slashes);
    }

    logic_ctrl.ReplaceSel(escaped_text);
}


void CodeMenu::OnStringEncoder(const LogicSettings& logic_settings, std::wstring initial_text)
{
    StringEncoderDlg dlg(logic_settings, std::move(initial_text));
    dlg.DoModal();
}


void CodeMenu::OnPathAdjuster(int lexer_language, std::wstring initial_path)
{
    PathAdjusterDlg dlg(lexer_language, std::move(initial_path));
    dlg.DoModal();
}
