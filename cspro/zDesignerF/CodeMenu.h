#pragma once

#include <zDesignerF/zDesignerF.h>
#include <zDesignerF/resource_shared.h>
#include <zToolsO/EnumHelpers.h>
#include <zToolsO/WinSettings.h>
#include <zUtilO/WindowHelpers.h>
#include <zEdit2O/LogicCtrl.h>


namespace CodeMenu
{
    // --------------------------------------------------------------------------
    // Menu actions shared by CSPro and CSCode
    // --------------------------------------------------------------------------

    constexpr unsigned FirstCodeMenuItemId = ID_CODE_PASTE_STRING_LITERAL;

    CLASS_DECL_ZDESIGNERF void ReplaceCodeMenuPopups(CMenu* pPopupMenu, int document_lexer_language, int view_lexer_language);

    CLASS_DECL_ZDESIGNERF void OnPasteStringLiteral(CLogicCtrl& logic_ctrl);

    CLASS_DECL_ZDESIGNERF void OnStringEncoder(const LogicSettings& logic_settings, std::wstring initial_text);

    CLASS_DECL_ZDESIGNERF void OnPathAdjuster(int lexer_language, std::wstring initial_path);



    // --------------------------------------------------------------------------
    // Lexer helpers
    // --------------------------------------------------------------------------

    constexpr bool CanCommentCode(int lexer_language)
    {
        return ( Lexers::IsCSProLogic(lexer_language) ||
                 Lexers::IsCSProMessage(lexer_language) );
    }


    constexpr bool CanPasteStringLiteral(int lexer_language)
    {
        return ( Lexers::UsesCSProLogic(lexer_language) ||
                 lexer_language == SCLEX_CSPRO_MESSAGE_V8_0 ||
                 lexer_language == SCLEX_JSON ||
                 lexer_language == SCLEX_JAVASCRIPT );
    }



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



    // --------------------------------------------------------------------------
    // Code Folding
    // --------------------------------------------------------------------------

    namespace CodeFolding
    {
        enum class Level { None, ProcsAndFunctions, All };
        constexpr Level DefaultLevel = Level::All;


        inline Level GetLevel()
        {
            return static_cast<Level>(WinSettings::Read<DWORD>(WinSettings::Type::CodeFolding, static_cast<DWORD>(DefaultLevel)));
        }


        inline void SetLevel(Level level)
        {
            WinSettings::Write<DWORD>(WinSettings::Type::CodeFolding, static_cast<DWORD>(level));
        }


        // for menu processing
        // --------------------------------------------------------------------------

        constexpr Level GetLevelFromId(UINT nID)
        {
            return AddToEnum(CodeFolding::Level::None, nID - ID_CODE_FOLDING_LEVEL_NONE);
        }


        inline void OnCodeFoldingLevel(UINT nID, CLogicCtrl* logic_ctrl)
        {
            SetLevel(GetLevelFromId(nID));

            if( logic_ctrl != nullptr )
            {
                logic_ctrl->SetFolding();
                logic_ctrl->SetFocus();
            }
        }


        inline void OnUpdateCodeFoldingLevel(CCmdUI* pCmdUI)
        {
            pCmdUI->SetCheck(GetLevel() == GetLevelFromId(pCmdUI->m_nID));
        }


        inline void OnCodeFoldingAction(UINT nID, CLogicCtrl* logic_ctrl)
        {
            ASSERT(logic_ctrl != nullptr && Lexers::CanFoldCode(logic_ctrl->GetLexer()));

            if( nID == ID_CODE_FOLDING_FOLD_ALL )
            {
                logic_ctrl->FoldAll(Scintilla::FoldAction::Contract);
            }

            else if( nID == ID_CODE_FOLDING_UNFOLD_ALL )
            {
                logic_ctrl->FoldAll(Scintilla::FoldAction::Expand);
            }

            else
            {
                ASSERT(nID == ID_CODE_FOLDING_TOGGLE);
                int line_number = logic_ctrl->LineFromPosition(logic_ctrl->GetCurrentPos());
                logic_ctrl->FoldLine(line_number, Scintilla::FoldAction::Toggle);
            }
        }


        inline void OnUpdateCodeFoldingAction(CCmdUI* pCmdUI, CLogicCtrl* logic_ctrl)
        {
            pCmdUI->Enable(logic_ctrl != nullptr &&
                           GetLevel() != CodeFolding::Level::None &&
                           Lexers::CanFoldCode(logic_ctrl->GetLexer()));
        }
    }
}
