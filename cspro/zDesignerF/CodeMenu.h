#pragma once

#include <zDesignerF/zDesignerF.h>
#include <zEditO/resource_shared.h>


namespace CodeMenu
{
    // --------------------------------------------------------------------------
    // Menu actions shared by CSPro and CSCode
    // --------------------------------------------------------------------------

    constexpr unsigned FirstCodeMenuItemId = ID_CODE_PASTE_STRING_LITERAL;

    CLASS_DECL_ZDESIGNERF void ReplaceCodeMenuPopups(CMenu* pPopupMenu, int document_lexer_language, int view_lexer_language);

    CLASS_DECL_ZDESIGNERF void OnStringEncoder(const LogicSettings& logic_settings, std::string initial_text);

    CLASS_DECL_ZDESIGNERF void OnPathAdjuster(int lexer_language, std::string initial_path);
}
