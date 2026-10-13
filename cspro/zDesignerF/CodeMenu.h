#pragma once

#include <zDesignerF/zDesignerF.h>
#include <zDesignerF/resource_shared.h>
#include <zToolsO/EnumHelpers.h>
#include <zToolsO/WinSettings.h>
#include <zUtilO/WindowHelpers.h>
#include <zEditO/LogicCtrl.h>
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



    // --------------------------------------------------------------------------
    // Deprecation Warnings
    // --------------------------------------------------------------------------

    namespace DeprecationWarnings
    {
        enum class Level { None, Most, All };
        constexpr Level DefaultLevel = Level::Most;


        inline Level GetLevel()
        {
            return static_cast<Level>(WinSettings::Read<DWORD>(WinSettings::Type::DeprecationWarnings, static_cast<DWORD>(DefaultLevel)));
        }


        inline void SetLevel(Level level)
        {
            WinSettings::Write<DWORD>(WinSettings::Type::DeprecationWarnings, static_cast<DWORD>(level));
        }


        // for menu processing
        // --------------------------------------------------------------------------

        constexpr Level GetLevelFromId(UINT nID)
        {
            return AddToEnum(Level::None, nID - ID_CODE_DEPRECATION_WARNINGS_NONE);
        }


        inline void OnDeprecationWarnings(UINT nID)
        {
            SetLevel(GetLevelFromId(nID));
        }


        inline void OnUpdateDeprecationWarnings(CCmdUI* pCmdUI)
        {
            pCmdUI->SetCheck(GetLevel() == GetLevelFromId(pCmdUI->m_nID));
        }
    }
}
