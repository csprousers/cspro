#include "StdAfx.h"
#include "CodeMenu.h"
#include "PathAdjusterDlg.h"
#include "StringEncoderDlg.h"
#include <zUtilO/WindowHelpers.h>


void CodeMenu::ReplaceCodeMenuPopups(CMenu* pPopupMenu, int document_lexer_language, int view_lexer_language)
{
    ASSERT(pPopupMenu->GetMenuItemID(0) == FirstCodeMenuItemId);

    if( Lexers::UsesCSProLogic(document_lexer_language) )
        WindowHelpers::ReplaceMenuItemWithPopupMenu(pPopupMenu, ID_CODE_DEPRECATION_WARNINGS, zDesignerFDLL.hModule, IDR_DEPRECATION_WARNINGS);

    if( Lexers::IsCSProLogic(view_lexer_language) )
        WindowHelpers::ReplaceMenuItemWithPopupMenu(pPopupMenu, ID_CODE_FOLDING, zDesignerFDLL.hModule, IDR_CODE_FOLDING);
}


void CodeMenu::OnStringEncoder(const LogicSettings& logic_settings, std::string initial_text)
{
    StringEncoderDlg dlg(logic_settings, std::move(initial_text));
    dlg.DoModal();
}


void CodeMenu::OnPathAdjuster(const int lexer_language, std::string initial_path)
{
    PathAdjusterDlg dlg(lexer_language, std::move(initial_path));
    dlg.DoModal();
}
